
#pragma once

#include <vector>
#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include "../idma.hpp"
#include "idma_be.hpp"



class fifo_req_t
{
public:
    bool push;
    uint8_t *data;

    uint64_t size;
};

class fifo_resp_t
{
public:
    // 1 push , 0 pop
    bool push;


    bool valid;
    uint8_t *data;

    uint64_t size;

};


class IDmaBeFifo: public vp::Block, public IdmaBeConsumer
{
public:
    /**
     * @brief Construct a new backend
     *
     * @param idma The top iDMA block.
     * @param itf_name Name of the interface where the backend should send requests.
     * @param be The top backend.
     */
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

    static void fifo_response(vp::Block *__this,  fifo_resp_t *fifo_resp);

    // Write a line to TCDM
    void write_chunk();

    // Read a line from TCDM
    void read_data();

    // Handle the end of a write request
    void write_handle_req_ack();

    void write_handle_req_end( fifo_resp_t *fifo_resp );

    void read_handle_req_end( fifo_resp_t *fifo_resp );

    // Remove a chunk of data from current burst. This is used to track when a burst is done 
    void remove_chunk_from_current_burst(uint64_t size);
    
    // Enqueue a burst to pending queue. Burst will be processed in order
    void enqueue_burst(uint64_t base, uint64_t size, bool is_write);

    uint64_t get_line_size(uint64_t base, uint64_t size);

    
    IdmaBeProducer *be;

    // change IoMaster with WireMaster
    vp::WireMaster<fifo_req_t *> ico_itf;

    vp::WireSlave<fifo_resp_t *> fifo_resp_itf; 


    vp::Trace trace;
    
    vp::ClockEvent fsm_event;

    uint64_t current_burst_size;
    uint64_t current_burst_base;
    bool current_burst_is_write;
    uint64_t fifo_data_width;

    uint64_t write_current_chunk_size;

    uint64_t write_ack_size;

    uint64_t size_to_ack;

    uint64_t write_current_chunk_ack_size;
    uint64_t write_current_chunk_base;
    uint8_t *write_current_chunk_data_start;
    uint8_t *write_current_chunk;

    uint64_t new_current_chunk_size;

    uint64_t read_pending_data_size;

    uint8_t *read_pending_data;

    int64_t last_chunk_timestamp;

    int count;
    uint64_t write_size_inc;

    uint64_t write_chunk_to_remove;

};
