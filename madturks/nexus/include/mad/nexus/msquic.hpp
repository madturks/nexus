#pragma once

#include <mad/nexus/quic.hpp>
#include <memory>
#include <system_error>
#include <expected>

namespace mad::nexus {

    struct connection_context;
    struct stream_context;

    class msquic_server final : public quic_server {
    public:
        [[nodiscard]] virtual std::error_code init() override final;
        [[nodiscard]] virtual std::error_code listen() override;
        [[nodiscard]] virtual auto
        open_stream(quic_connection_handle * cctx) -> std::expected<quic_stream_handle *, std::error_code> override final;
        [[nodiscard]] virtual auto close_stream(quic_stream_handle * sctx) -> std::error_code override final;
        [[nodiscard]] virtual auto send(quic_stream_handle * sctx, flatbuffers::DetachedBuffer buf) -> std::size_t override final;

        virtual ~msquic_server() override;

        [[nodiscard]] static std::unique_ptr<quic_server> make(quic_server_configuration config) {
            return std::unique_ptr<msquic_server>(new msquic_server(config));
        }

        auto msquic_impl() -> const std::shared_ptr<void> & {
            return pimpl;
        }

    private:
        msquic_server(quic_server_configuration config);
        // Hide implementation details behind a opaque
        // smart pointer to avoid exposing msquic's internals.
        std::shared_ptr<void> pimpl;
    };

} // namespace mad::nexus
