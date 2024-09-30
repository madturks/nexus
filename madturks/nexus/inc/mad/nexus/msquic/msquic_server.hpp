#pragma once

#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_server.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>
#include <mad/concurrent.hpp>

#include <memory>
#include <system_error>
#include <expected>

namespace mad::nexus {

    class msquic_server final : virtual public quic_server,
                                virtual public msquic_base {
    public:
        msquic_server(quic_configuration cfg);
        virtual ~msquic_server() override;

        [[nodiscard]] virtual std::error_code listen() override;

        [[nodiscard]] static std::unique_ptr<quic_server> make(quic_configuration cfg) {
            return std::unique_ptr<msquic_server>(new msquic_server(cfg));
        }

        [[nodiscard]] auto & connections() {
            return connection_map;
        }

    private:
        mad::concurrent<std::unordered_map<std::shared_ptr<void>, connection_context, shared_ptr_raw_hash, shared_ptr_raw_equal>>
            connection_map;
    };

} // namespace mad::nexus
