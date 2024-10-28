#include <mad/circular_buffer_vm.hpp>
#include <mad/concurrent.hpp>
#include <mad/log>
#include <mad/macro>
#include <mad/nexus/msquic/msquic_server.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_error_code.hpp>

#include <flatbuffers/detached_buffer.h>
#include <fmt/format.h>
#include <msquic.h>

#include <netinet/in.h>

#include <expected>
#include <memory>
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
    static QUIC_ADDR_STR get_remote_address(QUIC_HANDLE * connection,
                                            const QUIC_API_TABLE * api) {
        QUIC_ADDR remote_addr;
        std::uint32_t addr_size = sizeof(QUIC_ADDR);
        api->GetParam(connection, QUIC_PARAM_CONN_REMOTE_ADDRESS, &addr_size,
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
        auto remote = get_remote_address(
            new_connection, server.application.api());
        MAD_LOG_INFO_I(server, "New client connected: {}", remote.Address);

        std::shared_ptr<QUIC_HANDLE> connection_shared_ptr{
            new_connection,
            [api = server.application.api()](QUIC_HANDLE * sp) {
                api->ConnectionClose(sp);
            }
        };

        return server.add(connection_shared_ptr, connection_shared_ptr.get())
            .and_then([&](auto && v) {
                server.application.api()->ConnectionSendResumptionTicket(
                    v.get().template handle_as<HQUIC>(),
                    QUIC_SEND_RESUMPTION_FLAG_NONE, 0, nullptr);
                MAD_EXPECTS(server.callbacks.on_connected);
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
        QUIC_HANDLE * connection, const shutdown_complete_event & event,
        msquic_server & server) {

        if (event.AppCloseInProgress) {
            return QUIC_STATUS_SUCCESS;
        }

        return server.erase(connection)
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
                server.application.api()->StreamClose(
                    event->PEER_STREAM_STARTED.Stream);
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
        server.application.api()->SetCallbackHandler(
            event.Connection,
            reinterpret_cast<void *>(ServerConnectionCallback), &server);
        return server.application.api()->ConnectionSetConfiguration(
            event.Connection, server.application.configuration());
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
    static QUIC_STATUS ServerListenerCallback(HQUIC hlistener, void * context,
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

msquic_server::~msquic_server() = default;

auto msquic_server::listen(std::string_view alpn,
                           std::uint16_t port) -> result<> {

    // Omit nul terminators from alpn if present
    if (auto fi = alpn.find('\n'); fi != alpn.npos) {
        alpn.remove_suffix(alpn.size() - fi);
    }

    {
        HQUIC listener_handle{ nullptr };

        if (auto r = application.api()->ListenerOpen(
                application.registration(),
                &MsQuicServerCallbacks::ServerListenerCallback, this,
                &listener_handle);
            QUIC_FAILED(r)) {
            return std::unexpected{
                quic_error_code::listener_initialization_failed
            };
        }

        listener = std::shared_ptr<QUIC_HANDLE>(
            listener_handle, [api = application.api()](QUIC_HANDLE * h) {
                MAD_EXPECTS(h);
                MAD_EXPECTS(api);
                api->ListenerClose(h);
            });

        MAD_ENSURES(listener);
    }

    // QUIC_BUFFER's Buffer pointer is mutable. Make a copy.
    std::vector<std::uint8_t> alpn_tmp{ alpn.begin(), alpn.end() };
    const QUIC_BUFFER q_alpn{ static_cast<std::uint32_t>(alpn_tmp.size()),
                              alpn_tmp.data() };

    QUIC_ADDR addr = {};
    // Bind to 0.0.0.0
    QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(&addr, port);

    if (auto r = application.api()->ListenerStart(
            listener.get(), &q_alpn, 1, &addr);
        QUIC_FAILED(r)) {
        return std::unexpected(quic_error_code::stream_start_failed);
    }

    return {};
}

} // namespace mad::nexus
