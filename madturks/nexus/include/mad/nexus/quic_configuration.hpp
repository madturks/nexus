#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <optional>

namespace mad::nexus {
    struct quic_credentials {
        /**
         * Path to the certificate which will be used to initialize TLS.
         */
        std::string certificate_path;

        /**
         * Path to the private key of the certificate
         */
        std::string private_key_path;
    };

    struct quic_configuration {
        std::string alpn    = {"test"};
        std::string appname = {"test"};
        std::optional<std::chrono::milliseconds> idle_timeout;
        quic_credentials credentials;
        std::uint16_t udp_port_number;
    };

} // namespace mad::nexus
