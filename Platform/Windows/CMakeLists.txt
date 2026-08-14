cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(MyGameWindows LANGUAGES C CXX)

include(FetchContent)

# Dependency Declarations
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
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

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Common" "${CMAKE_CURRENT_BINARY_DIR}/CommonBuild")

add_executable(MyGame
    src/main.cpp
)

target_link_libraries(MyGame PRIVATE
    MyGameCommon
    ws2_32
    winmm
)

if(DEFINED OUT_DIR)
    set_target_properties(MyGame PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}"
    )
endif()