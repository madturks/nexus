#pragma once

#include <mad/macro>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/send_buffer.hpp>

#include <flatbuffers/flatbuffer_builder.h>

#include <expected>
#include <system_error>

namespace mad::nexus {

class quic_base {
public:
    using open_stream_result =
        std::expected<std::reference_wrapper<stream>, std::error_code>;

    quic_base() = default;

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
    open_stream(connection & cctx,
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
    close_stream(stream & sctx) -> std::error_code = 0;

    /**
     * Send data to an already open stream
     *
     * @param sctx The stream's context ojbect
     * @param buf The buffer to send
     *
     * @return std::size_t The amount of bytes successfully written to the send
     * buffer.
     */
    [[nodiscard]] virtual auto send(stream & sctx,
                                    send_buffer<true> buf) -> std::size_t = 0;

    /**
     * Register a callback function for a specific event.
     *
     * @param args The callback function, and the user-defined context pointer
     */
    template <callback_type T, typename... Args>
    void register_callback(Args &&... args) noexcept {
        decltype(auto) callback =
            quic_callback_function{ std::forward<Args>(args)... };

        if constexpr (T == callback_type::connected) {
            static_assert(std::same_as<decltype(callback),
                                       decltype(callbacks.on_connected)>,
                          "Given callback function's signature does not match "
                          "the target callback.");
            callbacks.on_connected = callback;
        } else if constexpr (T == callback_type::disconnected) {
            static_assert(std::same_as<decltype(callback),
                                       decltype(callbacks.on_disconnected)>,
                          "Given callback function's signature does not match "
                          "the target callback.");
            callbacks.on_disconnected = callback;
        } else if constexpr (T == callback_type::stream_start) {
            static_assert(std::same_as<decltype(callback),
                                       decltype(callbacks.on_stream_start)>,
                          "Given callback function's signature does not match "
                          "the target callback.");
            callbacks.on_stream_start = callback;
        } else if constexpr (T == callback_type::stream_end) {
            static_assert(std::same_as<decltype(callback),
                                       decltype(callbacks.on_stream_close)>,
                          "Given callback function's signature does not match "
                          "the target callback.");
            callbacks.on_stream_close = callback;
        } else if constexpr (T == callback_type::stream_data) {
            static_assert(
                std::same_as<decltype(callback),
                             decltype(callbacks.on_stream_data_received)>,
                "Given callback function's signature does not match the target "
                "callback.");
            callbacks.on_stream_data_received = callback;
        } else if consteval {
            static_assert(0, "Unhandled callback type");
        }
    }

    /**
     * Build a flatbuffers message for sending.
     *
     * The @p callable is an user-provided callable that takes
     * the ::flatbuffers::builder& as argument. The user is expected
     * to use this builder to construct their message. The user returns
     * the constructed message's Finish() result as a return value from
     * the lambda.
     *
     * The build_message function automatically calculate and add the size
     * of the user's message to the payload.
     *
     * The resulting message is released as a raw buffer into send_buffer
     * structure. The send_buffer object by default deallocates the buffer
     * upon destruction, unless .auto_cleanup is set to false.
     *
     * @tparam F Function type
     * @param [in] callable Callable that builds the user message
     *
     * @return send_buffer Send buffer containing the resulting message
     */
    template <typename F>
    static auto build_message(F && callable) -> send_buffer<true> {
        // Each thread gets its own builder.
        thread_local ::flatbuffers::FlatBufferBuilder fbb;

        // Align the buffer properly.
        fbb.PreAlign(send_buffer<true>::k_QuicBufStructSize,
                     send_buffer<true>::k_QuicBufStructAlignment);

        // Push the sentinel value to the buffer. Having a sentinel value in
        // place allows the code to verify we're indeed placed the buffer at the
        // right position and reading from the correct position.
        fbb.PushBytes(send_buffer<true>::quic_buf_sentinel,
                      sizeof(send_buffer<true>::quic_buf_sentinel));
        MAD_ENSURES(fbb.GetSize() == send_buffer<true>::k_QuicBufStructSize);

        const std::uint32_t size_before = fbb.GetSize();
        auto root_offset = callable(fbb);
        fbb.Finish(root_offset);
        const std::uint32_t size_after = fbb.GetSize();
        const std::uint32_t size = size_after - size_before;
        // Push the size to the payload
        fbb.PushBytes(reinterpret_cast<const std::uint8_t *>(&size),
                      sizeof(std::uint32_t));

        // Release the built message into a send_buffer
        send_buffer<true> buf{};
        buf.buf = fbb.ReleaseRaw(buf.buf_size, buf.offset);
        fbb.Clear();
        return buf;
    }

    virtual ~quic_base();

protected:
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
         * Invoked when a new stream is started.
         */
        stream_callback_t on_stream_start;

        /**
         * Invoked when a stream is about to be destroyed.
         */
        stream_callback_t on_stream_close;

        /**
         * Invoked when data is received from a stream.
         */
        stream_data_callback_t on_stream_data_received;
    } callbacks;
};
} // namespace mad::nexus
