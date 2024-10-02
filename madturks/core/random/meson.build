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

madturks_core_random_string_ut = executable(
    'ut-madturks-core-random-string',
    'tests/unit/ut_random_string.cpp',
    dependencies: [madturks_core_random, gtest, gmock],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables'],
)

madturks_core_random_ut = executable(
    'ut-madturks-core-random',
    'tests/unit/ut_random.cpp',
    dependencies: [madturks_core_random, gtest, gmock],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables'],
)

test('Random string unit tests', madturks_core_random_string_ut)
test('Random unit tests', madturks_core_random_ut)