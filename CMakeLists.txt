cmake_minimum_required(VERSION 3.20)
project(ds-gen CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)            # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)    # compile_commands.json for clangd

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp                       # optional, but ShowDemoWindow() is great for learning
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp            # InputText() with std::string
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ${IMGUI_DIR}/misc/cpp)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

if(APPLE)
    target_compile_definitions(imgui PUBLIC GL_SILENCE_DEPRECATION)  # hides macOS "OpenGL is deprecated" warnings
endif()

add_executable(ds-gen
    src/main.cpp
)

target_link_libraries(ds-gen PRIVATE imgui)
target_include_directories(ds-gen PRIVATE include)
target_compile_options(ds-gen PRIVATE
    -Wall -Wextra -Wconversion -Wsign-conversion -pedantic-errors
)
