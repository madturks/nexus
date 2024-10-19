
#include <mad/concurrent.hpp>
#include <mad/log_printer.hpp>
#include <mad/macro>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/quic_connection_context.hpp>
#include <mad/nexus/quic_error_code.hpp>
#include <mad/nexus/quic_stream_context.hpp>
#include <mad/nexus/schemas/chat_generated.h>
#include <mad/nexus/schemas/main_generated.h>
#include <mad/nexus/schemas/monster_generated.h>

#include <cxxopts.hpp>
#include <flatbuffers/default_allocator.h>
#include <flatbuffers/detached_buffer.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <flatbuffers/flatbuffers.h>
#include <fmt/format.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <span>
#include <thread>
#include <unordered_map>

#include "lorem_ipsum.hpp"

struct session {

    static constexpr auto kStreamCount = 10;
    static constexpr auto kThreadSleepAmountMs =
        std::chrono::milliseconds{ 10 };

    session(mad::nexus::quic_server & srv, mad::nexus::connection & cctx) :
        server(srv), connection_context(cctx) {}

    session(const session &) = delete;
    session & operator=(const session &) = delete;
    session(session &&) = default;
    session & operator=(session &&) = delete;

    ~session() {
        stop_source.request_stop();
    }

    static std::size_t
    stream_data_received(void * uctx,
                         [[maybe_unused]] std::span<const std::uint8_t> buf) {

        [[maybe_unused]] auto & conn = *static_cast<session *>(uctx);
        return 0;
    }

    void start_streams() {
        for (auto i = 0u; i < kStreamCount; i++) {
            if (auto r = server.open_stream(connection_context,
                                            mad::nexus::stream_data_callback_t{
                                                &stream_data_received, this });
                !r) {
                continue;
            }
        }

        // Dispatch threads
        for (int i = 0; i < 32; i++) {
            threads.emplace_back(&session::thread_routine, this,
                                 stop_source.get_token(), std::ref(server));
        }
    }

    void thread_routine(std::stop_token st,
                        mad::nexus::quic_server & quic_server) {

        while (!st.stop_requested()) {
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
                                env.add_message_type(
                                    mad::schemas::Message::Monster);
                                return env.Finish();
                            } else {
                                auto msg = fbb.CreateString(lorem_ipsum);
                                mad::schemas::ChatBuilder cb{ fbb };
                                cb.add_message(msg);
                                cb.add_timestamp(123456789);
                                auto f = cb.Finish();
                                mad::schemas::EnvelopeBuilder env{ fbb };
                                env.add_message(f.Union());
                                env.add_message_type(
                                    mad::schemas::Message::Chat);
                                return env.Finish();
                            }
                        }));
            }
            std::this_thread::sleep_for(kThreadSleepAmountMs);
        }
    }

    mad::nexus::quic_server & server;
    mad::nexus::connection & connection_context;

    std::stop_source stop_source;
    std::vector<std::jthread> threads;
    std::unordered_map<std::uint64_t,
                       std::reference_wrapper<mad::nexus::stream>>
        streams;
};

static mad::concurrent<std::unordered_map<std::uint64_t, session>> connections;

static std::string message = "hello from the other side";

static mad::log_printer logger{ "console" };

static void server_on_connected(void * uctx, mad::nexus::connection & cctx) {
    assert(uctx);

    MAD_LOG_INFO_I(logger, "app received new connection!");

    auto & quic_server = *static_cast<mad::nexus::quic_server *>(uctx);

    auto map = connections.exclusive_access();
    const auto & [pair, emplaced] = map->emplace(
        cctx.serial_number(), session{ quic_server, cctx });

    if (emplaced) {
        pair->second.start_streams();
    }

    MAD_LOG_INFO_I(logger, "server_on_connected return");
}

static void
server_on_disconnected([[maybe_unused]] void * uctx,
                       [[maybe_unused]] mad::nexus::connection & cctx) {
    assert(uctx);

    MAD_LOG_INFO_I(logger, "app received disconnection !");

    auto map = connections.exclusive_access();

    if (auto itr = map->find(cctx.serial_number()); map->end() != itr) {
        map->erase(itr);
    }
}

static void server_on_stream_start([[maybe_unused]] void * uctx,
                                   [[maybe_unused]] mad::nexus::stream & sctx) {
    auto map = connections.exclusive_access();
    auto fi = map->find(sctx.connection().serial_number());
    if (fi == map->end()) {
        MAD_LOG_WARN_I(logger, "No such connection with serial {}",
                       sctx.connection().serial_number());
        return;
    }

    fi->second.streams.emplace(sctx.serial_number(), std::ref(sctx));
}

static void server_on_stream_end([[maybe_unused]] void * uctx,
                                 [[maybe_unused]] mad::nexus::stream & sctx) {
    auto map = connections.exclusive_access();
    auto fi = map->find(sctx.connection().serial_number());
    if (fi == map->end()) {
        MAD_LOG_WARN_I(logger, "No such connection with serial {}",
                       sctx.connection().serial_number());
        return;
    }

    auto fi2 = fi->second.streams.find(sctx.serial_number());
    if (fi2 == fi->second.streams.end()) {
        return;
    }

    fi->second.streams.erase(fi2);
}

int main(int argc, char * argv []) {
    logger.set_log_level(mad::log_level::debug);
    MAD_LOG_INFO_I(logger, "{}", __cplusplus);

    cxxopts::ParseResult parsed_options{};

    try {
        // Create options parser object
        cxxopts::Options options(
            "msquic-test-server",
            "Sample server application demonstrating usage of nexus.");

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
                                        mad::nexus::e_role::server };
    cfg.alpn = "test";
    cfg.credentials.certificate_path =
        parsed_options ["cert"].as<std::string>();
    cfg.credentials.private_key_path = parsed_options ["key"].as<std::string>();
    cfg.idle_timeout = std::chrono::milliseconds{ 10000 };
    cfg.udp_port_number = 6666;

    auto app = mad::nexus::make_quic_application(cfg);
    auto server = app->make_server();

    // Register callback functions
    {
        using enum mad::nexus::callback_type;
        server->register_callback<connected>(
            &server_on_connected, server.get());
        server->register_callback<disconnected>(
            &server_on_disconnected, server.get());
        server->register_callback<stream_start>(
            &server_on_stream_start, server.get());
        server->register_callback<stream_end>(
            &server_on_stream_end, server.get());
    }

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
    {
        auto map = connections.exclusive_access();
        map->clear();
    }
}
