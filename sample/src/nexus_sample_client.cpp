#include <mad/log_printer.hpp>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/quic_callback_function.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/schemas/chat_generated.h>
#include <mad/nexus/schemas/main_generated.h>
#include <mad/nexus/schemas/monster_generated.h>

#include <cxxopts.hpp>

#include "lorem_ipsum.hpp"

static mad::log_printer logger{ "console" };

static void
client_on_connected([[maybe_unused]] void * uctx,
                    [[maybe_unused]] mad::nexus::connection & cctx) {}

static void
client_on_disconnected([[maybe_unused]] void * uctx,
                       [[maybe_unused]] mad::nexus::connection & cctx) {}

static void client_on_stream_start([[maybe_unused]] void * uctx,
                                   [[maybe_unused]] mad::nexus::stream & sctx) {

}

static void client_on_stream_end([[maybe_unused]] void * uctx,
                                 [[maybe_unused]] mad::nexus::stream & sctx) {}

[[maybe_unused]] static std::size_t
client_stream_data_received([[maybe_unused]] void * uctx,
                            std::span<const std::uint8_t> buf) {

    ::flatbuffers::Verifier vrf{ buf.data(), buf.size() };
    if (!mad::schemas::VerifyEnvelopeBuffer(vrf)) {
        MAD_LOG_ERROR_I(logger, "app_stream_data_received: corrupt data!");
        return 0;
    }

    auto envelope = mad::schemas::GetEnvelope(buf.data());

    switch (envelope->message_type()) {
        case mad::schemas::Message::Monster: {
            auto v = envelope->message_as_Monster();

            assert(v->hp() == 120);
            assert(v->mana() == 80);
            assert(v->name()->string_view() == "Deruvish");
            MAD_LOG_INFO_I(logger,
                           "app_stream_data_received: {} byte(s), received "
                           "monster {} , hp:{} mana:{}",
                           buf.size_bytes(), v->name()->c_str(), v->hp(),
                           v->mana());

        } break;
        case mad::schemas::Message::Chat: {

            auto v = envelope->message_as_Chat();
            MAD_LOG_INFO_I(logger,
                           "app_stream_data_received: {} byte(s), received "
                           "chat timestamp {}",
                           buf.size_bytes(), v->timestamp());
            assert(v->timestamp() == 123456789);
            assert(v->message()->string_view() == lorem_ipsum);
        } break;
        case mad::schemas::Message::NONE: {
            assert(0);
        } break;
    }

    return 0;
}

int main(int argc, char * argv []) {
    MAD_LOG_INFO_I(logger, "{}", __cplusplus);

    cxxopts::ParseResult parsed_options{};

    try {
        // Create options parser object
        cxxopts::Options options(
            "msquic-test-client",
            "Sample client application demonstrating usage of nexus.");

        // Define options
        options.add_options()("c,cert", "Certificate file path",
                              cxxopts::value<std::string>()->default_value(
                                  "/workspaces/nexus/vendor/msquic/test-cert/"
                                  "server.cert")) // Certificate file path
            ("k,key", "Private key file path",
             cxxopts::value<std::string>()->default_value(
                 "/workspaces/nexus/vendor/msquic/test-cert/"
                 "server.key"))        // Private key file path
            ("h,help", "Print usage"); // Help option

        // Parse command-line arguments
        parsed_options = options.parse(argc, argv);

        // Handle help option
        if (parsed_options.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }
    }
    catch (const std::exception & e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }

    mad::nexus::quic_configuration cfg{ mad::nexus::e_quic_impl_type::msquic,
                                        mad::nexus::e_role::client };
    cfg.alpn = "test";

    cfg.credentials.certificate_path =
        parsed_options ["cert"].as<std::string>();
    cfg.credentials.private_key_path = parsed_options ["key"].as<std::string>();
    cfg.idle_timeout = std::chrono::milliseconds{ 10000 };
    cfg.udp_port_number = 6666;

    auto app = mad::nexus::make_quic_application(cfg);

    auto client = app->make_client();

    // Register callback functions
    {
        using enum mad::nexus::callback_type;
        client->register_callback<connected>(
            &client_on_connected, client.get());
        client->register_callback<disconnected>(
            &client_on_disconnected, client.get());
        client->register_callback<stream_start>(
            &client_on_stream_start, client.get());
        client->register_callback<stream_data>(
            &client_stream_data_received, client.get());
        client->register_callback<stream_end>(
            &client_on_stream_end, client.get());
    }

    if (auto r = client->init(); mad::nexus::failed(r)) {
        MAD_LOG_ERROR_I(logger, "QUIC server initialization failed: {}, {}",
                        r.value(), r.message());
        return -1;
    }

    if (auto r = client->connect("127.0.0.1", 6666); mad::nexus::failed(r)) {
        MAD_LOG_ERROR_I(logger, "QUIC server listen failed: {}, {}", r.value(),
                        r.message());
        return -2;
    }

    MAD_LOG_INFO_I(logger, "QUIC client connected to the destination.");
    MAD_LOG_INFO_I(logger, "Press any key to stop.");
    getchar();
}
