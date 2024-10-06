#include <mad/log>
#include <mad/macro>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>

#include <flatbuffers/default_allocator.h>
#include <flatbuffers/detached_buffer.h>
#include <msquic.hpp>

#include <bit>
#include <iomanip>
#include <iostream>
#include <thread>
#include <utility>

namespace mad::nexus {

namespace {

    static mad::log_printer & stream_logger() {
        static auto sl = [] {
            static mad::log_printer stream_logger{ "quic-stream" };
            stream_logger.set_log_level(mad::log_level::debug);
            return stream_logger;
        }();
        return sl;
    }

    using send_complete_event = decltype(QUIC_STREAM_EVENT::SEND_COMPLETE);
    using receive_event = decltype(QUIC_STREAM_EVENT::RECEIVE);
    using shutdown_complete_event =
        decltype(QUIC_STREAM_EVENT::SHUTDOWN_COMPLETE);

    /**
     * @brief Quic stream event type to string conversion
     *
     * @param eid The stream event type (QUIC_STREAM_EVENT_TYPE)
     *
     * @return constexpr std::string_view The string representation
     */
    constexpr std::string_view quic_stream_event_to_str(int etype) {
        MAD_EXHAUSTIVE_SWITCH_BEGIN
        switch (static_cast<QUIC_STREAM_EVENT_TYPE>(etype)) {
            using enum QUIC_STREAM_EVENT_TYPE;
            case QUIC_STREAM_EVENT_START_COMPLETE:
                return "QUIC_STREAM_EVENT_START_COMPLETE";
            case QUIC_STREAM_EVENT_RECEIVE:
                return "QUIC_STREAM_EVENT_RECEIVE";
            case QUIC_STREAM_EVENT_SEND_COMPLETE:
                return "QUIC_STREAM_EVENT_SEND_COMPLETE";
            case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
                return "QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN";
            case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
                return "QUIC_STREAM_EVENT_PEER_SEND_ABORTED";
            case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
                return "QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED";
            case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE:
                return "QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE";
            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
                return "QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE";
            case QUIC_STREAM_EVENT_IDEAL_SEND_BUFFER_SIZE:
                return "QUIC_STREAM_EVENT_IDEAL_SEND_BUFFER_SIZE";
            case QUIC_STREAM_EVENT_PEER_ACCEPTED:
                return "QUIC_STREAM_EVENT_PEER_ACCEPTED";
            case QUIC_STREAM_EVENT_CANCEL_ON_LOSS:
                return "QUIC_STREAM_EVENT_CANCEL_ON_LOSS";
        }
        MAD_EXHAUSTIVE_SWITCH_END
        std::unreachable();
    }

    /**
     * @brief Send completion callback.
     *
     * The client_context will contain the buffer data in flight,
     * and the code performs the required cleanups, if any.
     *
     * @param sctx The owning stream context
     * @param event Send complete event details
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    MAD_ALWAYS_INLINE QUIC_STATUS StreamCallbackSendComplete(
        [[maybe_unused]] stream_context & sctx, send_complete_event & event) {
        //
        // A previous StreamSend call has completed, and the context is
        // being returned back to the app.
        //
        MAD_LOG_DEBUG_I(
            stream_logger(), "data sent to stream %p", event.ClientContext);

        // The size does not matter for the default allocator.
        // FIXME: Get this dynamically from the user
        flatbuffers::DefaultAllocator::dealloc(event.ClientContext, 0);
        return QUIC_STATUS_SUCCESS;
    }

    /**
     * @brief The callback function for incoming stream data.
     *
     * The callback transfers the received data to stream-specific
     * circular receive buffer for parsing.
     *
     * @param sctx The owning stream
     * @param event The receive event details
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    MAD_ALWAYS_INLINE QUIC_STATUS StreamCallbackReceive(stream_context & sctx,
                                                        receive_event & event) {

        std::size_t buffer_offset = 0;
        for (std::uint32_t buffer_idx = 0; buffer_idx < event.BufferCount;) {

            const auto & buffer = event.Buffers [buffer_idx];

            const auto pull_amount = std::min(
                sctx.rbuf().empty_space(),
                static_cast<std::size_t>(buffer.Length - buffer_offset));
            assert(pull_amount > 0);
            if (pull_amount == 0) {
                MAD_LOG_ERROR_I(stream_logger(),
                                "No empty space left in the receive buffer!");
                // Probably corrupt message, disconnect and break
                break;
            }
            assert(sctx.rbuf().put(buffer.Buffer + buffer_offset, pull_amount));
            buffer_offset += pull_amount;

            std::uint32_t push_payload_cnt = 0;

            // Deliver all complete messages to the app layer
            for (auto available_span = sctx.rbuf().available_span();
                 available_span.size_bytes() >= sizeof(std::uint32_t);
                 available_span = sctx.rbuf().available_span()) {
                // Read the size of the message
                auto size = *reinterpret_cast<const std::uint32_t *>(
                    available_span.data());
                // Size is little-endian.
                if constexpr (std::endian::native == std::endian::big) {
                    size = std::byteswap(size);
                }

                // Continue here!
                MAD_LOG_DEBUG_I(stream_logger(), "Message size {}", size);
                if ((available_span.size_bytes() - sizeof(std::uint32_t)) >=
                    size) {
                    auto message = available_span.subspan(
                        sizeof(std::uint32_t), size);

                    // Only deliver complete messages to the application layer.
                    [[maybe_unused]] auto consumed_bytes =
                        sctx.on_data_received(message);
                    push_payload_cnt++;
                    MAD_LOG_DEBUG_I(stream_logger(), "Push payload count {}",
                                    push_payload_cnt);

                    sctx.rbuf().mark_as_read(sizeof(std::uint32_t) +
                                             message.size_bytes());
                    continue;
                }
                MAD_LOG_DEBUG_I(
                    stream_logger(),
                    "Partial data received {}, need {} more byte(s)",
                    available_span.size_bytes(),
                    size - available_span.size_bytes());
                break;
            }

            // Not sure about this
            if (buffer_offset == buffer.Length) {
                buffer_idx++;
            }
        }

        MAD_LOG_DEBUG_I(
            stream_logger(), "total {}", sctx.rbuf().consumed_space());

        MAD_LOG_DEBUG_I(stream_logger(),
                        "Received data from the remote count:{} total_size:{}",
                        event.BufferCount, event.TotalBufferLength);
        return QUIC_STATUS_SUCCESS;
    }

    /**
     * @brief Callback function for stream shutdown.
     *
     * It is called when a stream is completely closed and being destructed.
     *
     * @param sctx The stream context of the shutdown stream
     * @param event Shutdown complete event details
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    MAD_ALWAYS_INLINE QUIC_STATUS StreamCallbackShutdownComplete(
        stream_context & sctx, [[maybe_unused]] shutdown_complete_event & event)

    {
        return sctx.connection()
            .remove_stream(sctx.stream())
            .and_then([&](auto &&) {
                MAD_LOG_DEBUG_I(
                    stream_logger(), "stream erased from connection map");
                return std::optional{ QUIC_STATUS_SUCCESS };
            })
            .value_or(QUIC_STATUS_SUCCESS);
    }

} // namespace

/**
 * @brief The stream event callback dispatcher.
 *
 * The function is shared between the client and the server code.
 *
 * @param stream The stream that is source of the event
 * @param context The context associated with the stream (stream_context)
 * @param event The event (what happened)
 *
 * @return QUIC_STATUS Return code indicating callback result
 */
QUIC_STATUS StreamCallback(HQUIC stream, void * context,
                           QUIC_STREAM_EVENT * event) {
    assert(stream);
    assert(context);
    assert(event);

    auto & sctx = *static_cast<stream_context *>(context);
    MAD_LOG_DEBUG_I(stream_logger(), "StreamCallback  - {} - {}",
                    quic_stream_event_to_str(event->Type),
                    std::to_underlying(event->Type));

    // we can use the connection as context here?
    switch (event->Type) {
        case QUIC_STREAM_EVENT_SEND_COMPLETE:
            return StreamCallbackSendComplete(sctx, event->SEND_COMPLETE);
        case QUIC_STREAM_EVENT_RECEIVE:
            return StreamCallbackReceive(sctx, event->RECEIVE);
        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
            return StreamCallbackShutdownComplete(
                sctx, event->SHUTDOWN_COMPLETE);
        default: {
            MAD_LOG_WARN_I(stream_logger(), "Unhandled stream event: {} {}",
                           std::to_underlying(event->Type),
                           quic_stream_event_to_str(event->Type));
        }
    }

    // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
    return QUIC_STATUS_SUCCESS;
};

msquic_base::msquic_base() : log_printer("console") {
    set_log_level(log_level::trace);
}

msquic_base::~msquic_base() = default;

std::error_code msquic_base::init() {
    return quic_error_code::success;
}

auto msquic_base::open_stream(
    connection_context & cctx,
    std::optional<stream_data_callback_t> data_callback) -> open_stream_result {
    MAD_LOG_INFO("new stream open call");

    HQUIC new_stream = nullptr;

    if (auto status = MsQuic->StreamOpen(
            static_cast<HQUIC>(cctx.connection_handle),
            QUIC_STREAM_OPEN_FLAG_NONE, StreamCallback, nullptr, &new_stream);
        QUIC_FAILED(status)) {
        MAD_LOG_ERROR("stream open failed with {}", status);
        return std::unexpected(quic_error_code::stream_open_failed);
    }

    // The user may decide to use different callbacks per stream.
    auto callback_to_use = data_callback ? data_callback.value()
                                         : callbacks.on_stream_data_received;
    return cctx
        .add_new_stream({ new_stream,
                          [](void * sp) {
                              MsQuic->StreamClose(static_cast<HQUIC>(sp));
                          } },
                        callback_to_use)
        .and_then([&](auto && v) -> open_stream_result {
            MsQuic->SetContext(static_cast<HQUIC>(v.get().stream()),
                               static_cast<void *>(&v.get()));
            return std::move(v);
        })
        .and_then([&](auto && v) -> open_stream_result {
            if (QUIC_FAILED(MsQuic->StreamStart(
                    static_cast<HQUIC>(v.get().stream()),
                    QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL))) {
                // TODO: Check if it triggers a stream shutdown event
                // cctx->streams.erase(itr);?
                return std::unexpected(quic_error_code::stream_start_failed);
            }
            return std::move(v);
        });
}

auto msquic_base::close_stream([[maybe_unused]] stream_context & sctx)
    -> std::error_code {

    // FIXME: Implement this
    return quic_error_code::success;
}

auto msquic_base::send(stream_context & sctx, send_buffer buf) -> std::size_t {

    // This function is used to queue data on a stream to be sent.
    // The function itself is non-blocking and simply queues the data and
    // returns. The app may pass zero or more buffers of data that will be sent
    // on the stream in the order they are passed. The buffers (both the
    // QUIC_BUFFERs and the memory they reference) are "owned" by MsQuic (and
    // must not be modified by the app) until MsQuic indicates the
    // QUIC_STREAM_EVENT_SEND_COMPLETE event for the send.

    // We have 16 bytes of reserved space at the beginning of 'buf'
    // We're gonna use it for storing QUIC_BUF.

    const auto used = buf.buf_size - buf.offset;

    auto quic_resv = (buf.buf + buf.buf_size) - sizeof(QUIC_BUFFER);
    [[maybe_unused]] constexpr std::uint8_t res [] = { 0xDE, 0xAD, 0xBE, 0xEF,
                                                       0xBA, 0xAD, 0xC0, 0xDE,
                                                       0xCA, 0xFE, 0xBA, 0xBE,
                                                       0xDE, 0xAD, 0xFA, 0xCE };
    assert(0 == std::memcmp(quic_resv, res, sizeof(res)));

    MAD_LOG_DEBUG("encoded size = {}",
                  *reinterpret_cast<std::uint32_t *>(buf.buf + buf.offset));

    QUIC_BUFFER * qbuf = reinterpret_cast<QUIC_BUFFER *>(quic_resv);
    qbuf->Length = static_cast<std::uint32_t>(used - sizeof(QUIC_BUFFER));
    qbuf->Buffer = reinterpret_cast<std::uint8_t *>(buf.buf + buf.offset);

    MAD_LOG_DEBUG("sending {} bytes of data of {}, offset: {}, size: {}",
                  qbuf->Length, used, buf.offset, buf.buf_size);

    // We're using the context pointer here to store the key.
    if (auto status = MsQuic->StreamSend(static_cast<HQUIC>(sctx.stream()),
                                         qbuf, 1, QUIC_SEND_FLAG_NONE, buf.buf);
        QUIC_FAILED(status)) {
        return 0;
    }
    return used - sizeof(QUIC_BUFFER);
}

} // namespace mad::nexus
