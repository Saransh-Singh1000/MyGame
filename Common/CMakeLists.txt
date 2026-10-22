cmake_minimum_required(VERSION 3.20)

project(MyGameCommon LANGUAGES CXX C)

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
)

if(NOT ANDROID)
    find_package(OpenGL REQUIRED)
    target_link_libraries(MyGameCommon PUBLIC OpenGL::GL)
    target_compile_definitions(MyGameCommon PRIVATE MYGAME_ENABLE_OPENGL=1)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/VulkanShaders.cmake")
mygame_embed_vulkan_shaders(MyGameCommon)