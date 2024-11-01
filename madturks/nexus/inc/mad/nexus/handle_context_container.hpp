/******************************************************
 * Generic value container type.
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
 * A container for associating a handle with an user-defined
 * handle context. The handle context's lifetime is bound to
 * the handle itself.
 *
 * The container is useful in scenarios like there's an opaque
 * handle provided by a third-party library or API, and the code
 * needs to store associated data with that context (e.g. connection
 * state, stream state).
 *
 * The handle context can be retrieved by either providing the
 * shared_ptr of the handle, or the raw handle (void*) itself.
 *
 * @tparam HandleContextType The handle context type
 ******************************************************/
template <typename HandleContextType>
struct handle_context_container {
    using underlying_map_t =
        std::unordered_map<std::shared_ptr<void>, HandleContextType,
                           shared_ptr_raw_hash, shared_ptr_raw_equal>;

    handle_context_container() = default;
    handle_context_container(const handle_context_container &) = default;
    handle_context_container &
    operator=(const handle_context_container &) = default;
    handle_context_container(handle_context_container &&) = default;
    handle_context_container & operator=(handle_context_container &&) = default;

    /******************************************************
     * Add a new handle/handle context pair.
     *
     * @param ptr Type-erased shared_ptr handle type
     * @param value_args Arguments to be forwarded to the handle_context
     * constructor
     * @return The handle context reference on success, error_code otherwise.
     ******************************************************/
    template <typename... Args>
    auto add(std::shared_ptr<void> handle, Args &&... value_args)
        -> result<std::reference_wrapper<HandleContextType>> {

        auto present = storage.find(handle);

        if (!(storage.end() == present)) {
            // The same connection already exists?
            assert(false);
            return std::unexpected(quic_error_code::value_already_exists);
        }

        const auto & [emplaced_itr, emplace_status] = storage.emplace(
            std::move(handle),
            HandleContextType{ std::forward<Args>(value_args)... });

        if (!emplace_status) {
            return std::unexpected(quic_error_code::value_emplace_failed);
        }
        return emplaced_itr->second;
    }

    /******************************************************
     * Remove a handle (and its context) from the map
     *
     * @param handle The handle
     * @return Extracted node from the map on success,
     *         error code otherwise.
     ******************************************************/
    [[nodiscard]] auto
    erase(void * handle) -> result<typename underlying_map_t::node_type> {
        auto present = storage.find(handle);

        if (storage.end() == present) {
            return std::unexpected(quic_error_code::value_does_not_exists);
        }

        // Remove the key-value pair from the map
        // and return it to caller. The caller might
        // have some unfinished business with it.
        return storage.extract(present);
    }

protected:
    ~handle_context_container() = default;

private:
    /******************************************************
     * Underlying storage for handle/handle context pairs.
     ******************************************************/
    underlying_map_t storage{};
};
} // namespace mad::nexus
