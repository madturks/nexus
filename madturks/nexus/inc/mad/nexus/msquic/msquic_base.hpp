#pragma once

#include <mad/log_printer.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_callback_types.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>

namespace mad::nexus {

/**
 * Base class for msquic client an the server
 */
class msquic_base : virtual public quic_base,
                    public log_printer {
public:
    msquic_base();

    std::error_code init() override final;
    auto open_stream(connection_context & cctx,
                     std::optional<stream_data_callback_t> data_callback)
        -> open_stream_result override final;
    auto close_stream(stream_context & sctx) -> std::error_code override final;
    auto send(stream_context & sctx,
              send_buffer<true> buf) -> std::size_t override final;

    virtual ~msquic_base() override;
};

} // namespace mad::nexus
