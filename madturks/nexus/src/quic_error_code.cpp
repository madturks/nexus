/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/macros.hpp>
#include <mad/nexus/quic_error_code.hpp>

namespace mad::nexus {
const char * quic_error_code_category::name() const noexcept {
    return "QUIC Server";
}

std::string quic_error_code_category::message(int condition) const {
    auto e = static_cast<quic_error_code>(condition);

    MAD_EXHAUSTIVE_SWITCH_BEGIN
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
        case connection_start_failed:
            return "Connection start failed.";
        case connection_initialization_failed:
            return "Connection initialization failed.";
        case send_failed:
            return "Send failed.";
        case not_yet_implemented:
            return "Not yet implemented.";
        case client_not_connected:
            return "Client is not connected.";
        case client_already_connected:
            return "Client is already in connected state.";
        case value_already_exists:
            return "Value already exists.";
        case value_does_not_exists:
            return "Value does not exist.";
        case value_emplace_failed:
            return "Value emplace failed.";
        case memory_allocation_failed:
            return "Memory allocation failed.";
        case no_such_implementation:
            return "No such implementation!";
    }

    MAD_EXHAUSTIVE_SWITCH_END
    std::unreachable();
}
} // namespace mad::nexus
