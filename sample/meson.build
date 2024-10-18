cxxopts = dependency('cxxopts', include_type: 'system')

executable(
    'msquic-test-server',
    'src/msquic_test_server.cpp',
    dependencies: [nexus, madturks_core_log, flatbuffers, fbs_schemas, cxxopts],
)

executable(
    'msquic-test-client',
    'src/msquic_test_client.cpp',
    dependencies: [nexus, madturks_core_log, flatbuffers, fbs_schemas, cxxopts],
)