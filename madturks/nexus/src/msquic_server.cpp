

#include <flatbuffers/detached_buffer.h>
#include <mad/nexus/msquic/msquic_server.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/circular_buffer_vm.hpp>
#include <mad/concurrent.hpp>
#include <mad/nexus/quic_connection_context.hpp>

#include <memory>
#include <expected>
#include <netinet/in.h>
#include <system_error>
#include <thread>

#include <msquic.h>
#include <fmt/format.h>

#include <mad/nexus/msquic/msquic_api.inl>

namespace mad::nexus {

    msquic_server::~msquic_server() = default;

    /*
        Generally, MsQuic creates multiple threads to parallelize work, and therefore will make parallel/overlapping upcalls to the
       application, but not for the same connection. All upcalls to the app for a single connection and all child streams are always
       delivered serially. This is not to say, though, it will always be on the same thread. MsQuic does support the ability to shuffle
       connections around to better balance the load.
    */

    static QUIC_STATUS ServerConnectionCallback(HQUIC chandle, void * context, QUIC_CONNECTION_EVENT * event) {
        assert(context);
        auto & server        = *static_cast<msquic_server *>(context);
        auto & msquic_ctx    = o2i(server.msquic_impl());

        QUIC_ADDR_STR remote = [&]() {
            QUIC_ADDR remote_addr;
            std::uint32_t addr_size = sizeof(QUIC_ADDR);
            msquic_ctx.api->GetParam(chandle, QUIC_PARAM_CONN_REMOTE_ADDRESS, &addr_size, &remote_addr);
            QUIC_ADDR_STR str;
            QuicAddrToString(&remote_addr, &str);
            return str;
        }();

        switch (event->Type) {
            case QUIC_CONNECTION_EVENT_CONNECTED: {
                fmt::println("New client connected: {}", remote.Address);

                auto wa = server.connections().exclusive_access();
                if (auto itr = wa->find(static_cast<void *>(chandle)); itr == wa->end()) {
                    if (const auto & [citr, emplaced] = wa->emplace(chandle, connection_context{chandle, {}}); !emplaced) {
                        fmt::println("connection could not be stored!");
                        msquic_ctx.api->ConnectionClose(chandle);
                        return QUIC_STATUS_SUCCESS;
                    } else {
                        msquic_ctx.api->ConnectionSendResumptionTicket(chandle, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, nullptr);

                        // Notify app
                        assert(server.callbacks.on_connected);
                        server.callbacks.on_connected(&citr->second);
                    }
                } else {
                    fmt::println("duplicate connection event for connection {}!", static_cast<void *>(chandle));
                }
            } break;
            case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
                auto wa = server.connections().exclusive_access();
                if (auto itr = wa->find(chandle); itr == wa->end()) {
                    fmt::println("connection shutdown complete but no such connection in map!");
                } else {
                    fmt::println("Removed connection from the connection map");
                    // Notify app
                    assert(server.callbacks.on_disconnected);
                    server.callbacks.on_disconnected(&itr->second);
                    wa->erase(itr);
                }
                fmt::println("Connection shutdown complete {}", remote.Address);
                // Release the connection for good.
                msquic_ctx.api->ConnectionClose(chandle);
            } break;

            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {
                if (event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_IDLE) {
                    fmt::println("Connection shut down on idle.");
                } else {
                    fmt::println("Connection shut down by transport, status: {}", event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
                }
            } break;
            case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
                fmt::println("Connection shut down by peer, error code: {}", event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
            } break;

            case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
                // We shouldn't receive peer_stream_started event
                // as we're not allowing peer initiated streams.
                // Shutdown them directly.
                auto shandle = event->PEER_STREAM_STARTED.Stream;
                msquic_ctx.api->StreamClose(shandle);
            } break;
            case QUIC_CONNECTION_EVENT_RESUMED: {
                fmt::println("Connection resumed!");
            } break;
        }

        return QUIC_STATUS_SUCCESS;
    }

    static QUIC_STATUS ServerListenerCallback([[maybe_unused]] HQUIC Listener, void * context, QUIC_LISTENER_EVENT * Event) {

        assert(context);
        auto & server     = *static_cast<msquic_server *>(context);
        auto & msquic_ctx = o2i(server.msquic_impl());

        fmt::println("ServerListenerCallback() - Event Type: `{}`", static_cast<int>(Event->Type));

        switch (Event->Type) {
            case QUIC_LISTENER_EVENT_NEW_CONNECTION: {
                fmt::println("Listener received a new connection.");
                // A new connection is being attempted by a client. For the handshake to
                // proceed, the server must provide a configuration for QUIC to use. The
                // app MUST set the callback handler before returning.
                msquic_ctx.api->SetCallbackHandler(Event->NEW_CONNECTION.Connection, reinterpret_cast<void *>(ServerConnectionCallback),
                                                   &server);
                return msquic_ctx.api->ConnectionSetConfiguration(Event->NEW_CONNECTION.Connection, msquic_ctx.configuration.get());
            }
            case QUIC_LISTENER_EVENT_STOP_COMPLETE:
                break;
        }
        return QUIC_STATUS_NOT_SUPPORTED;
    }

    std::error_code msquic_server::listen() {
        if (!msquic_impl()) {
            return quic_error_code::uninitialized;
        }

        return o2i(msquic_impl()).listen(config.udp_port_number, config.alpn, ServerListenerCallback, this);
    }

} // namespace mad::nexus
