# The project's meson file.

project(
    'nexus',
    'cpp',
    version: '0.0.0',
    default_options: [
        'cpp_std=c++23',
        'cpp_compiler=clang++-18',
        'cpp_rtti=false',
        # 'b_sanitize=address,undefined',
        'b_lundef=false',
    ],
    license: 'GPL',
)

# Project-level macro definitions goes here
add_project_arguments('-DFMT_HEADER_ONLY=1', language: 'cpp')

# add_project_arguments('', language:'cpp')

compiler = meson.get_compiler('cpp')

clang_compiler_flags = [
    '-stdlib=libc++',
    '-Wno-c++98-compat-pedantic',
    '-Weverything',
    '-Wno-switch-default',
    '-Wno-exit-time-destructors',
    '-Wno-padded',
    '-fsafe-buffer-usage-suggestions',
    '-Wno-unsafe-buffer-usage',
    '-Wno-documentation',
    '-Wno-c++20-compat',
    '-Wno-gnu-zero-variadic-macro-arguments',
    '-Wno-missing-prototypes',
    '-Wno-documentation-unknown-command',
    '-Wno-reserved-identifier',
    '-Wno-global-constructors',
    # Use absolute file paths on diagnostic messages.
    '-fdiagnostics-absolute-paths',
]

clang_linker_flags = ['-stdlib=libc++']

if compiler.get_id() == 'clang'
    foreach flag : clang_compiler_flags
        add_project_arguments(flag, language: 'cpp')
    endforeach

    foreach flag : clang_linker_flags
        add_project_link_arguments(flag, language: 'cpp')
    endforeach
endif

# The project uses conan as a dependency manager. Call conan to fetch the dependencies.
conan_profile = meson.project_source_root() + '/.config/conan-profiles/' + compiler.get_id()
message('Running `conan` with profile: @0@'.format(conan_profile))
run_command(
    'conan',
    'install',
    '--output-folder', '/opt/conan-pkg-root',
    meson.project_source_root() + '/.config/conanfile.py',
    '--build=missing',
    '--profile:all=' + conan_profile,
    check: true,
)

# Conan emits dependency files using pkg-config format and the code overrides
# PKG_CONFIG_PATH with /opt/conan-pkg-root in devcontainer.json, therefore the
# dependency(...) calls below should be able to find the dependencies installed
# by Conan.

flatbuffers = dependency('flatbuffers', include_type: 'system')
fmt = dependency('fmt', include_type: 'system')
gtest = dependency('gtest_main', include_type: 'system')
gmock = dependency('gmock', include_type: 'system')
gbench = dependency('benchmark', include_type: 'system')
stduuid = dependency('stduuid', include_type: 'system')
catch2 = dependency('catch2-with-main', include_type: 'system')

# retrieve the path to the flatbuffers schema compiler
flatbuffers_bindir = flatbuffers.get_variable('bindir')
flatc_compiler = files(flatbuffers_bindir + '/flatc')
meson.override_find_program('flatc', flatc_compiler)
flatc = find_program('flatc')

fbs_schemas = [
    'sample/flatbuffers_schemas/main.fbs',
    'sample/flatbuffers_schemas/chat.fbs',
    'sample/flatbuffers_schemas/monster.fbs',
]

# Compile the flatbuffers schemas.
run_command(
    flatc.full_path(),
    '--cpp',
    '--scoped-enums',
    '-o', '@0@/fbs-schemas/include/fbs-schemas'.format(meson.current_build_dir()),

    fbs_schemas,
    # capnp compile needs to locate `capnpc-c++` plugin
    # so set the PATH environment variable.
    env: {'PATH': flatbuffers_bindir},
    check: true,
)

# Define a target to the schemas.
fbs_schemas = declare_dependency(
    include_directories: include_directories('tgt/fbs-schemas/include', is_system: true),
    
)

# msquic is a vendored library, and we have to declare it manually since it
# does not have a pkg-config file.
msquic = declare_dependency(
    dependencies: [
        compiler.find_library(
            'msquic',
            dirs: meson.project_source_root() + '/vendor/msquic/2.4.4/bin',
            has_headers: [
                'msquic.h',
                'msquic_posix.h',
                'quic_sal_stub.h',
                'msquic.hpp',
                'msquicp.h',
            ],
            header_include_directories: include_directories('vendor/msquic/2.4.4/include', is_system: true),
            required: true,
        ),
    ],
    include_directories: include_directories('vendor/msquic/2.4.4/include', is_system: true),
    version: '2.4.4',
    compile_args: '-DQUIC_API_ENABLE_PREVIEW_FEATURES=1',
)

subdir('madturks/core/macro')
subdir('madturks/core/meta')
subdir('madturks/core/concept')
subdir('madturks/core/random')
subdir('madturks/core/container')
subdir('madturks/core/concurrency')
subdir('madturks/core/temputils')
subdir('madturks/core/log')
subdir('madturks/nexus')

executable(
    'msquic-test-server',
    'sample/src/msquic_test_server.cpp',
    dependencies: [nexus, madturks_core_log, flatbuffers, fbs_schemas],
)

executable(
    'msquic-test-client',
    'sample/src/msquic_test_client.cpp',
    dependencies: [nexus, madturks_core_log, flatbuffers, fbs_schemas],
)