#include <mad/log_macros.hpp>
#include <mad/log_printer.hpp>
#include <mad/macros.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>

#include <msquic.hpp>

#include <utility>

namespace mad::nexus {
QUIC_STATUS
StreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event);
} // namespace mad::nexus

/**
 */
static MAD_ALWAYS_INLINE QUIC_STATUS ClientConnectionEventPeerStreamStarted(
    const decltype(QUIC_CONNECTION_EVENT::PEER_STREAM_STARTED) & event,
    mad::nexus::msquic_client & client,
    mad::nexus::connection_context & connection_context) {
    auto new_stream = event.Stream;

    std::shared_ptr<void> stream_shared_ptr{ new_stream, [](void * sp) {
                                                MsQuic->StreamClose(
                                                    static_cast<HQUIC>(sp));
                                            } };

    const auto & [itr, inserted] = connection_context.streams.emplace(
        std::move(stream_shared_ptr),
        mad::nexus::stream_context{ new_stream, connection_context,
                                    client.callbacks.on_stream_data_received });
    if (inserted) {
        MAD_LOG_DEBUG_I(client, "Client peer stream started!");
        auto & sctx = itr->second;
        MsQuic->SetCallbackHandler(
            new_stream, reinterpret_cast<void *>(mad::nexus::StreamCallback),
            static_cast<void *>(&sctx));
    } else {
        // What to return here?
    }

    return QUIC_STATUS_SUCCESS;
}

static MAD_ALWAYS_INLINE QUIC_STATUS ClientConnectionCallbackImpl(
    MsQuicConnection * chandle, mad::nexus::msquic_client & client,
    mad::nexus::connection_context & connection_context,
    QUIC_CONNECTION_EVENT & event) {
    MAD_LOG_INFO_I(client, "ClientConnectionCallback() - Event Type: `{}`",
                   std::to_underlying(event.Type));

    switch (event.Type) {
        case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
            return ClientConnectionEventPeerStreamStarted(
                event.PEER_STREAM_STARTED, client, connection_context);

        case QUIC_CONNECTION_EVENT_CONNECTED: {
            auto & v = event.CONNECTED;
            MAD_LOG_INFO_I(client,
                           "Client connected, resumed: {}, negotiated_alpn: {}",
                           v.SessionResumed,
                           std::string_view{
                               reinterpret_cast<const char *>(v.NegotiatedAlpn),
                               v.NegotiatedAlpnLength });
            client.connection_ctx =
                std::make_unique<mad::nexus::connection_context>(chandle);
            assert(client.callbacks.on_connected);
            client.callbacks.on_connected(client.connection_ctx.get());
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
            auto & v = event.SHUTDOWN_INITIATED_BY_PEER;
            MAD_LOG_INFO_I(client, "connection shutdown by peer, error code {}",
                           v.ErrorCode);
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
            MAD_LOG_INFO_I(
                client,
                "ClientConnectionCallback - shutdown initiated by peer");
        } break;
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
            // TODO: Invoke on_client_disconnect callback?
            client.callbacks.on_disconnected(client.connection_ctx.get());
            client.connection_ctx.reset(nullptr);
            // If the app not initiated the cleanup for the connection
            // do it ourselves.
            if (!event.SHUTDOWN_COMPLETE.AppCloseInProgress) {
                MsQuic->ConnectionClose(chandle->Handle);
            }
        } break;
        case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED: {
            // TODO: Store resumption ticket for later?
            MAD_LOG_DEBUG_I(
                client, "Resumption ticket received {} byte(s)",
                event.RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
        } break;
        default: {
            MAD_LOG_WARN_I(
                client, "ClientConnectionCallback - unhandled event type: {}",
                std::to_underlying(event.Type));
        }
    }

    return QUIC_STATUS_NOT_SUPPORTED;
}

static QUIC_STATUS ClientConnectionCallback(MsQuicConnection * connection,
                                            void * context,
                                            QUIC_CONNECTION_EVENT * event) {
    assert(connection);
    assert(context);
    assert(event);

    auto & client = *static_cast<mad::nexus::msquic_client *>(context);
    auto & cctx = *client.connection_ctx;
    return ClientConnectionCallbackImpl(connection, client, cctx, *event);
}

namespace mad::nexus {
msquic_client::msquic_client(const msquic_application & app) :
    msquic_base(), application(app) {}

msquic_client::~msquic_client() = default;

std::error_code msquic_client::connect(std::string_view target,
                                       std::uint16_t port) {

    connection_ptr = std::make_shared<MsQuicConnection>(
        application.registration(), MsQuicCleanUpMode::CleanUpManual,
        ClientConnectionCallback, this);

    auto & connection = *reinterpret_cast<MsQuicConnection *>(
        connection_ptr.get());

    if (!connection.IsValid()) {
        return quic_error_code::connection_initialization_failed;
    }

    auto target_str = std::string{ target };
    if (QUIC_FAILED(connection.Start(
            application.configuration(), target_str.c_str(), port))) {
        return quic_error_code::connection_start_failed;
    }

    return quic_error_code::success;
}
} // namespace mad::nexus
