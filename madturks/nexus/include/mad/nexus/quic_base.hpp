#pragma once

#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_configuration.hpp>


#include <expected>
#include <system_error>
#include <flatbuffers/detached_buffer.h>
#include <flatbuffers/flatbuffer_builder.h>

namespace mad::nexus {

    struct quic_connection {};

    struct quic_stream {};

    class quic_base {
    public:
        quic_base(quic_configuration cfg) : config(cfg) {}

        [[nodiscard]] virtual std::error_code init()                                                                                  = 0;
        [[nodiscard]] virtual auto open_stream(connection_context * cctx,
                                               stream_data_callback_t data_callback) -> std::expected<stream_context *, std::error_code> = 0;
        [[nodiscard]] virtual auto close_stream(stream_context * sctx) -> std::error_code                                             = 0;
        [[nodiscard]] virtual auto send(stream_context * sctx, flatbuffers::DetachedBuffer buf) -> std::size_t                        = 0;

        struct callback_table {
            connection_callback_t on_connected;
            connection_callback_t on_disconnected;
            stream_data_callback_t on_stream_open;
        } callbacks;

        auto get_message_builder() -> flatbuffers::FlatBufferBuilder &;

        virtual ~quic_base();

    protected:
        quic_configuration config;
    };
} // namespace mad::nexus
