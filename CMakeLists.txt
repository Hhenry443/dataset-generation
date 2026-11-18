cmake_minimum_required(VERSION 3.20)
project(ds-gen CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)            # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)    # compile_commands.json for clangd

add_executable(ds-gen
    src/main.cpp
)

target_include_directories(ds-gen PRIVATE include)
target_compile_options(ds-gen PRIVATE
    -Wall -Wextra -Wconversion -Wsign-conversion -pedantic-errors
)
