#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

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

enum class e_quic_impl_type : std::uint32_t
{
    msquic
};

enum class e_role
{
    client,
    server
};

struct quic_configuration {

    quic_configuration(e_quic_impl_type impl_type, e_role role) :
        impl_type_(impl_type), role_(role) {}

    std::string alpn = { "test" };
    std::string appname = { "test" };
    std::optional<std::chrono::milliseconds> idle_timeout;
    quic_credentials credentials;
    std::uint16_t udp_port_number;
    std::uint32_t stream_receive_window{ 8192 };
    std::uint32_t stream_receive_buffer{ 4096 };

    e_role role() const {
        return role_;
    }

    e_quic_impl_type impl_type() const {
        return impl_type_;
    }

private:
    const e_quic_impl_type impl_type_;
    const e_role role_;
};

} // namespace mad::nexus
