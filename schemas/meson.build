# retrieve the path to the flatbuffers schema compiler
flatbuffers_bindir = flatbuffers.get_variable('bindir')
flatc_compiler = files(flatbuffers_bindir + '/flatc')
meson.override_find_program('flatc', flatc_compiler)
flatc = find_program('flatc')

fbs_schemas = [
    'flatbuffers/main.fbs',
    'flatbuffers/chat.fbs',
    'flatbuffers/monster.fbs',
]

# Compile the flatbuffers schemas.
run_command(
    flatc.full_path(),
    '--cpp',
    '--scoped-enums',
    '-o', '@0@/include/mad/nexus/schemas'.format(meson.current_build_dir()),

    fbs_schemas,
    # capnp compile needs to locate `capnpc-c++` plugin
    # so set the PATH environment variable.
    env: {'PATH': flatbuffers_bindir},
    check: true,
)

# Define a target to the schemas.
fbs_schemas = declare_dependency(
    include_directories: include_directories('../tgt/schemas/include', is_system: true),

)
