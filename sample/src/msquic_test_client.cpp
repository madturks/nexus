#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic.hpp>

#include <mad/log_printer.hpp>



static mad::log_printer logger{"console"};

static void client_on_connected([[maybe_unused]] void * uctx, [[maybe_unused]] mad::nexus::connection_context * cctx) {}

static void client_on_disconnected([[maybe_unused]] void * uctx, [[maybe_unused]] mad::nexus::connection_context * cctx) {}

[[maybe_unused]] static std::size_t client_stream_data_received([[maybe_unused]] void * uctx, std::span<const std::uint8_t> buf) {
    MAD_LOG_INFO_I(logger, "app_stream_data_received: received {} byte(s)", buf.size_bytes());
    return 0;
}

int main(void) {
    MAD_LOG_INFO_I(logger, "{}", __cplusplus);

    mad::nexus::quic_configuration cfg;
    cfg.alpn                                  = "test";
    cfg.credentials.certificate_path          = "/workspaces/nexus/vendor/msquic/test-cert/server.cert";
    cfg.credentials.private_key_path          = "/workspaces/nexus/vendor/msquic/test-cert/server.key";
    cfg.idle_timeout                          = std::chrono::milliseconds{10000};
    cfg.udp_port_number                       = 6666;

    auto app                                  = mad::nexus::make_quic_application(mad::nexus::e_quic_impl_type::msquic, cfg);
    auto client                               = app->make_client();

    client->callbacks.on_connected            = mad::nexus::quic_callback_function{&client_on_connected, client.get()};
    client->callbacks.on_disconnected         = mad::nexus::quic_callback_function{&client_on_disconnected, client.get()};
    client->callbacks.on_stream_data_received = mad::nexus::quic_callback_function{&client_stream_data_received, client.get()};
    //
    // server->callbacks.on_connected = ;

    if (auto r = client->init(); mad::nexus::failed(r)) {
        MAD_LOG_ERROR_I(logger, "QUIC server initialization failed: {}, {}", r.value(), r.message());
        return -1;
    }

    if (auto r = client->connect("127.0.0.1", 6666); mad::nexus::failed(r)) {
        MAD_LOG_ERROR_I(logger, "QUIC server listen failed: {}, {}", r.value(), r.message());
        return -2;
    }

    MAD_LOG_INFO_I(logger, "QUIC client connected to the destination.");
    MAD_LOG_INFO_I(logger, "Press any key to stop.");
    getchar();
}
