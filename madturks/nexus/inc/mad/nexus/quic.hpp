#pragma once

#include <mad/nexus/quic_application.hpp>

#include <memory>

namespace mad::nexus {

std::unique_ptr<quic_application>
make_quic_application(const struct quic_configuration & cfg);
} // namespace mad::nexus
