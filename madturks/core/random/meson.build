madturks_core_random = declare_dependency(
    link_with: library(
        'madturks.core.random',
        [
            'src/random_string.cpp',
            'src/random.cpp',
        ],
        include_directories: include_directories('inc'),
        install: true,
        dependencies: [madturks_core_concept],
    ),
    dependencies: [madturks_core_concept],
    include_directories: include_directories('inc'),
)