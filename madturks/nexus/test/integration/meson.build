nexus_stream_callback_integration_tests = executable(
    'it-madturks-nexus-stream-callback',
    'it_stream_callback_receive.cpp',
    #'it_stream_callback_send_complete.cpp',
    dependencies: [
        nexus,
        madturks_core_temputils,
        madturks_core_random,
        gtest,
        gmock,
        msquic,
        flatbuffers,
        fbs_schemas,
    ],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables'],
)


test(
    'Nexus stream callback integration tests',
    nexus_stream_callback_integration_tests,
)
