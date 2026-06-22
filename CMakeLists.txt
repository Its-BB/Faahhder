cmake_minimum_required(VERSION 3.22)

project(Faahhder
    VERSION 0.1.0
    DESCRIPTION "A compact 2D C++ game engine with runtime, editor, and asset packer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(Faahhder_USE_SDL2 "Use SDL2 for the real window/input backend when available" ON)
option(Faahhder_USE_OPENGL "Use OpenGL when available" ON)
option(Faahhder_USE_IMGUI "Use Dear ImGui for the editor when available" ON)
option(Faahhder_USE_LUA "Use Lua scripting when available" ON)

add_subdirectory(src/FaahhderRuntime)
add_subdirectory(src/Faahhder)
add_subdirectory(tools/FaahhderPacker)
