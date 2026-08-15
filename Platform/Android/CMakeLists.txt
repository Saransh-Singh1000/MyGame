cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

project(MyGameAndroid LANGUAGES C CXX)

include(FetchContent)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.0
)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        master
)

FetchContent_Declare(
    directxmath
    GIT_REPOSITORY https://github.com/microsoft/DirectXMath.git
    GIT_TAG        main
)

set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    jolt
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.2.0
    SOURCE_SUBDIR  Build
)

FetchContent_MakeAvailable(SDL3 enet directxmath jolt)

find_package(Vulkan REQUIRED)

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
        ${directxmath_SOURCE_DIR}/Inc
    PRIVATE
        "${COMMON_DIR}/src"
)

target_link_libraries(MyGameCommon
    PUBLIC
        SDL3::SDL3
        enet
        Vulkan::Vulkan
        Jolt
)

add_library(MyGame SHARED
    src/lib.cpp
)

target_link_libraries(MyGame PRIVATE
    MyGameCommon
    log
    android
)