#include <mad/nexus/quic_base.hpp>
#include <flatbuffers/flatbuffer_builder.h>

namespace mad::nexus {
    quic_base::~quic_base() = default;

    auto quic_base::get_message_builder() -> flatbuffers::FlatBufferBuilder & {
        thread_local ::flatbuffers::FlatBufferBuilder builder;
        // Reserved space for QUIC_BUFFER. One allocation is enough.
        const std::uint8_t reserved_space [16] = {};
        builder.PushBytes(reserved_space, sizeof(reserved_space));
        return builder;
    }
} // namespace mad::nexus
