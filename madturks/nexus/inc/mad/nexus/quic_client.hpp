#pragma once

#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

class quic_client : virtual public quic_base {
public:
    virtual ~quic_client() override;

    // TODO: Resumption ticket logic
    [[nodiscard]] virtual auto connect(std::string_view target,
                                       std::uint16_t port) -> result<> = 0;
};
} // namespace mad::nexus
