set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-D_POSIX_C_SOURCE=200809L)
add_definitions(-D_DEFAULT_SOURCE)

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

add_executable(MyGame
    c/main.c
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

if(DEFINED OUT_DIR)
    set_target_properties(MyGame PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}"
    )
endif()