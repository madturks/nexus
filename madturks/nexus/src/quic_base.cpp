#include <mad/nexus/quic_base.hpp>
#include <flatbuffers/flatbuffer_builder.h>

namespace mad::nexus {
    quic_base::~quic_base() = default;

    auto quic_base::get_message_builder() -> flatbuffers::FlatBufferBuilder & {
        thread_local ::flatbuffers::FlatBufferBuilder builder;
        // Reserved space for QUIC_BUFFER. One allocation is enough.
        constexpr auto k_QuicBufferSize                                   = 16;
        constexpr auto k_QuicBufferAlignment                              = 8;
        // Align the buffer properly.
        builder.PreAlign(k_QuicBufferSize, k_QuicBufferAlignment);
        builder.Pad(k_QuicBufferSize);
        assert(builder.GetSize() == k_QuicBufferSize);
        return builder;
    }
} // namespace mad::nexus
