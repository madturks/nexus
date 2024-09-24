#pragma once

#include <mad/nexus/quic_stream_context.hpp>
#include <unordered_map>

namespace mad::nexus {
    struct connection_context {
        using connection_handle_t = void *;
        connection_handle_t connection_handle;
        std::unordered_map<connection_handle_t, stream_context> streams;
    };
} // namespace mad::nexus
