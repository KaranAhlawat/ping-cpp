cmake_minimum_required(VERSION 3.27)

project(ping-cpp VERSION 1.0
                 DESCRIPTION "my attempt at recreating ping"
                 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(asio REQUIRED)

add_executable(ping-cpp src/main.cpp src/icmp_header.hpp src/ipv4_header.hpp)
target_compile_features(ping-cpp PRIVATE cxx_std_20)
target_compile_options(ping-cpp PRIVATE -Wall -Wextra -Wpedantic -fcoroutines)
target_link_libraries(ping-cpp PRIVATE asio::asio)
