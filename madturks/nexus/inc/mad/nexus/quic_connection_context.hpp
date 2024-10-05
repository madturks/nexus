#pragma once

#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>

#include <expected>
#include <unordered_map>

namespace mad::nexus {
struct connection_context {
    using connection_handle_t = void *;
    connection_handle_t connection_handle;

    using stream_handle_t = std::shared_ptr<void>;
    using add_stream_result_t =
        std::expected<std::reference_wrapper<stream_context>, std::error_code>;

    /**
    * Add a new connection to the connection map.
    *
    * @param ptr The type-erased connection shared pointer
    -
    * @return reference to the connection_context if successful,
    * std::nullopt otherwise.
    */
    add_stream_result_t add_new_stream(std::shared_ptr<void> ptr,
                                       stream_data_callback_t data_callback) {

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

    auto remove_stream() {}

    /**
     * Enable lookup by raw pointer.
     */
    std::unordered_map<stream_handle_t, stream_context, shared_ptr_raw_hash,
                       shared_ptr_raw_equal>
        streams;
};
} // namespace mad::nexus
