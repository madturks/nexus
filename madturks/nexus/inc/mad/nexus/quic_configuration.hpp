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
private:
    const e_quic_impl_type impl_type_;
    const e_role role_;

public:
    quic_configuration(e_quic_impl_type impl_type, e_role role) :
        impl_type_(impl_type), role_(role) {}

    std::string alpn = { "test" };
    std::string appname = { "test" };
    std::optional<std::chrono::milliseconds> idle_timeout{ std::nullopt };
    std::optional<std::chrono::milliseconds> keep_alive_interval{
        std::nullopt
    };
    quic_credentials credentials{};
    std::uint32_t stream_receive_window{ 8192 };
    std::uint32_t stream_receive_buffer{ 4096 };
    std::uint16_t udp_port_number{ 6666 };

    e_role role() const {
        return role_;
    }

    e_quic_impl_type impl_type() const {
        return impl_type_;
    }
};

} // namespace mad::nexus
