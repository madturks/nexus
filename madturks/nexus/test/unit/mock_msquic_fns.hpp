/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/macro>
#include <mad/nexus/quic_stream.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <msquic.h>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;

template <typename MockType>
MAD_ALWAYS_INLINE void
MockStreamOpenCall(QUIC_STATUS ret, MockType & mock_stream_open,
                   HQUIC conn_object, HQUIC strm_object,
                   QUIC_STREAM_CALLBACK_HANDLER & strm_callback_handler,
                   void *& ctxt) {
    ON_CALL(*mock_stream_open, Call(_, _, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC conn, QUIC_STREAM_OPEN_FLAGS flags,
                       QUIC_STREAM_CALLBACK_HANDLER stream_callback_handler,
                       void * context, HQUIC * out_stream) {
                ASSERT_EQ(conn, conn_object);
                (void) flags;
                ASSERT_NE(nullptr, stream_callback_handler);
                ASSERT_EQ(nullptr, context);
                ASSERT_NE(nullptr, out_stream);
                if (ret == QUIC_STATUS_SUCCESS) {
                    *out_stream = strm_object;
                    strm_callback_handler = stream_callback_handler;
                    ctxt = context;
                }
            }),
            Return(ret)));

    EXPECT_CALL(*mock_stream_open, Call(_, _, _, _, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockStreamCloseCall(
    QUIC_STATUS ret, MockType & mock_stream_close, HQUIC strm_object,
    QUIC_STREAM_CALLBACK_HANDLER & strm_callback_handler, void *& ctxt) {
    ON_CALL(*mock_stream_close, Call(_))
        .WillByDefault(DoAll(Invoke([&](HQUIC strm) {
            ASSERT_EQ(strm, strm_object);
            if (ret == QUIC_STATUS_SUCCESS) {
                QUIC_STREAM_EVENT evt{};
                evt.Type = QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE;
                evt.SHUTDOWN_COMPLETE = {};
                evt.SHUTDOWN_COMPLETE.AppCloseInProgress = { true };
                strm_callback_handler(strm, ctxt, &evt);
            }
        })));

    EXPECT_CALL(*mock_stream_close, Call(_)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockStreamStartCall(
    QUIC_STATUS ret, MockType & mock_stream_start, HQUIC strm_object,
    QUIC_STREAM_CALLBACK_HANDLER & strm_callback_handler, void *& ctxt) {
    ON_CALL(*mock_stream_start, Call(_, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC strm, QUIC_STREAM_START_FLAGS flags) {
                      ASSERT_EQ(strm, strm_object);
                      (void) flags;
                      ASSERT_NE(nullptr, strm_callback_handler);
                      ASSERT_NE(nullptr, ctxt);
                      if (ret == QUIC_STATUS_SUCCESS) {
                          QUIC_STREAM_EVENT evt{};
                          evt.Type = QUIC_STREAM_EVENT_START_COMPLETE;
                          evt.START_COMPLETE = {};
                          evt.START_COMPLETE.Status = QUIC_STATUS_SUCCESS;
                          strm_callback_handler(strm, ctxt, &evt);
                      }
                  }),
                  Return(ret)));

    EXPECT_CALL(*mock_stream_start, Call(_, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockSetContextCall(
    MockType & mock_set_context, // Replace `MockType` with the actual type of
    HQUIC strm_object,
    void *& ctxt) // Pass `ctxt` by reference to modify it within the function
{
    ON_CALL(*mock_set_context, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](HQUIC strm, void * context) {
            ASSERT_EQ(strm, strm_object);
            ASSERT_NE(nullptr, context);
            ctxt = context;
        })));

    EXPECT_CALL(*mock_set_context, Call(_, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockStreamOnStartCall(MockType & mock_stream_on_start,
                                             void * mock_stream_on_start_ctx,
                                             HQUIC strm_object) {
    ON_CALL(*mock_stream_on_start, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](void * context,
                                        mad::nexus::stream & strm) {
            ASSERT_EQ(context, static_cast<void *>(mock_stream_on_start_ctx));
            ASSERT_EQ(strm.handle_as<HQUIC>(), strm_object);
        })));

    EXPECT_CALL(*mock_stream_on_start, Call(_, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockStreamOnCloseCall(MockType & mock_stream_on_close,
                                             void * mock_stream_on_close_ctx,
                                             HQUIC strm_object) {
    ON_CALL(*mock_stream_on_close, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](void * context,
                                        mad::nexus::stream & strm) {
            ASSERT_EQ(context, static_cast<void *>(mock_stream_on_close_ctx));
            ASSERT_EQ(strm.handle_as<HQUIC>(), strm_object);
        })));

    EXPECT_CALL(*mock_stream_on_close, Call(_, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockStreamSendCall(
    QUIC_STATUS ret, MockType & mock_stream_send, HQUIC strm_object,
    QUIC_STREAM_CALLBACK_HANDLER & strm_callback_handler, void *& ctxt) {
    ON_CALL(*mock_stream_send, Call(_, _, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC strm, const QUIC_BUFFER * qbuf, uint32_t bufcnt,
                       QUIC_SEND_FLAGS flags, void * context) {
                ASSERT_EQ(flags, QUIC_SEND_FLAG_NONE);
                ASSERT_EQ(strm, strm_object);
                ASSERT_EQ(bufcnt, 1);
                ASSERT_EQ(qbuf->Length, 20);
                ASSERT_NE(nullptr, context);

                if (ret == QUIC_STATUS_SUCCESS) {
                    QUIC_STREAM_EVENT evt{};
                    evt.Type = QUIC_STREAM_EVENT_SEND_COMPLETE;
                    evt.SEND_COMPLETE = {};
                    evt.SEND_COMPLETE.ClientContext = context;
                    strm_callback_handler(strm, ctxt, &evt);
                }
            }),
            Return(ret)));

    EXPECT_CALL(*mock_stream_send, Call(_, _, _, _, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void
MockListenerOpenCall(QUIC_STATUS ret, MockType & mock_listener_open,
                     HQUIC registration_object, HQUIC listener_object,
                     QUIC_LISTENER_CALLBACK_HANDLER & listener_callback_handler,
                     void *& ctxt) {
    ON_CALL(*mock_listener_open, Call(_, _, _, _))
        .WillByDefault(DoAll(Invoke([&](HQUIC registration,
                                        QUIC_LISTENER_CALLBACK_HANDLER handler,
                                        void * context, HQUIC * out_listener) {
                                 ASSERT_EQ(registration, registration_object);
                                 ASSERT_NE(nullptr, handler);
                                 ASSERT_NE(nullptr, context);
                                 ASSERT_NE(nullptr, out_listener);
                                 if (ret == QUIC_STATUS_SUCCESS) {
                                     *out_listener = listener_object;
                                     listener_callback_handler = handler;
                                     ctxt = context;
                                 }
                             }),
                             Return(ret)));

    EXPECT_CALL(*mock_listener_open, Call(_, _, _, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void MockListenerStartCall(
    QUIC_STATUS ret, MockType & mock_listener_start, HQUIC listener_object,
    QUIC_LISTENER_CALLBACK_HANDLER & listener_callback_handler,
    std::span<const QUIC_BUFFER> truth_alpn, const QUIC_ADDR * truth_local_addr,
    void *& ctxt) {
    ON_CALL(*mock_listener_start, Call(_, _, _, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC listener, const QUIC_BUFFER * alpn_buffers,
                       uint32_t alpn_buffer_count,
                       const QUIC_ADDR * local_address) {
                // ASSERT_EQ(alpn_buffers, truth_alpn.data());
                ASSERT_EQ(alpn_buffer_count, truth_alpn.size());

                for (auto i = 0zu; i < alpn_buffer_count; i++) {
                    ASSERT_EQ(alpn_buffers [i].Length, truth_alpn [i].Length);
                    ASSERT_EQ(0, std::memcmp(alpn_buffers [i].Buffer,
                                             truth_alpn [i].Buffer,
                                             alpn_buffers [i].Length));
                }

                ASSERT_EQ(local_address->Ip.sa_family,
                          truth_local_addr->Ip.sa_family);
                ASSERT_EQ(local_address->Ipv6.sin6_port,
                          truth_local_addr->Ipv6.sin6_port);
                // ASSERT_EQ(local_address, truth_local_addr);
                ASSERT_EQ(listener, listener_object);
                ASSERT_NE(nullptr, listener_callback_handler);
                ASSERT_NE(nullptr, ctxt);
            }),
            Return(ret)));

    EXPECT_CALL(*mock_listener_start, Call(_, _, _, _)).Times(1);
}

template <typename MockType>
MAD_ALWAYS_INLINE void
MockListenerCloseCall(QUIC_STATUS ret, MockType & mock_listener_close,
                      HQUIC listener_object,
                      std::reference_wrapper<QUIC_LISTENER_CALLBACK_HANDLER>
                          listener_callback_handler,
                      std::reference_wrapper<void *> ctxt) {
    ON_CALL(*mock_listener_close, Call(_))
        .WillByDefault(DoAll(Invoke([=](HQUIC listener) {
            ASSERT_TRUE(listener == listener_object);
            ASSERT_NE(listener_callback_handler.get(), nullptr);
            if (ret == QUIC_STATUS_SUCCESS) {
                QUIC_LISTENER_EVENT evt{};
                evt.Type = QUIC_LISTENER_EVENT_STOP_COMPLETE;
                evt.STOP_COMPLETE = {};
                evt.STOP_COMPLETE.AppCloseInProgress = { true };
                listener_callback_handler(listener, ctxt.get(), &evt);
            }
        })));

    EXPECT_CALL(*mock_listener_close, Call(_)).Times(1);
}
