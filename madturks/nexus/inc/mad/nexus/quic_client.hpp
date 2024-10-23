#pragma once

#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

class quic_client : virtual public quic_base {
public:
    virtual ~quic_client() override;

    /******************************************************
     * Connect to the target endpoint.
     *
     * @param target Target hostname or IP address
     * @param port The port number
     * @return
     ******************************************************/
    [[nodiscard]] virtual auto connect(std::string_view target,
                                       std::uint16_t port) -> result<> = 0;

    /******************************************************
     * Disconnect from the currently connected endpoint.
     *
     * @return true if disconnected, error code on failure
     ******************************************************/
    [[nodiscard]] virtual auto disconnect() -> result<> = 0;
};
} // namespace mad::nexus
