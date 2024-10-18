
#pragma once

#include <vector>
#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include "../idma.hpp"
#include "idma_be.hpp"


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
    IDmaBeFifo(vp::Component *idma, std::string itf_name, IdmaBeProducer *be);

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

    // called by be before calling read/write burst
    bool can_accept_burst() override;

    // called by src before sending data to dst (before be->write_data which call dst->write_data)
    bool can_accept_data() override;

    // if no current burst
    bool is_empty() override;

private:
    
    static void fsm_handler(vp::Block *__this, vp::ClockEvent *event);

    // Write a line to TCDM
    void write_line();

    // Read a line from TCDM
    void read_line();

    // Handle the end of a write request
    void write_handle_req_ack();

    // Remove a chunk of data from current burst. This is used to track when a burst is done
    void remove_chunk_from_current_burst(uint64_t size);

    // Extract first pending information to let FSM start writing and reading lines from it
    void activate_burst();


    
    // Enqueue a burst to pending queue. Burst will be processed in order
    void enqueue_burst(uint64_t base, uint64_t size, bool is_write);

    
    IdmaBeProducer *be;
    

    // IO da sostituire con Wire
    vp::IoMaster ico_itf;
   
    vp::Trace trace;
    
    vp::ClockEvent fsm_event;

    // IO da sostituire con Wire
    vp::IoReq req;

    // Current base of the first transfer. This is when a chunk of data to be written is received
    // to know the base where it should be written.
    uint64_t current_burst_base;

    // Size of currently active burst, the one from which lines are read or written
    uint64_t current_burst_size;

    // When a chunk is being written line by line, this gives the base address for next line
    uint64_t write_current_chunk_base;

    // When a chunk is being written line by line, this gives the remaining size
    uint64_t write_current_chunk_size;

    // Size to be acknowledged when write response is received
    uint64_t write_current_chunk_ack_size;

    // When a chunk is being written line by line, this gives the data pointer for next line
    uint8_t *write_current_chunk_data;

    // When a chunk is being written line by line, this gives the data pointer to the beginning
    // of the chunk


    // FORSE

    uint8_t *write_current_chunk_data_start;


    // Timestamp in cycles of the last time a line was read or written. Used to make sure we send
    // only one line per cycle
    int64_t last_line_timestamp;



    // FORSE


    // When a burst is being read, the last line read from TCDM may be blocked because the
    // backend is not ready to accept it. In this case this gives the data pointer containing the
    // data to be written
    uint8_t *read_pending_line_data;

    // When a burst is being read, the last line read from TCDM may be blocked because the
    // backend is not ready to accept it. In this case this gives the size of the
    // data to be written
    uint64_t read_pending_line_size;
};
