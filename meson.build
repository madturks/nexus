# The project's meson file.
project(
    'nexus',
    'cpp',
    version: '0.0.0',
    license: 'GPL',
)

compiler = meson.get_compiler('cpp')

# The project uses conan as a dependency manager. Call conan to fetch the dependencies.
conan_profile = meson.project_source_root() + '/.config/conan-profiles/' + compiler.get_id()
message('Running `conan` with profile: @0@'.format(conan_profile))
v = run_command(
    'conan',
    'install',
    '--output-folder', '/opt/conan-pkg-root',
    meson.project_source_root() + '/.config/conanfile.py',
    '--build=missing',
    '--profile:all=' + conan_profile,
    check: true,
    capture: true
)
message(v.stdout())

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
tcmalloc = dependency('libtcmalloc_minimal', include_type: 'system', static : false)

# For some unknown reason, meson does not seem to add the tcmalloc_libdir
# to RPATH, so the code below does exactly that.
tcmalloc_libdir = tcmalloc.get_variable(pkgconfig: 'libdir')
add_project_dependencies(tcmalloc, language: 'cpp')
add_project_link_arguments('-Wl,-rpath,' + tcmalloc_libdir, language: 'cpp')

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

subdir('schemas')
subdir('madturks/core/macro')
subdir('madturks/core/meta')
subdir('madturks/core/concept')
subdir('madturks/core/random')
subdir('madturks/core/container')
subdir('madturks/core/concurrency')
subdir('madturks/core/temputils')
subdir('madturks/core/log')
subdir('madturks/nexus')
subdir('sample')