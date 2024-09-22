#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <optional>
#include <system_error>
#include <utility>
#include <expected>
#include <span>
#include <flatbuffers/detached_buffer.h>
#include <flatbuffers/flatbuffer_builder.h>

namespace mad::nexus {

    enum class quic_error_code : std::int32_t
    {
        success = 0,
        missing_certificate,
        missing_private_key,
        configuration_load_credential_failed,
        uninitialized,
        already_initialized,
        already_listening,
        api_initialization_failed,
        registration_initialization_failed,
        configuration_initialization_failed,
        listener_initialization_failed,
        listener_start_failed,
        stream_open_failed,
        stream_start_failed,
        stream_insert_to_map_failed
    };

    struct quic_error_code_category : std::error_category {
        const char * name() const noexcept override;

        std::string message(int condition) const override;
    };

    // Mapping from error code enum to category
    inline std::error_code make_error_code(quic_error_code e) {
        static auto category = quic_error_code_category{};
        return std::error_code(std::to_underlying(e), category);
    }

    inline bool successful(std::error_code e) noexcept {
        return static_cast<quic_error_code>(e.value()) == quic_error_code::success;
    }

    inline bool failed(std::error_code e) noexcept {
        return !successful(e);
    }

    struct quic_credentials {
        /**
         * Path to the certificate which will be used to initialize TLS.
         */
        std::string certificate_path;

        /**
         * Path to the private key of the certificate
         */
        std::string private_key_path;
    };

    struct quic_server_configuration {
        /**
         */
        std::string alpn    = {"test"};
        std::string appname = {"test"};
        std::optional<std::chrono::milliseconds> idle_timeout;
        quic_credentials credentials;
        std::uint16_t udp_port_number;
    };

    struct quic_connection_handle {};

    struct quic_stream_handle {};

    template <typename>
    struct quic_callback_function;

    template <typename FunctionReturnType, typename... FunctionArgs>
    struct quic_callback_function<FunctionReturnType(FunctionArgs...)> {
        using callback_fn_t = FunctionReturnType (*)(void *, FunctionArgs...);
        using context_ptr_t = void *;

        quic_callback_function(callback_fn_t fn, context_ptr_t in) : fun_ptr(fn), ctx_ptr(in) {}

        quic_callback_function()                                                 = default;
        quic_callback_function(const quic_callback_function & other)             = default;
        quic_callback_function(quic_callback_function && other)                  = default;
        quic_callback_function & operator=(const quic_callback_function & other) = default;
        quic_callback_function & operator=(quic_callback_function && other)      = default;

        inline FunctionReturnType operator()(FunctionArgs &&... args) {
            return fun_ptr(ctx_ptr, std::forward<FunctionArgs>(args)...);
        }

        callback_fn_t fun_ptr = {nullptr};
        context_ptr_t ctx_ptr = {nullptr};

        void reset() {
            fun_ptr = nullptr;
            ctx_ptr = nullptr;
        }
    };

    // Deduction guide for CTAD
    template <typename FunctionReturnType, typename... FunctionArgs>
    quic_callback_function(FunctionReturnType (*)(void *, FunctionArgs...),
                           void *) -> quic_callback_function<FunctionReturnType(FunctionArgs...)>;

    /**
     * Base class for quic server implementations. Defines the
     * basic interface.
     */
    class quic_server {
    public:
        inline quic_server(quic_server_configuration config) : cfg(config) {}

        [[nodiscard]] virtual std::error_code init()                                                                                  = 0;
        [[nodiscard]] virtual std::error_code listen()                                                                                = 0;

        [[nodiscard]] virtual auto open_stream(quic_connection_handle * cctx) -> std::expected<quic_stream_handle *, std::error_code> = 0;
        [[nodiscard]] virtual auto close_stream(quic_stream_handle * sctx) -> std::error_code                                         = 0;
        [[nodiscard]] virtual auto send(quic_stream_handle * sctx, flatbuffers::DetachedBuffer buf) -> std::size_t                  = 0;

        struct callback_function {};

        struct callback_table {
            // TODO: Implement this!
            quic_callback_function<void(quic_connection_handle *)> on_connected;
            quic_callback_function<void(quic_connection_handle *)> on_disconnected;
            quic_callback_function<void(quic_connection_handle *, quic_stream_handle *)> on_stream_open;
        } callbacks;

        auto get_message_builder() -> flatbuffers::FlatBufferBuilder&;

        virtual ~quic_server();

    protected:
        quic_server_configuration cfg;
    };

} // namespace mad::nexus

template <>
struct std::is_error_code_enum<mad::nexus::quic_error_code> : public std::true_type {};
