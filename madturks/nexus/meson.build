nexus = declare_dependency(
    link_with: library(
        'nexus',
        [
            'src/quic_base.cpp',
            'src/quic_server.cpp',
            'src/quic_client.cpp',
            'src/quic_application.cpp',
            'src/quic.cpp',
            'src/msquic_base.cpp',
            'src/msquic_server.cpp',
            'src/msquic_client.cpp',
            'src/msquic_api_object.cpp',
            'src/msquic_application.cpp',
            'src/quic_error_code.cpp',
        ],
        include_directories: include_directories('inc'),
        install: true,
        dependencies: [
            madturks_core_macro,
            madturks_core_container,
            madturks_core_concurrency,
            madturks_core_log,
            msquic,
            flatbuffers,
            fmt,
        ],
    ),
    include_directories: include_directories('inc'),
    dependencies: [
        madturks_core_macro,
        madturks_core_concurrency,
        madturks_core_container,
        madturks_core_log,
    ],
)

# subdir('test/unit') # later
subdir('test/integration')