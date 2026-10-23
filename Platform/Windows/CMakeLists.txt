cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(MyGameWindows LANGUAGES C CXX)

include(FetchContent)

# Dependency Declarations
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
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

FetchContent_MakeAvailable(SDL3)

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

if(DEFINED ENV{VULKAN_SDK} AND
   EXISTS "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll")
    add_custom_command(TARGET MyGame POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll"
                "$<TARGET_FILE_DIR:MyGame>/vulkan-1.dll"
        VERBATIM
    )
endif()

if(DEFINED OUT_DIR)
    set_target_properties(MyGame PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}"
    )
endif()