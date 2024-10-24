#include <mad/log>
#include <mad/macro>
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream.hpp>

#include <flatbuffers/default_allocator.h>
#include <flatbuffers/detached_buffer.h>
#include <msquic.h>

#include <bit>
#include <utility>

namespace mad::nexus {

static mad::log_printer & stream_logger() {
    static log_printer stream_logger{ "quic-stream", log_level::info };
    return stream_logger;
}

struct events {
    using send_complete = decltype(QUIC_STREAM_EVENT::SEND_COMPLETE);
    using receive = decltype(QUIC_STREAM_EVENT::RECEIVE);
    using shutdown_complete = decltype(QUIC_STREAM_EVENT::SHUTDOWN_COMPLETE);
    using start_complete = decltype(QUIC_STREAM_EVENT::START_COMPLETE);
};

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
static QUIC_STATUS StreamCallbackSendComplete([[maybe_unused]] stream & sctx,
                                              events::send_complete & event) {
    //
    // A previous StreamSend call has completed, and the context is
    // being returned back to the app.
    //
    MAD_LOG_DEBUG_I(
        stream_logger(), "data sent to stream %p", event.ClientContext);

    // The size does not matter for the default allocator.
    // FIXME: Get this dynamically from the user
    flatbuffers::DefaultAllocator::dealloc(event.ClientContext, 0);
#ifndef NDEBUG
    sctx.sends_in_flight.fetch_sub(1);
#endif
    return QUIC_STATUS_SUCCESS;
}

// Chunked reader?

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
QUIC_STATUS StreamCallbackReceive(stream & sctx, events::receive & event) {

    MAD_EXPECTS(sctx.callbacks.on_data_received);
    MAD_EXPECTS(event.BufferCount > 0);
    MAD_EXPECTS(event.TotalBufferLength > 0);
    auto & receive_buffer = sctx.rbuf();

    std::size_t buffer_offset = 0;

    // FIXME: Optimize this.
    for (std::uint32_t buffer_idx = 0; buffer_idx < event.BufferCount;) {

        const auto & received_data = event.Buffers [buffer_idx];

        const auto pull_amount = std::min(
            receive_buffer.empty_space(), received_data.Length - buffer_offset);

        MAD_LOG_DEBUG_I(stream_logger(),
                        "Pulled {} byte(s) into the receive buffer (rb "
                        "allocation size: {})",
                        pull_amount, receive_buffer.total_size());

        if (pull_amount == 0) {
            MAD_LOG_ERROR_I(
                stream_logger(), "No empty space left in the receive buffer!");
            // FIXME: What to do here? close stream? close connection?
            // skip the message?
            // Probably corrupt message, disconnect and break
            break;
        }
        [[maybe_unused]] auto pull_r = receive_buffer.put(
            received_data.Buffer + buffer_offset, pull_amount);
        MAD_ASSERT(pull_r);
        buffer_offset += pull_amount;

        std::uint32_t push_payload_cnt = 0;

        // Deliver all complete messages to the app layer
        for (auto available_span = receive_buffer.available_span();
             available_span.size_bytes() >= sizeof(std::uint32_t);
             available_span = receive_buffer.available_span()) {

            // Read the size of the message
            // Comply with the strict aliasing rules.
            std::uint32_t size{ 0 };
            std::memcpy(&size, available_span.data(), sizeof(std::uint32_t));

            // Size is little-endian.
            if constexpr (std::endian::native == std::endian::big) {
                size = std::byteswap(size);
            }

            MAD_LOG_DEBUG_I(stream_logger(), "Message size {}", size);
            if ((available_span.size_bytes() - sizeof(std::uint32_t)) >= size) {
                auto message = available_span.subspan(
                    sizeof(std::uint32_t), size);

                // Only deliver complete messages to the application layer.
                [[maybe_unused]] auto consumed_bytes =
                    sctx.callbacks.on_data_received(message);
                push_payload_cnt++;
                MAD_LOG_DEBUG_I(
                    stream_logger(), "Push payload count {}", push_payload_cnt);

                receive_buffer.mark_as_read(sizeof(std::uint32_t) +
                                            message.size_bytes());
                continue;
            }
            MAD_LOG_DEBUG_I(
                stream_logger(),
                "Partial data received({}), need {} more byte(s)",
                available_span.size_bytes() - sizeof(std::uint32_t),
                (size + sizeof(std::uint32_t)) - available_span.size_bytes());
            break;
        }

        if (buffer_offset == received_data.Length) {
            buffer_idx++;
            buffer_offset = 0;
            MAD_LOG_DEBUG_I(
                stream_logger(), "proceed to next buffer(idx: {})", buffer_idx);
        }
    }

    // MsQuic->StreamReceiveComplete()

    MAD_LOG_DEBUG_I(stream_logger(),
                    "Processed {} QUIC_BUFFER(s), total {} byte(s). "
                    "Receive buffer has {} byte(s) inside.",
                    event.BufferCount, event.TotalBufferLength,
                    receive_buffer.consumed_space());
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
    stream & sctx, [[maybe_unused]] events::shutdown_complete & event)

{
    MAD_EXPECTS(sctx.callbacks.on_close);
    sctx.callbacks.on_close(sctx);

    if (event.AppCloseInProgress) {
        // If we initiated this
        return QUIC_STATUS_SUCCESS;
    }

    return sctx.connection()
        .remove_stream(sctx.handle_as<>())
        .and_then([&](auto &&) -> result<QUIC_STATUS> {
            MAD_LOG_DEBUG_I(stream_logger(),
                            "StreamCallbackShutdownComplete - stream "
                            "erased from connection map");
            return QUIC_STATUS_SUCCESS;
        })
        .value_or(QUIC_STATUS_SUCCESS);
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
MAD_ALWAYS_INLINE QUIC_STATUS
StreamCallbackStartComplete(stream & sctx, events::start_complete & event)

{
    if (QUIC_FAILED(event.Status)) {
        return sctx.connection()
            .remove_stream(sctx.handle_as<>())
            .and_then([&](auto &&) -> result<QUIC_STATUS> {
                MAD_LOG_DEBUG_I(stream_logger(),
                                "StreamCallbackStartComplete - stream "
                                "start failure, erasing stream");
                return QUIC_STATUS_SUCCESS;
            })
            .value_or(QUIC_STATUS_SUCCESS);
    }

    MAD_EXPECTS(sctx.callbacks.on_start);
    sctx.callbacks.on_start(sctx);
    return QUIC_STATUS_SUCCESS;
}

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
QUIC_STATUS StreamCallback([[maybe_unused]] HQUIC stream_handle, void * context,
                           QUIC_STREAM_EVENT * event) {
    assert(stream_handle);
    assert(context);
    assert(event);

    auto & sctx = *static_cast<stream *>(context);
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
        case QUIC_STREAM_EVENT_START_COMPLETE:
            return StreamCallbackStartComplete(sctx, event->START_COMPLETE);
        default: {
            MAD_LOG_WARN_I(stream_logger(), "Unhandled stream event: {} {}",
                           std::to_underlying(event->Type),
                           quic_stream_event_to_str(event->Type));
        }
    }

    // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
    return QUIC_STATUS_SUCCESS;
};

msquic_base::msquic_base(const msquic_application & app) :
    log_printer("console"), application(app) {
    set_log_level(log_level::info);
}

msquic_base::~msquic_base() = default;

auto msquic_base::init() -> result<> {
    return {};
}

auto msquic_base::open_stream(
    connection & cctx, std::optional<stream_data_callback_t> data_callback)
    -> result<std::reference_wrapper<stream>> {
    MAD_LOG_INFO("new stream open call");

    HQUIC new_stream = nullptr;

    if (auto result = application.api()->StreamOpen(
            cctx.handle_as<QUIC_HANDLE *>(), QUIC_STREAM_OPEN_FLAG_NONE,
            StreamCallback, nullptr, &new_stream);
        QUIC_FAILED(result)) {
        MAD_LOG_ERROR("stream open failed with {}", result);
        return std::unexpected(quic_error_code::stream_open_failed);
    }

    // The user may decide to use different callbacks per stream.
    stream_callbacks scb{
        .on_start = callbacks.on_stream_start,
        .on_close = callbacks.on_stream_close,

        .on_data_received = data_callback ? data_callback.value()
                                          : callbacks.on_stream_data_received
    };

    return cctx
        .add_new_stream({ new_stream,
                          [api = application.api()](QUIC_HANDLE * h) {
                              api->StreamClose(h);
                          } },
                        scb)
        .and_then([api = application.api()](
                      auto && v) -> result<std::reference_wrapper<stream>> {
            api->SetContext(v.get().template handle_as<HQUIC>(),
                            static_cast<void *>(&v.get()));
            return std::move(v);
        })
        .and_then([api = application.api()](
                      auto && v) -> result<std::reference_wrapper<stream>> {
            if (QUIC_FAILED(api->StreamStart(
                    v.get().template handle_as<HQUIC>(),
                    QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL))) {

                // This function starts the processing of the stream by the
                // connection. Once called, the stream can start receiving
                // events to the handler passed into StreamOpen. If the start
                // operation fails, the only event that will be delivered is
                // QUIC_STREAM_EVENT_START_COMPLETE with the failure status
                // code.

                return std::unexpected(quic_error_code::stream_start_failed);
            }
            return std::move(v);
        });
}

auto msquic_base::close_stream(stream & sctx) -> result<> {

    return sctx.connection()
        .remove_stream(sctx.handle_as<>())
        .and_then([&](auto &&) {
            MAD_LOG_DEBUG_I(
                stream_logger(), "stream erased from connection map");
            return result<>{};
        });
}

auto msquic_base::send(stream & sctx,
                       send_buffer<true> buf) -> result<std::size_t> {

    // This function is used to queue data on a stream to be sent.
    // The function itself is non-blocking and simply queues the data and
    // returns. The app may pass zero or more buffers of data that will be sent
    // on the stream in the order they are passed. The buffers (both the
    // QUIC_BUFFERs and the memory they reference) are "owned" by MsQuic (and
    // must not be modified by the app) until MsQuic indicates the
    // QUIC_STREAM_EVENT_SEND_COMPLETE event for the send.

    // We have 16 bytes of reserved space at the beginning of 'buf'
    // We're gonna use it for storing QUIC_BUF.

    // These are not invalidated after move.
    auto quic_buffer_span = buf.quic_buffer_span();
    auto data_span = buf.data_span();

    QUIC_BUFFER * qbuf = reinterpret_cast<QUIC_BUFFER *>(
        quic_buffer_span.data());
    qbuf->Length = static_cast<std::uint32_t>(data_span.size_bytes());
    qbuf->Buffer = data_span.data();

    MAD_LOG_DEBUG("sending {} bytes of data of {}, offset: {}, allocation "
                  "size: {}, encoded size: {}",
                  qbuf->Length, buf.size(), buf.offset, buf.buf_size,
                  buf.encoded_data_size());

    // We're using the context pointer here to store the key.
    if (auto status = application.api()->StreamSend(
            sctx.handle_as<HQUIC>(), qbuf, 1, QUIC_SEND_FLAG_NONE, buf.buf);
        QUIC_FAILED(status)) {
        MAD_LOG_ERROR("stream send failed!");
        return std::unexpected(quic_error_code::send_failed);
    }
    // The object is in use by MSQUIC.
    // The STREAM_SEND_COMPLETE callback will handle the cleanup.
    send_buffer<false> _{ std::move(buf) };

#ifndef NDEBUG
    sctx.sends_in_flight.fetch_add(1);
#endif
    return data_span.size_bytes();
}

} // namespace mad::nexus
