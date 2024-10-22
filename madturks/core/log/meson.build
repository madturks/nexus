spdlog = dependency('spdlog', include_type: 'system')
spdlog_setup = dependency('gegles-spdlog_setup', include_type: 'system')

madturks_core_log = declare_dependency(
    link_with: library(
        'madturks.core.log',
        [
            'src/log_printer.cpp',
        ],
        include_directories: include_directories('inc'),
        install: true,
        dependencies: [madturks_core_meta, fmt, spdlog, spdlog_setup],
    ),
    include_directories: include_directories('inc'),
    dependencies: [madturks_core_meta, fmt],
)

madturks_core_log_ut = executable(
    'ut-madturks-core-log',
    'test/unit/ut_log.cpp',
    dependencies: [madturks_core_log, madturks_core_temputils, gtest, gmock],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables'],
)

asan_options = environment(
    {
        'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1:print_summary=1:alloc_dealloc_mismatch=0',
    },
)
test('Log unit tests', madturks_core_log_ut, env: asan_options)