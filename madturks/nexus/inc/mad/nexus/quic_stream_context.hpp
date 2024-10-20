#pragma once

#include <mad/circular_buffer_vm.hpp>
#include <mad/nexus/handle_carrier.hpp>
#include <mad/nexus/quic_callback_types.hpp>
#include <mad/nexus/serial_number_carrier.hpp>

namespace mad::nexus {

struct stream_callbacks {
    stream_callback_t on_start;
    stream_callback_t on_close;
    stream_data_callback_t on_data_received;
};

struct stream : public serial_number_carrier,
                handle_carrier {

    using circular_buffer_t = mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;

    /**
     * Construct a new stream context object
     *
     * @param owner The owning MSQUIC connection
     * @param receive_buffer_size Size of the receive circular buffer.
     * @param send_buffer_size  Size of the send circular buffer.
     */
    stream(void * hstream, connection & cctx, stream_callbacks cbks,
           std::size_t receive_buffer_size = 32768) :
        handle_carrier(hstream), connection_context_(cctx), callbacks(cbks),
        receive_buffer(
            receive_buffer_size, circular_buffer_t::auto_align_to_page{}) {}

    inline struct connection & connection() const {
        return connection_context_;
    }

    inline circular_buffer_t & rbuf() {
        return receive_buffer;
    }

    inline const circular_buffer_t & rbuf() const {
        return receive_buffer;
    }

private:
    /**
     * The connection that stream belongs to.
     */
    struct connection & connection_context_;

public:
    stream_callbacks callbacks;

private:
    /**
     * Data received from the stream
     * will go into this buffer.
     *
     * We don't need to protect the receive buffer
     * as all received stream data for a specific
     * connection guaranteed to happen serially.
     */
    circular_buffer_t receive_buffer;
};

// Stream contexts are going to be stored in connection context.
} // namespace mad::nexus
