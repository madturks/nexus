#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>

#include <mad/nexus/msquic/msquic_api.inl>
#include <mad/log_printer.hpp>
#include <mad/log_macros.hpp>
#include <msquic.h>
#include <utility>

namespace mad::nexus {
    QUIC_STATUS StreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event);
}

static std::size_t app_stream_data_received([[maybe_unused]] void * uctx, std::span<const std::uint8_t> buf) {
    // TODO: Fix this logging
    fmt::println("app_stream_data_received: received {} byte(s)", buf.size_bytes());
    return 0;
}

static __attribute__((always_inline)) QUIC_STATUS ClientConnectionCallbackImpl([[maybe_unused]] HQUIC chandle,
                                                                               mad::nexus::msquic_client & client,
                                                                               const QUIC_API_TABLE & api,
                                                                               mad::nexus::connection_context & connection_context,
                                                                               QUIC_CONNECTION_EVENT & event) {

    MAD_LOG_INFO_I(client, "ClientConnectionCallback() - Event Type: `{}`", std::to_underlying(event.Type));

    switch (event.Type) {
        case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
            auto new_stream = event.PEER_STREAM_STARTED.Stream;

            std::shared_ptr<void> stream_shared_ptr{new_stream, [&api](void * sp) {
                                                        api.StreamClose(static_cast<HQUIC>(sp));
                                                    }};

            const auto & [itr, inserted] = connection_context.streams.emplace(
                std::move(stream_shared_ptr),
                mad::nexus::stream_context{
                    new_stream, connection_context, mad::nexus::stream_data_callback_t{app_stream_data_received, nullptr}
            });
            if (inserted) {
                MAD_LOG_DEBUG_I(client, "Client peer stream started!");
                auto & sctx = itr->second;
                api.SetCallbackHandler(new_stream, reinterpret_cast<void *>(mad::nexus::StreamCallback), static_cast<void *>(&sctx));
            }

            return QUIC_STATUS_SUCCESS;
        }
        case QUIC_CONNECTION_EVENT_CONNECTED: {
            auto & v = event.CONNECTED;
            MAD_LOG_INFO_I(client, "Client connected, resumed: {}, negotiated_alpn: {}", v.SessionResumed,
                           std::string_view{reinterpret_cast<const char *>(v.NegotiatedAlpn), v.NegotiatedAlpnLength});
            client.connection_ctx = std::make_unique<mad::nexus::connection_context>(chandle);
            assert(client.callbacks.on_connected);
            client.callbacks.on_connected(client.connection_ctx.get());
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
            auto & v = event.SHUTDOWN_INITIATED_BY_PEER;
            MAD_LOG_INFO_I(client, "connection shutdown by peer, error code {}", v.ErrorCode);
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {
            auto & v = event.SHUTDOWN_INITIATED_BY_TRANSPORT;
            switch (v.Status) {
                case QUIC_STATUS_CONNECTION_IDLE: {
                    MAD_LOG_DEBUG_I(client, "connection shutdown on idle");
                } break;
                case QUIC_STATUS_CONNECTION_REFUSED: {
                    MAD_LOG_DEBUG_I(client, "connection refused");
                } break;
                case QUIC_STATUS_CONNECTION_TIMEOUT: {
                    MAD_LOG_DEBUG_I(client, "connection attempt timed out");
                } break;
            }
            MAD_LOG_INFO_I(client, "ClientConnectionCallback - shutdown initiated by peer");
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
            // TODO: Invoke on_client_disconnect callback?
            client.callbacks.on_disconnected(client.connection_ctx.get());
            client.connection_ctx.reset(nullptr);
            // If the app not initiated the cleanup for the connection
            // do it ourselves.
            if (!event.SHUTDOWN_COMPLETE.AppCloseInProgress) {
                api.ConnectionClose(chandle);
            }
        } break;
        case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED: {
            // TODO: Store resumption ticket for later?
            MAD_LOG_DEBUG_I(client, "Resumption ticket received {} byte(s)", event.RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
        } break;
        default: {
            MAD_LOG_WARN_I(client, "ClientConnectionCallback - unhandled event type: {}", std::to_underlying(event.Type));
        }
    }

    return QUIC_STATUS_NOT_SUPPORTED;
}

static QUIC_STATUS ClientConnectionCallback(HQUIC chandle, void * context, QUIC_CONNECTION_EVENT * event) {
    assert(chandle);
    assert(context);
    assert(event);

    auto & client     = *static_cast<mad::nexus::msquic_client *>(context);
    auto & msquic_ctx = mad::nexus::o2i(client.msquic_ctx());
    auto & cctx       = *client.connection_ctx;
    auto & api        = *msquic_ctx.api.get();
    return ClientConnectionCallbackImpl(chandle, client, api, cctx, *event);
}

namespace mad::nexus {
    msquic_client::msquic_client(quic_configuration cfg) : quic_base(cfg), msquic_base(cfg) {}

    msquic_client::~msquic_client() = default;

    std::error_code msquic_client::connect(std::string_view target, std::uint16_t port) {
        return o2i(msquic_ctx()).connect(target, port, ClientConnectionCallback, this);
    }
} // namespace mad::nexus
