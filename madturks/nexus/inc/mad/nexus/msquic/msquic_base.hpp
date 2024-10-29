/******************************************************
 * Base class type for msquic server / client classes.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/log_printer.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_callback_types.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_stream.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

/******************************************************
 * Base class for msquic client & server.
 *
 * This class implements the common interface functions listed in quic_base that
 * is shareed between the msquic_server and msquic_client.
 ******************************************************/
class msquic_base : virtual public quic_base,
                    public log_printer {
public:
    auto open_stream(connection & cctx,
                     std::optional<stream_data_callback_t> data_callback)
        -> result<std::reference_wrapper<stream>> override;
    auto close_stream(stream & sctx) -> result<> override;
    auto send(stream & sctx,
              send_buffer<true> buf) -> result<std::size_t> override;

    virtual ~msquic_base() override;

protected:
    msquic_base(const class msquic_application & app);
    /******************************************************
     * The application that client belongs to.
     ******************************************************/
    const class msquic_application & application;
};

} // namespace mad::nexus
