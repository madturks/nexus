#include <mad/nexus/quic_error_code.hpp>

namespace mad::nexus {
const char * quic_error_code_category::name() const noexcept {
    return "QUIC Server";
}

std::string quic_error_code_category::message(int condition) const {
    auto e = static_cast<quic_error_code>(condition);

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(error : 4062) // MSVC specific pragma
#endif
    switch (e) {
        using enum quic_error_code;
        case success:
            return "Success!";
        case missing_certificate:
            return "Certificate file could not be located in specified path.";
        case missing_private_key:
            return "Private key file could not be located in the specified "
                   "path.";
        case configuration_load_credential_failed:
            return "Credentials (certificate, private key) could not be "
                   "loaded.";
        case uninitialized:
            return "The server object needs to be initialized first.";
        case already_initialized:
            return "The server object is already initialized.";
        case already_listening:
            return "The server object is already listening.";
        case api_initialization_failed:
            return "API object initialization failed. Check "
                   "implementation-specific error code.";
        case registration_initialization_failed:
            return "Registration object initialization failed. Check "
                   "implementation-specific error code.";
        case configuration_initialization_failed:
            return "Configuration object initialization failed. Check "
                   "implementation-specific error code.";
        case listener_initialization_failed:
            return "Listener object initialization failed. Check "
                   "implementation-specific error code.";
        case listener_start_failed:
            return "Listener could not be started. Check "
                   "implementation-specific error code.";
        case stream_open_failed:
            return "Stream open failed.";
        case stream_start_failed:
            return "Stream start failed.";
        case stream_emplace_failed:
            return "Stream insert to map failed.";
        case stream_already_exists:
            return "Stream already exists.";
        case connection_start_failed:
            return "Connection start failed.";
        case connection_initialization_failed:
            return "Connection initialization failed.";
    }

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

    std::unreachable();
}
} // namespace mad::nexus
