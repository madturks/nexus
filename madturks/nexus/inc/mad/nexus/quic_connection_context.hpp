#pragma once

#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>
#include <unordered_map>

namespace mad::nexus {
    struct connection_context {
        using connection_handle_t = void *;
        connection_handle_t connection_handle;

        using stream_handle_t = std::shared_ptr<void>;
        /**
         * Enable lookup by raw pointer.
         */
        std::unordered_map<stream_handle_t, stream_context, shared_ptr_raw_hash, shared_ptr_raw_equal> streams;
    };
} // namespace mad::nexus
