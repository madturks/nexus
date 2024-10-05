#pragma once

#include <mad/nexus/quic_callback_function.hpp>
#include <span>
#include <cstdint>

namespace mad::nexus {
    using connection_callback_t  = quic_callback_function<void(struct connection_context &)>;
    using stream_data_callback_t = quic_callback_function<std::size_t(std::span<const std::uint8_t>)>;
} // namespace mad::nexus
