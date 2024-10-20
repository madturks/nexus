#pragma once

namespace mad::nexus {

struct handle_carrier {

    handle_carrier(void * native_handle) : handle(native_handle) {}

    /**
     * @brief Get the connection handle as type T.
     */
    template <typename T = void *>
    [[nodiscard]] T handle_as() const {
        return static_cast<T>(handle);
    }

protected:
    /**
     * @brief The implementation-specific handle of the connection.
     */
    void * handle;
};
} // namespace mad::nexus
