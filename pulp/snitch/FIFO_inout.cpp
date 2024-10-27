#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include <queue>


class fifo_req_t
{
public:
    // 1 push 0 pop
    bool push;
    uint8_t *data;

    uint64_t size;
};

class fifo_resp_t
{
public: // if push 1 and valid 1: we correctly pushed data; if push 0 and valid 1: we correctly pop data.
    // 1 push 0 pop
    bool push;

    bool valid;
    uint8_t *data;

    uint64_t size;


};



class FIFO_inout : public vp::Component
{

public:
    FIFO_inout(vp::ComponentConf &config);

private:

    static void idma_req(vp::Block *__this,  fifo_req_t *fifo_req);
    vp::WireSlave<fifo_req_t *> input_itf;

    vp::WireMaster<fifo_resp_t *> fifo_out_resp_itf;
    
    vp::WireMaster<fifo_resp_t *> fifo_in_resp_itf;

    std::queue<uint8_t>fifo_inout;
    std::queue<uint64_t> size_queue;


    //std::queue<uint8_t> fifo_out;

    // uncommnet
    vp::Trace trace;


};


FIFO_inout::FIFO_inout(vp::ComponentConf &config)
    : vp::Component(config)
{
    this->input_itf.set_sync_meth(&FIFO_inout::idma_req);
    this->new_slave_port("input", &this->input_itf);

    this->new_master_port("fifo_out_resp_o", &this->fifo_out_resp_itf);

    this->new_master_port("fifo_in_resp_o", &this->fifo_in_resp_itf);

    this->traces.new_trace("trace", &this->trace);

}


void FIFO_inout::idma_req(vp::Block *__this, fifo_req_t *fifo_req)
{
    FIFO_inout *_this = (FIFO_inout *)__this;

    if(fifo_req->push)
    {
       
       // size of the req
        _this->size_queue.push(fifo_req->size);

        for( int i = 0; i < fifo_req->size; i++ )
        {
            _this->fifo_inout.push(*(fifo_req->data + i ));

            _this->trace.msg(vp::Trace::LEVEL_TRACE,"data (size %lx) 0x%x: push value  %x  \n", 
                    fifo_req->size,  fifo_req->data + i, *(fifo_req->data + i )  );
        }

        fifo_resp_t resp = {.push= true, .valid=true};
        
        _this->fifo_out_resp_itf.sync(&resp);

    }
    else
    {
        uint64_t size = _this->size_queue.front();
        _this->size_queue.pop();
        uint8_t data[size];
        uint8_t * data_ptr = &data[0];

        for(int i=0; i < size ; i++ )
        {   
            data[i] = _this->fifo_inout.front();
            _this->trace.msg(vp::Trace::LEVEL_TRACE,"data (size %lx) %x: pop value %x \n", size, data_ptr + i, data[i] );
            _this->fifo_inout.pop();
        }

        fifo_resp_t resp = {.push= false, .valid=true, .data = data_ptr, .size = size };
       

        _this->fifo_in_resp_itf.sync(&resp);
        
    }

}


extern "C" vp::Component *gv_new(vp::ComponentConf &config)
{
    return new FIFO_inout(config);
}
