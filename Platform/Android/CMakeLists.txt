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
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.0
)

# Download and populate dependencies
FetchContent_MakeAvailable(SDL3)

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

include("${COMMON_DIR}/cmake/VulkanShaders.cmake")
mygame_embed_vulkan_shaders(MyGameCommon)

# Link general engine dependencies to static common target
target_link_libraries(MyGameCommon
    PUBLIC
        SDL3::SDL3-shared
        Vulkan::Vulkan
)

# ------------------------------------------------------------------------------
# 3. Main Android Native Shared Library (.so Output)
# ------------------------------------------------------------------------------
add_library(MyGame SHARED
    src/lib.cpp
)

target_link_libraries(MyGame PRIVATE
    SDL3::SDL3-shared
    MyGameCommon
    log
    android
)