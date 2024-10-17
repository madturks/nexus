#pragma once

#include <mad/concurrent.hpp>
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_server.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>

#include <expected>
#include <memory>
#include <system_error>

namespace mad::nexus {

class msquic_server final : virtual public quic_server,
                            virtual public msquic_base {

    using connection_map_t =
        std::unordered_map<std::shared_ptr<void>, connection_context,
                           shared_ptr_raw_hash, shared_ptr_raw_equal>;

    std::shared_ptr<void> listener_opaque;
    // Might be moved to quic_server?
    mad::concurrent<connection_map_t> connection_map;

public:
    const msquic_application & application;

    virtual ~msquic_server() override;

    virtual std::error_code listen() override;

    /**
     * Add a new connection to the connection map.
     *
     * @param ptr The type-erased connection shared pointer
     -
     * @return reference to the connection_context if successful,
     * std::nullopt otherwise.
     */
    auto add_new_connection(std::shared_ptr<void> ptr)
        -> std::optional<std::reference_wrapper<connection_context>> {

        auto connection_handle = ptr.get();
        auto writer = connection_map.exclusive_access();
        auto present = writer->find(ptr);

        if (!(writer.end() == present)) {
            // The same connection already exists?
            assert(false);
            return std::nullopt;
        }

        const auto & [emplaced_itr, emplace_status] = writer->emplace(
            std::move(ptr), connection_context{ connection_handle });

        if (!emplace_status) {
            return std::nullopt;
        }
        return emplaced_itr->second;
    }

    /**
     * Remove the connection key-value pair by
     * @p connection_handle.
     *
     * @param connection_handle The connection's handle
     *
     * @return extracted node (key, value) if connection exists,
     * std::nullopt otherwise.
     */
    auto remove_connection(void * connection_handle)
        -> std::optional<connection_map_t::node_type> {
        auto writer = connection_map.exclusive_access();
        auto present = writer->find(connection_handle);

        if (writer.end() == present) {
            return std::nullopt;
        }

        // Remove the key-value pair from the map
        // and return it to caller. The caller might
        // have some unfinished business with it.
        return writer->extract(present);
    }

private:
    friend std::unique_ptr<quic_server> msquic_application::make_server();
    /**
     * Construct a new msquic server object
     *
     * @param app MSQUIC application
     */
    msquic_server(const msquic_application & app);
};

} // namespace mad::nexus
