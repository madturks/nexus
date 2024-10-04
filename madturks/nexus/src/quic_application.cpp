#include <mad/nexus/quic_application.hpp>

namespace mad::nexus {
    quic_application::quic_application(const quic_configuration & cfg) : config(cfg) {}
    quic_application::~quic_application() = default; 
} // namespace mad::nexus
