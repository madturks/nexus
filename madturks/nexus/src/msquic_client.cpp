#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>

#include <mad/nexus/msquic/msquic_api.inl>
#include <msquic.h>

namespace mad::nexus {
    QUIC_STATUS StreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event);
}

static std::size_t app_stream_data_received(void * uctx, std::span<const std::uint8_t> buf) {
    fmt::println("app_stream_data_received: received {} byte(s)", buf.size_bytes());
    return 0;
}

static QUIC_STATUS ClientConnectionCallback([[maybe_unused]] HQUIC chandle, void * context, QUIC_CONNECTION_EVENT * event) {

    assert(context);
    auto & client     = *static_cast<mad::nexus::msquic_client *>(context);
    auto & msquic_ctx = mad::nexus::o2i(client.msquic_impl());
    auto & cctx       = client.connection_ctx;

    fmt::println("ClientConnectionCallback() - Event Type: `{}`", static_cast<int>(event->Type));

    switch (event->Type) {
        case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
            auto new_stream      = event->PEER_STREAM_STARTED.Stream;

            auto [itr, inserted] = cctx->streams.emplace(
                new_stream, mad::nexus::stream_context{
                                new_stream, *cctx.get(), mad::nexus::stream_data_callback_t{app_stream_data_received, nullptr}
            });
            if (!inserted) {
                msquic_ctx.api->StreamClose(new_stream);
            } else {
                auto & sctx = itr->second;
                msquic_ctx.api->SetCallbackHandler(new_stream, reinterpret_cast<void *>(mad::nexus::StreamCallback),
                                                   static_cast<void *>(&sctx));
            }

            return QUIC_STATUS_SUCCESS;

        } break;
        case QUIC_CONNECTION_EVENT_CONNECTED: {
            auto & v = event->CONNECTED;
            fmt::println("Client connected, resumed: {}, negotiated_alpn: {}", v.SessionResumed,
                         std::string_view{reinterpret_cast<const char *>(v.NegotiatedAlpn), v.NegotiatedAlpnLength});
            client.connection_ctx = std::make_unique<mad::nexus::connection_context>(chandle);
            assert(client.callbacks.on_connected);
            client.callbacks.on_connected(client.connection_ctx.get());
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
            auto & v = event->SHUTDOWN_INITIATED_BY_PEER;
            fmt::println("connection shutdown by peer, error code {}", v.ErrorCode);
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {
            auto & v = event->SHUTDOWN_INITIATED_BY_TRANSPORT;
            switch (v.Status) {
                case QUIC_STATUS_CONNECTION_IDLE: {
                    fmt::println("connection shutdown on idle");
                } break;
                case QUIC_STATUS_CONNECTION_REFUSED: {
                    fmt::println("connection refused");
                } break;
                case QUIC_STATUS_CONNECTION_TIMEOUT: {
                    fmt::println("connection attempt timed out");
                } break;
            }
            fmt::println("ClientConnectionCallback - shutdown initiated by peer");
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
            // TODO: Invoke on_client_disconnect callback?
            client.callbacks.on_disconnected(client.connection_ctx.get());
            client.connection_ctx.reset(nullptr);
            // If the app not initiated the cleanup for the connection
            // do it ourselves.
            if (!event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
                msquic_ctx.api->ConnectionClose(chandle);
            }
        } break;
        case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED: {
            // TODO: Store resumption ticket for later?
            fmt::println("Resumption ticket received {} byte(s)", event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
        } break;
    }

    return QUIC_STATUS_NOT_SUPPORTED;
}

namespace mad::nexus {

    msquic_client::~msquic_client() = default;

    std::error_code msquic_client::connect(std::string_view target, std::uint16_t port) {
        return o2i(msquic_impl()).connect(target, port, ClientConnectionCallback, this);
    }
} // namespace mad::nexus
