#pragma once

#include <mad/log_printer.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_callback_types.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_stream.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

/**
 * Base class for msquic client an the server
 */
class msquic_base : virtual public quic_base,
                    public log_printer {
public:
    msquic_base();

    auto init() -> result<> override;
    auto open_stream(connection & cctx,
                     std::optional<stream_data_callback_t> data_callback)
        -> result<std::reference_wrapper<stream>> override;
    auto close_stream(stream & sctx) -> result<> override;
    auto send(stream & sctx,
              send_buffer<true> buf) -> result<std::size_t> override;

    virtual ~msquic_base() override;
};

} // namespace mad::nexus
