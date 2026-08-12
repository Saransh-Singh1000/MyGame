cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(MyGame LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(enet)

find_package(Vulkan REQUIRED)

get_filename_component(PROJECT_ROOT
    ${CMAKE_SOURCE_DIR}/../..
    ABSOLUTE
)

add_library(MyGame SHARED
    c/lib.c
)

target_include_directories(MyGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Common
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(MyGame PRIVATE
    SDL3-static
    enet
    Vulkan::Vulkan
)