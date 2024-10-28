/******************************************************
 * Stream container type.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream.hpp>
#include <mad/nexus/result.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>

#include <memory>
#include <unordered_map>

namespace mad::nexus {

/******************************************************
 * Convenience type for storing values.
 *
 * @tparam Derived The derived type
 ******************************************************/
template <typename ValueType>
struct value_container {
    using underlying_map_t =
        std::unordered_map<std::shared_ptr<void>, ValueType,
                           shared_ptr_raw_hash, shared_ptr_raw_equal>;

    /******************************************************
     * Add a new stream to the stream map.
     *
     * @param ptr The stream handle wrapped into a shared_ptr
     * @param callbacks The callback table
     * @return The stream's reference on success, error_code otherwise.
     ******************************************************/

    template <typename... Args>
    auto add(std::shared_ptr<void> ptr, Args &&... value_args)
        -> result<std::reference_wrapper<ValueType>> {

        auto present = storage.find(ptr);

        if (!(storage.end() == present)) {
            // The same connection already exists?
            assert(false);
            return std::unexpected(quic_error_code::value_already_exists);
        }

        const auto & [emplaced_itr, emplace_status] = storage.emplace(
            std::move(ptr), ValueType{ std::forward<Args>(value_args)... });

        if (!emplace_status) {
            return std::unexpected(quic_error_code::value_emplace_failed);
        }
        return emplaced_itr->second;
    }

    /******************************************************
     * Remove a stream from the stream map
     *
     * @param stream_handle The stream handle
     * @return Extracted node from the stream map on success,
     *         error code otherwise.
     ******************************************************/
    [[nodiscard]] auto erase(void * stream_handle)
        -> result<typename underlying_map_t::node_type> {
        auto present = storage.find(stream_handle);

        if (storage.end() == present) {
            return std::unexpected(quic_error_code::value_does_not_exists);
        }

        // Remove the key-value pair from the map
        // and return it to caller. The caller might
        // have some unfinished business with it.
        return storage.extract(present);
    }

private:
    /******************************************************
     * Map of all streams owned by the connection
     ******************************************************/
    underlying_map_t storage{};
};
} // namespace mad::nexus
