cmake_minimum_required(VERSION 3.20)

project(MyGameCommon LANGUAGES CXX C)

include(FetchContent)

set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v5.3.0
    SOURCE_SUBDIR  Build
)
FetchContent_MakeAvailable(JoltPhysics)

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")
file(GLOB_RECURSE COMMON_HEADERS CONFIGURE_DEPENDS "include/*.hpp" "include/*.h")

add_library(MyGameCommon STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_compile_features(MyGameCommon PUBLIC cxx_std_20)

target_include_directories(MyGameCommon
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/init"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(MyGameCommon
    PUBLIC
        SDL3::SDL3-static
        Vulkan::Vulkan
        Jolt
)

if(NOT ANDROID)
    find_package(OpenGL REQUIRED)
    target_link_libraries(MyGameCommon PUBLIC OpenGL::GL)
    target_compile_definitions(MyGameCommon PRIVATE MYGAME_ENABLE_OPENGL=1)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/VulkanShaders.cmake")
mygame_embed_vulkan_shaders(MyGameCommon)