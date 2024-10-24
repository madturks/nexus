/******************************************************
 * QUIC configuration type.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace mad::nexus {
/******************************************************
 * Credential paths.
 ******************************************************/
struct quic_credentials {
    /******************************************************
     * Path to the certificate which will be used to initialize TLS.
     ******************************************************/
    std::string certificate_path;

    /******************************************************
     * Path to the private key of the certificate
     ******************************************************/
    std::string private_key_path;
};

/******************************************************
 * The available QUIC implementation types.
 ******************************************************/
enum class e_quic_impl_type : std::uint32_t
{
    msquic
};

/******************************************************
 * Available QUIC roles.
 ******************************************************/
enum class e_role
{
    client,
    server
};

/******************************************************
 * Implementation-agnostic configuration values for QUIC
 ******************************************************/
struct quic_configuration {
private:
    const e_quic_impl_type impl_type_;
    const e_role role_;

public:
    /******************************************************
     * Construct a new quic configuration object
     *
     * @param impl_type The implementaton type
     * @param role The role
     ******************************************************/
    quic_configuration(e_quic_impl_type impl_type, e_role role) :
        impl_type_(impl_type), role_(role) {}

    /******************************************************
     * QUIC ALPN value
     ******************************************************/
    std::string alpn = { "test" };
    /******************************************************
     * The application name
     ******************************************************/
    std::string appname = { "test" };
    /******************************************************
     * The idle timeout period. The connections will be dropped
     * after this many of milliseconds of inactivity.
     ******************************************************/
    std::optional<std::chrono::milliseconds> idle_timeout{ std::nullopt };

    std::optional<std::chrono::milliseconds> keep_alive_interval{
        std::nullopt
    };
    /******************************************************
     * QUIC crypto credentials
     ******************************************************/
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
