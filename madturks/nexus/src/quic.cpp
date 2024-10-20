#include <mad/nexus/quic.hpp>

#include <format>

namespace mad::nexus {
extern std::unique_ptr<quic_application>
make_msquic_application(const struct quic_configuration &);

std::unique_ptr<quic_application>
make_quic_application(const struct quic_configuration & cfg) {
    switch (cfg.impl_type()) {
        using enum e_quic_impl_type;
        case msquic:
            return make_msquic_application(cfg);
    }

    throw std::runtime_error{ std::format(
        "No such implementation as {}", std::to_underlying(cfg.impl_type())) };
}

} // namespace mad::nexus
