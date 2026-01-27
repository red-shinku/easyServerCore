cmake_minimum_required(VERSION 3.20)

# ===============================
# Project
# ===============================
project(easysv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ===============================
# Options
# ===============================
option(BUILD_SHARED_LIBS "Build as shared library" ON)

# ===============================
# spdlog (auto build)
# ===============================
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)

# Disable spdlog options we don't need
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(spdlog)

# ===============================
# Source files
# ===============================
file(GLOB_RECURSE EASYSV_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

file(GLOB_RECURSE EASYSV_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

# ===============================
# Library target
# ===============================
add_library(easysv SHARED
    ${EASYSV_HEADERS}
    ${EASYSV_SOURCES}
)

# ===============================
# Include directories
# ===============================
target_include_directories(easysv
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ===============================
# Link libraries
# ===============================
target_link_libraries(easysv
    PUBLIC
        spdlog::spdlog
        pthread
)

# ===============================
# Compile options
# ===============================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(easysv PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
endif()

# ===============================
# Output
# ===============================
set_target_properties(easysv PROPERTIES
    OUTPUT_NAME "easysv"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# ===============================
# Install (optional)
# ===============================
install(TARGETS easysv
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)

# ===============================
# Summary
# ===============================
message(STATUS "\n=========== easysv build config ===========")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Shared lib: ${BUILD_SHARED_LIBS}")
message(STATUS "CXX: ${CMAKE_CXX_COMPILER}")
message(STATUS "==========================================\n")
