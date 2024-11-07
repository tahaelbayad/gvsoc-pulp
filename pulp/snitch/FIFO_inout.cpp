#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include <queue>


class fifo_reqrsp_t
{
public:
    bool push;
    uint8_t *data;
};




class FIFO_inout : public vp::Component
{

public:
    FIFO_inout(vp::ComponentConf &config);

private:

    static void idma_req_handler(vp::Block *__this,  fifo_reqrsp_t *fifo_reqrsp);

    vp::WireSlave<fifo_reqrsp_t *> i_dma_req_itf;

    vp::WireMaster<fifo_reqrsp_t *> o_fifo_out_itf;
    
    vp::WireMaster<fifo_reqrsp_t *> o_fifo_in_itf;

    std::queue<uint8_t> fifo_inout;
    
    vp::Trace trace;


};


FIFO_inout::FIFO_inout(vp::ComponentConf &config)
    : vp::Component(config)
{
    this->i_dma_req_itf.set_sync_meth(&FIFO_inout::idma_req_handler);

    this->new_slave_port("i_dma_req", &this->i_dma_req_itf);

    this->new_master_port("o_fifo_out", &this->o_fifo_out_itf);

    this->new_master_port("o_fifo_in", &this->o_fifo_in_itf);

    this->traces.new_trace("trace", &this->trace, vp::DEBUG);

}


void FIFO_inout::idma_req_handler(vp::Block *__this, fifo_reqrsp_t *fifo_reqrsp)
{
    FIFO_inout *_this = (FIFO_inout *)__this;

    // FIFO OUT
    if(fifo_reqrsp->push)
    {
        // push 8 bytes (64 bits)
        for( int i = 0; i < 8; i++ )
        {
            // fifo_reqrsp->data points to the first byte
            _this->fifo_inout.push(*(fifo_reqrsp->data + i ));
            _this->trace.msg(vp::Trace::LEVEL_TRACE, "push data %d, (address %x)\n", *(fifo_reqrsp->data + i ), fifo_reqrsp->data + i  );
        }

        // prepare resp
        fifo_reqrsp_t resp = {.push= true};
        
        // send resp
        _this->o_fifo_out_itf.sync(&resp);

    }

    // FIFO IN
    else
    {
        uint8_t data[64];
        
        // pointer to the first byte
        uint8_t * data_ptr = &data[0];

        for(int i=0; i < 8; i++ )
        {   
            // pops all 8 bytes            
            data[i] = _this->fifo_inout.front();
            _this->trace.msg(vp::Trace::LEVEL_TRACE, "pop data %d\n", data[i] );
            _this->fifo_inout.pop();
        }

        // prepare resp
        fifo_reqrsp_t resp = {.push= false, .data = data_ptr };
       
        // send resp 
        _this->o_fifo_in_itf.sync(&resp);
        
    }

}


extern "C" vp::Component *gv_new(vp::ComponentConf &config)
{
    return new FIFO_inout(config);
}
