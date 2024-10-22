#pragma once

#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_client.hpp>
#include <mad/nexus/quic_connection.hpp>

namespace mad::nexus {

class msquic_client final : public quic_client,
                            public msquic_base {
public:
    friend struct MsQuicClientCallbacks;
    virtual ~msquic_client() override;

    virtual auto connect(std::string_view target,
                         std::uint16_t port) -> result<> override;

    std::unique_ptr<connection> connection_ctx{};

    const msquic_application & application;

private:
    friend std::unique_ptr<quic_client> msquic_application::make_client();
    msquic_client(const msquic_application & app);

    std::shared_ptr<void> connection_ptr{};
};
} // namespace mad::nexus
