#include <algorithm>
#include <vp/vp.hpp>
#include "idma_be_fifo.hpp"



IDmaBeFifo::IDmaBeFifo(vp::Component *idma, std::string itf_name, std::string slave_itf, IdmaBeProducer *be)
:   Block(idma, itf_name),
    fsm_event(this, &IDmaBeFifo::fsm_handler)
{
    this->be = be;    
    this->traces.new_trace("trace", &this->trace, vp::DEBUG);
    
    idma->new_master_port(itf_name, &this->ico_itf, this);
    idma->new_slave_port(slave_itf, &this->fifo_resp_itf, this);

    this->fifo_resp_itf.set_sync_meth(&IDmaBeFifo::fifo_response);

    // where to use it?
    this->fifo_data_width = 0x40;

    this->count = 0;
}


void IDmaBeFifo::reset(bool active)
{
    if(active)
    {
        // da migliorare 
    this->current_burst_size = 0;   
    this->write_current_chunk_size = 0;
    this->write_ack_size = 0;
    this->read_pending_data_size = 0;
    this->last_chunk_timestamp = -1;
    this->size_to_ack = 0;
    this->write_size_inc = 0;
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

    // do i need it?
    this->current_burst_base = base;

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
    return this->write_current_chunk_size == 0;
}


bool IDmaBeFifo::is_empty()
{
    return this->current_burst_size == 0;
}





// Called by backend to push data for the current burst
void IDmaBeFifo::write_data(uint8_t *data, uint64_t size)
{
    this->trace.msg(vp::Trace::LEVEL_TRACE, " Writing data (size: 0x%lx) data %x\n", size, data);
    
    // do I need it?
    this->write_current_chunk_base = this->current_burst_base;

    // size of data read by src be
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
    // no other chunks has been sent this cycle
    if( this->last_chunk_timestamp == -1 || this->last_chunk_timestamp < this->clock.get_cycles() )
    {
        // update last timestamp so that this is the last time we sent a chunk
        this->last_chunk_timestamp = this->clock.get_cycles();

        // do i need it?
        uint64_t base = this->write_current_chunk_base;

        // this->new_current_chunk_size = std::min( this->write_current_chunk_size, this->fifo_data_width );
        // this->new_current_chunk_size = std::min( this->write_current_chunk_size, this->fifo_data_width );
        // this->new_current_chunk_size = this->get_line_size( base , this->write_current_chunk_size );
        this->new_current_chunk_size = this->write_current_chunk_size;

        this->trace.msg(vp::Trace::LEVEL_TRACE, " sending to fifo size 0x%lx data %x\n", this->new_current_chunk_size, this->write_current_chunk);



        
        // prepare req
        fifo_req_t req = { .push=true, .data = this->write_current_chunk, .size = this->new_current_chunk_size};

        this->write_current_chunk_size -= this->new_current_chunk_size;

        
        // sending req to fifo
        this->ico_itf.sync( &req );
    }
    else
    {
        // go to next cycle so that last_chunk_timestamp < this->clock.get_cycles() is met
        this->fsm_event.enqueue(1);
    } 
}


// response from fifo
void IDmaBeFifo::fifo_response(vp::Block *__this,  fifo_resp_t *fifo_resp)
{
    IDmaBeFifo *_this = (IDmaBeFifo *)__this;
    
    _this->count++;

    if(fifo_resp->valid)
    {
        // push
        if (fifo_resp->push)
        {
            _this->remove_chunk_from_current_burst(_this->new_current_chunk_size);
            _this->write_handle_req_ack();
        }
        // pop
        else
        {
            // be accept data
            if(_this->be->is_ready_to_accept_data())
            {
                // uint64_t size = _this->read_pending_data_size;
                //_this->read_pending_data_size = 0;
                _this->trace.msg(vp::Trace::LEVEL_TRACE, "sending data from fifo: data %x and size %lx \n", fifo_resp->data, fifo_resp->size );

 


                _this->remove_chunk_from_current_burst( fifo_resp->size );
                _this->be->write_data(fifo_resp->data, fifo_resp->size);
       
            }
            // be not ready to accept data
            else
            {
                _this->trace.msg(vp::Trace::LEVEL_TRACE, "backend not ready to accept data \n");

                _this->read_pending_data = fifo_resp->data;
                _this->read_pending_data_size = fifo_resp->size;

                _this->fsm_event.enqueue(1);

            }
        }
    }
    else
    {
        // fifo full or empty
    }
}


void IDmaBeFifo::remove_chunk_from_current_burst(uint64_t size)
{
    this->current_burst_base += size;
    this->current_burst_size -= size;

    if(this->current_burst_size == 0)
    {
        this->write_size_inc = 0;

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

    
    if(_this->count > 2)
    {
        // _this->trace.force_warning("count > 2 \n");
    }

    // if there's a pending write chunk
    if (_this->write_current_chunk_size > 0 )
    {
        _this->write_chunk();
    }
    else
    {
        _this->fsm_event.enqueue();
    }

    // if read burst is pendings and no previous chunk has been sent
    if( _this->current_burst_size > 0  && !_this->current_burst_is_write && _this->read_pending_data_size == 0)
    {
        _this->read_data();
    }


    // in case a read pending data is stuck because be wasn't ready to receive it, check if it's possible now
    if( _this->read_pending_data_size > 0 && _this->be->is_ready_to_accept_data() )
    {
        uint64_t size = _this->read_pending_data_size;
        _this->read_pending_data_size = 0;
        _this->remove_chunk_from_current_burst(size);

        _this->be->write_data(_this->read_pending_data, size);
    }
    else
    {
        _this->fsm_event.enqueue(1);
    }
    
}



void IDmaBeFifo::read_data()
{
   //  uint64_t base = this->current_burst_base;
    
    // this->read_pending_data_size = std::min(this->current_burst_size, this->fifo_data_width);
    //this->read_pending_data_size = this->get_line_size(base, this->current_burst_size);

    this->trace.msg(vp::Trace::LEVEL_TRACE, "sending read req to fifo \n");

    // prepare fifo req
    fifo_req_t req = { .push=false };

    // send fifo req
    this->ico_itf.sync( &req );

}


// Called by destination backend to ack the data we sent for writing
void IDmaBeFifo::write_data_ack(uint8_t *data)
{
    // And check if there is any action to take since backend may became ready
    this->trace.msg(vp::Trace::LEVEL_TRACE," ack data %x \n", data);

    this->update();
}



uint64_t IDmaBeFifo::get_line_size(uint64_t base, uint64_t size)
{
    // Make sure we don't go over interface width
    size = std::min(size, this->fifo_data_width);
    

    // And that we don't cross the line
    uint64_t next_page = (base + this->fifo_data_width - 1) & ~(this->fifo_data_width - 1);
    if (next_page > base)
    {
        size = std::min(next_page - base, size);
    }

    return size;
}