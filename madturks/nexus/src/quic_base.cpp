#include <mad/nexus/quic_base.hpp>

#include <flatbuffers/flatbuffer_builder.h>

namespace mad::nexus {
quic_base::~quic_base() = default;

auto quic_base::get_message_builder() -> flatbuffers::FlatBufferBuilder & {

    // For structs, layout is deterministic and guaranteed to be the same across
    // platforms (scalars are aligned to their own size, and structs themselves
    // to their largest member), and you are allowed to access this memory
    // directly by using sizeof() and memcpy on the pointer to a struct, or even
    // an array of structs.

    // Reading a FlatBuffer does not touch any memory outside the original
    // buffer, and is entirely read-only (all const), so is safe to access
    // from multiple threads even without synchronisation primitives.

    // Creating a FlatBuffer is not thread safe. All state related to building a
    // FlatBuffer is contained in a FlatBufferBuilder instance, and no memory
    // outside of it is touched. To make this thread safe, either do not share
    // instances of FlatBufferBuilder between threads (recommended), or manually
    // wrap it in synchronisation primitives. There's no automatic way to
    // accomplish this, by design, as we feel multithreaded construction of a
    // single buffer will be rare, and synchronisation overhead would be costly.


    // Each thread gets its own builder.
    thread_local ::flatbuffers::FlatBufferBuilder builder;

    // Reserved space for QUIC_BUFFER.
    constexpr auto k_QuicBufferSize = 16;
    [[maybe_unused]] constexpr auto k_QuicBufferAlignment = 8;

    constexpr std::uint8_t res [k_QuicBufferSize] = { 0xDE, 0xAD, 0xBE, 0xEF,
                                                      0xBA, 0xAD, 0xC0, 0xDE,
                                                      0xCA, 0xFE, 0xBA, 0xBE,
                                                      0xDE, 0xAD, 0xFA, 0xCE };

    // Align the buffer properly.
    builder.PreAlign(k_QuicBufferSize, k_QuicBufferAlignment);
    builder.PushBytes(res, sizeof(res));
    assert(builder.GetSize() == k_QuicBufferSize);
    return builder;
}
} // namespace mad::nexus
