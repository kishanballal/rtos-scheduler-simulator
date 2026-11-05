cmake_minimum_required(VERSION 3.16)

project(RtosSchedulerSimulator
    VERSION 0.1.0
    DESCRIPTION "Tick-based RTOS task scheduler simulator"
    LANGUAGES CXX)

# ---- Language standard -------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++17, not -std=gnu++17
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd / VS Code IntelliSense

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---- Warnings, applied per target (not globally) -----------------------------
function(rtos_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    endif()
endfunction()

# ---- Core library: all simulator logic lives here ----------------------------
add_library(rtos_core STATIC
    src/Version.cpp
    src/Task.cpp
    src/TaskLoader.cpp
    src/Schedulers.cpp
    src/Simulator.cpp
    src/Statistics.cpp
    src/Schedulability.cpp
    src/GanttRenderer.cpp
    src/Report.cpp
)
target_include_directories(rtos_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
rtos_set_warnings(rtos_core)

# ---- CLI executable: a thin main() over the library --------------------------
add_executable(rtos_sim src/main.cpp)
target_link_libraries(rtos_sim PRIVATE rtos_core)
rtos_set_warnings(rtos_sim)

# ---- Unit tests (GoogleTest, fetched at configure time) ----------------------
option(RTOS_BUILD_TESTS "Build unit tests" ON)
if(RTOS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # MSVC runtime match
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(rtos_tests
        tests/test_version.cpp
        tests/test_data_structures.cpp
        tests/test_task_loader.cpp
        tests/test_schedulers.cpp
        tests/test_analysis.cpp
    )
    target_compile_definitions(rtos_tests PRIVATE RTOS_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
    target_link_libraries(rtos_tests PRIVATE rtos_core GTest::gtest_main)
    rtos_set_warnings(rtos_tests)

    include(GoogleTest)
    gtest_discover_tests(rtos_tests)
endif()
