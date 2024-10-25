#pragma once

#include <mad/concurrent.hpp>
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_server.hpp>
#include <mad/nexus/shared_ptr_raw_equal.hpp>
#include <mad/nexus/shared_ptr_raw_hash.hpp>

#include <memory>

struct QUIC_HANDLE;

namespace mad::nexus {

class msquic_server final : virtual public quic_server,
                            virtual public msquic_base {

    friend struct MsQuicServerCallbacks;

    using connection_map_t =
        std::unordered_map<std::shared_ptr<void>, connection,
                           shared_ptr_raw_hash, shared_ptr_raw_equal>;

    std::shared_ptr<QUIC_HANDLE> listener{};
    // Might be moved to quic_server?
    mad::concurrent<connection_map_t> connection_map{};

public:
    virtual ~msquic_server() override;

    virtual result<> listen(std::string_view alpn, std::uint16_t port) override;

    /**
     * Add a new connection to the connection map.
     *
     * @param ptr The type-erased connection shared pointer
     -
     * @return reference to the connection_context if successful,
     * std::nullopt otherwise.
     */
    auto add_new_connection(std::shared_ptr<void> ptr)
        -> result<std::reference_wrapper<connection>> {

        auto connection_handle = ptr.get();
        auto writer = connection_map.exclusive_access();
        auto present = writer->find(ptr);

        if (!(writer.end() == present)) {
            // The same connection already exists?
            MAD_ASSERT(false);
            return std::unexpected(quic_error_code::connection_already_exists);
        }

        const auto & [emplaced_itr, emplace_status] = writer->emplace(
            std::move(ptr), connection{ connection_handle });

        if (!emplace_status) {
            return std::unexpected(quic_error_code::connection_emplace_failed);
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
        -> result<connection_map_t::node_type> {
        auto writer = connection_map.exclusive_access();
        auto present = writer->find(connection_handle);

        if (writer.end() == present) {
            return std::unexpected{
                quic_error_code::connection_does_not_exists
            };
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
    using msquic_base::msquic_base;
};

} // namespace mad::nexus
