/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/nexus/msquic/msquic_application.hpp>

#include "mock_msquic_api.hpp"

namespace mad::nexus {
/******************************************************
 * A mock implementation of the msquic_application
 * for testing purposes.
 ******************************************************/
class mock_msquic_application : public msquic_application {
public:
    mock_msquic_api api_table = {};

    static inline auto reg_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xDEADBEEF);
    }();

    static inline auto cfg_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xBADCAFE);
    }();

    mock_msquic_application() :
        msquic_application(
            std::shared_ptr<const QUIC_API_TABLE>(&api_table,
                                                  [](const QUIC_API_TABLE *) {
                                                  }),
            std::shared_ptr<QUIC_HANDLE>(reg_object,
                                         [](QUIC_HANDLE *) {
                                         }),
            std::shared_ptr<QUIC_HANDLE>(cfg_object, [](QUIC_HANDLE *) {
            })) {}

    const QUIC_API_TABLE * api() const noexcept override {
        return msquic_api.get();
    }

    QUIC_HANDLE * registration() const noexcept override {
        return registration_ptr.get();
    }

    QUIC_HANDLE * configuration() const noexcept override {
        return configuration_ptr.get();
    }
};
} // namespace mad::nexus
