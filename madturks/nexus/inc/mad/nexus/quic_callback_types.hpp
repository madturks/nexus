#pragma once

#include <mad/nexus/quic_callback_function.hpp>

#include <cstdint>
#include <span>

namespace mad::nexus {

enum class callback_type
{
    connected,
    disconnected,
    stream_start,
    stream_end,
    stream_data
};

using connection_callback_t = quic_callback_function<void(struct connection &)>;

using stream_callback_t = quic_callback_function<void(struct stream &)>;
using stream_data_callback_t =
    quic_callback_function<std::size_t(std::span<const std::uint8_t>)>;
} // namespace mad::nexus
