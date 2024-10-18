#pragma once

#include <mad/nexus/quic_application.hpp>

// Forward declarations
struct MsQuicConfiguration;
struct MsQuicRegistration;

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

    /**
     *
     *
     * @return * MsQuicRegistration const&
     */
    const MsQuicRegistration & registration() const;
    /**
     * MsQuicConfiguration object of the application.
     */
    const MsQuicConfiguration & configuration() const;

    virtual ~msquic_application() override;

private:
    // Befriend the make_msquic_application to allow access to the private
    // constructor
    friend std::unique_ptr<quic_application>
    make_msquic_application(const struct quic_configuration &);
    /**
     * Prevent direct construction of the class and only
     * allow new instances through make_msquic_application.
     */
    msquic_application(const struct quic_configuration & cfg);

    std::shared_ptr<void> api_ptr;

    // The MSQUIC registration object.
    std::shared_ptr<MsQuicRegistration> registration_ptr;
    // The MSQUIC configuration object.
    std::shared_ptr<MsQuicConfiguration> configuration_ptr;
};

} // namespace mad::nexus
