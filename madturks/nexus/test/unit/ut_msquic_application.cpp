/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/nexus/msquic/msquic_application.hpp>
#include <mad/nexus/quic.hpp>
#include <mad/nexus/quic_configuration.hpp>
#include <mad/static_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <msquic.h>

#include <memory>

namespace mad::nexus {

struct mock_msquic_api : public QUIC_API_TABLE {
    struct default_mock_functions {
        static_mock<QUIC_SET_CONTEXT_FN> SetContext;
        static_mock<QUIC_GET_CONTEXT_FN> GetContext;
        static_mock<QUIC_SET_CALLBACK_HANDLER_FN> SetCallbackHandler;
        static_mock<QUIC_SET_PARAM_FN> SetParam;
        static_mock<QUIC_GET_PARAM_FN> GetParam;
        static_mock<QUIC_REGISTRATION_OPEN_FN> RegistrationOpen;
        static_mock<QUIC_REGISTRATION_CLOSE_FN> RegistrationClose;
        static_mock<QUIC_REGISTRATION_SHUTDOWN_FN> RegistrationShutdown;
        static_mock<QUIC_CONFIGURATION_OPEN_FN> ConfigurationOpen;
        static_mock<QUIC_CONFIGURATION_CLOSE_FN> ConfigurationClose;
        static_mock<QUIC_CONFIGURATION_LOAD_CREDENTIAL_FN>
            ConfigurationLoadCredential;
        static_mock<QUIC_LISTENER_OPEN_FN> ListenerOpen;
        static_mock<QUIC_LISTENER_CLOSE_FN> ListenerClose;
        static_mock<QUIC_LISTENER_START_FN> ListenerStart;
        static_mock<QUIC_LISTENER_STOP_FN> ListenerStop;
        static_mock<QUIC_CONNECTION_OPEN_FN> ConnectionOpen;
        static_mock<QUIC_CONNECTION_CLOSE_FN> ConnectionClose;
        static_mock<QUIC_CONNECTION_SHUTDOWN_FN> ConnectionShutdown;
        static_mock<QUIC_CONNECTION_START_FN> ConnectionStart;
        static_mock<QUIC_CONNECTION_SET_CONFIGURATION_FN>
            ConnectionSetConfiguration;
        static_mock<QUIC_CONNECTION_SEND_RESUMPTION_FN>
            ConnectionSendResumptionTicket;
        static_mock<QUIC_STREAM_OPEN_FN> StreamOpen;
        static_mock<QUIC_STREAM_CLOSE_FN> StreamClose;
        static_mock<QUIC_STREAM_START_FN> StreamStart;
        static_mock<QUIC_STREAM_SHUTDOWN_FN> StreamShutdown;
        static_mock<QUIC_STREAM_SEND_FN> StreamSend;
        static_mock<QUIC_STREAM_RECEIVE_COMPLETE_FN> StreamReceiveComplete;
        static_mock<QUIC_STREAM_RECEIVE_SET_ENABLED_FN> StreamReceiveSetEnabled;
        static_mock<QUIC_DATAGRAM_SEND_FN> DatagramSend;
        static_mock<QUIC_CONNECTION_COMP_RESUMPTION_FN>
            ConnectionResumptionTicketValidationComplete;
        static_mock<QUIC_CONNECTION_COMP_CERT_FN>
            ConnectionCertificateValidationComplete;
    } default_mock;

    mock_msquic_api() {
        SetContext = default_mock.SetContext;
        GetContext = default_mock.GetContext;
        SetCallbackHandler = default_mock.SetCallbackHandler;
        SetParam = default_mock.SetParam;
        GetParam = default_mock.GetParam;
        RegistrationOpen = default_mock.RegistrationOpen;
        RegistrationClose = default_mock.RegistrationClose;
        RegistrationShutdown = default_mock.RegistrationShutdown;
        ConfigurationOpen = default_mock.ConfigurationOpen;
        ConfigurationClose = default_mock.ConfigurationClose;
        ConfigurationLoadCredential = default_mock.ConfigurationLoadCredential;
        ListenerOpen = default_mock.ListenerOpen;
        ListenerClose = default_mock.ListenerClose;
        ListenerStart = default_mock.ListenerStart;
        ListenerStop = default_mock.ListenerStop;
        ConnectionOpen = default_mock.ConnectionOpen;
        ConnectionClose = default_mock.ConnectionClose;
        ConnectionShutdown = default_mock.ConnectionShutdown;
        ConnectionStart = default_mock.ConnectionStart;
        ConnectionSetConfiguration = default_mock.ConnectionSetConfiguration;
        ConnectionSendResumptionTicket =
            default_mock.ConnectionSendResumptionTicket;
        StreamOpen = default_mock.StreamOpen;
        StreamClose = default_mock.StreamClose;
        StreamStart = default_mock.StreamStart;
        StreamShutdown = default_mock.StreamShutdown;
        StreamSend = default_mock.StreamSend;
        StreamReceiveComplete = default_mock.StreamReceiveComplete;
        StreamReceiveSetEnabled = default_mock.StreamReceiveSetEnabled;
        DatagramSend = default_mock.DatagramSend;
        ConnectionResumptionTicketValidationComplete =
            default_mock.ConnectionResumptionTicketValidationComplete;
        ConnectionCertificateValidationComplete =
            default_mock.ConnectionCertificateValidationComplete;
    }
};

struct tf_msquic_application : public ::testing::Test {

    void SetUp() override {

        mock_api_table = std::shared_ptr<const QUIC_API_TABLE>(
            &api, [](const QUIC_API_TABLE *) {
            });

        // Patch the MSQUIC api with the mock one.
        msquic_api = mock_api_table;

        mock_registration = std::shared_ptr<QUIC_HANDLE>(
            reg_object, [](QUIC_HANDLE *) {
            });

        mock_configuration = std::shared_ptr<QUIC_HANDLE>(
            cfg_object, [](QUIC_HANDLE *) {
            });
    }

    virtual void TearDown() override {
        stub_mocks.clear();
    }

    std::vector<std::shared_ptr<void>> stub_mocks;

    template <typename... Args>
    auto construct_uut(Args &&... args) {
        return std::unique_ptr<msquic_application>(
            new msquic_application(std::forward<Args>(args)...));
    }

    mock_msquic_api api = {};

    // Mock addresses for the registration and configuration
    // objects.

    static inline auto reg_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xDEADBEEF);
    }();

    static inline auto cfg_object = []() {
        return reinterpret_cast<QUIC_HANDLE *>(0xBADCAFE);
    }();

    std::shared_ptr<QUIC_HANDLE> mock_registration;
    std::shared_ptr<QUIC_HANDLE> mock_configuration;
    std::shared_ptr<const QUIC_API_TABLE> mock_api_table;
    std::unique_ptr<quic_application> app;
};

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;

/******************************************************
 * Test the factory method.
 *
 * The factory method uses MSQUIC API calls to initialize
 * the msquic_application object.
 ******************************************************/
TEST_F(tf_msquic_application, factory) {
    quic_configuration config{ e_quic_impl_type::msquic, e_role::client };

    static_mock<QUIC_REGISTRATION_OPEN_FN> mock_registration_open;
    static_mock<QUIC_REGISTRATION_CLOSE_FN> mock_registration_close;
    static_mock<QUIC_CONFIGURATION_OPEN_FN> mock_configuration_open;
    static_mock<QUIC_CONFIGURATION_CLOSE_FN> mock_configuration_close;
    static_mock<QUIC_CONFIGURATION_LOAD_CREDENTIAL_FN>
        mock_configuration_load_credential;

    api.RegistrationOpen = mock_registration_open;
    api.RegistrationClose = mock_registration_close;
    api.ConfigurationOpen = mock_configuration_open;
    api.ConfigurationClose = mock_configuration_close;
    api.ConfigurationLoadCredential = mock_configuration_load_credential;

    ON_CALL(*mock_registration_open, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](const QUIC_REGISTRATION_CONFIG * cfg,
                                        QUIC_HANDLE ** reg) {
                                 (void) cfg;
                                 // Set registration handle to the mock
                                 // registration object.
                                 *reg = reg_object;
                             }),
                             Return(0)));

    ON_CALL(*mock_registration_close, Call(_))
        .WillByDefault(Invoke([&](QUIC_HANDLE * hnd) {
            // Verify that the given handle is the handle set by the
            // mock registration open function.
            ASSERT_EQ(hnd, reg_object);
        }));

    ON_CALL(*mock_configuration_open, Call(_, _, _, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC reg, const QUIC_BUFFER * const alpn,
                             std::uint32_t alpn_buf_count,
                             [[maybe_unused]] const QUIC_SETTINGS * settings,
                             std::uint32_t settings_size, void * ctx,
                             HQUIC * cfgout) {
                      ASSERT_EQ(reg, reg_object);
                      ASSERT_EQ(alpn->Length, config.alpn.length());
                      ASSERT_EQ(alpn_buf_count, 1);
                      ASSERT_EQ(settings_size, sizeof(QUIC_SETTINGS));
                      ASSERT_EQ(ctx, nullptr);
                      ASSERT_TRUE(std::memcmp(config.alpn.data(), alpn->Buffer,
                                              config.alpn.size()) == 0);

                      *cfgout = cfg_object;
                  }),

                  Return(0)));

    ON_CALL(*mock_configuration_load_credential, Call(_, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC cfg, const QUIC_CREDENTIAL_CONFIG * cred_cfg) {
                ASSERT_EQ(cfg, cfg_object);
                ASSERT_NE(nullptr, cred_cfg);
                ASSERT_EQ(cred_cfg->Type, QUIC_CREDENTIAL_TYPE_NONE);
                ASSERT_EQ(cred_cfg->Flags,
                          QUIC_CREDENTIAL_FLAG_CLIENT |
                              QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION);
                ASSERT_EQ(cred_cfg->CertificateFile, nullptr);
            }),
            Return(0)));

    ON_CALL(*mock_configuration_close, Call(_))
        .WillByDefault(Invoke([&](QUIC_HANDLE * hnd) {
            // Verify that the given handle is the handle set by the
            // mock configuration open function.
            ASSERT_EQ(hnd, cfg_object);
        }));

    EXPECT_CALL(*mock_registration_open, Call(_, _)).Times(1);
    EXPECT_CALL(*mock_registration_close, Call(_)).Times(1);
    EXPECT_CALL(*mock_configuration_open, Call(_, _, _, _, _, _, _)).Times(1);
    EXPECT_CALL(*mock_configuration_close, Call(_)).Times(1);
    EXPECT_CALL(*mock_configuration_load_credential, Call(_, _)).Times(1);
    auto f = make_quic_application(config);
    ASSERT_TRUE(f.has_value());
}

TEST_F(tf_msquic_application, factory_registration_fail) {
    quic_configuration config{ e_quic_impl_type::msquic, e_role::client };

    static_mock<QUIC_REGISTRATION_OPEN_FN> mock_registration_open;

    ON_CALL(*mock_registration_open, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](const QUIC_REGISTRATION_CONFIG * cfg,
                                        QUIC_HANDLE ** reg) {
                                 (void) cfg;
                                 (void) reg;
                             }),
                             Return(1)));

    EXPECT_CALL(*mock_registration_open, Call(_, _)).Times(1);

    api.RegistrationOpen = mock_registration_open;

    auto f = make_quic_application(config);
    ASSERT_FALSE(f.has_value());
    ASSERT_EQ(f.error(), quic_error_code::registration_initialization_failed);
}

TEST_F(tf_msquic_application, factory_configuration_fail) {
    quic_configuration config{ e_quic_impl_type::msquic, e_role::client };

    static_mock<QUIC_REGISTRATION_OPEN_FN> mock_registration_open;
    static_mock<QUIC_REGISTRATION_CLOSE_FN> mock_registration_close;
    static_mock<QUIC_CONFIGURATION_OPEN_FN> mock_configuration_open;

    ON_CALL(*mock_registration_open, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](const QUIC_REGISTRATION_CONFIG * cfg,
                                        QUIC_HANDLE ** reg) {
                                 (void) cfg;
                                 *reg = reg_object;
                             }),
                             Return(0)));

    ON_CALL(*mock_registration_close, Call(_))
        .WillByDefault(Invoke([&](QUIC_HANDLE * hnd) {
            // Verify that the given handle is the handle set by the
            // mock registration open function.
            ASSERT_EQ(hnd, reg_object);
        }));

    ON_CALL(*mock_configuration_open, Call(_, _, _, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC reg, const QUIC_BUFFER * const alpn,
                             std::uint32_t alpn_buf_count,
                             [[maybe_unused]] const QUIC_SETTINGS * settings,
                             std::uint32_t settings_size, void * ctx,
                             HQUIC * cfgout) {
                      (void) reg;
                      (void) alpn;
                      (void) alpn_buf_count;
                      (void) settings;
                      (void) settings_size;
                      (void) ctx;
                      (void) cfgout;
                  }),

                  Return(1)));

    EXPECT_CALL(*mock_registration_open, Call(_, _)).Times(1);
    EXPECT_CALL(*mock_registration_close, Call(_)).Times(1);
    EXPECT_CALL(*mock_configuration_open, Call(_, _, _, _, _, _, _)).Times(1);

    api.RegistrationOpen = mock_registration_open;
    api.RegistrationClose = mock_registration_close;
    api.ConfigurationOpen = mock_configuration_open;

    auto f = make_quic_application(config);
    ASSERT_FALSE(f.has_value());
    ASSERT_EQ(f.error(), quic_error_code::configuration_initialization_failed);
}

TEST_F(tf_msquic_application, factory_configuration_credential_fail) {
    quic_configuration config{ e_quic_impl_type::msquic, e_role::client };

    static_mock<QUIC_REGISTRATION_OPEN_FN> mock_registration_open;
    static_mock<QUIC_REGISTRATION_CLOSE_FN> mock_registration_close;
    static_mock<QUIC_CONFIGURATION_OPEN_FN> mock_configuration_open;
    static_mock<QUIC_CONFIGURATION_CLOSE_FN> mock_configuration_close;
    static_mock<QUIC_CONFIGURATION_LOAD_CREDENTIAL_FN>
        mock_configuration_load_credential;

    ON_CALL(*mock_registration_open, Call(_, _))
        .WillByDefault(DoAll(Invoke([&](const QUIC_REGISTRATION_CONFIG * cfg,
                                        QUIC_HANDLE ** reg) {
                                 (void) cfg;
                                 *reg = reg_object;
                             }),
                             Return(0)));

    ON_CALL(*mock_registration_close, Call(_))
        .WillByDefault(Invoke([&](QUIC_HANDLE * hnd) {
            // Verify that the given handle is the handle set by the
            // mock registration open function.
            ASSERT_EQ(hnd, reg_object);
        }));

    ON_CALL(*mock_configuration_open, Call(_, _, _, _, _, _, _))
        .WillByDefault(
            DoAll(Invoke([&](HQUIC reg, const QUIC_BUFFER * const alpn,
                             std::uint32_t alpn_buf_count,
                             [[maybe_unused]] const QUIC_SETTINGS * settings,
                             std::uint32_t settings_size, void * ctx,
                             HQUIC * cfgout) {
                      (void) reg;
                      (void) alpn;
                      (void) alpn_buf_count;
                      (void) settings;
                      (void) settings_size;
                      (void) ctx;
                      *cfgout = cfg_object;
                  }),

                  Return(0)));

    ON_CALL(*mock_configuration_close, Call(_))
        .WillByDefault(Invoke([&](QUIC_HANDLE * hnd) {
            // Verify that the given handle is the handle set by the
            // mock configuration open function.
            ASSERT_EQ(hnd, cfg_object);
        }));

    ON_CALL(*mock_configuration_load_credential, Call(_, _))
        .WillByDefault(DoAll(
            Invoke([&](HQUIC cfg, const QUIC_CREDENTIAL_CONFIG * cred_cfg) {
                (void) cfg;
                (void) cred_cfg;
            }),
            Return(1)));

    EXPECT_CALL(*mock_registration_open, Call(_, _)).Times(1);
    EXPECT_CALL(*mock_registration_close, Call(_)).Times(1);
    EXPECT_CALL(*mock_configuration_open, Call(_, _, _, _, _, _, _)).Times(1);
    EXPECT_CALL(*mock_configuration_close, Call(_)).Times(1);
    EXPECT_CALL(*mock_configuration_load_credential, Call(_, _)).Times(1);

    api.RegistrationOpen = mock_registration_open;
    api.RegistrationClose = mock_registration_close;
    api.ConfigurationOpen = mock_configuration_open;
    api.ConfigurationClose = mock_configuration_close;
    api.ConfigurationLoadCredential = mock_configuration_load_credential;

    auto f = make_quic_application(config);
    ASSERT_FALSE(f.has_value());
    ASSERT_EQ(f.error(), quic_error_code::configuration_load_credential_failed);
}

TEST_F(tf_msquic_application, construct) {
    auto appl = construct_uut(
        mock_api_table, mock_registration, mock_configuration);

    EXPECT_EQ(appl->configuration(), mock_configuration.get());
    EXPECT_EQ(appl->registration(), mock_registration.get());
    EXPECT_EQ(appl->api(), mock_api_table.get());
}

TEST_F(tf_msquic_application, make_client) {
    auto appl = construct_uut(
        mock_api_table, mock_registration, mock_configuration);

    auto result = appl->make_client();
    ASSERT_TRUE(result.has_value());
    auto client = std::move(result.value());
    ASSERT_NE(nullptr, client);
}

TEST_F(tf_msquic_application, make_server) {
    auto appl = construct_uut(
        mock_api_table, mock_registration, mock_configuration);

    auto result = appl->make_server();
    ASSERT_TRUE(result.has_value());
    auto server = std::move(result.value());
    ASSERT_NE(nullptr, server);
}

} // namespace mad::nexus
