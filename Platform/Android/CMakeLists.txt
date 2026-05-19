cmake_minimum_required(VERSION 3.20)
cmake_policy(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(MyGame)

get_filename_component(PROJECT_ROOT
    ${CMAKE_SOURCE_DIR}/../..
    ABSOLUTE
)

set(CORE_SOURCE
    ${PROJECT_ROOT}/Core/cpp/Log/Log.cpp
)

set(CORE_INCLUDE
    ${PROJECT_ROOT}/Core/cpp/include
)

add_library(MyGame SHARED
    cpp/lib.cpp
    ${CORE_SOURCE}
)

target_include_directories(MyGame PRIVATE
    ${CORE_INCLUDE}
)

find_library(
    log-lib
    log
)

target_link_libraries(MyGame
    ${log-lib}
)