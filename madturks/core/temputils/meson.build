madturks_core_temputils = declare_dependency(
    link_with: library(
        'madturks.core.temputils',
        [
            'src/temputils.cpp',
        ],
        include_directories: include_directories('inc'),
        install: true,
        dependencies: [fmt, madturks_core_random],
    ),
    dependencies: [],
    include_directories: include_directories('inc'),
)