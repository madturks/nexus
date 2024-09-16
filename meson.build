# The project's meson file.

project(
    'nexus', 'cpp',
    version: '0.0.0',
    default_options: ['cpp_std=c++20', 'cpp_compiler=clang++-18']
)

conan_profile = meson.project_source_root() + '/.config/conan-profiles/' + meson.get_compiler('cpp').get_id()

message('conan profile is : @0@'.format(conan_profile))

# The project uses conan as a dependency manager. Call conan to fetch the dependencies.
run_command('conan', 'install', '--output-folder', 
    '/opt/conan-pkg-root', meson.project_source_root() + '/.config/conanfile.py', 
    '--build=missing', '--profile:all=' + conan_profile, check: true)

# Conan emits dependency files using pkg-config format and the code overrides
# PKG_CONFIG_PATH with /opt/conan-pkg-root in devcontainer.json, therefore the
# dependency(...) call below should be able to find the dependencies installed
# by Conan.

boost = dependency('boost')
capnproto = dependency('capnproto')

executable('hello-world', 'nexus/src/main.cpp', dependencies: [boost, capnproto])
