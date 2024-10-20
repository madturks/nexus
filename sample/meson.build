cxxopts = dependency('cxxopts', include_type: 'system')

executable(
    'nexus-sample-server',
    'src/nexus_sample_server.cpp',
    dependencies: [nexus, madturks_core_log, flatbuffers, fbs_schemas, cxxopts],
)

executable(
    'nexus-sample-client',
    'src/nexus_sample_client.cpp',
    dependencies: [nexus, madturks_core_log, flatbuffers, fbs_schemas, cxxopts],
)