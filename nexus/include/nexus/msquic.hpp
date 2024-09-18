#pragma once

#include <nexus/quic.hpp>
#include <memory>

namespace mt::nexus {

    class msquic_server : public quic_server {
    public:
        [[nodiscard]] virtual std::error_code init() override;
        [[nodiscard]] virtual std::error_code listen() override;

        [[nodiscard]] static std::unique_ptr<quic_server> make(quic_server_configuration config) {
            return std::unique_ptr<msquic_server>(new msquic_server(config));
        }

        virtual ~msquic_server() override;

    private:
        msquic_server(quic_server_configuration config);
        // Hide implementation details behind a opaque
        // smart pointer to avoid exposing msquic's internals.
        std::shared_ptr<void> pimpl;
    };


} // namespace mt::nexus
