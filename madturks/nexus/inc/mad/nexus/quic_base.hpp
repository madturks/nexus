#pragma once

#include <mad/macro>
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

/**
 * It's flatbuffers::detachedbuffer but:
 *
 * - without a allocator
 * - with a toggle to control the deallocation
 *   on destruction
 *
 * @tparam AutoCleanup Whether to destroy the buffer
 * upon destruction or not.
 */
template <bool AutoCleanup = true>
struct send_buffer {
    static constexpr std::size_t k_QuicBufStructSize = 16;
    [[maybe_unused]] static constexpr auto k_QuicBufStructAlignment = 8;
    /**
     * Sentinel value used as a placeholder for QUIC_BUFFER's space
     * until the real one is written.
     */
    [[maybe_unused]] constexpr static std::uint8_t
        quic_buf_sentinel [k_QuicBufStructSize] = { 0xDE, 0xAD, 0xBE, 0xEF,
                                                    0xBA, 0xAD, 0xC0, 0xDE,
                                                    0xCA, 0xFE, 0xBA, 0xBE,
                                                    0xDE, 0xAD, 0xFA, 0xCE };
    std::uint8_t * buf{ nullptr };
    std::size_t offset{ 0 };
    std::size_t buf_size{ 0 };

    send_buffer() {}

    send_buffer(const send_buffer &) = delete;
    send_buffer & operator=(const send_buffer &) = delete;
    send_buffer & operator=(send_buffer &&) = delete;

    auto size() const noexcept {
        return buf_size - offset;
    }

    std::span<std::uint8_t> quic_buffer_span() noexcept {
        MAD_EXPECTS(buf);
        MAD_EXPECTS(buf_size >= k_QuicBufStructSize);
        MAD_EXPECTS(size() >= k_QuicBufStructSize);
        auto ptr{ buf + buf_size - k_QuicBufStructSize };
        MAD_ENSURES(ptr >= buf && ptr <= (buf + buf_size));

        MAD_ENSURES(0 == std::memcmp(ptr, quic_buf_sentinel,
                                     sizeof(quic_buf_sentinel)));

        return { ptr, k_QuicBufStructSize };
    }

    std::uint32_t encoded_data_size() const noexcept {
        MAD_EXPECTS(size() >= sizeof(std::uint32_t));
        return *reinterpret_cast<const std::uint32_t *>(buf + offset);
    }

    std::span<std::uint8_t> data_span() const noexcept {
        MAD_EXPECTS(buf);
        MAD_EXPECTS(offset <= buf_size);
        MAD_EXPECTS(size() >= k_QuicBufStructSize);
        auto ptr{ buf + offset };
        auto sz{ size() - k_QuicBufStructSize };
        MAD_ENSURES(ptr >= buf && ptr <= (buf + buf_size));
        MAD_ENSURES(sz == encoded_data_size() + sizeof(std::uint32_t));
        return { ptr, sz };
    }

    template <bool B>
    send_buffer(send_buffer<B> && other) noexcept {
        // There's no possibility of being the same object
        // when the types are different for this class.
        std::swap(buf, other.buf);
        std::swap(offset, other.offset);
        std::swap(buf_size, other.buf_size);
    }

    ~send_buffer() {
        if (AutoCleanup) {
            // Deallocating a nullptr is defined behavior.
            ::flatbuffers::DefaultAllocator::dealloc(buf, buf_size);
        }
    }
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
                                    send_buffer<true> buf) -> std::size_t = 0;

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
        return buf;
    }

    virtual ~quic_base();
};
} // namespace mad::nexus
