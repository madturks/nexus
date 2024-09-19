#include <mad/nexus/msquic.hpp>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <chrono>

#include <fmt/format.h>

int main(void) {
    fmt::println("{}", __cplusplus);

    mad::nexus::quic_server_configuration cfg;
    cfg.alpn                         = "test";
    cfg.credentials.certificate_path = "/workspaces/nexus/vendor/msquic/test-cert/server.cert";
    cfg.credentials.private_key_path = "/workspaces/nexus/vendor/msquic/test-cert/server.key";
    cfg.idle_timeout                 = std::chrono::milliseconds{10000};
    cfg.udp_port_number              = 6666;
    auto server                      = mad::nexus::msquic_server::make(cfg);

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
