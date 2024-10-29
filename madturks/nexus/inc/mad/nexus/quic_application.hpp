/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/nexus/quic_client.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_server.hpp>

#include <memory>

namespace mad::nexus {

/******************************************************
 * Generic QUIC application interface.
 *
 * The implementations MUST inherit from this interface and
 * implement the make_server and make_client methods.
 ******************************************************/
class quic_application {
public:
    /******************************************************
     * Create a new server using application's details (configuration
     * and other implementation specific details)
     ******************************************************/
    [[nodiscard]] virtual result<std::unique_ptr<quic_server>>
    make_server() = 0;

    /******************************************************
     * Create a new client using application's details (configuration
     * and other implementation specific details)
     ******************************************************/
    [[nodiscard]] virtual result<std::unique_ptr<quic_client>>
    make_client() = 0;

    /******************************************************
     * Destroy the quic application object
     ******************************************************/
    virtual ~quic_application();

protected:
    /******************************************************
     * Prohibit direct construction.
     ******************************************************/
    quic_application();
};
} // namespace mad::nexus
