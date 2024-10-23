/******************************************************
 * quic_client implementation with MSQUIC.
 *
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/log_macros.hpp>
#include <mad/log_printer.hpp>
#include <mad/macros.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream.hpp>

#include <msquic.hpp>

#include <utility>

namespace mad::nexus {

// This function is defined in msquic_base.cpp.
QUIC_STATUS
StreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event);

/******************************************************
 * The type aliases for the connection events handled by
 * the callback functions. They don't have a named type
 * (anonymous) so the code uses decltype() to determine
 * the type and give them a nice name.
 ******************************************************/
namespace events {
    using peer_stream_started =
        decltype(QUIC_CONNECTION_EVENT::PEER_STREAM_STARTED);
    using connected = decltype(QUIC_CONNECTION_EVENT::CONNECTED);
    using shutdown_complete =
        decltype(QUIC_CONNECTION_EVENT::SHUTDOWN_COMPLETE);
}; // namespace events

/******************************************************
 * The callback functions given to the MSQUIC API functions.
 * All callbacks are wrapped into a struct to allow them
 * to access the msquic_client object's private parts.
 ******************************************************/
struct MsQuicClientCallbacks {

    /******************************************************
     * Convert a QUIC_CONNECTION_EVENT_TYPE value to a
     * human-readable string.
     *
     * It is guaranteed for QUIC_CONNECTION_EVENT_TYPE to have
     * a string representation.
     *
     * @param etype QUIC_CONNECTION_EVENT_TYPE to stringify
     * @return Enum's string description.
     ******************************************************/
    static constexpr std::string_view
    quic_connection_event_to_str(int etype) noexcept {
        MAD_EXHAUSTIVE_SWITCH_BEGIN
        switch (static_cast<QUIC_CONNECTION_EVENT_TYPE>(etype)) {
            using enum QUIC_CONNECTION_EVENT_TYPE;
            case QUIC_CONNECTION_EVENT_CONNECTED:
                return "QUIC_CONNECTION_EVENT_CONNECTED";
            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
                return "QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_"
                       "TRANSPORT";
            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
                return "QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER";
            case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
                return "QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE";
            case QUIC_CONNECTION_EVENT_LOCAL_ADDRESS_CHANGED:
                return "QUIC_CONNECTION_EVENT_LOCAL_ADDRESS_CHANGED";
            case QUIC_CONNECTION_EVENT_PEER_ADDRESS_CHANGED:
                return "QUIC_CONNECTION_EVENT_PEER_ADDRESS_CHANGED";
            case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
                return "QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED";
            case QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE:
                return "QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE";
            case QUIC_CONNECTION_EVENT_PEER_NEEDS_STREAMS:
                return "QUIC_CONNECTION_EVENT_PEER_NEEDS_STREAMS";
            case QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED:
                return "QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED";
            case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
                return "QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED";
            case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
                return "QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED";
            case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
                return "QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED";
            case QUIC_CONNECTION_EVENT_RESUMED:
                return "QUIC_CONNECTION_EVENT_RESUMED";
            case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
                return "QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED";
            case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED:
                return "QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED";
#ifdef QUIC_API_ENABLE_PREVIEW_FEATURES
            case QUIC_CONNECTION_EVENT_RELIABLE_RESET_NEGOTIATED:
                return "QUIC_CONNECTION_EVENT_RELIABLE_RESET_NEGOTIATED";
            case QUIC_CONNECTION_EVENT_ONE_WAY_DELAY_NEGOTIATED:
                return "QUIC_CONNECTION_EVENT_ONE_WAY_DELAY_NEGOTIATED";
            case QUIC_CONNECTION_EVENT_NETWORK_STATISTICS:
                return "QUIC_CONNECTION_EVENT_NETWORK_STATISTICS";
#endif
        }
        MAD_EXHAUSTIVE_SWITCH_END

        // If the code reaches here it's a hard failure
        // because the switch above is meant to be exhaustive.
        std::unreachable();
    }

    /******************************************************
     * PEER_STREAM_STARTED event handler
     *
     * @param client The client, who owns the event source
     * @param event Event details
     * @return QUIC_SUCCESS
     ******************************************************/
    static MAD_ALWAYS_INLINE QUIC_STATUS ClientConnectionEventPeerStreamStarted(
        msquic_client & client, const events::peer_stream_started & event) {
        assert(client.connection);
        auto new_stream = event.Stream;

        stream_callbacks scbs = {
            .on_start = client.callbacks.on_stream_start,
            .on_close = client.callbacks.on_stream_close,
            .on_data_received = client.callbacks.on_stream_data_received
        };

        return client.connection
            ->add_new_stream({ new_stream,
                               [](void * sp) {
                                   MsQuic->StreamClose(static_cast<HQUIC>(sp));
                               } },
                             scbs)
            .and_then([&](auto && v) noexcept
                      -> result<std::reference_wrapper<stream>> {
                MAD_LOG_DEBUG_I(client, "Client peer stream started!");
                MsQuic->SetCallbackHandler(
                    new_stream, reinterpret_cast<void *>(StreamCallback),
                    static_cast<void *>(&v.get()));
                return v;
            })
            .transform([](auto &&) noexcept {
                return QUIC_STATUS_SUCCESS;
            })
            .value();
    }

    /******************************************************
     * CONNECTED event handler
     *
     * @param msquic_conn MsQuicConnection object
     * @param client The client, who owns the event source
     * @param event Event details
     * @return QUIC_SUCCESS
     ******************************************************/
    static MAD_ALWAYS_INLINE QUIC_STATUS ClientConnectionEventConnected(
        MsQuicConnection & msquic_conn, msquic_client & client,
        const events::connected & event) {

        MAD_LOG_INFO_I(client,
                       "Client connected, resumed?: {}, negotiated_alpn: {}",
                       event.SessionResumed,
                       std::string_view{
                           reinterpret_cast<const char *>(event.NegotiatedAlpn),
                           event.NegotiatedAlpnLength });
        client.connection = std::make_unique<connection>(&msquic_conn);
        assert(client.callbacks.on_connected);
        client.callbacks.on_connected(*(client.connection.get()));

        return QUIC_STATUS_SUCCESS;
    }

    /******************************************************
     * SHUTDOWN_COMPLETE event handler
     *
     *
     * @param client The client, who owns the event source
     * @param event Event details
     * @return QUIC_SUCCESS
     ******************************************************/
    static MAD_ALWAYS_INLINE QUIC_STATUS ClientConnectionEventShutdownComplete(
        msquic_client & client, const events::shutdown_complete & event) {
        MAD_LOG_DEBUG_I(client,
                        "ClientConnectionEventShutdownComplete - "
                        "HandshakeCompleted?:{} "
                        "PeerAckdShutdown?: {} AppCloseInProgress?: {}",
                        event.HandshakeCompleted,
                        event.PeerAcknowledgedShutdown,
                        event.AppCloseInProgress);
        // TODO: Invoke on_client_disconnect callback?
        if (client.connection) {
            client.callbacks.on_disconnected(*(client.connection.get()));
            client.connection.reset(nullptr);
        }

        MAD_ENSURES(nullptr == client.connection);
        return QUIC_STATUS_SUCCESS;
    }

    /******************************************************
     * The event dispatcher function.
     * Dispatches connection events to event handlers.
     *
     * @param connection The connection
     * @param context Callback context (msquic_client)
     * @param event Event details
     * @return Event handler's result when event is handled
     *         QUIC_STATUS_NOT_SUPPORTED otherwise.
     ******************************************************/
    static QUIC_STATUS ClientConnectionCallback(MsQuicConnection * connection,
                                                void * context,
                                                QUIC_CONNECTION_EVENT * event) {
        assert(connection);
        assert(context);
        assert(event);

        auto & client = *static_cast<msquic_client *>(context);

        MAD_LOG_INFO_I(client,
                       "ClientConnectionCallback() - Event Type: `{}` {}",
                       std::to_underlying(event->Type),
                       quic_connection_event_to_str(event->Type));

        switch (event->Type) {
            case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
                return ClientConnectionEventPeerStreamStarted(
                    client, event->PEER_STREAM_STARTED);
            case QUIC_CONNECTION_EVENT_CONNECTED:
                return ClientConnectionEventConnected(
                    *connection, client, event->CONNECTED);
            case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
                return ClientConnectionEventShutdownComplete(
                    client, event->SHUTDOWN_COMPLETE);
            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
                auto & v = event->SHUTDOWN_INITIATED_BY_PEER;
                MAD_LOG_INFO_I(client,
                               "connection shutdown by peer, error code {}",
                               v.ErrorCode);
                return QUIC_STATUS_SUCCESS;
            }

            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {
                auto & v = event->SHUTDOWN_INITIATED_BY_TRANSPORT;
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
                MAD_LOG_INFO_I(client, "ClientConnectionCallback - "
                                       "shutdown initiated by peer");
                return QUIC_STATUS_SUCCESS;
            }

            case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED: {
                // TODO: Store resumption ticket for later?
                MAD_LOG_DEBUG_I(
                    client, "Resumption ticket received {} byte(s)",
                    event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
                return QUIC_STATUS_SUCCESS;
            }

            default: {
                MAD_LOG_WARN_I(
                    client,
                    "ClientConnectionCallback - unhandled event type: {}",
                    std::to_underlying(event->Type));
            }
        }

        return QUIC_STATUS_NOT_SUPPORTED;
    }
};

/******************************************************/

msquic_client::msquic_client(const msquic_application & app) :
    msquic_base(), application(app) {}

/******************************************************/

msquic_client::~msquic_client() = default;

/******************************************************/

auto msquic_client::connect(std::string_view target,
                            std::uint16_t port) -> result<> {

    auto ptr = std::make_shared<MsQuicConnection>(
        application.registration(), MsQuicCleanUpMode::CleanUpManual,
        &MsQuicClientCallbacks::ClientConnectionCallback, this);

    auto & conn = *ptr.get();

    if (!conn.IsValid()) {
        return std::unexpected(
            quic_error_code::connection_initialization_failed);
    }

    auto target_str = std::string{ target };
    if (QUIC_FAILED(conn.Start(
            application.configuration(), target_str.c_str(), port))) {
        return std::unexpected(quic_error_code::connection_start_failed);
    }

    msquic_connection_ptr = ptr;

    return {};
}

/******************************************************/

auto msquic_client::disconnect() -> result<> {
    return std::unexpected(quic_error_code::not_yet_implemented);
}
} // namespace mad::nexus
