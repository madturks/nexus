/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/nexus/quic_application.hpp>
#include <mad/nexus/result.hpp>

struct QUIC_API_TABLE;
struct QUIC_HANDLE;

extern std::weak_ptr<const ::QUIC_API_TABLE> msquic_api;

namespace mad::nexus {

/******************************************************
 * msquic-based quic_application
 ******************************************************/
class msquic_application : public quic_application {
public:
    /******************************************************
     * Create a quic_server with the registration & configuration
     * of the application.
     *
     * @return The new server
     ******************************************************/
    virtual result<std::unique_ptr<quic_server>> make_server() override;

    /******************************************************
     * Create a quic_client with the registration & configuration
     * of the application.
     *
     * @return The new client
     ******************************************************/
    virtual result<std::unique_ptr<quic_client>> make_client() override;

    /******************************************************
     * Get the QUIC_API_TABLE object pointer of the application.
     * @return const QUIC_API_TABLE*
     ******************************************************/
    virtual const QUIC_API_TABLE * api() const noexcept;

    /******************************************************
     * Get the QUIC_REGISTRATION object handle of the application
     * @return  QUIC_HANDLE*
     ******************************************************/
    virtual QUIC_HANDLE * registration() const noexcept;

    /******************************************************
     * Get the QUIC_CONFIGURATION object handle of the application
     * @return  QUIC_HANDLE*
     ******************************************************/
    virtual QUIC_HANDLE * configuration() const noexcept;

    /******************************************************
     * Destroy the msquic application object
     ******************************************************/
    virtual ~msquic_application() override;

protected:
    // Befriend the make_msquic_application to allow access to the private
    // constructor
    friend result<std::unique_ptr<quic_application>>
    make_msquic_application(const struct quic_configuration &);

    // Befriend the unit test class.
    friend struct tf_msquic_application;

    /******************************************************
     * Construct a new msquic application object.
     *
     * Prevent direct construction of the class and only
     * allow new instances through make_msquic_application.
     *
     * @param api_object
     ******************************************************/
    msquic_application(std::shared_ptr<const QUIC_API_TABLE> api_object,
                       std::shared_ptr<QUIC_HANDLE> registration,
                       std::shared_ptr<QUIC_HANDLE> configuration);

    /******************************************************
     * Ideally this should be wrapped with std::atomic but
     * libc++ is still lagging behind and does not support
     * it.
     ******************************************************/
    std::shared_ptr<const QUIC_API_TABLE> msquic_api;
    // The MSQUIC registration object.
    std::shared_ptr<QUIC_HANDLE> registration_ptr{};
    // The MSQUIC configuration object.
    std::shared_ptr<QUIC_HANDLE> configuration_ptr{};
};

} // namespace mad::nexus
