#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <optional>
#include <system_error>
#include <utility>

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
        listener_start_failed
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

    /**
     * Base class for quic server implementations. Defines the
     * basic interface.
     */
    class quic_server {
    public:
        inline quic_server(quic_server_configuration config) : cfg(config) {}

        [[nodiscard]] virtual std::error_code init()   = 0;
        [[nodiscard]] virtual std::error_code listen() = 0;

        struct quic_connection_handle {};
        struct quic_stream_handle {};

        struct callback_table {
            void(on_connected)(quic_connection_handle *);
            void(on_stream_open)(quic_connection_handle *, quic_stream_handle*);
        } callbacks;

        virtual ~quic_server();

    protected:
        quic_server_configuration cfg;
    };

} // namespace mad::nexus

template <>
struct std::is_error_code_enum<mad::nexus::quic_error_code> : public std::true_type {};
