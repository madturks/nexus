#include <flatbuffers/flatbuffer_builder.h>
#include <mad/nexus/quic.hpp>

namespace mad::nexus {
    quic_server::~quic_server() = default;

    auto quic_server::get_message_builder() -> flatbuffers::FlatBufferBuilder & {
        thread_local ::flatbuffers::FlatBufferBuilder builder;
        // Reserved space for QUIC_BUFFER. One allocation is enough.
        const std::uint8_t reserved_space [16] = {};
        builder.PushBytes(reserved_space, sizeof(reserved_space));
        return builder;
    }
} // namespace mad::nexus
