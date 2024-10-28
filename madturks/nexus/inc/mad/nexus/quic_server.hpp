/******************************************************
 * QUIC server interface
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/result.hpp>

#include <string_view>

namespace mad::nexus {

/******************************************************
 * The QUIC server interface.
 ******************************************************/
class quic_server : virtual public quic_base,
                    public value_container<connection> {
public:
    /******************************************************
     * Destroy the quic server object
     ******************************************************/
    virtual ~quic_server() override;

    /******************************************************
     * Start listening for connections
     *
     * @param [in] alpn Which alpn
     * @param [in] port Which port
     * @return  Listen result
     ******************************************************/
    [[nodiscard]] virtual auto listen(std::string_view alpn,
                                      std::uint16_t port) -> result<> = 0;
};

} // namespace mad::nexus
