#pragma once

#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>

namespace mad::nexus {

class quic_server : virtual public quic_base {
public:
    virtual ~quic_server() override;

    [[nodiscard]] virtual std::error_code listen() = 0;
};

} // namespace mad::nexus
