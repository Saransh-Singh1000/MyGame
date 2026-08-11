cmake_minimum_required(VERSION 3.20)
cmake_policy(VERSION 3.20)

project(MyGame)

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