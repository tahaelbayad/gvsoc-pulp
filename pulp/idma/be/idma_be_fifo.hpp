
#pragma once

#include <vector>
#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include "../idma.hpp"
#include "idma_be.hpp"



class fifo_reqrsp_t
{
public:
    bool push;
    uint8_t *data;

};

class IDmaBeFifo: public vp::Block, public IdmaBeConsumer
{
public:
    IDmaBeFifo(vp::Component *idma, std::string itf_name, std::string slave_itf, IdmaBeProducer *be);

    // reset all values
    void reset(bool active) override;

    // trigger fsm
    void update();

    // called by be
    void read_burst(uint64_t base, uint64_t size) override;

    // called by be
    void write_burst(uint64_t base, uint64_t size) override;

    // called by dst be to tell that data has been written
    void write_data_ack(uint8_t *data) override;

    // write data (when dst)
    void write_data(uint8_t *data, uint64_t size) override;

    // called by be before calling read/write burst
    uint64_t get_burst_size(uint64_t base, uint64_t size) override;

    // called by backend before calling read/write burst
    bool can_accept_burst() override;

    // called by src before sending data to dst (before be->write_data which call dst->write_data)
    bool can_accept_data() override;

    // if no current burst
    bool is_empty() override;

private:
    
    static void fsm_handler(vp::Block *__this, vp::ClockEvent *event);
    static void fifo_response(vp::Block *__this,  fifo_reqrsp_t *fifo_resp);
    void write_chunk();
    void read_data();
    void write_handle_req_ack();
    void write_handle_req_end( fifo_reqrsp_t *fifo_resp );
    void read_handle_req_end( fifo_reqrsp_t *fifo_resp );
    void remove_chunk_from_current_burst(uint64_t size);
    void enqueue_burst(uint64_t base, uint64_t size, bool is_write);


    IdmaBeProducer *be;

    vp::WireMaster<fifo_reqrsp_t *> fifo_req_itf;
    vp::WireSlave<fifo_reqrsp_t *> fifo_resp_itf; 

    vp::Trace trace;
    vp::ClockEvent fsm_event;
    
    int fifo_size;
    uint64_t fifo_data_width;

    uint64_t current_burst_size;
    bool current_burst_is_write;
  

    uint64_t write_current_chunk_size;
    uint64_t write_current_chunk_ack_size;

    uint8_t *write_current_chunk_data_start;
    uint8_t *write_current_chunk;
    uint64_t write_chunk_size_to_remove;

    uint64_t read_pending_data_size;
    uint8_t *read_pending_data;

    int64_t last_chunk_timestamp;
};