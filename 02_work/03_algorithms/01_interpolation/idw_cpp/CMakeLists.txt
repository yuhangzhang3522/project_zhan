cmake_minimum_required(VERSION 3.16)
project(IDWInterpolation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(IDWInterpolation
    src/main.cpp
    src/csv_io.cpp
    src/idw.cpp
    src/raster_io.cpp
)

target_include_directories(IDWInterpolation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(IDWInterpolation PRIVATE /W3 /utf-8)
else()
    target_compile_options(IDWInterpolation PRIVATE -Wall -Wextra -O2)
endif()
