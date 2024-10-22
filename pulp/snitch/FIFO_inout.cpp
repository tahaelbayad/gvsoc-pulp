#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include <queue>


class fifo_req_t
{
public:
    // 1 push 0 pop
    bool push;
    uint8_t *data;
};

class fifo_resp_t
{
public: // if push 1 and valid 1: we correctly pushed data; if push 0 and valid 1: we correctly pop data.
    // 1 push 0 pop
    bool push;

    bool valid;
    uint8_t *data;

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

    std::queue<uint8_t *> fifo_inout;
    //std::queue<uint8_t> fifo_out;

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
        _this->fifo_inout.push(fifo_req->data);
        _this->trace.msg(vp::Trace::LEVEL_TRACE,"push data %x \n", fifo_req->data );

        fifo_resp_t resp = {.push= true, .valid=true};
        
        _this->fifo_in_resp_itf.sync(&resp);

    }
    else
    {
        fifo_resp_t resp = {.push= false, .valid=true, .data = _this->fifo_inout.front()};
        _this->trace.msg("pop data %x \n", _this->fifo_inout.front() );
        _this->fifo_inout.pop();
        _this->fifo_in_resp_itf.sync(&resp);
        
    }

}


extern "C" vp::Component *gv_new(vp::ComponentConf &config)
{
    return new FIFO_inout(config);
}
