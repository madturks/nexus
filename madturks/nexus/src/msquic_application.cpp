#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/nexus/msquic/msquic_server.hpp>

#include <msquic.hpp>

#include <memory>

namespace {
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
    // Configures the server's settings to allow for the peer to open a single
    // bidirectional stream. By default connections are not configured to allow
    // any streams from the peer.
    settings.SetPeerBidiStreamCount(1);

    return settings;
}
} // namespace

namespace mad::nexus {
std::unique_ptr<quic_application>
make_msquic_application(const struct quic_configuration & cfg) {
    return std::unique_ptr<msquic_application>(new msquic_application(cfg));
}

msquic_application::msquic_application(const quic_configuration & cfg) :
    quic_application(cfg) {
    registration_ptr = std::make_shared<MsQuicRegistration>(
        cfg.appname.c_str(), QUIC_EXECUTION_PROFILE_LOW_LATENCY, true);

    MsQuicAlpn alpn{ cfg.alpn.c_str() };

    QUIC_CERTIFICATE_FILE certificate;
    certificate.CertificateFile = cfg.credentials.certificate_path.c_str();
    certificate.PrivateKeyFile = cfg.credentials.private_key_path.c_str();

    QUIC_CREDENTIAL_CONFIG credential_config;
    credential_config.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
    credential_config.CertificateFile = &certificate;

    MsQuicCredentialConfig cred_cfg{ credential_config };
    auto msquic_cfg = settings_to_msquic(cfg);

    configuration_ptr = std::make_shared<MsQuicConfiguration>(
        *registration_ptr.get(), alpn, msquic_cfg, credential_config);
}

msquic_application::~msquic_application() = default;

std::unique_ptr<quic_server> msquic_application::make_server() {
    assert(registration_ptr);
    assert(configuration_ptr);
    return std::unique_ptr<msquic_server>(new msquic_server(*this));
}

std::unique_ptr<quic_client> msquic_application::make_client() {
    assert(registration_ptr);
    assert(configuration_ptr);
    return std::unique_ptr<msquic_client>(new msquic_client(*this));
}

const MsQuicRegistration & msquic_application::registration() const {
    assert(registration_ptr);
    return *static_cast<MsQuicRegistration *>(registration_ptr.get());
}

const MsQuicConfiguration & msquic_application::configuration() const {
    assert(configuration_ptr);
    return *static_cast<MsQuicConfiguration *>(configuration_ptr.get());
}

} // namespace mad::nexus
