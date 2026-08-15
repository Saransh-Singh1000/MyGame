cmake_minimum_required(VERSION 3.20)

# Language and C/C++ Standards Setup
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

project(MyGameAndroid LANGUAGES C CXX)

include(FetchContent)

# ------------------------------------------------------------------------------
# 1. Dependency Configurations
# ------------------------------------------------------------------------------

# --- SDL3 Setup ---
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.0
)

# --- ENet Setup ---
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        master
)

# --- DirectXMath Setup ---
FetchContent_Declare(
    directxmath
    GIT_REPOSITORY https://github.com/microsoft/DirectXMath.git
    GIT_TAG        main
)

# --- Jolt Physics Setup (Optimized for Android NEON SIMD) ---
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(USE_SSE4_1 OFF CACHE BOOL "" FORCE)
set(USE_SSE4_2 OFF CACHE BOOL "" FORCE)
set(USE_AVX OFF CACHE BOOL "" FORCE)
set(USE_AVX2 OFF CACHE BOOL "" FORCE)
set(USE_NEON ON CACHE BOOL "" FORCE) # Required for ARM64 Android hardware

FetchContent_Declare(
    jolt
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.2.0
    SOURCE_SUBDIR  Build
)

# Download and populate dependencies
FetchContent_MakeAvailable(SDL3 enet directxmath jolt)

# System dependencies
find_package(Vulkan REQUIRED)

# ------------------------------------------------------------------------------
# 2. Engine Common Library Target (Static)
# ------------------------------------------------------------------------------
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Common")

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS "${COMMON_DIR}/src/*.cpp" "${COMMON_DIR}/src/*.c")
file(GLOB_RECURSE COMMON_HEADERS CONFIGURE_DEPENDS "${COMMON_DIR}/include/*.hpp" "${COMMON_DIR}/include/*.h")

add_library(MyGameCommon STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_compile_features(MyGameCommon PUBLIC cxx_std_20)

target_include_directories(MyGameCommon
    PUBLIC
        "${COMMON_DIR}/include"
        "${COMMON_DIR}/include/init"
    PRIVATE
        "${COMMON_DIR}/src"
)

# Link general engine dependencies to static common target (SDL3 included for header propagation)
target_link_libraries(MyGameCommon
    PUBLIC
        SDL3::SDL3
        DirectXMath
        enet
        Vulkan::Vulkan
        Jolt
)

# ------------------------------------------------------------------------------
# 3. Main Android Native Shared Library (.so Output)
# ------------------------------------------------------------------------------
add_library(MyGame SHARED
    src/lib.cpp
)

# Crucial Linkage Step: 
# Wrap SDL3 inside -Wl,--whole-archive to prevent NDK from stripping JNI symbols
target_link_libraries(MyGame PRIVATE
    -Wl,--whole-archive
    SDL3::SDL3
    -Wl,--no-whole-archive
    MyGameCommon
    log
    android
)