#pragma once

#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

class quic_server : virtual public quic_base {
public:
    virtual ~quic_server() override;
    [[nodiscard]] virtual auto listen() -> result<> = 0;
};

} // namespace mad::nexus
