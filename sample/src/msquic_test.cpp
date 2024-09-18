// #include <boost/asio.hpp>

#include "fmt/base.h"
#include "nexus/quic.hpp"
#include <nexus/msquic.hpp>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <chrono>

#include <fmt/format.h>

namespace mt { namespace exceptions {
    class file_not_found_exception : public std::exception {};
}} // namespace mt::exceptions

int main(void) {
    fmt::println("{}", __cplusplus);

    mt::nexus::quic_server_configuration cfg;
    cfg.alpn                         = "test";
    cfg.credentials.certificate_path = "/workspaces/nexus/vendor/msquic/test-cert/server.cert";
    cfg.credentials.private_key_path = "/workspaces/nexus/vendor/msquic/test-cert/server.key";
    cfg.idle_timeout                 = std::chrono::milliseconds{10000};
    auto server                      = mt::nexus::msquic_server::make(cfg);

    if (auto r = server->init(); !mt::nexus::successful(r)) {
        fmt::println("QUIC server initialization failed: {}, {}", r.value(), r.message());
        return -1;
    }

    if (auto r = server->listen(); !mt::nexus::successful(r)) {
        fmt::println("QUIC server listen failed: {}, {}", r.value(), r.message());
        return -2;
    }

    fmt::println("QUIC server is listening for incoming connections.");
    fmt::println("Press any key to stop.");
    getchar();
}
