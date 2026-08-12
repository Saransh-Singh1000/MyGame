cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0091 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(MyGame LANGUAGES C CXX)

set(OUTPUT_ROOT "${CMAKE_SOURCE_DIR}/Executables")

if (LINUX)
    set(OUT_DIR "${OUTPUT_ROOT}/Linux")
    add_subdirectory(Platform/Linux)
elseif (WIN32)
    set(OUT_DIR "${OUTPUT_ROOT}/Windows")
    add_subdirectory(Platform/Windows)
else ()
    message(FATAL_ERROR "Unsupported Platform")
endif()