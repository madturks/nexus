/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/circular_buffer_vm.hpp>
#include <mad/nexus/handle_carrier.hpp>
#include <mad/nexus/quic_callback_types.hpp>
#include <mad/nexus/serial_number_carrier.hpp>

#include <atomic>

namespace mad::nexus {

struct stream_callbacks {
    /******************************************************
     * Called when the stream is started.
     ******************************************************/
    stream_callback_t on_start;

    /******************************************************
     * Called when stream is about to be destroyed.
     ******************************************************/
    stream_callback_t on_close;

    /******************************************************
     * Called when new data is received from the stream.
     ******************************************************/
    stream_data_callback_t on_data_received;
};

struct debug_iface {

#ifndef NDEBUG

    static inline std::atomic<std::uint64_t> sends_in_flight;

#endif
};

/******************************************************
 * The stream type. Represents a quic stream.
 ******************************************************/
struct stream : public serial_number_carrier,
                handle_carrier,
                debug_iface {

    using circular_buffer_t = mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;

    /******************************************************
     * Construct a new stream object
     *
     * @param hstream The stream handle
     * @param cctx The owning connection
     * @param cbks Stream callbacks
     * @param receive_buffer_size The receive buffer size
     ******************************************************/
    stream(void * hstream, struct connection & cctx, stream_callbacks cbks,
           std::size_t receive_buffer_size = 32768) :
        handle_carrier(hstream), connection_context_(cctx), callbacks(cbks),
        receive_buffer(
            receive_buffer_size, circular_buffer_t::auto_align_to_page{}) {}

    /******************************************************
     * The owning connection
     ******************************************************/
    inline struct connection & connection() const {
        return connection_context_;
    }

    /******************************************************
     * Receive buffer.
     ******************************************************/
    template <typename Self>
    auto && rbuf(this Self && self) {
        return std::forward<Self>(self).receive_buffer;
    }

private:
    /******************************************************
     * The connection that the stream belongs to.
     ******************************************************/
    struct connection & connection_context_;

public:
    /******************************************************
     * Stream callbacks table
     ******************************************************/
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
