#pragma once

#include <mad/nexus/quic_application.hpp>

struct QUIC_API_TABLE;
struct QUIC_HANDLE;

namespace mad::nexus {

/**
 * quic_application implementation using MSQUIC
 */
class msquic_application : public quic_application {
public:
    /**
     * Create a QUIC server of msquic implementation.
     */
    virtual std::unique_ptr<quic_server> make_server() override;

    /**
     * Create a QUIC client of msquic implementation.
     */
    virtual std::unique_ptr<quic_client> make_client() override;

    const QUIC_API_TABLE * api() const noexcept;

    /**
     *
     *
     * @return * MsQuicRegistration const&
     */
    QUIC_HANDLE * registration() const noexcept;
    /**
     * MsQuicConfiguration object of the application.
     */
    QUIC_HANDLE * configuration() const noexcept;

    virtual ~msquic_application() override;

private:
    // Befriend the make_msquic_application to allow access to the private
    // constructor
    friend std::unique_ptr<quic_application>
    make_msquic_application(const struct quic_configuration &);

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
