# The project's meson file.

project(
    'nexus',
    'cpp',
    version: '0.0.0',
    default_options: ['cpp_std=c++23', 'cpp_compiler=clang++-18', 'cpp_rtti=false'],
    license: 'GPL'
)

# Project-level macro definitions goes here
add_project_arguments('-DFMT_HEADER_ONLY=1', language : 'cpp')
# add_project_arguments('', language:'cpp')

compiler = meson.get_compiler('cpp')

if compiler.get_id() == 'clang'
    add_project_arguments('-stdlib=libc++', language: 'cpp')
    add_project_link_arguments('-stdlib=libc++',language: 'cpp')
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
# dependency(...) call below should be able to find the dependencies installed
# by Conan.

message('msquic path @0@'.format(meson.project_source_root() + '/vendor/'))

boost = dependency('boost')
capnproto = dependency('capnproto')
fmt = dependency('fmt')
spdlog = dependency('spdlog')

# msquic is a vendored library, and we have to declare it manually since it
# does not have a pkg-config file.
msquic = declare_dependency(
    dependencies: compiler.find_library(
        'msquic',
        dirs: meson.project_source_root() + '/vendor/msquic/bin',
        has_headers: ['msquic.h', 'msquic_posix.h', 'quic_sal_stub.h'],
        header_include_directories: include_directories('vendor/msquic/include'),
        required: true,
    ),
    include_directories: include_directories('vendor/msquic/include'),
    version: '1.2.3'
)

base_dependencies = [fmt, msquic]

executable(
    'msquic-test',
    'nexus/src/msquic_test.cpp',
    dependencies: base_dependencies
)