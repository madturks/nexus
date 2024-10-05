
#include <mad/log_printer.hpp>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>

#include <flatbuffers/default_allocator.h>
#include <flatbuffers/detached_buffer.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <flatbuffers/flatbuffers.h>
#include <fmt/format.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <fbs-schemas/chat_generated.h>
#include <fbs-schemas/main_generated.h>
#include <fbs-schemas/monster_generated.h>
#include <span>
#include <thread>
#include <unordered_map>

#include "lorem_ipsum.hpp"

static std::atomic_bool stop;
static std::vector<std::thread> threads;

static std::unordered_map<std::string,
                          std::reference_wrapper<mad::nexus::stream_context>>
    streams;
static std::string message = "hello from the other side";

static mad::log_printer logger{ "console" };

static void test_loop(mad::nexus::quic_server & quic_server) {

    for (auto & [str, stream] : streams) {
        (void) quic_server.send(
            stream.get(),
            quic_server.build_message(
                [](::flatbuffers::FlatBufferBuilder & fbb) {
                    if (std::rand() % 2 == 0) {
                        mad::schemas::Vec3 coords{ 10, 20, 30 };
                        auto name = fbb.CreateString("Deruvish");
                        mad::schemas::MonsterBuilder mb{ fbb };
                        mb.add_hp(120);
                        mb.add_mana(80);
                        mb.add_name(name);
                        mb.add_pos(&coords);
                        auto f = mb.Finish();

                        mad::schemas::EnvelopeBuilder env{ fbb };
                        env.add_message(f.Union());
                        env.add_message_type(mad::schemas::Message::Monster);
                        return env.Finish();
                    } else {
                        auto msg = fbb.CreateString(lorem_ipsum);
                        mad::schemas::ChatBuilder cb{ fbb };
                        cb.add_message(msg);
                        cb.add_timestamp(123456789);
                        auto f = cb.Finish();
                        mad::schemas::EnvelopeBuilder env{ fbb };
                        env.add_message(f.Union());
                        env.add_message_type(mad::schemas::Message::Chat);
                        return env.Finish();
                    }
                }));
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
}

static std::size_t app_stream_data_received([[maybe_unused]] void * uctx,
                                            std::span<const std::uint8_t> buf) {
    MAD_LOG_INFO_I(logger, "app_stream_data_received: received {} byte(s)",
                   buf.size_bytes());
    return 0;
}

static void server_on_connected(void * uctx,
                                mad::nexus::connection_context & cctx) {
    assert(uctx);

    MAD_LOG_INFO_I(logger, "app received new connection!");

    auto & quic_server = *static_cast<mad::nexus::quic_server *>(uctx);

    for (int i = 0; i < 10; i++) {

        if (auto r = quic_server.open_stream(
                cctx,
                mad::nexus::stream_data_callback_t{ &app_stream_data_received,
                                                    nullptr });
            !r) {
            MAD_LOG_INFO_I(logger,
                           "server_on_connected -- stream open failed: {}!",
                           r.error().message());
            continue;
        } else {
            streams.emplace("stream_" + std::to_string(i), r.value());

            auto msg = quic_server.build_message([](auto & fbb) {
                mad::schemas::Vec3 coords{ 10, 20, 30 };
                auto name = fbb.CreateString("Deruvish");
                mad::schemas::MonsterBuilder mb{ fbb };
                mb.add_hp(120);
                mb.add_mana(80);
                mb.add_name(name);
                mb.add_pos(&coords);
                auto f = mb.Finish();

                mad::schemas::EnvelopeBuilder env{ fbb };
                env.add_message(f.Union());
                env.add_message_type(mad::schemas::Message::Monster);
                return env.Finish();
            });

            if (!quic_server.send(r.value().get(), std::move(msg))) {
                MAD_LOG_ERROR_I(logger, "could not send msg!");
            }
        }
    }

    for (int i = 0; i < 32; i++) {
        std::thread thr{ [&]() {
            while (!stop.load())
                test_loop(quic_server);
        } };
        threads.push_back(std::move(thr));
    }
    MAD_LOG_INFO_I(logger, "server_on_connected return");
}

static void
server_on_disconnected(void * uctx,
                       [[maybe_unused]] mad::nexus::connection_context & cctx) {
    assert(uctx);

    MAD_LOG_INFO_I(logger, "app received disconnection !");

    [[maybe_unused]] auto & quic_server =
        *static_cast<mad::nexus::quic_server *>(uctx);
    stop.store(true);
    for (auto & thr : threads) {
        thr.join();
    }
    threads.clear();
    streams.clear();
}

int main(void) {
    logger.set_log_level(mad::log_level::debug);
    MAD_LOG_INFO_I(logger, "{}", __cplusplus);
    stop.store(false);

    mad::nexus::quic_configuration cfg;
    cfg.alpn = "test";
    cfg.credentials.certificate_path =
        "/workspaces/nexus/vendor/msquic/test-cert/server.cert";
    cfg.credentials.private_key_path =
        "/workspaces/nexus/vendor/msquic/test-cert/server.key";
    cfg.idle_timeout = std::chrono::milliseconds{ 10000 };
    cfg.udp_port_number = 6666;

    auto app = mad::nexus::make_quic_application(
        mad::nexus::e_quic_impl_type::msquic, cfg);
    auto server = app->make_server();

    server->callbacks.on_connected = mad::nexus::quic_callback_function{
        &server_on_connected, server.get()
    };
    server->callbacks.on_disconnected = mad::nexus::quic_callback_function{
        &server_on_disconnected, server.get()
    };

    if (auto r = server->init(); mad::nexus::failed(r)) {
        MAD_LOG_ERROR_I(logger, "QUIC server initialization failed: {}, {}",
                        r.value(), r.message());
        return -1;
    }

    if (auto r = server->listen(); mad::nexus::failed(r)) {
        MAD_LOG_ERROR_I(logger, "QUIC server listen failed: {}, {}", r.value(),
                        r.message());
        return -2;
    }

    MAD_LOG_INFO_I(
        logger, "QUIC server is listening for incoming connections.");
    MAD_LOG_INFO_I(logger, "Press any key to stop the app.");
    getchar();
}
