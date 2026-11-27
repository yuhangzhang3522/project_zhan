cmake_minimum_required(VERSION 3.16)
project(KrigingInterpolation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(KrigingInterpolation
    src/main.cpp
    src/csv_io.cpp
    src/variogram.cpp
    src/kriging.cpp
    src/raster_io.cpp
)

target_include_directories(KrigingInterpolation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(KrigingInterpolation PRIVATE /W3 /utf-8)
else()
    target_compile_options(KrigingInterpolation PRIVATE -Wall -Wextra -O2)
endif()
