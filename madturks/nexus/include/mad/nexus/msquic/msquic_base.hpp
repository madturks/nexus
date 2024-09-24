#pragma once

#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/quic_base.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_callback_types.hpp>

namespace mad::nexus {

    class msquic_base : virtual public quic_base {
    public:
        using quic_base::quic_base;
        [[nodiscard]] virtual std::error_code init() override final;
        [[nodiscard]] virtual auto open_stream(connection_context * cctx, stream_data_callback_t data_callback)
            -> std::expected<stream_context *, std::error_code> override final;

        [[nodiscard]] virtual auto close_stream(stream_context * sctx) -> std::error_code override final;
        [[nodiscard]] virtual auto send(stream_context * sctx, flatbuffers::DetachedBuffer buf) -> std::size_t override final;

        virtual ~msquic_base() override;

        auto msquic_impl() -> const std::shared_ptr<void> & {
            return msquic_pimpl;
        }

    private:
        // Hide implementation details behind a opaque
        // smart pointer to avoid exposing msquic's internals.
        std::shared_ptr<void> msquic_pimpl;
    };

} // namespace mad::nexus
