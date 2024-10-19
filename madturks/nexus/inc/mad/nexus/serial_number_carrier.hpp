#pragma once

#include <atomic>
#include <cstdint>

namespace mad::nexus {
struct serial_number_carrier {

    /**
     * @brief The connection serial number.
     *
     * This is an auto-generated value and guaranteed to be unique per
     * connection object.
     */
    [[nodiscard]] auto serial_number() const noexcept {
        return serial_number_;
    }

private:
    /**
     * @brief Generate an unique identifier for the connection.
     *
     * The value space is large enough to ensure that we won't hit any
     * collisions for a foreseeable future.
     */
    static auto generate_serial_number() -> std::uint64_t {
        static std::atomic<std::uint64_t> id_provider{ 0ull };
        return id_provider.fetch_add(1);
    }

    const std::uint64_t serial_number_{ generate_serial_number() };
};
} // namespace mad::nexus
