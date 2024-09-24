#pragma once

#include <mad/nexus/quic_callback_types.hpp>
#include <mad/circular_buffer_vm.hpp>

#include <memory>
#include <cstdint>
#include <map>
#include <mutex>

#include <flatbuffers/detached_buffer.h>

namespace mad::nexus {
    struct stream_context {

        using circular_buffer_t = mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;
        using send_buffer_map_t = std::map<std::uint64_t, flatbuffers::DetachedBuffer>;

        /**
         * Construct a new stream context object
         *
         * @param owner The owning MSQUIC connection
         * @param receive_buffer_size Size of the receive circular buffer.
         * @param send_buffer_size  Size of the send circular buffer.
         */
        stream_context(void * stream, void * owner, stream_data_callback_t data_callback, std::size_t receive_buffer_size = 16384) :
            stream_handle(stream), connection_handle(owner), on_data_received(data_callback),
            receive_buffer(receive_buffer_size, circular_buffer_t::auto_align_to_page{}), mtx(std::make_unique<std::mutex>()) {}

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

        template <typename... Args>
        inline auto store_buffer(Args &&... args) -> std::optional<typename send_buffer_map_t::iterator> {

            std::unique_lock<std::mutex> lock(*mtx);

            auto result = buffers_in_flight_.emplace(send_buffer_serial, std::forward<Args>(args)...);

            if (!result.second) {
                return std::nullopt;
            }

            // Increase the serial for the next packet.
            ++send_buffer_serial;
            return std::optional{result.first};
        }

        /**
         * Release a send buffer from in-flight buffer map.
         *
         * @param key The key of the buffer, previously obtained from
         *            the store_buffer(...) call.
         *
         * @return true when release successful
         * @return false otherwise
         */
        inline auto release_buffer(std::uint64_t key) -> bool {
            std::unique_lock<std::mutex> lock(*mtx);
            return buffers_in_flight_.erase(key) > 0;
        }

        auto in_flight_count() const {
            return buffers_in_flight_.size();
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

        std::unique_ptr<std::mutex> mtx;
        std::uint64_t send_buffer_serial = {0};

        send_buffer_map_t buffers_in_flight_;
    };

    // Stream contexts are going to be stored in connection context.
} // namespace mad::nexus
