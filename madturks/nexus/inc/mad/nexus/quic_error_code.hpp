#pragma once

#include <cstdint>
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
    listener_start_failed,
    stream_open_failed,
    stream_start_failed,
    stream_emplace_failed,
    stream_already_exists,
    connection_initialization_failed,
    connection_start_failed
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
} // namespace mad::nexus

// Template specialization for indication that quic_error_code enum is
// std::error_code compatible (i.e. has category defns)
template <>
struct std::is_error_code_enum<mad::nexus::quic_error_code>
    : public std::true_type {};
