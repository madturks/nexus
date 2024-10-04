#pragma once

#include <mad/nexus/quic_application.hpp>

// Forward declarations
struct MsQuicConfiguration;
struct MsQuicRegistration;

namespace mad::nexus {

    class msquic_application : public quic_application {
    public:
        virtual std::unique_ptr<quic_server> make_server() override;
        virtual std::unique_ptr<quic_client> make_client() override;
        virtual ~msquic_application() override;

        const MsQuicRegistration& registration() const;
        const MsQuicConfiguration& configuration() const;
    private:
        friend std::unique_ptr<quic_application> make_msquic_application(const struct quic_configuration &);
        msquic_application(const struct quic_configuration& cfg);
        std::shared_ptr<MsQuicRegistration> registration_ptr;
        std::shared_ptr<MsQuicConfiguration> configuration_ptr;
    };

} // namespace mad::nexus
