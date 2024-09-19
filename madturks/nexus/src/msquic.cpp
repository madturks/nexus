

#include <fmt/base.h>
#include <mutex>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/msquic.hpp>
#include <mad/circular_buffer_vm.hpp>

#include <memory>
#include <expected>
#include <functional>
#include <filesystem>

#include <msquic.h>
#include <fmt/format.h>
#include <string_view>

namespace {

    using circular_buffer_t = mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;

    /**
     * Since HQUIC is a pointer type itself, we have to define
     * a deleter type with ::pointer typedef, otherwise the default
     * deleter assumes HQUIC*.
     */
    struct msquic_handle_deleter {
        using pointer = HQUIC;

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
    struct msquic_ctx {

        using msquic_api_uptr_t            = std::unique_ptr<const QUIC_API_TABLE, decltype(&MsQuicClose)>;
        using msquic_handle_uptr_t         = std::unique_ptr<QUIC_HANDLE, msquic_handle_deleter>;

        // Do not reorder these, as it will change the destruction
        // order and will crash the application on shutdown..
        msquic_api_uptr_t api              = {nullptr, {}};
        msquic_handle_uptr_t registration  = {nullptr, {}};
        msquic_handle_uptr_t configuration = {nullptr, {}};
        msquic_handle_uptr_t listener      = {nullptr, {}};

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

static circular_buffer_t buffer{16384, circular_buffer_t::auto_align_to_page{}};
static std::mutex mtx;

namespace mad::nexus {

    /**
     * Opaque to impl.
     */
    inline msquic_ctx & o2i(std::shared_ptr<void> & ptr) {
        return *static_cast<msquic_ctx *>(ptr.get());
    }

    static QUIC_STATUS ServerStreamCallback([[maybe_unused]] HQUIC stream, [[maybe_unused]] void * context,
                                            [[maybe_unused]] QUIC_STREAM_EVENT * event) {
        auto ctx = static_cast<msquic_ctx *>(context);
        assert(ctx);

        switch (event->Type) {
            case QUIC_STREAM_EVENT_RECEIVE: {
                fmt::println("Received data from the remote count:{} total_size:{}", event->RECEIVE.BufferCount,
                             event->RECEIVE.TotalBufferLength);

                {
                    std::unique_lock<std::mutex> lock;
                    for (std::uint32_t i = 0; i < event->RECEIVE.BufferCount; i++) {
                        buffer.put(event->RECEIVE.Buffers [i].Buffer, event->RECEIVE.Buffers [i].Length);
                    }
                }
                // event->RECEIVE.TotalBufferLength = 5;

                // ctx->api->StreamReceiveSetEnabled(stream, true);
                //  event->RECEIVE.Buffers
                //  pending?
                //  ctx->api->StreamReceiveComplete(stream, event->RECEIVE.TotalBufferLength);
                //  event->RECEIVE.TotalBufferLength -> Indicate how much of the buffer is consumed.
            } break;
        }
        fmt::println("total {} ", buffer.consumed_space());

        // TODO: Implement this!
        // https://microsoft.github.io/msquic/msquicdocs/docs/Deployment.html#nat-rebindings-without-load-balancing-support
        return QUIC_STATUS_SUCCESS;
    }

    static QUIC_STATUS ServerConnectionCallback(HQUIC connection, void * context, QUIC_CONNECTION_EVENT * event) {
        auto ctx = static_cast<msquic_ctx *>(context);
        assert(ctx);
        // Handover the connection to the client?

        switch (event->Type) {
            case QUIC_CONNECTION_EVENT_CONNECTED:
                // TODO: Invoke a callback to indicate that a new connection has been established?
                fmt::println("Connection received.");
                ctx->api->ConnectionSendResumptionTicket(connection, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, nullptr);
                break;
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
            case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
                fmt::println("Connection shutdown complete.");
                ctx->api->ConnectionClose(connection);
            } break;
            case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
                fmt::println("Peer started a stream.");
                ctx->api->SetCallbackHandler(event->PEER_STREAM_STARTED.Stream, reinterpret_cast<void *>(ServerStreamCallback), context);
            } break;
            case QUIC_CONNECTION_EVENT_RESUMED: {
                fmt::println("Connection resumed!");
            } break;
        }
        return QUIC_STATUS_SUCCESS;
    }

    static QUIC_STATUS ServerListenerCallback([[maybe_unused]] HQUIC Listener, void * context, QUIC_LISTENER_EVENT * Event) {

        auto ctx = static_cast<msquic_ctx *>(context);
        assert(ctx);

        fmt::println("ServerListenerCallback() - Event Type: `{}`", static_cast<int>(Event->Type));

        switch (Event->Type) {
            case QUIC_LISTENER_EVENT_NEW_CONNECTION: {
                fmt::println("Listener received a new connection.");
                // A new connection is being attempted by a client. For the handshake to
                // proceed, the server must provide a configuration for QUIC to use. The
                // app MUST set the callback handler before returning.
                ctx->api->SetCallbackHandler(Event->NEW_CONNECTION.Connection, reinterpret_cast<void *>(ServerConnectionCallback), context);
                return ctx->api->ConnectionSetConfiguration(Event->NEW_CONNECTION.Connection, ctx->configuration.get());
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

        return o2i(pimpl).listen(cfg.udp_port_number, cfg.alpn, ServerListenerCallback, pimpl.get());
    }

} // namespace mad::nexus
