#include <algorithm>
#include <vp/vp.hpp>
#include "idma_be_fifo.hpp"



IDmaBeFifo::IDmaBeFifo(vp::Component *idma, std::string itf_name, std::string slave_itf, IdmaBeProducer *be)
:   Block(idma, itf_name),
    fsm_event(this, &IDmaBeFifo::fsm_handler)
{
    this->be = be;    
    this->traces.new_trace("trace", &this->trace, vp::DEBUG);
    
    idma->new_master_port(itf_name, &this->fifo_req_itf, this);

    idma->new_slave_port(slave_itf, &this->fifo_resp_itf, this);

    this->fifo_resp_itf.set_sync_meth(&IDmaBeFifo::fifo_response);

    // fifo size // REMEMBER TO ADD FIFO SIZE IN ADD PROPERTIES and in befifo.hpp
    this->fifo_size = idma->get_js_config()->get_int("fifo_size");
    this->fifo_data_width = 0x08;
}


void IDmaBeFifo::reset(bool active)
{
    if(active)
    {
    this->write_current_chunk_size = 0;
    this->read_pending_data_size = 0;
    this->last_chunk_timestamp = -1;
    }
}


void IDmaBeFifo::update()
{
     this->fsm_event.enqueue();
}


// enqueue 1 burst only
void IDmaBeFifo::enqueue_burst(uint64_t base, uint64_t size, bool is_write)
{
    
    // burst size
    this->current_burst_size = size;

    // is write
    this->current_burst_is_write = is_write;

    this->update();
}

// called by backend
void IDmaBeFifo::read_burst(uint64_t base, uint64_t size)
{
    this->enqueue_burst(base, size, false);
}


// called by backend
void IDmaBeFifo::write_burst(uint64_t base, uint64_t size)
{
    this->enqueue_burst(base, size, true);
}


// called by backend to find the best size that fits in both src and dst backends 
uint64_t IDmaBeFifo::get_burst_size(uint64_t base, uint64_t size)
{
    // burst size will be resized later on by write data
    return size;
}


// only one burst allowed
bool IDmaBeFifo::can_accept_burst()
{
    return this->current_burst_size == 0;
}


// only if no other pending chunks 
bool IDmaBeFifo::can_accept_data()
{
    return !this->be->is_fifo_full;
}


bool IDmaBeFifo::is_empty()
{
    return this->current_burst_size == 0;
}


// Called by backend to push data for the current burst
void IDmaBeFifo::write_data(uint8_t *data, uint64_t size)
{
    this->trace.msg(vp::Trace::LEVEL_TRACE, " Writing data (size: 0x%lx) data %x\n", size, data);

    // size of data read by src be (can this be longer than 64 bits? )
    this->write_current_chunk_size = size;

    // data pointer from src be
    this->write_current_chunk = data;

    // ack data and size used later for ack
    this->write_current_chunk_ack_size = size;
    this->write_current_chunk_data_start = data;

    this->write_chunk();
}


void IDmaBeFifo::write_chunk()
{
    // no other chunks has been sent this cycle (we write only one chunk (64 bits) per cycle )
    if( this->last_chunk_timestamp == -1 || this->last_chunk_timestamp < this->clock.get_cycles() )
    {
        // update last timestamp so that this is the last time we sent a chunk
        this->last_chunk_timestamp = this->clock.get_cycles();

        //  if chunksize is longer than 64 bits (assuming fifo data width = 64 bits) 
        this->write_chunk_size_to_remove = std::min( this->write_current_chunk_size, this->fifo_data_width );

        this->trace.msg(vp::Trace::LEVEL_TRACE, "Sending data %x to fifo_out\n", this->write_current_chunk);
        
        // prepare req
        fifo_reqrsp_t req = { .push=true, .data = this->write_current_chunk };

        // update chunk size
        this->write_current_chunk_size -= this->write_chunk_size_to_remove;
        this->write_current_chunk += 0x08; // update after pushing 8 bytes
        
        // update fifo counter
        this->be->fifo_elements++;
        this->trace.msg(vp::Trace::LEVEL_TRACE, "fifo counter %d (+8 bytes to the fifo)\n", this->be->fifo_elements );
        if(this->be->fifo_elements == this->fifo_size)
            this->be->is_fifo_full = 1;
        
        // sending req to fifo
        this->fifo_req_itf.sync( &req );
    }
    else
    {
        // go to next cycle so that last_chunk_timestamp < this->clock.get_cycles() is met
        this->fsm_event.enqueue(1);
    } 
}


// response from fifo
void IDmaBeFifo::fifo_response(vp::Block *__this,  fifo_reqrsp_t *fifo_resp)
{
    IDmaBeFifo *_this = (IDmaBeFifo *)__this;
    
    // response from fifo_out
    if (fifo_resp->push)
    {
        // update current burst
        _this->remove_chunk_from_current_burst( _this->write_chunk_size_to_remove );  
        _this->write_handle_req_ack();
    }

    // response from fifo_in
    else
    {
        // be accept data (if dst be is ready to receive data)
        if(_this->be->is_ready_to_accept_data())
        {
            _this->trace.msg(vp::Trace::LEVEL_TRACE, "[fifo response] sending data from fifo: data %x and size %lx \n", fifo_resp->data, 0x08 );
            _this->be->fifo_elements--;
            _this->trace.msg(vp::Trace::LEVEL_TRACE, "fifo counter %d (-8 bytes from the fifo)\n", _this->be->fifo_elements);
            _this->remove_chunk_from_current_burst( 0x08 );
            _this->be->write_data(fifo_resp->data, 0x08 );
        }
        // be not ready to accept data
        else
        {
        _this->trace.msg(vp::Trace::LEVEL_TRACE, "backend not ready to accept data \n");

        _this->read_pending_data = fifo_resp->data;
        _this->read_pending_data_size = 0x08;

        _this->fsm_event.enqueue(1);
        }
    }
}


void IDmaBeFifo::remove_chunk_from_current_burst(uint64_t size)
{
    this->current_burst_size -= size;

    if(this->current_burst_size == 0)
    {
        this->be->update();
        this->update();
    }
}


void IDmaBeFifo::write_handle_req_ack()
{
    if (this->write_current_chunk_size == 0)
    {
        this->be->update();
        this->be->ack_data(this->write_current_chunk_data_start, this->write_current_chunk_ack_size);
    }
    else
    {
        this->fsm_event.enqueue();
    }
}



void IDmaBeFifo::fsm_handler(vp::Block *__this, vp::ClockEvent *event)
{   
    IDmaBeFifo *_this = (IDmaBeFifo *)__this;


    // if there's a pending write chunk
    if (_this->write_current_chunk_size > 0 )
    {
        _this->write_chunk();
    }

    //else
    //{
    //    _this->fsm_event.enqueue();
    //}

    // if read burst is pendings and no previous chunk has been sent
    if( _this->current_burst_size > 0  && !_this->current_burst_is_write && _this->read_pending_data_size == 0)
    {
        _this->read_data();
    }


    // in case a read pending data is stuck because be wasn't ready to receive it, check if it's possible now
    if( _this->read_pending_data_size > 0 && _this->be->is_ready_to_accept_data() )
    {
        _this->read_pending_data_size = 0;
        _this->remove_chunk_from_current_burst(0x08);
        _this->be->fifo_elements--;
        _this->trace.msg(vp::Trace::LEVEL_TRACE, "fifo counter %d (-8 bytes to the fifo)\n", _this->be->fifo_elements );

        _this->be->write_data(_this->read_pending_data, 0x08);
    }
    else
    {
        _this->fsm_event.enqueue(1);
    }
    
}



void IDmaBeFifo::read_data()
{

    uint64_t size = min(  this->current_burst_size, this->fifo_data_width );

    this->trace.msg(vp::Trace::LEVEL_TRACE, "sending read req to fifo \n");

    // prepare fifo req
    fifo_reqrsp_t req = { .push=false};

    // send fifo req
    this->fifo_req_itf.sync( &req );

}


// Called by destination backend to ack the data we sent for writing
void IDmaBeFifo::write_data_ack(uint8_t *data)
{
    // And check if there is any action to take since backend may became ready
    this->trace.msg(vp::Trace::LEVEL_TRACE," ack data %x \n", data);

    this->update();
}
