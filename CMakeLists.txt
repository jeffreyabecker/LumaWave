cmake_minimum_required(VERSION 3.25)
project(LumaWave VERSION 2.8.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build native tests" ON)

add_library(LumaWave INTERFACE)
add_library(LumaWave::LumaWave ALIAS LumaWave)

target_include_directories(LumaWave INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(LumaWave INTERFACE cxx_std_23)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

message(STATUS "Configured LumaWave: BUILD_TESTS=${BUILD_TESTS}")
