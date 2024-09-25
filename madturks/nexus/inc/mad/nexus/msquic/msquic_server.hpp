#pragma once

#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_server.hpp>
#include <mad/concurrent.hpp>
#include <mad/nexus/quic_connection_context.hpp>

#include <memory>
#include <system_error>
#include <expected>

namespace mad::nexus {

    class msquic_server final : virtual public quic_server,
                                virtual public msquic_base {
    public:
        using msquic_base::msquic_base;
        virtual ~msquic_server() override;

        [[nodiscard]] virtual std::error_code listen() override;

        [[nodiscard]] static std::unique_ptr<quic_server> make(quic_configuration cfg) {
            return std::unique_ptr<msquic_server>(new msquic_server(cfg));
        }

        [[nodiscard]] auto & connections() {
            return connection_map;
        }

    private:
        mad::concurrent<std::unordered_map<void *, connection_context>> connection_map;
    };

} // namespace mad::nexus
