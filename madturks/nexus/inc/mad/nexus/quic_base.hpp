#pragma once

#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>

#include <flatbuffers/flatbuffer_builder.h>

#include <expected>
#include <system_error>

namespace mad::nexus {

struct quic_connection {};

struct quic_stream {};

struct send_buffer {
    std::uint8_t * buf;
    std::size_t offset;
    std::size_t buf_size;
};

class quic_base {
public:
    using open_stream_result =
        std::expected<std::reference_wrapper<stream_context>, std::error_code>;

    quic_base() {}

    /**
     * Initialize the needed resources
     *
     * @return std::error_code indicating the status.
     */
    [[nodiscard]] virtual std::error_code init() = 0;

    /**
     * Open a new stream in given connection context (@p cctx)
     *
     * @param cctx The owner connection context
     * @param data_callback The function to call when stream data arrives
     * (optional).
     *
     * @return stream_context* when successful, std::error_code otherwise.
     */
    [[nodiscard]] virtual auto
    open_stream(connection_context & cctx,
                std::optional<stream_data_callback_t> data_callback)
        -> open_stream_result = 0;

    /**
     * Close the given stream.
     *
     * @param sctx Stream's context object.
     *
     * @return std::error_code Error code indicating the close status.
     */
    [[nodiscard]] virtual auto
    close_stream(stream_context & sctx) -> std::error_code = 0;

    /**
     * Send data to an already open stream
     *
     * @param sctx The stream's context ojbect
     * @param buf The buffer to send
     *
     * @return std::size_t The amount of bytes successfully written to the send
     * buffer.
     */
    [[nodiscard]] virtual auto send(stream_context & sctx,
                                    send_buffer buf) -> std::size_t = 0;

    /**
     * The callback functions for user application.
     */
    struct callback_table {
        /**
         * Invoked when a new connection is established.
         */
        connection_callback_t on_connected;
        /**
         * Invoked when a connection is disconnected and about
         * to be destroyed.
         */
        connection_callback_t on_disconnected;
        /**
         * Invoked when data is received from a stream.
         */
        stream_data_callback_t on_stream_data_received;
    } callbacks;

    template <typename F>
    auto build_message(F && lambda) -> send_buffer {
        auto & fbb = get_message_builder();
        const std::uint32_t size_before = fbb.GetSize();
        auto root_offset = lambda(fbb);
        fbb.Finish(root_offset);
        const std::uint32_t size_after = fbb.GetSize();
        const std::uint32_t size = size_after - size_before;
        fbb.PushBytes(reinterpret_cast<const std::uint8_t *>(&size),
                      sizeof(std::uint32_t));
        mad::nexus::send_buffer buf{};
        buf.buf = fbb.ReleaseRaw(buf.buf_size, buf.offset);
        return buf;
    }

    auto get_message_builder() -> flatbuffers::FlatBufferBuilder &;

    virtual ~quic_base();
};
} // namespace mad::nexus
