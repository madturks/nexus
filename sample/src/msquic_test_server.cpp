#include "mad/nexus/quic_connection_context.hpp"
#include "mad/nexus/quic_stream_context.hpp"
#include <flatbuffers/default_allocator.h>
#include <flatbuffers/detached_buffer.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <fmt/base.h>
#include <mad/nexus/quic_server.hpp>
#include <mad/nexus/msquic/msquic_server.hpp>
#include <mad/nexus/quic_error_code.hpp>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <chrono>
#include <unordered_map>

#include <fmt/format.h>
#include <flatbuffers/flatbuffers.h>
#include <fbs-schemas/main_generated.h>
#include <thread>
#include <atomic>

std::atomic_bool stop;
static std::vector<std::thread> threads;

static std::unordered_map<std::string, mad::nexus::stream_context *> streams;
static std::string message = "hello from the other side";

static void test_loop(mad::nexus::quic_server & server) {

    for (auto & [str, stream] : streams) {
        auto & fbb = server.get_message_builder();
        auto v     = mad::schemas::Vec3(1, 2, 3);
        auto o     = mad::schemas::CreateMonster(fbb, &v, 150, 80);
        fbb.Finish(o);
        auto a = fbb.Release();
        server.send(stream, std::move(a));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
}

static std::size_t app_stream_data_received(void * uctx, std::span<const std::uint8_t> buf) {
    fmt::println("app_stream_data_received: received {} byte(s)", buf.size_bytes());
    return 0;
}

static void server_on_connected(void * uctx, mad::nexus::connection_context * cctx) {
    assert(cctx);
    assert(uctx);

    fmt::println("app received new connection!");

    auto & quic_server = *static_cast<mad::nexus::msquic_server *>(uctx);
    stop.store(false);

    for (int i = 0; i < 10; i++) {
        auto & fbb = quic_server.get_message_builder();
        auto v     = mad::schemas::Vec3(1, 2, 3);
        auto o     = mad::schemas::CreateMonster(fbb, &v, 150, 80);
        fbb.Finish(o);
        auto a = fbb.Release();

        if (auto r = quic_server.open_stream(cctx, mad::nexus::stream_data_callback_t{&app_stream_data_received, nullptr}); !r) {
            fmt::println("server_on_connected -- stream open failed: {}!", r.error().message());
            continue;
        } else {
            streams.emplace("stream_" + std::to_string(i), r.value());
            if (!quic_server.send(r.value(), std::move(a))) {
                fmt::println("could not send msg!");
            }
        }
    }

    for (int i = 0; i < 10; i++) {

        std::thread thr{[&]() {
            while (!stop.load())
                test_loop(quic_server);
        }};
        threads.push_back(std::move(thr));
    }
}

static void server_on_disconnected(void * uctx, mad::nexus::connection_context * cctx) {
    assert(cctx);
    assert(uctx);

    fmt::println("app received disconnection !");

    auto & quic_server = *static_cast<mad::nexus::msquic_server *>(uctx);
    stop.store(true);
    for (auto & thr : threads) {
        thr.join();
    }
    threads.clear();
    streams.clear();
}

int main(void) {
    fmt::println("{}", __cplusplus);

    mad::nexus::quic_configuration cfg;
    cfg.alpn                          = "test";
    cfg.credentials.certificate_path  = "/workspaces/nexus/vendor/msquic/test-cert/server.cert";
    cfg.credentials.private_key_path  = "/workspaces/nexus/vendor/msquic/test-cert/server.key";
    cfg.idle_timeout                  = std::chrono::milliseconds{10000};
    cfg.udp_port_number               = 6666;
    auto server                       = mad::nexus::msquic_server::make(cfg);

    server->callbacks.on_connected    = mad::nexus::quic_callback_function{&server_on_connected, server.get()};
    server->callbacks.on_disconnected = mad::nexus::quic_callback_function{&server_on_disconnected, server.get()};
    //
    // server->callbacks.on_connected = ;

    if (auto r = server->init(); !mad::nexus::successful(r)) {
        fmt::println("QUIC server initialization failed: {}, {}", r.value(), r.message());
        return -1;
    }

    if (auto r = server->listen(); !mad::nexus::successful(r)) {
        fmt::println("QUIC server listen failed: {}, {}", r.value(), r.message());
        return -2;
    }

    fmt::println("QUIC server is listening for incoming connections.");
    fmt::println("Press any key to stop.");
    getchar();
}
