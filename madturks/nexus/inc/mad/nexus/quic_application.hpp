#pragma once

#include <mad/nexus/quic_client.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_server.hpp>

#include <memory>

namespace mad::nexus {

/**
 * Generic QUIC application interface.
 *
 * The implementations MUST inherit from this interface and
 * implement the make_server and make_client methods.
 */
class quic_application {
public:
    /**
     * Create a new server using application's details (configuration
     - and other implementation specific details)
     */
    [[nodiscard]] virtual std::unique_ptr<quic_server> make_server() = 0;

    /**
     * Create a new client using application's details (configuration
     - and other implementation specific details)
     */
    [[nodiscard]] virtual std::unique_ptr<quic_client> make_client() = 0;

    /**
     * Get the configuration associated with the application
     *
     * @return Immutable ref to the configuration
     */
    [[nodiscard]] inline const quic_configuration &
    get_config() const noexcept {
        return config;
    }

    virtual ~quic_application();

protected:
    /**
     * Prohibit direct construction.
     */
    quic_application(const quic_configuration & cfg);

    /**
     * The application's configuration.
     *
     * This configuration will be used for producing new
     * clients and servers.
     */
    quic_configuration config;
};
} // namespace mad::nexus
