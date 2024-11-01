/******************************************************
 * msquic_client unit tests
 *
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_client.hpp>
#include <mad/static_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <msquic.h>

#include "mock_msquic_application.hpp"

namespace mad::nexus {

struct tf_msquic_client : public ::testing::Test {
    mock_msquic_application mock_app = {};
    // Alias for convenience.
    QUIC_API_TABLE & api = mock_app.api_table;

    void SetUp() override {}

    template <typename... Args>
    auto construct_uut(Args &&... args) {
        return std::unique_ptr<msquic_client>(
            new msquic_client(std::forward<Args>(args)...));
    }

    static inline auto conn_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xDEADC0DE);
    }();

    connection & connection_field(auto & p) {
        return *p->connection;
    }
};

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;

/******************************************************
 * Try to construct a msquic_client object.
 * It should succeed.
 ******************************************************/
TEST_F(tf_msquic_client, construct) {
    auto f = construct_uut(mock_app);
    ASSERT_NE(nullptr, f);
}

/******************************************************
 * Try to use the connect function.
 * It should succeed, but the user on_connected callback
 * must not be triggered since the mock API does not invoke
 * the handler with a CONNECTED event.
 ******************************************************/
TEST_F(tf_msquic_client, connect) {
    auto f = construct_uut(mock_app);

    static_mock<QUIC_CONNECTION_OPEN_FN> mock_connection_open;
    static_mock<QUIC_CONNECTION_START_FN> mock_connection_start;

    ON_CALL(*mock_connection_open, Call(_, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC reg, QUIC_CONNECTION_CALLBACK_HANDLER handler,
                       void * context, HQUIC * conn) {
                ASSERT_EQ(reg, mock_app.registration());
                ASSERT_NE(nullptr, handler);
                ASSERT_NE(context, nullptr);
                ASSERT_NE(nullptr, conn);
                // Set registration handle to the mock
                // registration object.
                *conn = conn_object;
            }),
            Return(0)));

    ON_CALL(*mock_connection_start, Call(_, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC conn, HQUIC conf, QUIC_ADDRESS_FAMILY family,
                             const char * ServerName, uint16_t port) {
                      ASSERT_EQ(conn, conn_object);
                      ASSERT_EQ(conf, mock_app.configuration());
                      ASSERT_EQ(family, QUIC_ADDRESS_FAMILY_UNSPEC);
                      ASSERT_STREQ("127.0.0.1", ServerName);
                      ASSERT_EQ(1234, port);
                  }),
                  Return(0)));

    EXPECT_CALL(*mock_connection_open, Call(_, _, _, _)).Times(1);
    EXPECT_CALL(*mock_connection_start, Call(_, _, _, _, _)).Times(1);
    api.ConnectionOpen = mock_connection_open;
    api.ConnectionStart = mock_connection_start;

    auto result = f->connect("127.0.0.1", 1234);
    ASSERT_TRUE(result.has_value());
}

/******************************************************
 * Try to connect, but MSQUIC ConnectionOpen is going
 * to indicate a failure. connect result must reflect
 * that.
 ******************************************************/
TEST_F(tf_msquic_client, connect_open_fail) {
    auto f = construct_uut(mock_app);

    static_mock<QUIC_CONNECTION_OPEN_FN> mock_connection_open;

    ON_CALL(*mock_connection_open, Call(_, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC reg, QUIC_CONNECTION_CALLBACK_HANDLER handler,
                       void * context, HQUIC * conn) {
                ASSERT_EQ(reg, mock_app.registration());
                ASSERT_NE(nullptr, handler);
                ASSERT_NE(context, nullptr);
                ASSERT_NE(nullptr, conn);
            }),
            Return(1)));

    EXPECT_CALL(*mock_connection_open, Call(_, _, _, _)).Times(1);
    api.ConnectionOpen = mock_connection_open;
    auto result = f->connect("127.0.0.1", 1234);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(
        result.error(), quic_error_code::connection_initialization_failed);
}

/******************************************************
 * Try to connect, but MSQUIC ConnectionStart is going
 * to indicate a failure. connect result must reflect
 * that.
 ******************************************************/
TEST_F(tf_msquic_client, connect_start_fail) {
    auto f = construct_uut(mock_app);

    static_mock<QUIC_CONNECTION_OPEN_FN> mock_connection_open;
    static_mock<QUIC_CONNECTION_START_FN> mock_connection_start;
    static_mock<QUIC_CONNECTION_CLOSE_FN> mock_connection_close;

    ON_CALL(*mock_connection_open, Call(_, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC reg, QUIC_CONNECTION_CALLBACK_HANDLER handler,
                       void * context, HQUIC * conn) {
                ASSERT_EQ(reg, mock_app.registration());
                ASSERT_NE(nullptr, handler);
                ASSERT_NE(context, nullptr);
                ASSERT_NE(nullptr, conn);
                // Set registration handle to the mock
                // registration object.
                *conn = conn_object;
            }),
            Return(0)));

    ON_CALL(*mock_connection_start, Call(_, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC conn, HQUIC conf, QUIC_ADDRESS_FAMILY family,
                             const char * ServerName, uint16_t port) {
                      ASSERT_EQ(conn, conn_object);
                      ASSERT_EQ(conf, mock_app.configuration());
                      ASSERT_EQ(family, QUIC_ADDRESS_FAMILY_UNSPEC);
                      ASSERT_STREQ("127.0.0.1", ServerName);
                      ASSERT_EQ(1234, port);
                  }),
                  Return(1)));

    ON_CALL(*mock_connection_close, Call(_))
        .WillByDefault(DoAll(Invoke([&](HQUIC handle) {
            ASSERT_EQ(handle, conn_object);
        })));

    EXPECT_CALL(*mock_connection_open, Call(_, _, _, _)).Times(1);
    EXPECT_CALL(*mock_connection_close, Call(_)).Times(1);
    EXPECT_CALL(*mock_connection_start, Call(_, _, _, _, _)).Times(1);
    api.ConnectionOpen = mock_connection_open;
    api.ConnectionClose = mock_connection_close;
    api.ConnectionStart = mock_connection_start;

    auto result = f->connect("127.0.0.1", 1234);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), quic_error_code::connection_start_failed);
}

/******************************************************
 * Try to connect, and while the connection attempt is
 * in progress, try to disconnect. It should fail since
 * the connection is not established yet.
 ******************************************************/
TEST_F(tf_msquic_client, disconnect_connection_in_progress) {
    auto f = construct_uut(mock_app);

    static_mock<QUIC_CONNECTION_OPEN_FN> mock_connection_open;
    static_mock<QUIC_CONNECTION_START_FN> mock_connection_start;

    ON_CALL(*mock_connection_open, Call(_, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC reg, QUIC_CONNECTION_CALLBACK_HANDLER handler,
                       void * context, HQUIC * conn) {
                (void) reg;
                (void) handler;
                (void) context;
                *conn = conn_object;
            }),
            Return(0)));

    ON_CALL(*mock_connection_start, Call(_, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC conn, HQUIC conf, QUIC_ADDRESS_FAMILY family,
                             const char * ServerName, uint16_t port) {
                      (void) conn;
                      (void) conf;
                      (void) family;
                      (void) ServerName;
                      (void) port;
                  }),
                  Return(0)));

    EXPECT_CALL(*mock_connection_open, Call(_, _, _, _)).Times(1);
    EXPECT_CALL(*mock_connection_start, Call(_, _, _, _, _)).Times(1);
    api.ConnectionOpen = mock_connection_open;
    api.ConnectionStart = mock_connection_start;

    auto result = f->connect("127.0.0.1", 1234);
    ASSERT_TRUE(result.has_value());
    auto r = f->disconnect();
    ASSERT_FALSE(r.has_value());
}

/******************************************************
 * Test the whole connection lifetime.
 ******************************************************/
TEST_F(tf_msquic_client, connect_and_disconnect) {
    auto f = construct_uut(mock_app);

    /******************************************************
     * Mock user callback functions
     ******************************************************/
    static_mock<void (*)(void *, mad::nexus::connection &)>
        mock_client_connected;

    int mock_client_connected_ctx = 0;
    int mock_client_disconnected_ctx = 0;
    static_mock<void (*)(void *, mad::nexus::connection &)>
        mock_client_disconnected;
    f->register_callback<callback_type::connected>(
        mock_client_connected.fn(),
        static_cast<void *>(&mock_client_connected_ctx));
    f->register_callback<callback_type::disconnected>(
        mock_client_disconnected.fn(),
        static_cast<void *>(&mock_client_disconnected_ctx));

    /******************************************************
     * Mock MSQUIC API functions
     ******************************************************/
    static_mock<QUIC_CONNECTION_OPEN_FN> mock_connection_open;
    static_mock<QUIC_CONNECTION_START_FN> mock_connection_start;
    static_mock<QUIC_CONNECTION_SHUTDOWN_FN> mock_connection_shutdown;
    static_mock<QUIC_CONNECTION_CLOSE_FN> mock_connection_close;

    /******************************************************
     * Storage for user-specified parameters over the MSQUIC
     * API calls
     ******************************************************/
    QUIC_CONNECTION_CALLBACK_HANDLER conn_callback_handler = { nullptr };
    void * ctx = { nullptr };

    /******************************************************
     * Mock connection_open function.
     *
     * It'll save the conn_callback_handler and user-provider
     * context to the their respective variables. They will be
     * used for delivering event-drivent results, like connection
     * and disconnection results.
     ******************************************************/
    ON_CALL(*mock_connection_open, Call(_, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC reg, QUIC_CONNECTION_CALLBACK_HANDLER handler,
                       void * context, HQUIC * conn) {
                (void) reg;
                (void) handler;
                (void) context;
                conn_callback_handler = handler;
                ctx = context;
                *conn = conn_object;
            }),
            Return(0)));

    /******************************************************
     * Mock connection_start function.
     *
     * It'll call the callback handler to indicate that the
     * connection attempt was successful.
     ******************************************************/
    ON_CALL(*mock_connection_start, Call(_, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC conn, HQUIC conf, QUIC_ADDRESS_FAMILY family,
                             const char * ServerName, uint16_t port) {
                      (void) conn;
                      (void) conf;
                      (void) family;
                      (void) ServerName;
                      (void) port;
                      ASSERT_NE(conn_callback_handler, nullptr);

                      QUIC_CONNECTION_EVENT evt{};
                      evt.Type = QUIC_CONNECTION_EVENT_CONNECTED;
                      evt.CONNECTED = {};
                      // Raise connection successful event
                      conn_callback_handler(conn, ctx, &evt);
                  }),
                  Return(0)));

    /******************************************************
     * Mock connection shutdown function.
     *
     * It'll call the callback handler to indicate that the
     * shutdown request was successful.
     ******************************************************/
    ON_CALL(*mock_connection_shutdown, Call(_, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC conn, QUIC_CONNECTION_SHUTDOWN_FLAGS flags,
                             QUIC_UINT62 error_code) {
                ASSERT_EQ(conn, conn_object);
                (void) flags;
                (void) error_code;
                QUIC_CONNECTION_EVENT evt{};
                evt.Type = QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE;
                evt.SHUTDOWN_COMPLETE = {};
                conn_callback_handler(conn, ctx, &evt);
            })));

    /******************************************************
     * Verify that user callbacks are called with right
     * data.
     ******************************************************/
    ON_CALL(*mock_client_connected, Call(_, _))
        .WillByDefault(
            DoAll(Invoke([&](void * uctx, mad::nexus::connection & conn) {
                ASSERT_EQ(&mock_client_connected_ctx, uctx);
                ASSERT_EQ(&conn, &connection_field(f));
            })));

    ON_CALL(*mock_client_disconnected, Call(_, _))
        .WillByDefault(
            DoAll(Invoke([&](void * uctx, mad::nexus::connection & conn) {
                ASSERT_EQ(&mock_client_disconnected_ctx, uctx);
                ASSERT_EQ(&conn, &connection_field(f));
            })));

    EXPECT_CALL(*mock_connection_open, Call(_, _, _, _)).Times(1);
    EXPECT_CALL(*mock_connection_start, Call(_, _, _, _, _)).Times(1);
    EXPECT_CALL(*mock_client_connected, Call(_, _)).Times(1);
    EXPECT_CALL(*mock_client_disconnected, Call(_, _)).Times(1);
    EXPECT_CALL(*mock_connection_shutdown, Call(_, _, _)).Times(1);
    EXPECT_CALL(*mock_connection_close, Call(_)).Times(1);

    /******************************************************
     * Override the default mock functions.
     ******************************************************/
    api.ConnectionOpen = mock_connection_open;
    api.ConnectionStart = mock_connection_start;
    api.ConnectionShutdown = mock_connection_shutdown;
    api.ConnectionClose = mock_connection_close;

    /******************************************************
     * Everything is set, get the machinery going.
     ******************************************************/
    auto result = f->connect("127.0.0.1", 1234);
    ASSERT_TRUE(result.has_value());
    auto r = f->disconnect();
    ASSERT_TRUE(r.has_value());
}

} // namespace mad::nexus
