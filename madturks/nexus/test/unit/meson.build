ut_msquic_application = executable(
    'ut_msquic_appllication',
    'ut_msquic_application.cpp',
    dependencies: [
        nexus,
        gtest,
        gmock,
        msquic,
        flatbuffers,
        madturks_core_testutils
    ],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables', '-Wno-noexcept'],
)

test(
    'msquic_application unit tests',
    ut_msquic_application,
)


ut_msquic_client = executable(
    'ut_msquic_client',
    'ut_msquic_client.cpp',
    dependencies: [
        nexus,
        gtest,
        gmock,
        msquic,
        flatbuffers,
        madturks_core_testutils
    ],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables', '-Wno-noexcept'],
)

test(
    'msquic_client unit tests',
    ut_msquic_client,
)