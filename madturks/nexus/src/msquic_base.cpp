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

namespace {
static mad::log_printer & stream_logger() {
    static auto sl = [] {
        static mad::log_printer stream_logger{ "quic-stream" };
        stream_logger.set_log_level(mad::log_level::debug);
        return stream_logger;
    }();
    return sl;
}

} // namespace

namespace mad::nexus {

constexpr std::string_view quic_stream_event_to_str(int eid) {
    switch (eid) {
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
    return "undefined";
}

inline MAD_ALWAYS_INLINE QUIC_STATUS
StreamCallbackImpl([[maybe_unused]] HQUIC stream, stream_context & sctx,
                   QUIC_STREAM_EVENT & event) {

    MAD_LOG_DEBUG_I(stream_logger(), "StreamCallback  - {} - {}",
                    quic_stream_event_to_str(event.Type),
                    std::to_underlying(event.Type));

    // we can use the connection as context here?
    switch (event.Type) {
        case QUIC_STREAM_EVENT_SEND_COMPLETE: {
            //
            // A previous StreamSend call has completed, and the context is
            // being returned back to the app.
            //
            MAD_LOG_DEBUG_I(stream_logger(), "data sent to stream %p",
                            event.SEND_COMPLETE.ClientContext);

            // The size does not matter for the default allocator.
            // FIXME: Get this dynamically from the user
            flatbuffers::DefaultAllocator::dealloc(
                event.SEND_COMPLETE.ClientContext, 0);
        } break;

        case QUIC_STREAM_EVENT_RECEIVE: {
            // Pull the received data into user-space receive buffer
            for (std::uint32_t i = 0; i < event.RECEIVE.BufferCount; i++) {
                sctx.rbuf().put(event.RECEIVE.Buffers [i].Buffer,
                                event.RECEIVE.Buffers [i].Length);
            }

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
                        sizeof(std::uint32_t), size - sizeof(std::uint32_t));
                    // Only deliver complete messages to the application layer.
                    [[maybe_unused]] auto consumed_bytes =
                        sctx.on_data_received(message);
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

            MAD_LOG_DEBUG_I(
                stream_logger(), "total {}", sctx.rbuf().consumed_space());

            MAD_LOG_DEBUG_I(
                stream_logger(),
                "Received data from the remote count:{} total_size:{}",
                event.RECEIVE.BufferCount, event.RECEIVE.TotalBufferLength);

        } break;

        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
            std::stringstream aq;
            aq << std::this_thread::get_id();
            MAD_LOG_DEBUG_I(
                stream_logger(), "stream shutdown from thread {}", aq.str());

            if (auto itr = sctx.connection().streams.find(
                    static_cast<void *>(stream));
                itr == sctx.connection().streams.end()) {
                MAD_LOG_DEBUG_I(
                    stream_logger(), "stream erased from connection map");
                sctx.connection().streams.erase(itr);
            }

        } break;
        case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE: {
            std::stringstream aq;
            aq << std::this_thread::get_id();
            MAD_LOG_DEBUG_I(stream_logger(),
                            "stream send shutdown from thread {}", aq.str());

        } break;
        default: {
            MAD_LOG_WARN_I(stream_logger(), "Unhandled stream event: {}",
                           std::to_underlying(event.Type));
        }
    }

    // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
    return QUIC_STATUS_SUCCESS;
}

QUIC_STATUS StreamCallback(HQUIC stream, void * context,
                           QUIC_STREAM_EVENT * event) {
    assert(stream);
    assert(context);
    assert(event);
    return StreamCallbackImpl(
        stream, *static_cast<stream_context *>(context), *event);
};

msquic_base::msquic_base() : log_printer("console") {
    set_log_level(log_level::trace);
}

msquic_base::~msquic_base() = default;

std::error_code msquic_base::init() {
    return quic_error_code::success;
}

auto msquic_base::open_stream(
    connection_context * cctx,
    std::optional<stream_data_callback_t> data_callback)
    -> std::expected<stream_context *, std::error_code> {
    assert(cctx);
    MAD_LOG_INFO("new stream open call");

    HQUIC new_stream = nullptr;

    if (auto status = MsQuic->StreamOpen(
            static_cast<HQUIC>(cctx->connection_handle),
            QUIC_STREAM_OPEN_FLAG_NONE, StreamCallback, nullptr, &new_stream);
        QUIC_FAILED(status)) {
        MAD_LOG_ERROR("stream open failed with {}", status);
        return std::unexpected(quic_error_code::stream_open_failed);
    }

    std::shared_ptr<void> stream_shared_ptr{ new_stream, [](void * sp) {
                                                MsQuic->StreamClose(
                                                    static_cast<HQUIC>(sp));
                                            } };

    auto callback_to_use = data_callback ? data_callback.value()
                                         : callbacks.on_stream_data_received;

    auto [itr, inserted] = cctx->streams.emplace(
        std::move(stream_shared_ptr),
        stream_context{ new_stream, *cctx, callback_to_use });
    if (!inserted) {
        return std::unexpected(quic_error_code::stream_insert_to_map_failed);
    }

    auto & sctx = itr->second;

    // Set context for the callback.
    MsQuic->SetContext(new_stream, static_cast<void *>(&sctx));

    if (auto status = MsQuic->StreamStart(
            new_stream, QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL);
        QUIC_FAILED(status)) {
        MAD_LOG_ERROR("stream start failed with {}", status);
        cctx->streams.erase(itr);
        return std::unexpected(quic_error_code::stream_start_failed);
    }

    MAD_LOG_DEBUG("stream open ok!");

    return reinterpret_cast<stream_context *>(&sctx);
}

auto msquic_base::close_stream([[maybe_unused]] stream_context * sctx)
    -> std::error_code {

    // FIXME: Implement this
    return quic_error_code::success;
}

void prettyPrintHex(const void * data, size_t length) {
    const unsigned char * ptr = static_cast<const unsigned char *>(data);
    const size_t bytes_per_line = 16;

    for (size_t i = 0; i < length; i += bytes_per_line) {
        std::ostringstream oss;

        // Print the offset
        oss << std::setw(8) << std::setfill('0') << std::hex << i << ": ";

        // Print hexadecimal bytes
        for (size_t j = 0; j < bytes_per_line; ++j) {
            if (i + j < length) {
                oss << std::setw(2) << std::setfill('0')
                    << static_cast<int>(ptr [i + j]) << ' ';
            } else {
                oss << "   ";
            }
        }

        // Print ASCII characters
        oss << " ";
        for (size_t j = 0; j < bytes_per_line; ++j) {
            if (i + j < length) {
                unsigned char c = ptr [i + j];
                oss << (std::isprint(c) ? c : '.');
            } else {
                oss << ' ';
            }
        }

        // Log the line using spdlog
        MAD_LOG_INFO_I(stream_logger(), oss.str());
    }
}

auto msquic_base::send(stream_context * sctx, send_buffer buf) -> std::size_t {
    assert(sctx);

    // This function is used to queue data on a stream to be sent.
    // The function itself is non-blocking and simply queues the data and
    // returns. The app may pass zero or more buffers of data that will be sent
    // on the stream in the order they are passed. The buffers (both the
    // QUIC_BUFFERs and the memory they reference) are "owned" by MsQuic (and
    // must not be modified by the app) until MsQuic indicates the
    // QUIC_STREAM_EVENT_SEND_COMPLETE event for the send.

    // We have 16 bytes of reserved space at the beginning of 'buf'
    // We're gonna use it for storing QUIC_BUF.

    MAD_LOG_INFO("sending {} bytes of data, offset: {}, size: {}", buf.used,
                 buf.offset, buf.buf_size);

    prettyPrintHex(buf.buf + buf.offset, buf.used);

    QUIC_BUFFER * qbuf = reinterpret_cast<QUIC_BUFFER *>(buf.buf + buf.offset);
    qbuf->Buffer = reinterpret_cast<std::uint8_t *>(buf.buf + buf.offset +
                                                    sizeof(QUIC_BUFFER));
    qbuf->Length = static_cast<std::uint32_t>(buf.used - sizeof(QUIC_BUFFER));

    // We're using the context pointer here to store the key.
    if (auto status = MsQuic->StreamSend(static_cast<HQUIC>(sctx->stream()),
                                         qbuf, 1, QUIC_SEND_FLAG_NONE, buf.buf);
        QUIC_FAILED(status)) {
        return 0;
    }

    // MAD_LOG_DEBUG("sent, queue size {}", sctx->in_flight_count());
    // FIXME:
    return buf.used - sizeof(QUIC_BUFFER);
}

} // namespace mad::nexus
