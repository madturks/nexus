/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/static_mock.hpp>

#include <msquic.h>

struct mock_msquic_api : public QUIC_API_TABLE {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsubobject-linkage"

    struct default_mock_functions {
        mad::static_mock<QUIC_SET_CONTEXT_FN> SetContext{};
        mad::static_mock<QUIC_GET_CONTEXT_FN> GetContext{};
        mad::static_mock<QUIC_SET_CALLBACK_HANDLER_FN> SetCallbackHandler{};
        mad::static_mock<QUIC_SET_PARAM_FN> SetParam{};
        mad::static_mock<QUIC_GET_PARAM_FN> GetParam{};
        mad::static_mock<QUIC_REGISTRATION_OPEN_FN> RegistrationOpen{};
        mad::static_mock<QUIC_REGISTRATION_CLOSE_FN> RegistrationClose{};
        mad::static_mock<QUIC_REGISTRATION_SHUTDOWN_FN> RegistrationShutdown{};
        mad::static_mock<QUIC_CONFIGURATION_OPEN_FN> ConfigurationOpen{};
        mad::static_mock<QUIC_CONFIGURATION_CLOSE_FN> ConfigurationClose{};
        mad::static_mock<QUIC_CONFIGURATION_LOAD_CREDENTIAL_FN>
            ConfigurationLoadCredential{};
        mad::static_mock<QUIC_LISTENER_OPEN_FN> ListenerOpen{};
        mad::static_mock<QUIC_LISTENER_CLOSE_FN> ListenerClose{};
        mad::static_mock<QUIC_LISTENER_START_FN> ListenerStart{};
        mad::static_mock<QUIC_LISTENER_STOP_FN> ListenerStop{};
        mad::static_mock<QUIC_CONNECTION_OPEN_FN> ConnectionOpen{};
        mad::static_mock<QUIC_CONNECTION_CLOSE_FN> ConnectionClose{};
        mad::static_mock<QUIC_CONNECTION_SHUTDOWN_FN> ConnectionShutdown{};
        mad::static_mock<QUIC_CONNECTION_START_FN> ConnectionStart{};
        mad::static_mock<QUIC_CONNECTION_SET_CONFIGURATION_FN>
            ConnectionSetConfiguration{};
        mad::static_mock<QUIC_CONNECTION_SEND_RESUMPTION_FN>
            ConnectionSendResumptionTicket{};
        mad::static_mock<QUIC_STREAM_OPEN_FN> StreamOpen{};
        mad::static_mock<QUIC_STREAM_CLOSE_FN> StreamClose{};
        mad::static_mock<QUIC_STREAM_START_FN> StreamStart{};
        mad::static_mock<QUIC_STREAM_SHUTDOWN_FN> StreamShutdown{};
        mad::static_mock<QUIC_STREAM_SEND_FN> StreamSend{};
        mad::static_mock<QUIC_STREAM_RECEIVE_COMPLETE_FN>
            StreamReceiveComplete{};
        mad::static_mock<QUIC_STREAM_RECEIVE_SET_ENABLED_FN>
            StreamReceiveSetEnabled{};
        mad::static_mock<QUIC_DATAGRAM_SEND_FN> DatagramSend{};
        mad::static_mock<QUIC_CONNECTION_COMP_RESUMPTION_FN>
            ConnectionResumptionTicketValidationComplete{};
        mad::static_mock<QUIC_CONNECTION_COMP_CERT_FN>
            ConnectionCertificateValidationComplete{};
    } default_mock{};

#pragma GCC diagnostic pop

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
