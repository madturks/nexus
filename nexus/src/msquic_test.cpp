// #include <boost/asio.hpp>

#include "fmt/base.h"
#include <__expected/unexpected.h>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <msquic.h>
#include <filesystem>
#include <string>
#include <chrono>
#include <optional>
#include <memory>
#include <functional>
#include <expected>

#include <fmt/format.h>
#include <type_traits>

class quic_server {};

namespace mt { namespace exceptions {
    class file_not_found_exception : public std::exception {};
}} // namespace mt::exceptions

struct quic_certificate_config {
    std::filesystem::path cert_path;
    std::filesystem::path cert_private_key_path;
};

struct quic_server_config {
    std::string alpn;
    std::uint16_t port;
    std::optional<std::chrono::milliseconds> idle_timeout;
    quic_certificate_config cert;
};

enum class e_msquic_error_code : std::uint32_t
{
    success = 0,
    certificate_not_exist,
    private_key_not_exist,
    configuration_load_credential_failed,
    uninitialized,
    already_initialized,
    already_listening,
    api_object_initialization_failed,
    registration_object_initialization_failed,
    configuration_object_initialization_failed,
    listener_object_initialization_failed,
    listener_start_failed
};

// static

QUIC_STATUS ServerListenerCallback(HQUIC Listener, void * Context, QUIC_LISTENER_EVENT * Event) {
    switch (Event->Type) {
        case QUIC_LISTENER_EVENT_NEW_CONNECTION: {
            fmt::println("new connection!");
        } break;
        default:
            break;
    }
    return QUIC_STATUS_NOT_SUPPORTED;
}

/**
 * API wrapper for MSQUIC.
 */
struct msquic {

    e_msquic_error_code init(const quic_server_config & cfg) {

        if (msquic) {
            return e_msquic_error_code::already_initialized;
        }

        auto result = msquic_ctx::make(cfg);
        if (result) {
            msquic = std::move(result.value());
            return e_msquic_error_code::success;
        }

        return result.error();
    }

    e_msquic_error_code listen() {
        if (!msquic) {
            return e_msquic_error_code::uninitialized;
        }

        return msquic->listen(6666, ServerListenerCallback);
    }

private:
    // The order matters here, deleter functions has to be called in
    // configuration -> registration -> msquic_api order.

    struct msquic_ctx {

        /**
         * Since HQUIC is a pointer type itself, we have to define
         * a deleter type with ::pointer typedef, otherwise the default
         * deleter assumes HQUIC*.
         */
        struct msquic_handle_deleter {
            using pointer = HQUIC;

            msquic_handle_deleter(std::function<void(HQUIC)> deleter_fn = {}) : deleter(deleter_fn) {
                fmt::println("quic_handle_deleter");
            }

            void operator()(HQUIC h) {
                fmt::println("quic_handle_deleter::operator()");
                if (deleter) {
                    deleter(h);
                }
            }

        private:
            std::function<void(HQUIC)> deleter;
        };

        using msquic_api_uptr_t            = std::unique_ptr<const QUIC_API_TABLE, decltype(&MsQuicClose)>;
        using msquic_handle_uptr_t         = std::unique_ptr<QUIC_HANDLE, msquic_handle_deleter>;

        msquic_api_uptr_t api              = {nullptr, {}};
        msquic_handle_uptr_t registration  = {nullptr, {}};
        msquic_handle_uptr_t configuration = {nullptr, {}};
        msquic_handle_uptr_t listener      = {nullptr, {}};

        inline bool ready_to_listen() const {
            return api && registration && configuration;
        }

        e_msquic_error_code listen(std::uint16_t udp_port, QUIC_LISTENER_CALLBACK_HANDLER callback, void * context = nullptr) {

            if (!ready_to_listen()) {
                return e_msquic_error_code::uninitialized;
            }

            if (listener) {
                return e_msquic_error_code::already_listening;
            }

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

            if (!listener) {
                return e_msquic_error_code::listener_object_initialization_failed;
            }

            // Configures the address used for the listener to listen on all IP
            // addresses and the given UDP port.
            QUIC_ADDR Address = {0};
            QuicAddrSetFamily(&Address, QUIC_ADDRESS_FAMILY_UNSPEC);
            QuicAddrSetPort(&Address, udp_port);

            // FIXME: Move this to configuration.
            const QUIC_BUFFER Alpn = {sizeof("sample") - 1, (uint8_t *) "sample"};

            if (auto status = api->ListenerStart(listener.get(), &Alpn, 1, &Address); QUIC_FAILED(status)) {
                listener.reset(nullptr);
                return e_msquic_error_code::listener_start_failed;
            }

            return e_msquic_error_code::success;
        }

        static std::expected<std::unique_ptr<msquic_ctx>, e_msquic_error_code> make(const quic_server_config & cfg) {
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
                return std::unexpected(e_msquic_error_code::api_object_initialization_failed);
            }

            return initialize_msquic(cfg, std::move(ctx));
        }

    private:
        struct QUIC_CREDENTIAL_CONFIG_HELPER {
            QUIC_CREDENTIAL_CONFIG CredConfig;

            union {
                QUIC_CERTIFICATE_HASH CertHash;
                QUIC_CERTIFICATE_HASH_STORE CertHashStore;
                QUIC_CERTIFICATE_FILE CertFile;
                QUIC_CERTIFICATE_FILE_PROTECTED CertFileProtected;
            };
        };

        static std::expected<QUIC_CREDENTIAL_CONFIG_HELPER, e_msquic_error_code>
        load_certificate(const quic_certificate_config & cert_cfg) {
            if (!std::filesystem::exists(cert_cfg.cert_path)) {
                return std::unexpected(e_msquic_error_code::certificate_not_exist);
            }

            if (!std::filesystem::exists(cert_cfg.cert_private_key_path)) {
                return std::unexpected(e_msquic_error_code::private_key_not_exist);
            }

            QUIC_CREDENTIAL_CONFIG_HELPER config = {};
            config.CertFile.CertificateFile      = cert_cfg.cert_path.c_str();
            config.CertFile.PrivateKeyFile       = cert_cfg.cert_private_key_path.c_str();
            config.CredConfig.Type               = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
            config.CredConfig.CertificateFile    = &config.CertFile;
            return config;
        }

        bool initialize_registration() {
            const QUIC_REGISTRATION_CONFIG RegConfig = {"quicsample", QUIC_EXECUTION_PROFILE_LOW_LATENCY};
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

        bool initialize_configuration(const quic_server_config & cfg) {
            QUIC_SETTINGS settings = {};

            // Configures the server's idle timeout.
            if (cfg.idle_timeout) {
                settings.IdleTimeoutMs       = cfg.idle_timeout->count();
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

            configuration                        = msquic_handle_uptr_t(
                [this, &settings]() -> HQUIC {
                    HQUIC config;
                    const QUIC_BUFFER Alpn = {sizeof("sample") - 1, (uint8_t *) "sample"};
                    if (auto status =

                            api->ConfigurationOpen(registration.get(), &Alpn, 1, &settings, sizeof(settings), nullptr, &config);
                        QUIC_FAILED(status)) {
                        // TODO: Throw a proper exception for the operation.
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

        bool initialize_credentials(const QUIC_CREDENTIAL_CONFIG & cred_cfg) {
            return !QUIC_FAILED(api->ConfigurationLoadCredential(configuration.get(), &cred_cfg));
        }

        static std::expected<std::unique_ptr<msquic_ctx>, e_msquic_error_code> initialize_msquic(const quic_server_config & cfg,
                                                                                                 std::unique_ptr<msquic_ctx> ctx) {
            assert(ctx->api);
            if (!ctx->initialize_registration()) {
                return std::unexpected(e_msquic_error_code::registration_object_initialization_failed);
            }

            if (!ctx->initialize_configuration(cfg)) {
                return std::unexpected(e_msquic_error_code::configuration_object_initialization_failed);
            }

            auto cred_config = load_certificate(cfg.cert);
            if (!cred_config) {
                return std::unexpected(cred_config.error());
            }

            if (!ctx->initialize_credentials(cred_config->CredConfig)) {
                return std::unexpected(e_msquic_error_code::configuration_load_credential_failed);
            }

            return std::move(ctx);
        }
    };

    std::unique_ptr<msquic_ctx> msquic;
};

struct msquic_quic_server : quic_server {

    msquic_quic_server() {
        fmt::println("msquic_quic_server");
    }

    ~msquic_quic_server() {
        fmt::println("~msquic_quic_server");
    }

    void init_registration() {}
};

int main(void) {
    fmt::println("{}", __cplusplus);

    quic_server_config cfg;
    cfg.alpn                       = "test";
    cfg.cert.cert_path             = "/workspaces/nexus/vendor/msquic/test-cert/server.cert";
    cfg.cert.cert_private_key_path = "/workspaces/nexus/vendor/msquic/test-cert/server.key";
    cfg.idle_timeout               = std::chrono::milliseconds{10000};
    msquic quic{};
    quic.init(cfg);
    quic.listen();
    getchar();
    // quic_server.ini
}