/******************************************************
 * QUIC server interface
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

/******************************************************
 * The QUIC server interface.
 ******************************************************/
class quic_server : virtual public quic_base {
public:
    /******************************************************
     * Destroy the quic server object
     ******************************************************/
    virtual ~quic_server() override;

    /******************************************************
     * Start listening
     * @return  Listen result
     ******************************************************/
    [[nodiscard]] virtual auto listen() -> result<> = 0;
};

} // namespace mad::nexus
