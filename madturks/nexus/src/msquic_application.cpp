#include <mad/macros.hpp>
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/msquic/msquic_server.hpp>

#include <msquic.hpp>

#include <filesystem>
#include <memory>

/**
 * The global MSQUIC api object, used by the MSQUIC C++
 * API, and nexus MSQUIC based implementation.
 */
const MsQuicApi * MsQuic;

namespace {

/**
 * Wrapper type for automatically managing the lifetime
 * of the global MsQuic API object.
 *
 * This struct is intended to be used in junction with
 * shared_ptr/weak_ptr's to tie the lifetime to different
 * instances of the msquic_application.
 */
struct msquic_auto_init {
    msquic_auto_init() {
        MAD_EXPECTS(nullptr == MsQuic);
        MsQuic = new (std::nothrow) MsQuicApi();
        MAD_ENSURES(nullptr != MsQuic);
    }

    ~msquic_auto_init() {
        MAD_EXPECTS(nullptr != MsQuic);
        delete MsQuic;
        MsQuic = nullptr;
        MAD_ENSURES(nullptr == MsQuic);
    }
};

/**
 * This is a weak pointer to hold a reference to msquic_auto_init
 * object if it's already been initialized.
 */
static std::weak_ptr<void> auto_init_obj;

MsQuicSettings settings_to_msquic(const mad::nexus::quic_configuration & cfg) {
    MsQuicSettings settings{};

    // Configures the server's idle timeout.
    if (cfg.idle_timeout) {
        settings.SetIdleTimeoutMs(
            static_cast<std::uint64_t>(cfg.idle_timeout->count()));
    }

    // Configures the server's resumption level to allow for resumption and
    // 0-RTT.
    settings.SetServerResumptionLevel(QUIC_SERVER_RESUME_AND_ZERORTT);
    settings.SetSendBufferingEnabled(false);
    // Configures the server's settings to allow for the peer to open a
    // single bidirectional stream. By default connections are not
    // configured to allow any streams from the peer.
    settings.SetPeerBidiStreamCount(1);
    settings.SetStreamRecvWindowDefault(cfg.stream_receive_window);

    return settings;
}
} // namespace

namespace mad::nexus {
std::unique_ptr<quic_application>
make_msquic_application(const quic_configuration & cfg) {
    return std::unique_ptr<msquic_application>(new msquic_application(cfg));
}

msquic_application::msquic_application(const quic_configuration & cfg) :
    quic_application(cfg) {
    MAD_EXPECTS(!cfg.appname.empty());
    MAD_EXPECTS(!cfg.alpn.empty());

    // MSQUIC API init
    {
        /**
         * Check if the MsQuic API has been initialized already.
         *
         * If so, the weak_ptr would return the shared_ptr to the
         * existing msquic_auto_init object, which effectively increases
         * the reference count by one. Otherwise, a new one is created
         * from scratch and set to the weak_ptr as well.
         */
        api_ptr = auto_init_obj.lock();
        if (nullptr == api_ptr) {
            api_ptr = std::make_shared<msquic_auto_init>();
            auto_init_obj = api_ptr;
        }

        MAD_ENSURES(nullptr != api_ptr);
        MAD_ENSURES(!auto_init_obj.expired());
    }

    // Registration object init
    {
        registration_ptr = std::make_shared<MsQuicRegistration>(
            cfg.appname.c_str(), QUIC_EXECUTION_PROFILE_LOW_LATENCY, true);

        MAD_ENSURES(registration_ptr);
        MAD_ENSURES(registration_ptr->IsValid());
    }

    // Configuration init
    {
        MsQuicAlpn alpn{ cfg.alpn.c_str() };

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

        MsQuicCredentialConfig cred_cfg{ credential_config };
        auto msquic_cfg = settings_to_msquic(cfg);

        configuration_ptr = std::make_shared<MsQuicConfiguration>(
            *registration_ptr.get(), alpn, msquic_cfg, credential_config);

        MAD_ENSURES(configuration_ptr);
        MAD_ENSURES(configuration_ptr->IsValid());
    }
}

msquic_application::~msquic_application() = default;

std::unique_ptr<quic_server> msquic_application::make_server() {
    MAD_EXPECTS(registration_ptr && registration_ptr->IsValid());
    MAD_EXPECTS(configuration_ptr && configuration_ptr->IsValid());
    return std::unique_ptr<msquic_server>(new msquic_server(*this));
}

std::unique_ptr<quic_client> msquic_application::make_client() {
    MAD_EXPECTS(registration_ptr && registration_ptr->IsValid());
    MAD_EXPECTS(configuration_ptr && configuration_ptr->IsValid());
    return std::unique_ptr<msquic_client>(new msquic_client(*this));
}

const MsQuicRegistration & msquic_application::registration() const {
    MAD_EXPECTS(registration_ptr && registration_ptr->IsValid());
    return *static_cast<MsQuicRegistration *>(registration_ptr.get());
}

const MsQuicConfiguration & msquic_application::configuration() const {
    MAD_EXPECTS(configuration_ptr && configuration_ptr->IsValid());
    return *static_cast<MsQuicConfiguration *>(configuration_ptr.get());
}

} // namespace mad::nexus
