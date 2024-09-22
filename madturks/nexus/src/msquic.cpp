

#include <flatbuffers/detached_buffer.h>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/msquic.hpp>
#include <mad/circular_buffer_vm.hpp>
#include <mad/concurrent.hpp>

#include <memory>
#include <expected>
#include <functional>
#include <filesystem>
#include <netinet/in.h>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <msquic.h>
#include <fmt/format.h>
#include <list>
#include <map>
#include <optional>
#include <mutex>

namespace {

    using circular_buffer_t = mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;

    /**
     * Since HQUIC is a pointer type itself, we have to define
     * a deleter type with ::pointer typedef, otherwise the default
     * deleter assumes HQUIC*.
     */
    struct msquic_handle_deleter {
        using pointer = HQUIC;

        // Constructor that accepts a deleter function. If no deleter function is provided,
        // it defaults to an empty function.
        msquic_handle_deleter(std::function<void(HQUIC)> deleter_fn = {}) : deleter(deleter_fn) {}

        void operator()(HQUIC h) {
            if (deleter) {
                deleter(h);
            }
        }

    private:
        std::function<void(HQUIC)> deleter;
    };
} // namespace

namespace mad::nexus {

    struct stream_context {

        using send_buffer_map_t = std::map<std::uint64_t, flatbuffers::DetachedBuffer>;

        /**
         * Construct a new stream context object
         *
         * @param owner The owning MSQUIC connection
         * @param receive_buffer_size Size of the receive circular buffer.
         * @param send_buffer_size  Size of the send circular buffer.
         */
        stream_context(HQUIC stream, HQUIC owner, std::size_t receive_buffer_size = 16384) :
            stream_handle(stream), owning_connection(owner), receive_buffer(receive_buffer_size, circular_buffer_t::auto_align_to_page{}),
            mtx(std::make_unique<std::mutex>()) {}

        inline HQUIC stream() const {
            return stream_handle;
        }

        inline HQUIC connection() const {
            return owning_connection;
        }

        inline circular_buffer_t & rbuf() {
            return receive_buffer;
        }

        inline const circular_buffer_t & rbuf() const {
            return receive_buffer;
        }

        template <typename... Args>
        inline auto store_buffer(Args &&... args) -> std::optional<typename send_buffer_map_t::iterator> {

            std::unique_lock<std::mutex> lock(*mtx);

            auto result = buffers_in_flight_.emplace(send_buffer_serial, std::forward<Args>(args)...);

            if (!result.second) {
                fmt::println("insert failed somehow?");
                return std::nullopt;
            }

            // Increase the serial for the next packet.
            ++send_buffer_serial;
            fmt::println("new serial {}", send_buffer_serial);

            return std::optional{result.first};
        }

        /**
         * Release a send buffer from in-flight buffer map.
         *
         * @param key The key of the buffer, previously obtained from
         *            the store_buffer(...) call.
         *
         * @return true when release successful
         * @return false otherwise
         */
        inline auto release_buffer(std::uint64_t key) -> bool {
            std::unique_lock<std::mutex> lock(*mtx);
            return buffers_in_flight_.erase(key) > 0;
        }

        auto in_flight_count() const {
            return buffers_in_flight_.size();
        }

    private:
        /**
         * Msquic stream handle
         */
        HQUIC stream_handle;
        /**
         * The connection that stream belongs to.
         */
        HQUIC owning_connection;
        /**
         * Data received from the stream
         * will go into this buffer.
         *
         * We don't need to protect the receive buffer
         * as all received stream data for a specific
         * connection guaranteed to happen serially.
         */
        circular_buffer_t receive_buffer;

        std::unique_ptr<std::mutex> mtx;
        std::uint64_t send_buffer_serial = {0};

        std::map<std::uint64_t, flatbuffers::DetachedBuffer> buffers_in_flight_;
    };

    // Stream contexts are going to be stored in connection context.
    struct connection_context {
        HQUIC connection;
        std::unordered_map<HQUIC, stream_context> streams;
    };

    struct msquic_ctx {

        using msquic_api_uptr_t            = std::unique_ptr<const QUIC_API_TABLE, decltype(&MsQuicClose)>;
        using msquic_handle_uptr_t         = std::unique_ptr<QUIC_HANDLE, msquic_handle_deleter>;

        // Do not reorder these, as it will change the destruction
        // order and will crash the application on shutdown..
        msquic_api_uptr_t api              = {nullptr, {}};
        msquic_handle_uptr_t registration  = {nullptr, {}};
        msquic_handle_uptr_t configuration = {nullptr, {}};
        msquic_handle_uptr_t listener      = {nullptr, {}};

        mad::concurrent<std::unordered_map<HQUIC, connection_context>> connection_map;

        /**
         * Start listening on @p udp_port.
         *
         * @param udp_port UDP port number
         * @param alpn_sv ALPN string
         * @param callback Listener callback handler
         * @param context Context to pass the listener callback
         *
         * @returns success if listening started without any issues
         * @returns quic_error_code::uninitialized if context is uninitialized
         * @returns quic_error_code::already_listening if there's another listener alive
         * @returns quic_error_code::listener_initialization_failed if ListenerOpen fails
         * @returns quic_error_code::listener_start_failed if ListenerStart fails
         */
        quic_error_code listen(std::uint16_t udp_port, std::string_view alpn_sv, QUIC_LISTENER_CALLBACK_HANDLER callback,
                               void * context = nullptr) {

            // Listening requires api, registration and configuration to be initialized.
            if (!(api && registration && configuration)) {
                return quic_error_code::uninitialized;
            }

            // If there's already a listener object active, that means we're already listening.
            if (listener) {
                return quic_error_code::already_listening;
            }

            // Create a new listener.
            listener = msquic_handle_uptr_t(
                [this, callback, context]() -> HQUIC {
                    HQUIC ptr = nullptr;
                    if (auto status = api->ListenerOpen(registration.get(), callback, context, &ptr); QUIC_FAILED(status)) {
                        // TODO: Maybe log?
                        return ptr;
                    }
                    return ptr;
                }(),
                msquic_handle_deleter([this](HQUIC handle) {
                    fmt::println("msquic_listener deleter called {}", static_cast<void *>(handle));
                    api->ListenerClose(handle);
                    fmt::println("msquic_listener deleter done");
                }));

            // If failed, return.
            if (!listener) {
                return mad::nexus::quic_error_code::listener_initialization_failed;
            }

            // Configures the address used for the listener to listen on all IP
            // addresses and the given UDP port.
            QUIC_ADDR Address = {};
            QuicAddrSetFamily(&Address, QUIC_ADDRESS_FAMILY_UNSPEC);
            QuicAddrSetPort(&Address, udp_port);

            const QUIC_BUFFER alpn = {static_cast<std::uint32_t>(alpn_sv.length()),
                                      reinterpret_cast<std::uint8_t *>(const_cast<char *>(alpn_sv.data()))};

            fmt::println("test {} {}", alpn_sv, alpn_sv.length());

            // Start listening
            if (auto status = api->ListenerStart(listener.get(), &alpn, 1, &Address); QUIC_FAILED(status)) {
                listener.reset(nullptr);
                return quic_error_code::listener_start_failed;
            }

            return quic_error_code::success;
        }

        static std::expected<std::unique_ptr<msquic_ctx>, quic_error_code> make(const quic_server_configuration & cfg) {

            auto ctx = std::make_unique<msquic_ctx>();

            ctx->api = msquic_api_uptr_t(
                []() -> const QUIC_API_TABLE * {
                    const QUIC_API_TABLE * ptr = {nullptr};
                    if (auto status = MsQuicOpen2(&ptr); QUIC_FAILED(status)) {
                        // TODO: Log status?
                        return ptr;
                    }
                    return ptr;
                }(),
                MsQuicClose);

            if (!ctx->api) {
                return std::unexpected(mad::nexus::quic_error_code::api_initialization_failed);
            }

            return initialize_msquic(cfg, std::move(ctx));
        }

    private:
        bool initialize_registration(std::string_view appname) {
            const QUIC_REGISTRATION_CONFIG RegConfig = {appname.data(), QUIC_EXECUTION_PROFILE_LOW_LATENCY};
            registration                             = msquic_handle_uptr_t(
                [this, &RegConfig]() -> HQUIC {
                    HQUIC reg;

                    if (auto status = api->RegistrationOpen(&RegConfig, &reg); QUIC_FAILED(status)) {
                        // TODO: Throw a proper exception for the operation.
                        fmt::println("registration open failed!");
                        return nullptr;
                    }

                    fmt::println("registration open OK");
                    return reg;
                }(),
                msquic_handle_deleter([this](HQUIC handle) {
                    fmt::println("msquic_registration deleter called {}", static_cast<void *>(handle));
                    api->RegistrationClose(handle);
                    fmt::println("msquic_registration deleter done");
                }));
            return static_cast<bool>(registration);
        }

        QUIC_SETTINGS settings_to_msquic(const quic_server_configuration & cfg) {
            QUIC_SETTINGS settings = {};

            // Configures the server's idle timeout.
            if (cfg.idle_timeout) {
                settings.IdleTimeoutMs       = static_cast<std::uint64_t>(cfg.idle_timeout->count());
                settings.IsSet.IdleTimeoutMs = true;
            }

            // Configures the server's resumption level to allow for resumption and
            // 0-RTT.
            settings.ServerResumptionLevel       = QUIC_SERVER_RESUME_AND_ZERORTT;
            settings.IsSet.ServerResumptionLevel = true;

            // Configures the server's settings to allow for the peer to open a single
            // bidirectional stream. By default connections are not configured to allow
            // any streams from the peer.
            settings.PeerBidiStreamCount         = 1;
            settings.IsSet.PeerBidiStreamCount   = true;
            return settings;
        }

        bool initialize_configuration(const mad::nexus::quic_server_configuration & cfg) {

            auto settings = settings_to_msquic(cfg);

            configuration = msquic_handle_uptr_t(
                [this, &cfg, &settings]() -> HQUIC {
                    HQUIC config;
                    const QUIC_BUFFER alpn = {static_cast<std::uint32_t>(cfg.alpn.length()),
                                              reinterpret_cast<std::uint8_t *>(const_cast<char *>(cfg.alpn.c_str()))};

                    if (auto status =

                            api->ConfigurationOpen(registration.get(), &alpn, 1, &settings, sizeof(settings), nullptr, &config);
                        QUIC_FAILED(status)) {

                        fmt::println("configuration open failed!");
                        return nullptr;
                    }
                    return config;
                }(),
                msquic_handle_deleter([this](HQUIC handle) {
                    fmt::println("msquic_configuration deleter called");
                    api->ConfigurationClose(handle);
                    fmt::println("msquic_configuration deleter done");
                }));

            return static_cast<bool>(configuration);
        }

        /**
         * Checks for the existence of certificate and private key files, loads them into a
         * QUIC credential configuration, and returns an error code if any step fails.
         */
        quic_error_code initialize_credentials(const mad::nexus::quic_credentials & creds) {

            if (!std::filesystem::exists(creds.certificate_path)) {
                return quic_error_code::missing_certificate;
            }

            if (!std::filesystem::exists(creds.private_key_path)) {
                return quic_error_code::missing_private_key;
            }

            QUIC_CERTIFICATE_FILE certificate;
            certificate.CertificateFile = creds.certificate_path.c_str();
            certificate.PrivateKeyFile  = creds.private_key_path.c_str();

            QUIC_CREDENTIAL_CONFIG credential_config;
            credential_config.Type            = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
            credential_config.CertificateFile = &certificate;

            if (auto s = api->ConfigurationLoadCredential(configuration.get(), &credential_config); QUIC_FAILED(s)) {
                return quic_error_code::configuration_load_credential_failed;
            }

            return quic_error_code::success;
        }

        /**
         * The function `initialize_msquic` initializes the msquic context with the provided server
         * configuration and returns an expected result with a unique pointer to the msquic context
         * or an error code.
         **/
        static std::expected<std::unique_ptr<msquic_ctx>, quic_error_code> initialize_msquic(const quic_server_configuration & cfg,
                                                                                             std::unique_ptr<msquic_ctx> ctx) {
            assert(ctx->api);
            if (!ctx->initialize_registration(cfg.appname)) {
                return std::unexpected(quic_error_code::registration_initialization_failed);
            }

            if (!ctx->initialize_configuration(cfg)) {
                return std::unexpected(quic_error_code::configuration_initialization_failed);
            }

            if (failed(ctx->initialize_credentials(cfg.credentials))) {
                return std::unexpected(quic_error_code::configuration_load_credential_failed);
            }

            return std::move(ctx);
        }
    };
} // namespace mad::nexus

namespace mad::nexus {

    /**
     * Opaque to impl.
     */
    inline msquic_ctx & o2i(const std::shared_ptr<void> & ptr) {
        return *static_cast<msquic_ctx *>(ptr.get());
    }

    static QUIC_STATUS ServerStreamCallback(HQUIC stream, void * context, QUIC_STREAM_EVENT * event) {
        // FIXME: This should be stream_context!
        assert(context);
        auto & sctx = *static_cast<stream_context *>(context);

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
                fmt::println("total {}", sctx.rbuf().consumed_space());

                fmt::println("Received data from the remote count:{} total_size:{}", event->RECEIVE.BufferCount,
                             event->RECEIVE.TotalBufferLength);

            } break;

            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
                std::stringstream aq;
                aq << std::this_thread::get_id();
                fmt::println("stream shutdown from thread {}", aq.str());
                // FIXME: Fix this::::
                // if (auto itr = ctx->streams.find(stream); itr == ctx->streams.end()) {
                //     fmt::println("stream shutdown");
                //     ctx->streams.erase(itr);
                // }
            } break;
        }

        // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
        return QUIC_STATUS_SUCCESS;
    }

    // connection type

    /*
        Generally, MsQuic creates multiple threads to parallelize work, and therefore will make parallel/overlapping upcalls to the
       application, but not for the same connection. All upcalls to the app for a single connection and all child streams are always
       delivered serially. This is not to say, though, it will always be on the same thread. MsQuic does support the ability to shuffle
       connections around to better balance the load.
    */

    static QUIC_STATUS ServerConnectionCallback(HQUIC connection, void * context, QUIC_CONNECTION_EVENT * event) {
        assert(context);
        auto & server        = *static_cast<msquic_server *>(context);
        auto & msquic_ctx    = o2i(server.msquic_impl());

        QUIC_ADDR_STR remote = [&]() {
            QUIC_ADDR remote_addr;
            std::uint32_t addr_size = sizeof(QUIC_ADDR);
            msquic_ctx.api->GetParam(connection, QUIC_PARAM_CONN_REMOTE_ADDRESS, &addr_size, &remote_addr);
            QUIC_ADDR_STR str;
            QuicAddrToString(&remote_addr, &str);
            return str;
        }();

        switch (event->Type) {
            case QUIC_CONNECTION_EVENT_CONNECTED: {
                // TODO: Invoke a callback to indicate that a new connection has been established?
                fmt::println("New client connected: {}", remote.Address);

                auto wa = msquic_ctx.connection_map.exclusive_access();
                if (auto itr = wa->find(connection); itr == wa->end()) {
                    auto result = wa->emplace(connection, connection_context{connection, {}});
                    if (!result.second) {
                        fmt::println("connection could not be stored!");
                        msquic_ctx.api->ConnectionClose(connection);
                        return QUIC_STATUS_SUCCESS;
                    }
                    msquic_ctx.api->ConnectionSendResumptionTicket(connection, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, nullptr);
                    server.callbacks.on_connected(reinterpret_cast<mad::nexus::quic_connection_handle *>(&result.first->second));

                } else {
                    fmt::println("duplicate connection event for connection {}!", static_cast<void *>(connection));
                }
            } break;
            case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {

                
                auto wa = msquic_ctx.connection_map.exclusive_access();
                if (auto itr = wa->find(connection); itr == wa->end()) {
                    fmt::println("connection shutdown complete but no such connection in map!");
                } else {
                    fmt::println("Removed connection from the connection map");
                    server.callbacks.on_disconnected(reinterpret_cast<mad::nexus::quic_connection_handle *>(&itr->second));
                    wa->erase(itr);
                }
                fmt::println("Connection shutdown complete {}", remote.Address);

                
                msquic_ctx.api->ConnectionClose(connection);
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
                // This will not happen in server scenario as we will not allow
                // client initiated streams.
                auto ra = msquic_ctx.connection_map.shared_access();
                fmt::println("New stream {} by connection {}", static_cast<void *>(event->PEER_STREAM_STARTED.Stream),
                             static_cast<void *>(connection));
                if (auto itr = ra->find(connection); !(itr == ra->end())) {
                    // Create new stream
                    // TODO: Check emplace result
                    auto result = itr->second.streams.emplace(event->PEER_STREAM_STARTED.Stream,
                                                              stream_context{event->PEER_STREAM_STARTED.Stream, connection});

                    msquic_ctx.api->SetCallbackHandler(event->PEER_STREAM_STARTED.Stream, reinterpret_cast<void *>(ServerStreamCallback),
                                                       &result.first->second);

                } else {
                    fmt::println("Client tried to initiate stream but associated connection not found!");
                }

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

    msquic_server::msquic_server(quic_server_configuration config) : quic_server(std::move(config)) {}

    msquic_server::~msquic_server() {}

    std::error_code msquic_server::init() {
        if (pimpl) {
            return quic_error_code::already_initialized;
        }

        auto result = msquic_ctx::make(cfg);
        if (result) {
            pimpl = std::move(result.value());
            return quic_error_code::success;
        }
        return result.error();
    }

    std::error_code msquic_server::listen() {
        if (!pimpl) {
            return quic_error_code::uninitialized;
        }

        return o2i(pimpl).listen(cfg.udp_port_number, cfg.alpn, ServerListenerCallback, this);
    }

    auto msquic_server::open_stream(quic_connection_handle * chandle) -> std::expected<quic_stream_handle *, std::error_code> {
        assert(chandle);

        // TODO: Thread safety?

        auto & cctx      = *reinterpret_cast<connection_context *>(chandle);

        HQUIC new_stream = nullptr;
        auto & api       = o2i(pimpl).api;

        if (auto status = api->StreamOpen(cctx.connection, QUIC_STREAM_OPEN_FLAG_NONE, ServerStreamCallback, nullptr, &new_stream);
            QUIC_FAILED(status)) {
            fmt::println("stream open failed with {}", status);
            return std::unexpected(quic_error_code::stream_open_failed);
        }

        auto [itr, inserted] = cctx.streams.emplace(new_stream, stream_context{new_stream, cctx.connection});
        if (!inserted) {
            return std::unexpected(quic_error_code::stream_insert_to_map_failed);
        }

        auto & stream_context = itr->second;

        // Set context for the callback.
        api->SetContext(new_stream, static_cast<void *>(&stream_context));
        if (auto status = api->StreamStart(new_stream, QUIC_STREAM_START_FLAG_SHUTDOWN_ON_FAIL); QUIC_FAILED(status)) {
            fmt::println("stream start failed with {}", status);
            cctx.streams.erase(itr);
            return std::unexpected(quic_error_code::stream_start_failed);
        }

        return reinterpret_cast<quic_stream_handle *>(&stream_context);
    }

    auto msquic_server::close_stream([[maybe_unused]] quic_stream_handle * shandle) -> std::error_code {

        // FIXME: Implement this
        return quic_error_code::success;
    }

    auto msquic_server::send(quic_stream_handle * shandle, flatbuffers::DetachedBuffer buf) -> std::size_t {
        assert(shandle);
        auto & sctx       = *reinterpret_cast<stream_context *>(shandle);
        auto & api        = o2i(pimpl).api;

        // This function is used to queue data on a stream to be sent.
        // The function itself is non-blocking and simply queues the data and returns.
        // The app may pass zero or more buffers of data that will be sent on the stream in the order they are passed.
        // The buffers (both the QUIC_BUFFERs and the memory they reference) are "owned" by MsQuic (and must not be modified by the app)
        // until MsQuic indicates the QUIC_STREAM_EVENT_SEND_COMPLETE event for the send.

        // We have 16 bytes of reserved space at the beginning of 'buf'
        // We're gonna use it for storing QUIC_BUF.

        // The address used as key is not important.

        auto store_result = sctx.store_buffer(std::move(buf));
        if (!store_result) {
            fmt::println("could not store packet data for send!");
            return 0;
        }
        auto itr      = store_result.value();

        // auto emplace_result = sctx.buffers_in_flight().emplace(reinterpret_cast<std::uint64_t>(&buf), std::move(buf));
        // {
        //     // Extract the node to update its key to the address of the value (DetachedBuffer)
        //     auto node  = sctx.buffers_in_flight().extract(itr);
        //     // Update the key to be the address of the value (DetachedBuffer)
        //     node.key() = reinterpret_cast<std::uint64_t>(&node.mapped());
        //     // FIXME: Check insert result
        //     sctx.buffers_in_flight().insert(std::move(node));
        // }

        auto & buffer = itr->second;

        fmt::println("sending {} bytes of data", buffer.size());
        QUIC_BUFFER * qbuf = reinterpret_cast<QUIC_BUFFER *>(const_cast<std::uint8_t *>(buffer.data()));
        qbuf->Buffer       = reinterpret_cast<std::uint8_t *>(qbuf + sizeof(QUIC_BUFFER));
        qbuf->Length       = static_cast<std::uint32_t>(buffer.size() - sizeof(QUIC_BUFFER));

        // We're using the context pointer here to store the key.
        if (auto status = api->StreamSend(sctx.stream(), qbuf, 1, QUIC_SEND_FLAG_NONE, reinterpret_cast<void *>(itr->first));
            QUIC_FAILED(status)) {
            return 0;
        }
        // FIXME:
        return buf.size() - sizeof(QUIC_BUFFER);
    }

} // namespace mad::nexus
