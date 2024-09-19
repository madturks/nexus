madturks_core_container = declare_dependency(
    link_with: library(
        'madturks.core.container',
        [
            'src/circular_buffer_base.cpp',
            'src/circular_buffer_pow2.cpp',
            'src/circular_buffer_vm.cpp',
        ],
        include_directories: include_directories('inc'),
        install: true,
        dependencies: [madturks_core_concept, stduuid],
    ),
    dependencies: [madturks_core_concept],
    include_directories: include_directories('inc'),
)

common_circular_buffer_ut = executable(
    'ut-madturks-core-container-circular-buffer',
    'tests/unit/ut_circular_buffer.cpp',
    dependencies: [madturks_core_algorithm, madturks_core_container, gtest],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables'],
    override_options: ['b_sanitize=address'],
)

test('Circular Buffer unit tests', common_circular_buffer_ut)



common_circular_buffer_benchmark = executable(
    'bench-madturks-core-container-circular-buffer',
    'tests/benchmark/circular_buffer_bench.cpp',
    dependencies: [madturks_core_algorithm, madturks_core_container, gbench],
    cpp_args: ['-Wno-global-constructors', '-Wno-weak-vtables'],
)

benchmark('Circular Buffer benchmarks', common_circular_buffer_benchmark)