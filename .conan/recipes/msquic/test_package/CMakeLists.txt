cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

message(STATUS "${msquic_DIR}")

find_package(msquic CONFIG REQUIRED)

message(STATUS "msquic found: ${msquic_DIR} ${msquic}")
add_executable(example src/example.cpp)
target_link_libraries(example msquic::msquic)
