#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include <queue>


class fifo_reqrsp_t
{
public:
    bool push;
    uint8_t *data;
};

class mesh_FIFOs : public vp::Component
{

public:
    mesh_FIFOs(vp::ComponentConf &config);

private:
    static void idma_req_handler(vp::Block *__this,  fifo_reqrsp_t *fifo_req);
    vp::WireSlave<fifo_reqrsp_t *> i_dma_req_itf;
    vp::WireMaster<fifo_reqrsp_t *> o_fifo_out_itf;    
    vp::WireMaster<fifo_reqrsp_t *> o_fifo_in_itf;
    std::queue<uint64_t> FIFO;
    vp::Trace trace;
};


mesh_FIFOs::mesh_FIFOs(vp::ComponentConf &config)
    : vp::Component(config)
{
    this->i_dma_req_itf.set_sync_meth(&mesh_FIFOs::idma_req_handler);
    this->new_slave_port("i_dma_req", &this->i_dma_req_itf);
    this->new_master_port("o_fifo_out_resp", &this->o_fifo_out_itf);
    this->new_master_port("o_fifo_in_resp", &this->o_fifo_in_itf);
    this->traces.new_trace("trace", &this->trace, vp::DEBUG);
}


void mesh_FIFOs::idma_req_handler(vp::Block *__this, fifo_reqrsp_t *fifo_req)
{
    mesh_FIFOs *_this = (mesh_FIFOs *)__this;
    
    // fifo mesh out
    if(fifo_req->push)
    {
        // read data sent from idma
        uint64_t fifo_element = 0;
        for( int i = 0; i < 8; i++ )
        {
            fifo_element |= (static_cast<uint64_t>( *(fifo_req->data + i )) << (i*8) );
            _this->trace.msg( vp::Trace::LEVEL_TRACE, "Byte %d, value %d\n", i, *(fifo_req->data + i ) );
        }
        // push data to fifo
        _this->FIFO.push( fifo_element );
        // prepare resp
        fifo_reqrsp_t resp = {.push= true};
        // send resp
        _this->o_fifo_out_itf.sync(&resp);
    }

    // fifo mesh in
    else
    {
        uint8_t data[8];
        // pointer to the first byte
        uint8_t * data_ptr = &data[0];
        for(int i=0; i < 8; i++ )
        {   
            // read data from fifo
            data[i] = (_this->FIFO.front() >> (i*8)) & 0xFF;
            _this->trace.msg(vp::Trace::LEVEL_TRACE, "byte %d, value %d\n", i, data[i] );
        }
        // pop data
        _this->FIFO.pop();
        // prepare resp
        fifo_reqrsp_t resp = {.push= false, .data = data_ptr };
        // send resp 
        _this->o_fifo_in_itf.sync(&resp);
    }

}

extern "C" vp::Component *gv_new(vp::ComponentConf &config)
{
    return new mesh_FIFOs(config);
}