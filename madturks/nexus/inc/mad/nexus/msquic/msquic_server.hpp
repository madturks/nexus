/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/nexus/handle_context_container.hpp>
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_server.hpp>

#include <memory>

struct QUIC_HANDLE;

namespace mad::nexus {

class msquic_server final : public quic_server,
                            public msquic_base {

    friend struct MsQuicServerCallbacks;

    std::shared_ptr<QUIC_HANDLE> listener{};

public:
    virtual ~msquic_server() override;

    virtual result<> listen(std::string_view alpn, std::uint16_t port) override;

private:
    friend struct tf_msquic_server;
    friend result<std::unique_ptr<quic_server>>
    msquic_application::make_server();
    /**
     * Construct a new msquic server object
     *
     * @param app MSQUIC application
     */
    using msquic_base::msquic_base;
};

} // namespace mad::nexus
