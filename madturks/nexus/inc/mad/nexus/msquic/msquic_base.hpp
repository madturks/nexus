#pragma once

#include <mad/log_printer.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_callback_types.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/result.hpp>

namespace mad::nexus {

/**
 * Base class for msquic client an the server
 */
class msquic_base : virtual public quic_base,
                    public log_printer {
public:
    msquic_base();

    auto init() -> result<> override final;
    auto open_stream(connection & cctx,
                     std::optional<stream_data_callback_t> data_callback)
        -> result<std::reference_wrapper<stream>> override final;
    auto close_stream(stream & sctx) -> result<> override final;
    auto send(stream & sctx,
              send_buffer<true> buf) -> result<std::size_t> override final;

    virtual ~msquic_base() override;
};

} // namespace mad::nexus
