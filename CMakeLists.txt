# aria_make - Aria Build System
# Build system and package manager for Aria projects
#
# Build:
#   mkdir build && cd build
#   cmake ..
#   make
#
# Test:
#   make test
#   ./test_state_manager

cmake_minimum_required(VERSION 3.16)
project(aria_make
    VERSION 0.1.0
    DESCRIPTION "Build System for Aria Language"
    LANGUAGES CXX
)

# Require C++17 for std::filesystem, std::shared_mutex, std::optional
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Threading support
find_package(Threads REQUIRED)

# -----------------------------------------------------------------------------
# Library: aria_make_state (StateManager component)
# -----------------------------------------------------------------------------
add_library(aria_make_state STATIC
    src/state/state_manager.cpp
)

target_include_directories(aria_make_state
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(aria_make_state PUBLIC Threads::Threads)

# Link filesystem library if needed (older GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(aria_make_state PUBLIC stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(aria_make_state PUBLIC c++fs)
endif()

set_target_properties(aria_make_state PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# Test executable
# -----------------------------------------------------------------------------
option(BUILD_TESTS "Build test executable" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(test_state_manager
        tests/test_state_manager.cpp
    )

    target_link_libraries(test_state_manager PRIVATE aria_make_state)

    add_test(NAME state_manager_tests COMMAND test_state_manager)
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS aria_make_state
    EXPORT aria_makeTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT aria_makeTargets
    FILE aria_makeTargets.cmake
    NAMESPACE aria::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aria_make
)

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "aria_make ${PROJECT_VERSION} - Aria Build System")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "")
