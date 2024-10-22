#include <mad/circular_buffer_vm.hpp>
#include <mad/concurrent.hpp>
#include <mad/log>
#include <mad/macro>
#include <mad/nexus/msquic/msquic_server.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_error_code.hpp>

#include <flatbuffers/detached_buffer.h>
#include <fmt/format.h>
#include <msquic.hpp>

#include <netinet/in.h>

#include <expected>
#include <memory>
#include <system_error>
#include <utility>

namespace mad::nexus {

/**
 * Convenience type for easy befriending.
 */
struct MsQuicServerCallbacks {

    using connected_event = decltype(QUIC_CONNECTION_EVENT::CONNECTED);
    using shutdown_complete_event =
        decltype(QUIC_CONNECTION_EVENT::SHUTDOWN_COMPLETE);
    using new_connection_event = decltype(QUIC_LISTENER_EVENT::NEW_CONNECTION);

    /**
     * Get the remote endpoint address of a connection
     * in QUIC_ADDR_STR forat.
     *
     * @param connection The connection
     *
     * @return QUIC_ADDR_STR address in QUIC_ADDR_STR forat.
     */
    static QUIC_ADDR_STR get_remote_address(HQUIC connection) {
        QUIC_ADDR remote_addr;
        std::uint32_t addr_size = sizeof(QUIC_ADDR);
        MsQuic->GetParam(connection, QUIC_PARAM_CONN_REMOTE_ADDRESS, &addr_size,
                         &remote_addr);
        QUIC_ADDR_STR str;
        QuicAddrToString(&remote_addr, &str);
        return str;
    }

    /**
     * New connection handler function
     *
     * @param new_connection New connection handle
     * @param event New connection event details
     * @param server The owning server
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    static MAD_ALWAYS_INLINE QUIC_STATUS ServerConnectionEventConnected(
        HQUIC new_connection, [[maybe_unused]] const connected_event & event,
        msquic_server & server) {
        auto remote = get_remote_address(new_connection);
        MAD_LOG_INFO_I(server, "New client connected: {}", remote.Address);

        std::shared_ptr<void> connection_shared_ptr{
            new_connection,
            [](void * sp) {
                MsQuic->ConnectionClose(static_cast<HQUIC>(sp));
            }
        };

        return server.add_new_connection(connection_shared_ptr)
            .and_then([&](auto && v) {
                MsQuic->ConnectionSendResumptionTicket(
                    v.get().template handle_as<HQUIC>(),
                    QUIC_SEND_RESUMPTION_FLAG_NONE, 0, nullptr);
                assert(server.callbacks.on_connected);
                // Notify app
                server.callbacks.on_connected(v.get());
                return result<QUIC_STATUS>{ QUIC_STATUS_SUCCESS };
            })
            .or_else([&](auto &&) {
                MAD_LOG_ERROR_I(server, "connection could not be stored!");
                return result<QUIC_STATUS>{ QUIC_STATUS_SUCCESS };
            })
            .value();
    }

    /**
     * Connection shutdown handler function.
     *
     * @param connection The connection that is shut down
     * @param event Shutdown event details
     * @param server Owning server
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    static MAD_ALWAYS_INLINE QUIC_STATUS ServerConnectionEventShutdownCompleted(
        HQUIC connection,
        [[maybe_unused]] const shutdown_complete_event & event,
        msquic_server & server) {

        if (event.AppCloseInProgress) {
            return QUIC_STATUS_SUCCESS;
        }

        return server.remove_connection(connection)
            .and_then([&](auto && v) {
                server.callbacks.on_disconnected(v.mapped());
                return result<QUIC_STATUS>{ QUIC_STATUS_SUCCESS };
            })
            .or_else([&](auto &&) {
                MAD_LOG_DEBUG_I(server,
                                "connection shutdown complete but no such "
                                "connection in map!");
                return result<QUIC_STATUS>{ QUIC_STATUS_SUCCESS };
            })
            .value();
    }

    /**
     * Server connection callback dispatcher function.
     *
     * Events from accepted connections will land here.
     *
     * @param chandle Subject
     * @param context Context pointer (owning server)
     * @param event MSQUIC event describing what happened
     -
     * @return QUIC_STATUS Return code indicating callback result
     */
    static QUIC_STATUS ServerConnectionCallback(HQUIC chandle, void * context,
                                                QUIC_CONNECTION_EVENT * event) {
        assert(chandle);
        assert(context);
        assert(event);
        auto & server = *static_cast<msquic_server *>(context);
        // We're only handling the connected and shutdown completed
        // events. Rest are for logging purposes.

        MAD_LOG_INFO_I(server, "Server connection callback {}",
                       std::to_underlying(event->Type));

        switch (event->Type) {
            case QUIC_CONNECTION_EVENT_CONNECTED:
                return ServerConnectionEventConnected(
                    chandle, event->CONNECTED, server);

            case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
                return ServerConnectionEventShutdownCompleted(
                    chandle, event->SHUTDOWN_COMPLETE, server);

            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {
                if (event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status ==
                    QUIC_STATUS_CONNECTION_IDLE) {
                    MAD_LOG_INFO_I(server, "Connection shut down on idle.");
                } else {
                    MAD_LOG_INFO_I(
                        server, "Connection shut down by transport, status: {}",
                        event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
                }
            } break;
            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
                MAD_LOG_INFO_I(server,
                               "Connection shut down by peer, error code: {}",
                               event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
            } break;

            case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
                // We shouldn't receive peer_stream_started event
                // as we're not allowing peer initiated streams.
                // Shutdown them directly.
                MsQuic->StreamClose(event->PEER_STREAM_STARTED.Stream);
            } break;
            case QUIC_CONNECTION_EVENT_RESUMED: {
                MAD_LOG_INFO_I(server, "Connection resumed!");
            } break;

            default: {
                MAD_LOG_WARN_I(server, "Unhandled connection event: {}",
                               std::to_underlying(event->Type));
            } break;
        }

        return QUIC_STATUS_SUCCESS;
    }

    /**
     * Called when listener receives a new connection
     *
     * @param event New connection event details
     * @param server The owning server
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    static MAD_ALWAYS_INLINE QUIC_STATUS ServerListenerNewConnection(
        const new_connection_event & event, msquic_server & server) {

        MAD_LOG_INFO_I(server, "Listener received a new connection.");
        // A new connection is being attempted by a client. For the handshake to
        // proceed, the server must provide a configuration for QUIC to use. The
        // app MUST set the callback handler before returning.
        MsQuic->SetCallbackHandler(
            event.Connection,
            reinterpret_cast<void *>(ServerConnectionCallback), &server);
        return MsQuic->ConnectionSetConfiguration(
            event.Connection, server.application.configuration().Handle);
    }

    /**
     * Listener event handler
     *
     * @param hlistener Handle of the listener
     * @param context The context pointer (owning server)
     * @param event Event object containing the event details
     *
     * @return QUIC_STATUS Return code indicating callback result
     */
    static QUIC_STATUS ServerListenerCallback(MsQuicListener * hlistener,
                                              void * context,
                                              QUIC_LISTENER_EVENT * event) {
        assert(hlistener);
        assert(context);
        assert(event);

        auto & server = *static_cast<msquic_server *>(context);

        MAD_LOG_INFO_I(server, "ServerListenerCallback() - Event Type: `{}`",
                       std::to_underlying(event->Type));

        switch (event->Type) {
            case QUIC_LISTENER_EVENT_NEW_CONNECTION: {
                return ServerListenerNewConnection(
                    event->NEW_CONNECTION, server);
            }
            case QUIC_LISTENER_EVENT_STOP_COMPLETE:
                break;
        }
        return QUIC_STATUS_NOT_SUPPORTED;
    }
};

msquic_server::msquic_server(const msquic_application & app) :
    msquic_base(), application(app) {}

msquic_server::~msquic_server() = default;

auto msquic_server::listen() -> result<> {

    const auto & config = application.get_config();

    listener_opaque = std::make_shared<MsQuicListener>(
        application.registration(), MsQuicCleanUpMode::CleanUpManual,
        &MsQuicServerCallbacks::ServerListenerCallback, this);

    MsQuicListener & listener = *reinterpret_cast<MsQuicListener *>(
        listener_opaque.get());
    if (!listener.IsValid()) {
        return std::unexpected{ quic_error_code::listener_start_failed };
    }

    MsQuicAlpn alpn{ config.alpn.c_str() };

    QUIC_ADDR Address = {};
    // Bind to 0.0.0.0
    QuicAddrSetFamily(&Address, QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(&Address, config.udp_port_number);

    if (QUIC_FAILED(listener.Start(alpn, &Address))) {
        return std::unexpected{ quic_error_code::listener_start_failed };
    }

    return {};
}

} // namespace mad::nexus
