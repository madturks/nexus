#include <mad/macros.hpp>
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/msquic/msquic_server.hpp>

#include <msquic.h>

#include <filesystem>
#include <memory>

/**
 * This is a weak pointer to hold a reference to msquic_auto_init
 * object if it's already been initialized.
 */
static std::weak_ptr<void> auto_init_obj{};

static QUIC_SETTINGS
settings_to_msquic(const mad::nexus::quic_configuration & cfg) {

    QUIC_SETTINGS settings{};

    // Configures the server's idle timeout.
    if (cfg.idle_timeout) {
        settings.IdleTimeoutMs = static_cast<std::uint64_t>(
            cfg.idle_timeout->count());
        settings.IsSet.IdleTimeoutMs = true;
    }

    if (cfg.keep_alive_interval) {
        settings.KeepAliveIntervalMs = static_cast<std::uint32_t>(
            cfg.keep_alive_interval->count());
        settings.IsSet.KeepAliveIntervalMs = true;
    }

    // Configures the server's resumption level to allow for resumption and
    // 0-RTT.
    settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT;
    settings.IsSet.ServerResumptionLevel = true;

    settings.SendBufferingEnabled = false;
    settings.IsSet.SendBufferingEnabled = true;

    // Configures the server's settings to allow for the peer to open a
    // single bidirectional stream. By default connections are not
    // configured to allow any streams from the peer.
    settings.PeerBidiStreamCount = 1;
    settings.IsSet.PeerBidiStreamCount = true;

    settings.StreamRecvWindowDefault = cfg.stream_receive_window;
    settings.IsSet.StreamRecvWindowDefault = true;

    return settings;
}

namespace mad::nexus {

result<std::unique_ptr<quic_application>>
make_msquic_application(const quic_configuration & cfg) {

    /******************************************************
     * MSQUIC API observer object.
     *
     * Keeps a weak reference to the MSQUIC API object so
     * the code can fetch the object for subsequent application
     * initializations.
     ******************************************************/
    static std::weak_ptr<const ::QUIC_API_TABLE> msquic_api{};

    // Try to retrieve the API pointer, if any.
    std::shared_ptr<const ::QUIC_API_TABLE> api = msquic_api.lock();
    std::shared_ptr<QUIC_HANDLE> registration = {};
    std::shared_ptr<QUIC_HANDLE> configuration = {};

    /******************************************************
     * Initialize MSQUIC API if not yet been initialized.
     *
     * The API is intended to be shared between all the
     * applications.
     ******************************************************/

    if (nullptr == api) {
        const ::QUIC_API_TABLE * api_table{ nullptr };
        if (auto result = MsQuicOpen2(&api_table); QUIC_FAILED(result)) {
            return std::unexpected(quic_error_code::api_initialization_failed);
        }
        MAD_EXPECTS(api_table);
        api = std::shared_ptr<const ::QUIC_API_TABLE>(api_table, MsQuicClose);
    }

    // At this point we should have a valid API object, either a new
    // one, or reusing an existing one.
    MAD_ENSURES(api);

    /******************************************************
     * Create a MSQUIC registration object
     ******************************************************/
    {
        MAD_EXPECTS(api);
        MAD_EXPECTS(nullptr == registration);
        MAD_EXPECTS(nullptr == configuration);
        QUIC_REGISTRATION_CONFIG regcfg{};
        regcfg.AppName = cfg.appname.c_str();
        regcfg.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;
        HQUIC registration_handle = { nullptr };

        if (auto r = api->RegistrationOpen(&regcfg, &registration_handle);
            QUIC_FAILED(r)) {
            return std::unexpected(
                quic_error_code::registration_initialization_failed);
        }

        registration = std::shared_ptr<QUIC_HANDLE>(
            registration_handle, [api](QUIC_HANDLE * h) {
                MAD_EXPECTS(h);
                api->RegistrationClose(h);
            });

        MAD_ENSURES(registration);
    }

    /******************************************************
     * Load configuration into MSQUIC configuration object
     ******************************************************/
    {
        MAD_EXPECTS(api);
        MAD_EXPECTS(registration);
        MAD_EXPECTS(nullptr == configuration);
        MAD_EXPECTS(!cfg.appname.empty());
        MAD_EXPECTS(!cfg.alpn.empty());
        HQUIC configuration_handle = { nullptr };

        const auto msquic_settings = settings_to_msquic(cfg);

        std::vector<std::uint8_t> a{ cfg.alpn.begin(), cfg.alpn.end() };
        const QUIC_BUFFER alpn = { static_cast<std::uint32_t>(a.size()),
                                   a.data() };

        if (auto r = api->ConfigurationOpen(
                registration.get(), &alpn, 1, &msquic_settings,
                sizeof(QUIC_SETTINGS), nullptr, &configuration_handle);
            QUIC_FAILED(r)) {
            return std::unexpected(
                quic_error_code::configuration_initialization_failed);
        }

        configuration = std::shared_ptr<QUIC_HANDLE>(
            configuration_handle, [api](QUIC_HANDLE * h) {
                MAD_EXPECTS(h);
                api->ConfigurationClose(h);
            });
        MAD_ENSURES(configuration);
    }

    /******************************************************
     * Load credentials info to configuration
     ******************************************************/
    {
        MAD_EXPECTS(api);
        MAD_EXPECTS(registration);
        MAD_EXPECTS(configuration);
        QUIC_CREDENTIAL_CONFIG credential_config = {};
        QUIC_CERTIFICATE_FILE certificate = {};
        MAD_ENSURES(credential_config.Reserved == nullptr);

        /**
         * Currently, we don't expect client to provide a certificate
         * but the server do need it.
         */
        MAD_EXHAUSTIVE_SWITCH_BEGIN
        switch (cfg.role()) {
            case e_role::client: {
                credential_config.Type = QUIC_CREDENTIAL_TYPE_NONE;
                credential_config.Flags = QUIC_CREDENTIAL_FLAG_CLIENT;
                // FIXME: Remove this later on.
                credential_config.Flags |=
                    QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
            } break;
            case e_role::server: {
                MAD_EXPECTS(
                    std::filesystem::exists(cfg.credentials.certificate_path));
                MAD_EXPECTS(
                    std::filesystem::exists(cfg.credentials.private_key_path));
                certificate.CertificateFile =
                    cfg.credentials.certificate_path.c_str();
                certificate.PrivateKeyFile =
                    cfg.credentials.private_key_path.c_str();
                credential_config.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
                credential_config.CertificateFile = &certificate;
            } break;
        }
        MAD_EXHAUSTIVE_SWITCH_END

        if (auto r = api->ConfigurationLoadCredential(
                configuration.get(), &credential_config);
            QUIC_FAILED(r)) {
            return std::unexpected(
                quic_error_code::configuration_load_credential_failed);
        }

        // Is there a way to assert?
    }

    // try to alloc?

    auto result = new (std::nothrow) msquic_application(
        std::move(api), std::move(registration), std::move(configuration));

    if (nullptr == result) {
        // allocation failure
        return std::unexpected(quic_error_code::memory_allocation_failed);
    }

    return std::unique_ptr<msquic_application>(result);
}

msquic_application::msquic_application(
    std::shared_ptr<const QUIC_API_TABLE> api_object,
    std::shared_ptr<QUIC_HANDLE> registration,
    std::shared_ptr<QUIC_HANDLE> configuration) :
    quic_application(), msquic_api(api_object), registration_ptr(registration),
    configuration_ptr(configuration) {
    MAD_EXPECTS(msquic_api);
    MAD_EXPECTS(registration_ptr);
    MAD_EXPECTS(configuration_ptr);
}

msquic_application::~msquic_application() = default;

result<std::unique_ptr<quic_server>> msquic_application::make_server() {
    MAD_EXPECTS(msquic_api);
    MAD_EXPECTS(registration_ptr);
    MAD_EXPECTS(configuration_ptr);
    return std::unique_ptr<msquic_server>(new msquic_server(*this));
}

result<std::unique_ptr<quic_client>> msquic_application::make_client() {
    MAD_EXPECTS(msquic_api);
    MAD_EXPECTS(registration_ptr);
    MAD_EXPECTS(configuration_ptr);
    return std::unique_ptr<msquic_client>(new msquic_client(*this));
}

const QUIC_API_TABLE * msquic_application::api() const noexcept {
    return msquic_api.get();
}

QUIC_HANDLE * msquic_application::registration() const noexcept {
    MAD_EXPECTS(registration_ptr);
    return registration_ptr.get();
}

QUIC_HANDLE * msquic_application::configuration() const noexcept {
    MAD_EXPECTS(configuration_ptr);
    return configuration_ptr.get();
}

} // namespace mad::nexus
