# The project's meson file.

project(
    'nexus',
    'cpp',
    version: '0.0.0',
    default_options: ['cpp_std=c++23', 'cpp_compiler=clang++-18', 'cpp_rtti=false'],
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

conan_profile = meson.project_source_root() + '/.config/conan-profiles/' + compiler.get_id()

message('conan profile is : @0@'.format(conan_profile))

# The project uses conan as a dependency manager. Call conan to fetch the dependencies.
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

# boost = dependency('boost')
capnproto = dependency('capnproto', include_type: 'system')
fmt = dependency('fmt', include_type: 'system')
spdlog = dependency('spdlog', include_type: 'system')
gtest = dependency('gtest', include_type: 'system')
gbench = dependency('benchmark', include_type: 'system')
stduuid = dependency('stduuid', include_type: 'system')

# msquic is a vendored library, and we have to declare it manually since it
# does not have a pkg-config file.
msquic = declare_dependency(
    dependencies: compiler.find_library(
        'msquic',
        dirs: meson.project_source_root() + '/vendor/msquic/2.3.6/bin',
        has_headers: ['msquic.h', 'msquic_posix.h', 'quic_sal_stub.h'],
        header_include_directories: include_directories('vendor/msquic/2.3.6/include', is_system: true),
        required: true,
    ),
    include_directories: include_directories('vendor/msquic/2.3.6/include', is_system: true),
    version: '2.3.6',
)

subdir('madturks/core/concept')
subdir('madturks/core/algorithm')
subdir('madturks/core/container')
subdir('madturks/core/concurrency')
subdir('madturks/nexus')

executable(
    'msquic-test-server',
    'sample/src/msquic_test_server.cpp',
    dependencies: [nexus, fmt],
)

executable(
    'msquic-sample',
    'sample/src/msquic_sample.cpp',
    dependencies: [msquic],
    cpp_args: '-Wno-everything',
)