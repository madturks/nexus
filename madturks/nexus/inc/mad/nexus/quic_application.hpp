#pragma once

#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_server.hpp>
#include <mad/nexus/quic_client.hpp>

#include <memory>

namespace mad::nexus {
    class quic_application {
    protected:
        quic_configuration config;

    public:
        virtual std::unique_ptr<quic_server> make_server() = 0;
        virtual std::unique_ptr<quic_client> make_client() = 0;
        virtual ~quic_application();

        inline const quic_configuration & get_config() const noexcept {
            return config;
        }

    protected:
        quic_application(const quic_configuration & cfg);
    };
} // namespace mad::nexus
