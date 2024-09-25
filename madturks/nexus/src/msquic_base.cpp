#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/quic_connection_context.hpp>

#include <mad/nexus/msquic/msquic_api.inl>

#include <thread>

namespace mad::nexus {

    QUIC_STATUS StreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event) {
        assert(context);
        auto & sctx = *static_cast<stream_context *>(context);

        auto ctx    = reinterpret_cast<msquic_base *>(context);

        fmt::println("ServerStreamCallback {}", static_cast<int>(event->Type));

        // we can use the connection as context here?
        switch (event->Type) {
            case QUIC_STREAM_EVENT_SEND_COMPLETE: {
                //
                // A previous StreamSend call has completed, and the context is being
                // returned back to the app.
                //
                fmt::println("data sent to stream %d", event->SEND_COMPLETE.ClientContext);
                auto key = reinterpret_cast<std::uint64_t>(event->SEND_COMPLETE.ClientContext);

                if (sctx.release_buffer(key)) {
                    fmt::println("send buffer with key {} erased from in-flight, {} remaining.", key, sctx.in_flight_count());

                } else {
                    fmt::println("send buffer with key {} not found in map!!!", key);
                }

            } break;

            case QUIC_STREAM_EVENT_RECEIVE: {
                for (std::uint32_t i = 0; i < event->RECEIVE.BufferCount; i++) {
                    sctx.rbuf().put(event->RECEIVE.Buffers [i].Buffer, event->RECEIVE.Buffers [i].Length);
                }

                auto consumed_bytes = sctx.on_data_received(sctx.rbuf().available_span());
                sctx.rbuf().mark_as_read(consumed_bytes);
                fmt::println("total {}", sctx.rbuf().consumed_space());

                fmt::println("Received data from the remote count:{} total_size:{}", event->RECEIVE.BufferCount,
                             event->RECEIVE.TotalBufferLength);

            } break;

            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
                std::stringstream aq;
                aq << std::this_thread::get_id();
                fmt::println("stream shutdown from thread {}", aq.str());
                if (!event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
                    o2i(ctx->msquic_impl()).api->StreamClose(stream);
                }
                // o2i(ctx->msquic_impl()).api->StreamClose(stream);
                //  FIXME: Fix this::::
                //  if (auto itr = ctx->streams.find(stream); itr == ctx->streams.end()) {
                //      fmt::println("stream shutdown");
                //      ctx->streams.erase(itr);
                //  }

            } break;
        }

        // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
        return QUIC_STATUS_SUCCESS;
    }

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
        fmt::println("new stream open call");

        HQUIC new_stream = nullptr;
        auto & api       = o2i(msquic_pimpl).api;

        if (auto status = api->StreamOpen(static_cast<HQUIC>(cctx->connection_handle), QUIC_STREAM_OPEN_FLAG_NONE, StreamCallback, nullptr,
                                          &new_stream);
            QUIC_FAILED(status)) {
            fmt::println("stream open failed with {}", status);
            return std::unexpected(quic_error_code::stream_open_failed);
        }

        auto [itr, inserted] = cctx->streams.emplace(new_stream, stream_context{new_stream, cctx->connection_handle, data_callback});
        if (!inserted) {
            api->StreamClose(new_stream);
            return std::unexpected(quic_error_code::stream_insert_to_map_failed);
        }

        auto & sctx = itr->second;

        // Set context for the callback.
        api->SetContext(new_stream, static_cast<void *>(&sctx));

        if (auto status = api->StreamStart(new_stream, QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL); QUIC_FAILED(status)) {
            fmt::println("stream start failed with {}", status);
            cctx->streams.erase(itr);
            return std::unexpected(quic_error_code::stream_start_failed);
        }

        fmt::println("stream open ok!");

        return reinterpret_cast<stream_context *>(&sctx);
    }

    auto msquic_base::close_stream([[maybe_unused]] stream_context * sctx) -> std::error_code {

        // FIXME: Implement this
        return quic_error_code::success;
    }

    auto msquic_base::send(stream_context * sctx, flatbuffers::DetachedBuffer buf) -> std::size_t {
        assert(sctx);

        auto & api        = o2i(msquic_pimpl).api;

        // This function is used to queue data on a stream to be sent.
        // The function itself is non-blocking and simply queues the data and returns.
        // The app may pass zero or more buffers of data that will be sent on the stream in the order they are passed.
        // The buffers (both the QUIC_BUFFERs and the memory they reference) are "owned" by MsQuic (and must not be modified by the app)
        // until MsQuic indicates the QUIC_STREAM_EVENT_SEND_COMPLETE event for the send.

        // We have 16 bytes of reserved space at the beginning of 'buf'
        // We're gonna use it for storing QUIC_BUF.

        // The address used as key is not important.

        auto store_result = sctx->store_buffer(std::move(buf));
        if (!store_result) {
            fmt::println("could not store packet data for send!");
            return 0;
        }
        auto itr      = store_result.value();
        auto & buffer = itr->second;

        fmt::println("sending {} bytes of data", buffer.size());
        QUIC_BUFFER * qbuf = reinterpret_cast<QUIC_BUFFER *>(const_cast<std::uint8_t *>(buffer.data()));
        qbuf->Buffer       = reinterpret_cast<std::uint8_t *>(qbuf + sizeof(QUIC_BUFFER));
        qbuf->Length       = static_cast<std::uint32_t>(buffer.size() - sizeof(QUIC_BUFFER));

        // We're using the context pointer here to store the key.
        if (auto status =
                api->StreamSend(static_cast<HQUIC>(sctx->stream()), qbuf, 1, QUIC_SEND_FLAG_NONE, reinterpret_cast<void *>(itr->first));
            QUIC_FAILED(status)) {

            return 0;
        }

        fmt::println("sent, queue size {}", sctx->in_flight_count());
        // FIXME:
        return buf.size() - sizeof(QUIC_BUFFER);
    }

} // namespace mad::nexus
