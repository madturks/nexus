/******************************************************
 * MSQUIC-based client implementation.
 *
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_client.hpp>
#include <mad/nexus/quic_connection.hpp>

namespace mad::nexus {

/******************************************************
 * QUIC client implementation using MSQUIC.
 *
 * The class implements the quic_client interface. The
 * common interface between msquic_client and msquic_server
 * are implemented in the msquic_base class. This is why
 * quic_base class is inherited using virtual inheritance.
 *
 ******************************************************/
class msquic_client final : public quic_client,
                            public msquic_base {
public:
    /******************************************************
     * Destroy the msquic client object
     ******************************************************/
    virtual ~msquic_client() override;

    /******************************************************
     * Connect to the target endpoint.
     *
     * @param target Target hostname or IP address
     * @param port The port number
     * @return
     ******************************************************/
    virtual auto connect(std::string_view target,
                         std::uint16_t port) -> result<> override;

    /******************************************************
     * Disconnect from the currently connected endpoint.
     *
     * @return true if disconnected, error code on failure
     ******************************************************/
    virtual auto disconnect() -> result<> override;

private:
    /******************************************************
     * MSQUIC client unit tests
     ******************************************************/
    friend struct tf_msquic_client;

    /******************************************************
     * The callbacks are initiated by the client and they
     * need to be able to access the connection_ctx to update
     * the connection state.
     ******************************************************/
    friend struct MsQuicClientCallbacks;

    /******************************************************
     * msquic_application::make_client is the factory method
     * that creates the msquic_client objects. It's the only
     * way for a API consumer to create a msquic client.
     ******************************************************/
    friend result<std::unique_ptr<quic_client>>
    msquic_application::make_client();

    /******************************************************
     * Construct a new msquic client object.
     *
     * The constructor is marked private because we don't
     * want clients to be able to directly instantiate this
     * class.
     *
     * @param app The MSQUIC application
     ******************************************************/
    using msquic_base::msquic_base;

    /******************************************************
     * The client's connection object.
     ******************************************************/
    std::unique_ptr<struct connection> connection{};
};
} // namespace mad::nexus
