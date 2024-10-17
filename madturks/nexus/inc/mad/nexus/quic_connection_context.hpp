#pragma once

#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>

#include <expected>
#include <unordered_map>

namespace mad::nexus {
struct connection_context {
    using stream_handle_t = std::shared_ptr<void>;
    using stream_map_t =
        std::unordered_map<stream_handle_t, stream_context, shared_ptr_raw_hash,
                           shared_ptr_raw_equal>;
    using add_stream_result_t =
        std::expected<std::reference_wrapper<stream_context>, std::error_code>;
    using remove_stream_result_t = std::optional<stream_map_t::node_type>;

    /**
     * Construct a new connection context object
     *
     * @param handle Connection handle
     */
    connection_context(void * handle) : connection_handle(handle) {}

    /**
    * Add a new connection to the connection map.
    *
    * @param ptr The type-erased connection shared pointer
    -
    * @return reference to the connection_context if successful,
    * std::nullopt otherwise.
    */
    auto add_new_stream(std::shared_ptr<void> ptr,
                        stream_data_callback_t data_callback)
        -> add_stream_result_t {

        auto stream_handle = ptr.get();
        auto present = streams.find(ptr);

        if (!(streams.end() == present)) {
            // The same connection already exists?
            assert(false);
            return std::unexpected(quic_error_code::stream_already_exists);
        }

        const auto & [emplaced_itr, emplace_status] = streams.emplace(
            std::move(ptr),
            stream_context{ stream_handle, *this, std::move(data_callback) });

        if (!emplace_status) {
            return std::unexpected(quic_error_code::stream_emplace_failed);
        }
        return emplaced_itr->second;
    }

    /**
     * Remove a stream from the stream map
     *
     * @param stream_handle Stream handle
     * @return Extracted node from the stream map. std::nullopt if no such
     * stream exists.
     */
    [[nodiscard]] auto
    remove_stream(void * stream_handle) -> remove_stream_result_t {
        auto present = streams.find(stream_handle);

        if (streams.end() == present) {
            return std::nullopt;
        }

        // Remove the key-value pair from the map
        // and return it to caller. The caller might
        // have some unfinished business with it.
        return streams.extract(present);
    }

    template <typename T = void *>
    [[nodiscard]] T handle_as() const {
        return static_cast<T>(connection_handle);
    }

private:
    void * connection_handle;
    /**
     * Enable lookup by raw pointer.
     */
    stream_map_t streams;
};
} // namespace mad::nexus
