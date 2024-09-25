#pragma once

#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_configuration.hpp>

#include <expected>
#include <system_error>

#include <flatbuffers/flatbuffer_builder.h>

namespace mad::nexus {

    struct quic_connection {};

    struct quic_stream {};

    struct send_buffer {
        std::uint8_t * buf;
        std::size_t offset;
        std::size_t buf_size;
        std::size_t used;
    };

    class quic_base {
    public:
        quic_base(quic_configuration cfg) : config(cfg) {}

        [[nodiscard]] virtual std::error_code init() = 0;
        [[nodiscard]] virtual auto open_stream(connection_context * cctx, stream_data_callback_t data_callback)
            -> std::expected<stream_context *, std::error_code>                                = 0;
        [[nodiscard]] virtual auto close_stream(stream_context * sctx) -> std::error_code      = 0;
        [[nodiscard]] virtual auto send(stream_context * sctx, send_buffer buf) -> std::size_t = 0;

        struct callback_table {
            connection_callback_t on_connected;
            connection_callback_t on_disconnected;
            stream_data_callback_t on_stream_open;
        } callbacks;

        template <typename MT, typename F>
        auto build_message(F && lambda) -> send_buffer {
            auto & fbb = get_message_builder();
            MT msgt{fbb};
            lambda(msgt);
            auto obj = msgt.Finish();
            fbb.Finish(obj);

            mad::nexus::send_buffer buf{};

            buf.used = fbb.GetSize();
            buf.buf  = fbb.ReleaseRaw(buf.buf_size, buf.offset);
            return buf;
        }

        auto get_message_builder() -> flatbuffers::FlatBufferBuilder &;

        virtual ~quic_base();

    protected:
        quic_configuration config;
    };
} // namespace mad::nexus
