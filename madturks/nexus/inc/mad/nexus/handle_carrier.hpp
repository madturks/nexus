/******************************************************
 * Handle wrapper base type.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <mad/macro>

namespace mad::nexus {

/******************************************************
 * A base type that carries an opaque handle.
 ******************************************************/
struct handle_carrier {

    /******************************************************
     *
     * Construct a new handle carrier object
     *
     * @param native_handle The handle
     ******************************************************/
    handle_carrier(void * handle) : handle_(handle) {}

    /**
     * @brief Get the connection handle as type T.
     */
    template <typename T = void *>
    [[nodiscard]] T handle_as() const noexcept {
        MAD_EXPECTS(handle_);
        return static_cast<T>(handle_);
    }

private:
    void * handle_;
};
} // namespace mad::nexus
