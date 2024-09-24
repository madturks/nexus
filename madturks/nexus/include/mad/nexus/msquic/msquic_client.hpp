#pragma once

#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_client.hpp>
#include <mad/nexus/quic_connection_context.hpp>

namespace mad::nexus {

    class msquic_client final : public quic_client,
                                public msquic_base {
    public:
        using msquic_base::msquic_base;
        virtual ~msquic_client() override;

        [[nodiscard]] virtual std::error_code connect(std::string_view target, std::uint16_t port) override;

        [[nodiscard]] static std::unique_ptr<quic_client> make(quic_configuration cfg) {
            return std::unique_ptr<msquic_client>(new msquic_client(cfg));
        }

        std::unique_ptr<connection_context> connection_ctx;
    };
} // namespace mad::nexus
