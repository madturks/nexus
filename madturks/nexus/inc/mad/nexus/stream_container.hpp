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
 * Convenience type for storing streams.
 *
 * @tparam Derived The derived type
 ******************************************************/
template <typename Derived>
struct stream_container {
    using stream_handle_t = std::shared_ptr<void>;
    using stream_map_t =
        std::unordered_map<stream_handle_t, stream, shared_ptr_raw_hash,
                           shared_ptr_raw_equal>;

    /******************************************************
     * Add a new stream to the stream map.
     *
     * @param ptr The stream handle wrapped into a shared_ptr
     * @param callbacks The callback table
     * @return The stream's reference on success, error_code otherwise.
     ******************************************************/
    auto add_new_stream(std::shared_ptr<void> ptr, stream_callbacks callbacks)
        -> result<std::reference_wrapper<stream>> {

        auto stream_handle = ptr.get();
        auto present = streams.find(ptr);

        if (!(streams.end() == present)) {
            // The same connection already exists?
            assert(false);
            return std::unexpected(quic_error_code::stream_already_exists);
        }

        const auto & [emplaced_itr, emplace_status] = streams.emplace(
            std::move(ptr),
            stream{ stream_handle, *static_cast<Derived *>(this),
                    std::move(callbacks) });

        if (!emplace_status) {
            return std::unexpected(quic_error_code::stream_emplace_failed);
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
    [[nodiscard]] auto
    remove_stream(void * stream_handle) -> result<stream_map_t::node_type> {
        auto present = streams.find(stream_handle);

        if (streams.end() == present) {
            return std::unexpected(quic_error_code::stream_does_not_exist);
        }

        // Remove the key-value pair from the map
        // and return it to caller. The caller might
        // have some unfinished business with it.
        return streams.extract(present);
    }

private:
    /******************************************************
     * Map of all streams owned by the connection
     ******************************************************/
    stream_map_t streams{};
};
} // namespace mad::nexus
