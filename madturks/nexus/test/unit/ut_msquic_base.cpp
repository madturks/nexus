/******************************************************
 * msquic_client unit tests
 *
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/msquic/msquic_base.hpp>
#include <mad/static_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <msquic.h>

#include "mock_msquic_application.hpp"
#include "mock_msquic_fns.hpp"

namespace mad::nexus {

struct tf_msquic_base : public ::testing::Test {
    mock_msquic_application mock_app = {};
    // Alias for convenience.
    QUIC_API_TABLE & api = mock_app.api_table;

    template <typename... Args>
    auto construct_uut(Args &&... args) {
        return std::unique_ptr<msquic_base>(
            new msquic_base(std::forward<Args>(args)...));
    }

    void SetUp() override {

        api.StreamOpen = mock_stream_open;
        api.StreamStart = mock_stream_start;
        api.StreamSend = mock_stream_send;
        api.SetContext = mock_set_context;
        api.StreamClose = mock_stream_close;

        uut = construct_uut(mock_app);

        uut->register_callback<callback_type::stream_start>(
            mock_stream_on_start.fn(),
            static_cast<void *>(&mock_stream_on_start_ctx));

        uut->register_callback<callback_type::stream_end>(
            mock_stream_on_close.fn(),
            static_cast<void *>(&mock_stream_on_close_ctx));
    }

    static inline auto conn_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xDEADC0DE);
    }();

    static inline auto strm_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xBAD1DEA);
    }();

    QUIC_STREAM_CALLBACK_HANDLER strm_callback_handler{ nullptr };
    void * ctxt{ nullptr };
    static_mock<QUIC_STREAM_OPEN_FN> mock_stream_open{};
    static_mock<QUIC_STREAM_START_FN> mock_stream_start{};
    static_mock<QUIC_STREAM_SEND_FN> mock_stream_send{};
    static_mock<QUIC_SET_CONTEXT_FN> mock_set_context{};
    static_mock<QUIC_STREAM_CLOSE_FN> mock_stream_close{};
    static_mock<void (*)(void *, struct stream &)> mock_stream_on_start{};
    static_mock<void (*)(void *, struct stream &)> mock_stream_on_close{};

    std::unique_ptr<msquic_base> uut{};
    int mock_stream_on_start_ctx = { 0 };
    int mock_stream_on_close_ctx = { 0 };
};

/******************************************************
 * Try to construct a msquic_client object.
 * It should succeed.
 ******************************************************/
TEST_F(tf_msquic_base, construct) {
    auto f = construct_uut(mock_app);
    ASSERT_NE(nullptr, f);
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_base, open_stream_success) {
    MockStreamOpenCall(QUIC_STATUS_SUCCESS, mock_stream_open, conn_object,
                       strm_object, strm_callback_handler, ctxt);
    MockStreamStartCall(QUIC_STATUS_SUCCESS, mock_stream_start, strm_object,
                        strm_callback_handler, ctxt);
    MockStreamCloseCall(QUIC_STATUS_SUCCESS, mock_stream_close, strm_object,
                        strm_callback_handler, ctxt);
    MockSetContextCall(mock_set_context, strm_object, ctxt);
    MockStreamOnStartCall(
        mock_stream_on_start, &mock_stream_on_start_ctx, strm_object);
    MockStreamOnCloseCall(
        mock_stream_on_close, &mock_stream_on_close_ctx, strm_object);

    connection mock_connection{ conn_object };
    auto result = uut->open_stream(mock_connection);
    ASSERT_TRUE(result.has_value());
    auto & stream = result.value().get();
    ASSERT_EQ(&stream.connection(), &mock_connection);
    ASSERT_EQ(stream.handle_as<HQUIC>(), strm_object);
    ASSERT_EQ(stream.callbacks.on_start.fn(), mock_stream_on_start.fn());
    ASSERT_EQ(stream.callbacks.on_close.fn(), mock_stream_on_close.fn());
    auto close_result = uut->close_stream(stream);
    ASSERT_TRUE(close_result.has_value());
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_base, close_stream_non_existent) {
    connection mock_connection{ conn_object };
    stream mock_stream{ strm_object, mock_connection, stream_callbacks{} };
    auto r = uut->close_stream(mock_stream);
    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(r.error(), quic_error_code::value_does_not_exists);
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_base, open_stream_stream_open_fail) {
    MockStreamOpenCall(QUIC_STATUS_ABORTED, mock_stream_open, conn_object,
                       strm_object, strm_callback_handler, ctxt);
    api.StreamOpen = mock_stream_open;

    connection mock_connection{ conn_object };
    auto result = uut->open_stream(mock_connection);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), quic_error_code::stream_open_failed);
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_base, open_stream_start_fail) {

    MockStreamOpenCall(QUIC_STATUS_SUCCESS, mock_stream_open, conn_object,
                       strm_object, strm_callback_handler, ctxt);
    MockStreamStartCall(QUIC_STATUS_ABORTED, mock_stream_start, strm_object,
                        strm_callback_handler, ctxt);
    MockStreamCloseCall(QUIC_STATUS_SUCCESS, mock_stream_close, strm_object,
                        strm_callback_handler, ctxt);
    MockSetContextCall(mock_set_context, strm_object, ctxt);
    MockStreamOnCloseCall(
        mock_stream_on_close, &mock_stream_on_close_ctx, strm_object);

    connection mock_connection{ conn_object };
    auto result = uut->open_stream(mock_connection);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), quic_error_code::stream_start_failed);
}

/******************************************************
 ******************************************************/
TEST_F(tf_msquic_base, send_success) {

    MockStreamOpenCall(QUIC_STATUS_SUCCESS, mock_stream_open, conn_object,
                       strm_object, strm_callback_handler, ctxt);
    MockStreamCloseCall(QUIC_STATUS_SUCCESS, mock_stream_close, strm_object,
                        strm_callback_handler, ctxt);
    MockStreamStartCall(QUIC_STATUS_SUCCESS, mock_stream_start, strm_object,
                        strm_callback_handler, ctxt);

    MockSetContextCall(mock_set_context, strm_object, ctxt);
    MockStreamOnStartCall(
        mock_stream_on_start, &mock_stream_on_start_ctx, strm_object);
    MockStreamOnCloseCall(
        mock_stream_on_close, &mock_stream_on_close_ctx, strm_object);
    MockStreamSendCall(QUIC_STATUS_SUCCESS, mock_stream_send, strm_object,
                       strm_callback_handler, ctxt);

    auto alloc_buf = new std::uint8_t [1024];

    std::uint32_t encoded_size = 16;
    send_buffer<true> buf;
    buf.buf = alloc_buf;
    buf.buf_size = 1024;
    buf.offset = 1024 - sizeof(send_buffer<true>::quic_buf_sentinel) -
                 sizeof(std::uint32_t) - encoded_size;

    std::memcpy(
        (alloc_buf + 1024) - sizeof(send_buffer<true>::quic_buf_sentinel),
        send_buffer<true>::quic_buf_sentinel,
        sizeof(send_buffer<true>::quic_buf_sentinel));

    std::memcpy((alloc_buf + 1024) -
                    sizeof(send_buffer<true>::quic_buf_sentinel) -
                    encoded_size - sizeof(std::uint32_t),
                &encoded_size, sizeof(std::uint32_t));

    connection mock_connection{ conn_object };
    // stream mock_stream{ strm_object, mock_connection, stream_callbacks{} };
    auto stream_open_result = uut->open_stream(mock_connection);
    ASSERT_TRUE(stream_open_result.has_value());
    auto result = uut->send(stream_open_result.value().get(), std::move(buf));
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), encoded_size + sizeof(std::uint32_t));
}

/******************************************************
******************************************************/
TEST_F(tf_msquic_base, send_failed) {

    MockStreamOpenCall(QUIC_STATUS_SUCCESS, mock_stream_open, conn_object,
                       strm_object, strm_callback_handler, ctxt);
    MockStreamCloseCall(QUIC_STATUS_SUCCESS, mock_stream_close, strm_object,
                        strm_callback_handler, ctxt);
    MockStreamStartCall(QUIC_STATUS_SUCCESS, mock_stream_start, strm_object,
                        strm_callback_handler, ctxt);

    MockSetContextCall(mock_set_context, strm_object, ctxt);
    MockStreamOnStartCall(
        mock_stream_on_start, &mock_stream_on_start_ctx, strm_object);
    MockStreamOnCloseCall(
        mock_stream_on_close, &mock_stream_on_close_ctx, strm_object);
    MockStreamSendCall(QUIC_STATUS_ABORTED, mock_stream_send, strm_object,
                       strm_callback_handler, ctxt);

    auto alloc_buf = new std::uint8_t [1024];

    std::uint32_t encoded_size = 16;
    send_buffer<true> buf;
    buf.buf = alloc_buf;
    buf.buf_size = 1024;
    buf.offset = 1024 - sizeof(send_buffer<true>::quic_buf_sentinel) -
                 sizeof(std::uint32_t) - encoded_size;

    std::memcpy(
        (alloc_buf + 1024) - sizeof(send_buffer<true>::quic_buf_sentinel),
        send_buffer<true>::quic_buf_sentinel,
        sizeof(send_buffer<true>::quic_buf_sentinel));

    std::memcpy((alloc_buf + 1024) -
                    sizeof(send_buffer<true>::quic_buf_sentinel) -
                    encoded_size - sizeof(std::uint32_t),
                &encoded_size, sizeof(std::uint32_t));

    connection mock_connection{ conn_object };
    // stream mock_stream{ strm_object, mock_connection, stream_callbacks{} };
    auto stream_open_result = uut->open_stream(mock_connection);
    ASSERT_TRUE(stream_open_result.has_value());
    auto result = uut->send(stream_open_result.value().get(), std::move(buf));
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), quic_error_code::send_failed);
}

} // namespace mad::nexus
