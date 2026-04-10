include(FetchContent)

# ---------------------------------------------------------------------------
# Unity test framework
# ---------------------------------------------------------------------------
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.1
)
FetchContent_MakeAvailable(unity)
add_library(unity_native STATIC ${unity_SOURCE_DIR}/src/unity.c)
target_include_directories(unity_native PUBLIC ${unity_SOURCE_DIR}/src)
add_library(unity::unity ALIAS unity_native)

# ---------------------------------------------------------------------------
# ArduinoFake (stubs + vendored FakeIt)
# Use FetchContent_Populate to get source only; ArduinoFake's CMakeLists.txt
# vendors its own Unity target which would conflict with ours.
# ---------------------------------------------------------------------------
FetchContent_Declare(
    arduinofake
    GIT_REPOSITORY    https://github.com/FabioBatSilva/ArduinoFake.git
    GIT_TAG           master
    GIT_SUBMODULES_RECURSE ON
)
FetchContent_GetProperties(arduinofake)
if(NOT arduinofake_POPULATED)
    FetchContent_Populate(arduinofake)
endif()

file(GLOB_RECURSE arduinofake_sources CONFIGURE_DEPENDS
    ${arduinofake_SOURCE_DIR}/src/*.cpp
    ${arduinofake_SOURCE_DIR}/src/**/*.cpp
)
add_library(arduinofake_lib STATIC ${arduinofake_sources})
target_include_directories(arduinofake_lib
    PUBLIC
        ${arduinofake_SOURCE_DIR}/src
        ${arduinofake_SOURCE_DIR}/lib/fakeit/single_include
)

# MSVC does not support GCC __attribute__ annotations used in ArduinoFake headers.
# Force-include a shim to discard them before any ArduinoFake header is processed.
if(MSVC)
    target_compile_options(arduinofake_lib PUBLIC
        /FI${CMAKE_CURRENT_SOURCE_DIR}/support/msvc_compat.h
    )

    # ArduinoFake's noniso.h declares itoa/ltoa/ultoa/utoa as plain extern "C"
    # which conflicts with MSVC CRT's dllimport versions in stdlib.h.
    # Patch the file once at configure time to guard those declarations on Windows.
    set(_noniso_path "${arduinofake_SOURCE_DIR}/src/arduino/noniso.h")
    file(READ "${_noniso_path}" _noniso_content)
    if(NOT _noniso_content MATCHES "_WIN32")
        string(REPLACE
            "#pragma once"
            "#pragma once\n\n#ifndef _WIN32"
            _noniso_content "${_noniso_content}"
        )
        string(APPEND _noniso_content "\n#endif // _WIN32\n")
        file(WRITE "${_noniso_path}" "${_noniso_content}")
        message(STATUS "ArduinoFake: patched noniso.h for MSVC compatibility")
    endif()
endif()

add_library(ArduinoFake::ArduinoFake ALIAS arduinofake_lib)

# ---------------------------------------------------------------------------
# Shared test support library (FakeArduinoSetup.cpp)
# ---------------------------------------------------------------------------
add_library(lumawave_test_support STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/support/FakeArduinoSetup.cpp
)

target_link_libraries(lumawave_test_support
    PUBLIC
        LumaWave::LumaWave
        ArduinoFake::ArduinoFake
        unity::unity
)

target_include_directories(lumawave_test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/support
)

if(MSVC)
    target_compile_options(lumawave_test_support PUBLIC /W3 /wd4100)
else()
    target_compile_options(lumawave_test_support PUBLIC -Wall -Wextra -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# Per-directory test executables
# ---------------------------------------------------------------------------
file(GLOB_RECURSE test_main_files CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*/test_main.cpp
)

foreach(test_file IN LISTS test_main_files)
    get_filename_component(test_dir  ${test_file} DIRECTORY)
    get_filename_component(test_name ${test_dir}  NAME)

    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE lumawave_test_support)

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
