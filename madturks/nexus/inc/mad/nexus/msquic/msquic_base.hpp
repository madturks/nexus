#pragma once

#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_callback_types.hpp>

#include <mad/log_printer.hpp>

namespace mad::nexus {

    class msquic_base : virtual public quic_base,
                        public log_printer {
    public:
        msquic_base();

        [[nodiscard]] virtual std::error_code init() override final;
        [[nodiscard]] virtual auto open_stream(connection_context * cctx, std::optional<stream_data_callback_t> data_callback)
            -> std::expected<stream_context *, std::error_code> override final;
        [[nodiscard]] virtual auto close_stream(stream_context * sctx) -> std::error_code override final;
        [[nodiscard]] virtual auto send(stream_context * sctx, send_buffer buf) -> std::size_t override final;

        virtual ~msquic_base() override;
    };

} // namespace mad::nexus
