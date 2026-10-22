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

set(ULTRALIGHT_SDK_ROOT "$ENV{ULTRALIGHT_SDK_ROOT}" CACHE PATH
    "Path to the extracted Ultralight SDK (download from https://ultralig.ht/download)")
if(NOT ULTRALIGHT_SDK_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDK/README.md")
    set(ULTRALIGHT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDK")
endif()

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
    comctl32
)

if(ULTRALIGHT_SDK_ROOT)
    set(ULTRALIGHT_INCLUDE_DIR "${ULTRALIGHT_SDK_ROOT}/include")
    set(ULTRALIGHT_LIB_DIR "${ULTRALIGHT_SDK_ROOT}/lib/win/x64")
    set(ULTRALIGHT_BIN_DIR "${ULTRALIGHT_SDK_ROOT}/bin")
    foreach(required_file
        "${ULTRALIGHT_INCLUDE_DIR}/AppCore/AppCore.h"
        "${ULTRALIGHT_INCLUDE_DIR}/JavaScriptCore/JavaScript.h"
        "${ULTRALIGHT_LIB_DIR}/Ultralight.lib"
        "${ULTRALIGHT_LIB_DIR}/UltralightCore.lib"
        "${ULTRALIGHT_LIB_DIR}/WebCore.lib"
        "${ULTRALIGHT_LIB_DIR}/AppCore.lib")
        if(NOT EXISTS "${required_file}")
            message(FATAL_ERROR "Ultralight SDK is incomplete: missing ${required_file}")
        endif()
    endforeach()
    if(NOT IS_DIRECTORY "${ULTRALIGHT_BIN_DIR}" OR
       NOT IS_DIRECTORY "${ULTRALIGHT_SDK_ROOT}/resources")
        message(FATAL_ERROR
            "Ultralight SDK must include its bin and resources directories: ${ULTRALIGHT_SDK_ROOT}")
    endif()
    if(NOT MSVC)
        message(FATAL_ERROR
            "The Ultralight Windows SDK requires an MSVC-compatible toolchain. "
            "Configure with Visual Studio or clang-cl.")
    endif()

    target_compile_definitions(MyGame PRIVATE MYGAME_HAS_ULTRALIGHT=1)
    target_include_directories(MyGame PRIVATE "${ULTRALIGHT_INCLUDE_DIR}")
    target_link_directories(MyGame PRIVATE "${ULTRALIGHT_LIB_DIR}")
    target_link_libraries(MyGame PRIVATE Ultralight UltralightCore WebCore AppCore)
    add_custom_command(TARGET MyGame POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/launcher"
                "$<TARGET_FILE_DIR:MyGame>/assets/launcher"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${ULTRALIGHT_BIN_DIR}"
                "$<TARGET_FILE_DIR:MyGame>"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${ULTRALIGHT_SDK_ROOT}/resources"
                "$<TARGET_FILE_DIR:MyGame>/assets/resources"
        VERBATIM
    )
else()
    message(WARNING
        "Ultralight SDK not found. MyGame will use a native renderer-selection dialog. "
        "Set ULTRALIGHT_SDK_ROOT to enable the HTML/CSS/JS launcher.")
    target_compile_definitions(MyGame PRIVATE MYGAME_HAS_ULTRALIGHT=0)
endif()

if(DEFINED OUT_DIR)
    set_target_properties(MyGame PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_DIR}"
    )
endif()