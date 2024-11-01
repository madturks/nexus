/******************************************************
 * QUIC connection type.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/nexus/handle_carrier.hpp>
#include <mad/nexus/handle_context_container.hpp>

namespace mad::nexus {

/******************************************************
 * Implementation agnostic type representing a QUIC
 * connection.
 ******************************************************/
struct connection : public serial_number_carrier,
                    handle_carrier,
                    handle_context_container<stream> {

    using handle_carrier::handle_carrier;
};
} // namespace mad::nexus
