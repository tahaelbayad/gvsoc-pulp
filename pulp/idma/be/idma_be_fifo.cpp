#include <algorithm>
#include <vp/vp.hpp>
#include "idma_be_fifo.hpp"



IDmaBeFifo::IDmaBeFifo(vp::Component *idma, std::string itf_name, IdmaBeProducer *be)
:   Block(idma, itf_name),
    fsm_event(this, &IDmaBeFifo::fsm_handler)
{
    // Backend will be used later for interaction
    this->be = be;

    // Declare master port to TCDM interface
    this->traces.new_trace("trace", &this->trace, vp::DEBUG);

    // Declare our own trace so that we can individually activate traces
    idma->new_master_port(itf_name, &this->ico_itf);


}


void IDmaBeFifo::reset(bool active)
{

}



void IDmaBeFifo::update()
{
     this->fsm_event.enqueue();
}


void IDmaBeFifo::read_burst(uint64_t base, uint64_t size)
{
    this->enqueue_burst(base, size, false);
}


void IDmaBeFifo::write_burst(uint64_t base, uint64_t size)
{
    this->enqueue_burst(base, size, false);
}

void IDmaBeFifo::enqueue_burst(uint64_t base, uint64_t size, bool is_read)
{
    
}


