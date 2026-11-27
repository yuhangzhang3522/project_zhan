cmake_minimum_required(VERSION 3.16)
project(RegressionKriging LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RegressionKriging
    src/main.cpp
    src/rk_solver.cpp
)

target_include_directories(RegressionKriging PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(RegressionKriging PRIVATE /W3 /utf-8)
else()
    target_compile_options(RegressionKriging PRIVATE -Wall -Wextra -O2)
endif()
