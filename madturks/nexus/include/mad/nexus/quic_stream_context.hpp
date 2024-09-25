#pragma once

#include <mad/nexus/quic_callback_types.hpp>
#include <mad/circular_buffer_vm.hpp>

namespace mad::nexus {
    struct stream_context {

        using circular_buffer_t = mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;

        /**
         * Construct a new stream context object
         *
         * @param owner The owning MSQUIC connection
         * @param receive_buffer_size Size of the receive circular buffer.
         * @param send_buffer_size  Size of the send circular buffer.
         */
        stream_context(void * stream, void * owner, stream_data_callback_t data_callback, std::size_t receive_buffer_size = 16384) :
            stream_handle(stream), connection_handle(owner), on_data_received(data_callback),
            receive_buffer(receive_buffer_size, circular_buffer_t::auto_align_to_page{}) {}

        inline void * stream() const {
            return stream_handle;
        }

        inline void * connection() const {
            return connection_handle;
        }

        inline circular_buffer_t & rbuf() {
            return receive_buffer;
        }

        inline const circular_buffer_t & rbuf() const {
            return receive_buffer;
        }

    private:
        /**
         * Msquic stream handle
         */
        void * stream_handle;

        /**
         * The connection that stream belongs to.
         */
        void * connection_handle;

    public:
        stream_data_callback_t on_data_received;

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
