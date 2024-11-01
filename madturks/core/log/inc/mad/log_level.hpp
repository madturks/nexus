/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <cstdint>

namespace mad {

/**
 * Direct mapping to spdlog log level values.
 */
enum class log_level : std::uint8_t
{
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,
    off = 6,
    max
};
} // namespace mad
