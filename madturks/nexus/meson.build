nexus = declare_dependency(
    link_with: library(
        'nexus',
        [
            'src/quic.cpp',
            'src/msquic.cpp',
            'src/quic_error_code_category.cpp',
        ],
        include_directories: include_directories('include'),
        install: true,
        dependencies: [
            madturks_core_container,
            madturks_core_concurrency,
            msquic,
            flatbuffers,
            fmt,
        ],
    ),
    include_directories: include_directories('include'),
)