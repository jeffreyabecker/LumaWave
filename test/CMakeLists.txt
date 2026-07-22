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
# Shared test support library (header-only: TestHelpers.h)
# ---------------------------------------------------------------------------
add_library(lumawave_test_support INTERFACE)

target_link_libraries(lumawave_test_support
    INTERFACE
        LumaWave::LumaWave
        unity::unity
)

target_include_directories(lumawave_test_support INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/support
)

if(MSVC)
    target_compile_options(lumawave_test_support INTERFACE /W3 /wd4100)
else()
    target_compile_options(lumawave_test_support INTERFACE -Wall -Wextra -Wno-unused-parameter)
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
