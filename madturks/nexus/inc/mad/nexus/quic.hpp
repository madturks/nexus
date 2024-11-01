/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/nexus/quic_application.hpp>
#include <mad/nexus/result.hpp>

#include <memory>

namespace mad::nexus {

/******************************************************
 * Create a new quic application with the given cfg.
 *
 * @param cfg The configuration
 * @return The quic application
 ******************************************************/
result<std::unique_ptr<quic_application>>
make_quic_application(const struct quic_configuration & cfg);
} // namespace mad::nexus
