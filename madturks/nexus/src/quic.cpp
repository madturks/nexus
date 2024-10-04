#include <mad/nexus/quic.hpp>

namespace mad::nexus {
    extern std::unique_ptr<quic_application> make_msquic_application(const struct quic_configuration &);

    std::unique_ptr<quic_application> make_quic_application(e_quic_impl_type impl_type, const struct quic_configuration & cfg) {
        switch (impl_type) {
            using enum e_quic_impl_type;
            case msquic:
                return make_msquic_application(cfg);
        }
        throw std::runtime_error{"No such implementation"};
    }

} // namespace mad::nexus
