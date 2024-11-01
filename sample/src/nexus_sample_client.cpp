/******************************************************
 * Nexus sample client application.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/log_printer.hpp>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/nexus/quic_connection.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/result.hpp>
#include <mad/nexus/schemas/chat_generated.h>
#include <mad/nexus/schemas/main_generated.h>
#include <mad/nexus/schemas/monster_generated.h>

#include <cxxopts.hpp>

#include <expected>
#include <system_error>

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
    logger.set_log_level(mad::log_level::info);
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

    auto application = mad::nexus::make_quic_application(cfg);

    auto client =
        application
            .and_then([](auto && app) {
                return app->make_client();
            })
            .transform(
                [](std::unique_ptr<mad::nexus::quic_client> && cl) noexcept {
                    using enum mad::nexus::callback_type;
                    cl->register_callback<connected>(
                        &client_on_connected, cl.get());
                    cl->register_callback<disconnected>(
                        &client_on_disconnected, cl.get());
                    cl->register_callback<stream_start>(
                        &client_on_stream_start, cl.get());
                    cl->register_callback<stream_data>(
                        &client_stream_data_received, cl.get());
                    cl->register_callback<stream_end>(
                        &client_on_stream_end, cl.get());
                    return std::move(cl);
                });

    auto result = client.and_then([](auto && cl) {
        return cl->connect("127.0.0.1", 6666);
    });

    if (!result) {
        const auto & error = result.error();
        MAD_LOG_ERROR_I(logger, "QUIC server listen failed: {}, {}",
                        error.value(), error.message());
        return error.value();
    }

    MAD_LOG_INFO_I(logger, "QUIC client connected to the destination.");
    MAD_LOG_INFO_I(logger, "Press any key to stop.");
    getchar();
}
