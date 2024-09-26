#include "mad/log_macros.hpp"
#include "mad/log_printer.hpp"
#include <flatbuffers/default_allocator.h>
#include <flatbuffers/detached_buffer.h>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/quic_connection_context.hpp>

#include <mad/nexus/msquic/msquic_api.inl>

#include <msquic.h>
#include <thread>
#include <utility>

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

    static QUIC_STATUS StreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event) {
        assert(context);
        auto & sctx = *static_cast<stream_context *>(context);

        auto ctx    = reinterpret_cast<msquic_base *>(context);

        MAD_LOG_DEBUG_I((*ctx), "StreamCallback  - {} - {}", quic_stream_event_to_str(event->Type), std::to_underlying(event->Type));

        // we can use the connection as context here?
        switch (event->Type) {
            case QUIC_STREAM_EVENT_SEND_COMPLETE: {
                //
                // A previous StreamSend call has completed, and the context is being
                // returned back to the app.
                //
                MAD_LOG_DEBUG_I((*ctx), "data sent to stream %d", event->SEND_COMPLETE.ClientContext);

                // The size does not matter for the default allocator.
                // FIXME: Get this dynamically from the user
                flatbuffers::DefaultAllocator::dealloc(event->SEND_COMPLETE.ClientContext, 0);
            } break;

            case QUIC_STREAM_EVENT_RECEIVE: {
                for (std::uint32_t i = 0; i < event->RECEIVE.BufferCount; i++) {
                    sctx.rbuf().put(event->RECEIVE.Buffers [i].Buffer, event->RECEIVE.Buffers [i].Length);
                }

                auto consumed_bytes = sctx.on_data_received(sctx.rbuf().available_span());
                sctx.rbuf().mark_as_read(consumed_bytes);
                MAD_LOG_DEBUG_I((*ctx), "total {}", sctx.rbuf().consumed_space());

                MAD_LOG_DEBUG_I((*ctx), "Received data from the remote count:{} total_size:{}", event->RECEIVE.BufferCount,
                             event->RECEIVE.TotalBufferLength);

            } break;

            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
                std::stringstream aq;
                aq << std::this_thread::get_id();
                MAD_LOG_DEBUG_I((*ctx), "stream shutdown from thread {}", aq.str());

                if (!event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
                    o2i(ctx->msquic_impl()).api->StreamClose(stream);
                }

                if (auto itr = sctx.connection().streams.find(stream); itr == sctx.connection().streams.end()) {
                    MAD_LOG_DEBUG_I((*ctx), "stream erased from connection map");
                    sctx.connection().streams.erase(itr);
                }

            } break;
            default: {
                MAD_LOG_WARN_I((*ctx), "Unhandled stream event: {}", std::to_underlying(event->Type));
            }
        }

        // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
        return QUIC_STATUS_SUCCESS;
    }

    msquic_base::msquic_base(quic_configuration cfg) : quic_base(cfg), log_printer("console") {}
    msquic_base::~msquic_base() = default;

    std::error_code msquic_base::init() {
        if (msquic_pimpl) {
            return quic_error_code::already_initialized;
        }

        auto result = msquic_api::make(config);
        if (result) {
            msquic_pimpl = std::move(result.value());
            return quic_error_code::success;
        }
        return result.error();
    }

    auto msquic_base::open_stream(connection_context * cctx,
                                  stream_data_callback_t data_callback) -> std::expected<stream_context *, std::error_code> {
        assert(cctx);
        MAD_LOG_INFO("new stream open call");

        HQUIC new_stream = nullptr;
        auto & api       = o2i(msquic_pimpl).api;

        if (auto status = api->StreamOpen(static_cast<HQUIC>(cctx->connection_handle), QUIC_STREAM_OPEN_FLAG_NONE, StreamCallback, nullptr,
                                          &new_stream);
            QUIC_FAILED(status)) {
            MAD_LOG_ERROR("stream open failed with {}", status);
            return std::unexpected(quic_error_code::stream_open_failed);
        }

        auto [itr, inserted] = cctx->streams.emplace(new_stream, stream_context{new_stream, *cctx, data_callback});
        if (!inserted) {
            api->StreamClose(new_stream);
            return std::unexpected(quic_error_code::stream_insert_to_map_failed);
        }

        auto & sctx = itr->second;

        // Set context for the callback.
        api->SetContext(new_stream, static_cast<void *>(&sctx));

        if (auto status = api->StreamStart(new_stream, QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL); QUIC_FAILED(status)) {
            MAD_LOG_ERROR("stream start failed with {}", status);
            cctx->streams.erase(itr);
            return std::unexpected(quic_error_code::stream_start_failed);
        }

        MAD_LOG_DEBUG("stream open ok!");

        return reinterpret_cast<stream_context *>(&sctx);
    }

    auto msquic_base::close_stream([[maybe_unused]] stream_context * sctx) -> std::error_code {

        // FIXME: Implement this
        return quic_error_code::success;
    }

    auto msquic_base::send(stream_context * sctx, send_buffer buf) -> std::size_t {
        assert(sctx);

        auto & api = o2i(msquic_pimpl).api;

        // This function is used to queue data on a stream to be sent.
        // The function itself is non-blocking and simply queues the data and returns.
        // The app may pass zero or more buffers of data that will be sent on the stream in the order they are passed.
        // The buffers (both the QUIC_BUFFERs and the memory they reference) are "owned" by MsQuic (and must not be modified by the app)
        // until MsQuic indicates the QUIC_STREAM_EVENT_SEND_COMPLETE event for the send.

        // We have 16 bytes of reserved space at the beginning of 'buf'
        // We're gonna use it for storing QUIC_BUF.

        MAD_LOG_INFO("sending {} bytes of data", buf.used);

        QUIC_BUFFER * qbuf = reinterpret_cast<QUIC_BUFFER *>(buf.buf + buf.offset);
        qbuf->Buffer       = reinterpret_cast<std::uint8_t *>(buf.buf + buf.offset + sizeof(QUIC_BUFFER));
        qbuf->Length       = static_cast<std::uint32_t>(buf.used - sizeof(QUIC_BUFFER));

        // We're using the context pointer here to store the key.
        if (auto status = api->StreamSend(static_cast<HQUIC>(sctx->stream()), qbuf, 1, QUIC_SEND_FLAG_NONE, buf.buf); QUIC_FAILED(status)) {
            return 0;
        }

        // MAD_LOG_DEBUG("sent, queue size {}", sctx->in_flight_count());
        // FIXME:
        return buf.used - sizeof(QUIC_BUFFER);
    }

} // namespace mad::nexus
