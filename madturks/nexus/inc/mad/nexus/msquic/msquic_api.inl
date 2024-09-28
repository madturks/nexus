#pragma once

#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_configuration.hpp>

#include <mad/log_printer.hpp>
#include <mad/log_macros.hpp>

#include <memory>
#include <functional>
#include <expected>
#include <filesystem>

#include <msquic.h>

namespace mad::nexus {

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

    struct msquic_api {

        using msquic_api_uptr_t            = std::unique_ptr<const QUIC_API_TABLE, decltype(&MsQuicClose)>;
        using msquic_handle_uptr_t         = std::unique_ptr<QUIC_HANDLE, msquic_handle_deleter>;

        // Do not reorder these, as it will change the destruction
        // order and will crash the application on shutdown..
        msquic_api_uptr_t api              = {nullptr, {}};
        msquic_handle_uptr_t registration  = {nullptr, {}};
        msquic_handle_uptr_t configuration = {nullptr, {}};
        msquic_handle_uptr_t listener      = {nullptr, {}};
        msquic_handle_uptr_t connection    = {nullptr, {}};

        mad::log_printer logger{"console"};

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
                    MAD_LOG_DEBUG_I(logger, "msquic_listener deleter called {}", static_cast<void *>(handle));
                    api->ListenerClose(handle);
                    MAD_LOG_DEBUG_I(logger, "msquic_listener deleter done");
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

            MAD_LOG_DEBUG_I(logger, "test {} {}", alpn_sv, alpn_sv.length());

            // Start listening
            if (auto status = api->ListenerStart(listener.get(), &alpn, 1, &Address); QUIC_FAILED(status)) {
                listener.reset(nullptr);
                return quic_error_code::listener_start_failed;
            }

            return quic_error_code::success;
        }

        quic_error_code connect(std::string_view target, std::uint16_t port, QUIC_CONNECTION_CALLBACK_HANDLER callback,
                                void * context = nullptr) {

            // Create a new connection..
            connection = msquic_handle_uptr_t(
                [this, callback, context]() -> HQUIC {
                    HQUIC ptr = nullptr;
                    if (auto status = api->ConnectionOpen(registration.get(), callback, context, &ptr); QUIC_FAILED(status)) {
                        // TODO: Maybe log?
                        return ptr;
                    }
                    return ptr;
                }(),
                msquic_handle_deleter([this](HQUIC handle) {
                    MAD_LOG_DEBUG_I(logger, "msquic_connection deleter called {}", static_cast<void *>(handle));
                    api->ListenerClose(handle);
                    MAD_LOG_DEBUG_I(logger, "msquic_connection deleter done");
                }));

            if (!connection) {
                return mad::nexus::quic_error_code::connection_initialization_failed;
            }

            // TODO: Connection resumption ticket?
            // TODO: Set SSL keylog file for getting decrypt key?

            // Ensure string is NUL terminated
            auto target_str = std::string{target};

            if (auto status =
                    api->ConnectionStart(connection.get(), configuration.get(), QUIC_ADDRESS_FAMILY_UNSPEC, target_str.c_str(), port);
                QUIC_FAILED(status)) {
                connection.reset(nullptr);
                return quic_error_code::connection_start_failed;
            }

            return quic_error_code::success;
        }

        static std::expected<std::unique_ptr<msquic_api>, quic_error_code> make(const quic_configuration & cfg) {

            auto ctx = std::make_unique<msquic_api>();

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
                        MAD_LOG_ERROR_I(logger, "registration open failed!");
                        return nullptr;
                    }

                    MAD_LOG_DEBUG_I(logger, "registration open OK");
                    return reg;
                }(),
                msquic_handle_deleter([this](HQUIC handle) {
                    MAD_LOG_DEBUG_I(logger, "msquic_registration deleter called {}", static_cast<void *>(handle));
                    api->RegistrationClose(handle);
                    MAD_LOG_DEBUG_I(logger, "msquic_registration deleter done");
                }));
            return static_cast<bool>(registration);
        }

        QUIC_SETTINGS settings_to_msquic(const quic_configuration & cfg) {
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

            settings.SendBufferingEnabled = false;
            settings.IsSet.SendBufferingEnabled = true;

            // Configures the server's settings to allow for the peer to open a single
            // bidirectional stream. By default connections are not configured to allow
            // any streams from the peer.
            settings.PeerBidiStreamCount         = 1;
            settings.IsSet.PeerBidiStreamCount   = true;
            return settings;
        }

        bool initialize_configuration(const mad::nexus::quic_configuration & cfg) {

            auto settings = settings_to_msquic(cfg);

            configuration = msquic_handle_uptr_t(
                [this, &cfg, &settings]() -> HQUIC {
                    HQUIC config;
                    const QUIC_BUFFER alpn = {static_cast<std::uint32_t>(cfg.alpn.length()),
                                              reinterpret_cast<std::uint8_t *>(const_cast<char *>(cfg.alpn.c_str()))};

                    if (auto status =

                            api->ConfigurationOpen(registration.get(), &alpn, 1, &settings, sizeof(settings), nullptr, &config);
                        QUIC_FAILED(status)) {

                        MAD_LOG_ERROR_I(logger, "configuration open failed!");
                        return nullptr;
                    }
                    return config;
                }(),
                msquic_handle_deleter([this](HQUIC handle) {
                    MAD_LOG_DEBUG_I(logger, "msquic_configuration deleter called");
                    api->ConfigurationClose(handle);
                    MAD_LOG_DEBUG_I(logger, "msquic_configuration deleter done");
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
        static std::expected<std::unique_ptr<msquic_api>, quic_error_code> initialize_msquic(const quic_configuration & cfg,
                                                                                             std::unique_ptr<msquic_api> ctx) {
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

    inline msquic_api & o2i(const std::shared_ptr<void> & ptr) {
        return *static_cast<msquic_api *>(ptr.get());
    }

} // namespace mad::nexus
