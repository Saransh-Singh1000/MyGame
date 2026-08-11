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

add_library(MyGame SHARED
    c/lib.c
)

target_include_directories(MyGame PRIVATE
)

target_link_libraries(MyGame
   
)