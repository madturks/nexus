/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/nexus/callback.hpp>

#include <cstdint>
#include <span>

namespace mad::nexus {

/******************************************************
 * The QUIC callback types.
 ******************************************************/
enum class callback_type
{
    connected,
    disconnected,
    stream_start,
    stream_end,
    stream_data
};

/******************************************************
 * Connection callback type.
 *
 * Used for connected / disconnected.
 ******************************************************/
using connection_callback_t = callback<void(struct connection &)>;

/******************************************************
 * Stream callback type.
 *
 * Used for stream start / stream end.
 ******************************************************/
using stream_callback_t = callback<void(struct stream &)>;

/******************************************************
 * Stream data callback type.
 ******************************************************/
using stream_data_callback_t =
    callback<std::size_t(std::span<const std::uint8_t>)>;
} // namespace mad::nexus
