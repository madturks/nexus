#pragma once

#include <mad/nexus/quic_application.hpp>

#include <memory>

namespace mad::nexus {

enum class e_quic_impl_type : std::uint32_t
{
    msquic
};

std::unique_ptr<quic_application>
make_quic_application(e_quic_impl_type impl_type,
                      const struct quic_configuration & cfg);
} // namespace mad::nexus
