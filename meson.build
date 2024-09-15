# The project's meson file.
project(
    'nexus', 'cpp',
    version: '0.0.0',
    default_options: ['cpp_std=c++20', 'cpp_compiler=clang++-18']
)

# Invoke conan
run_command('conan', 'install', '--output-folder', 
    meson.build_root(), meson.source_root() + '/.config/conanfile.txt', 
    '--build=missing', '--profile:all=' + meson.source_root() + '/.config/conan-profiles/clang', check: true)


boost = dependency('boost', cmake_module_path: meson.build_root())

executable('hello-world', 'nexus/src/main.cpp', dependencies: [boost])
