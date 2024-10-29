/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/nexus/quic.hpp>

namespace mad::nexus {

extern result<std::unique_ptr<quic_application>>
make_msquic_application(const struct quic_configuration &);

result<std::unique_ptr<quic_application>>
make_quic_application(const struct quic_configuration & cfg) {
    switch (cfg.impl_type()) {
        using enum e_quic_impl_type;
        case msquic:
            return make_msquic_application(cfg);
    }

    return std::unexpected(quic_error_code::no_such_implementation);
}

} // namespace mad::nexus
