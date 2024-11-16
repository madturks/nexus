/******************************************************
 * msquic_server unit tests
 *
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_server.hpp>
#include <mad/static_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <msquic.h>

#include "mock_msquic_application.hpp"
#include "mock_msquic_fns.hpp"

namespace mad::nexus {

struct tf_msquic_server : public ::testing::Test {
    mock_msquic_application mock_app = {};
    // Alias for convenience.
    QUIC_API_TABLE & api = mock_app.api_table;

    template <typename... Args>
    auto construct_uut(Args &&... args) {
        return std::unique_ptr<msquic_server>(
            new msquic_server(std::forward<Args>(args)...));
    }

    void SetUp() override {
        api.ListenerOpen = mock_listener_open;
        api.ListenerStart = mock_listener_start;
        api.ListenerClose = mock_listener_close;
        listen_addr.Ip.sa_family = AF_UNSPEC;
        listen_addr.Ipv6.sin6_port = htons(
            1243); // Set port number (e.g., 12345)
        uut = construct_uut(mock_app);
    }

    static inline auto lstnr_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0x1DEABAAD);
    }();

    static constexpr std::string_view test_alpn_const = "madturks/realm";
    static constexpr std::uint16_t test_port = 1243;
    std::vector<std::uint8_t> test_alpn{ test_alpn_const.begin(),
                                         test_alpn_const.end() };

    const QUIC_BUFFER alpn{ .Length = static_cast<std::uint32_t>(
                                test_alpn.size()),
                            .Buffer = test_alpn.data() };

    std::span<const QUIC_BUFFER> alpns{ &alpn, 1 };

    QUIC_ADDR listen_addr{};

    QUIC_LISTENER_CALLBACK_HANDLER listener_callback_handler{ nullptr };
    void * ctxt{ nullptr };

    static_mock<QUIC_LISTENER_OPEN_FN> mock_listener_open{};
    static_mock<QUIC_LISTENER_START_FN> mock_listener_start{};
    static_mock<QUIC_LISTENER_CLOSE_FN> mock_listener_close{};

    std::unique_ptr<msquic_server> uut{ nullptr };
};

/******************************************************
 * Try to construct a msquic_client object.
 * It should succeed.
 ******************************************************/
TEST_F(tf_msquic_server, construct) {
    auto f = construct_uut(mock_app);
    ASSERT_NE(nullptr, f);
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_server, listen_success) {
    MockListenerOpenCall(QUIC_STATUS_SUCCESS, mock_listener_open,
                         mock_app.registration(), lstnr_object,
                         listener_callback_handler, ctxt);
    MockListenerStartCall(QUIC_STATUS_SUCCESS, mock_listener_start,
                          lstnr_object, listener_callback_handler, alpns,
                          &listen_addr, ctxt);

    MockListenerCloseCall(QUIC_STATUS_SUCCESS, mock_listener_close,
                          lstnr_object, listener_callback_handler, ctxt);

    ASSERT_TRUE(uut->listen(test_alpn_const, test_port));
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_server, listen_open_fail) {
    MockListenerOpenCall(QUIC_STATUS_ABORTED, mock_listener_open,
                         mock_app.registration(), lstnr_object,
                         listener_callback_handler, ctxt);

    auto r = uut->listen(test_alpn_const, test_port);
    ASSERT_FALSE(r);
    ASSERT_EQ(r.error(), quic_error_code::listener_initialization_failed);
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_server, listen_start_fail) {
    MockListenerOpenCall(QUIC_STATUS_SUCCESS, mock_listener_open,
                         mock_app.registration(), lstnr_object,
                         listener_callback_handler, ctxt);
    MockListenerStartCall(QUIC_STATUS_ABORTED, mock_listener_start,
                          lstnr_object, listener_callback_handler, alpns,
                          &listen_addr, ctxt);
    MockListenerCloseCall(QUIC_STATUS_SUCCESS, mock_listener_close,
                          lstnr_object, listener_callback_handler, ctxt);
    auto r = uut->listen(test_alpn_const, test_port);
    ASSERT_FALSE(r);
    ASSERT_EQ(r.error(), quic_error_code::listener_start_failed);
}
} // namespace mad::nexus
