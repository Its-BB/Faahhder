add_library(FaahhderRuntime STATIC
    src/Application.cpp
    src/Assets.cpp
    src/EditorModel.cpp
    src/Event.cpp
    src/CodeWorkspace.cpp
    src/EditorState.cpp
    src/EditorPanels.cpp
    src/Input.cpp
    src/Math.cpp
    src/Physics.cpp
    src/Renderer2D.cpp
    src/Scene.cpp
    src/SceneManager.cpp
    src/Scripting.cpp
    src/Window.cpp
)

target_include_directories(FaahhderRuntime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(FaahhderRuntime PUBLIC Faahhder_VERSION="${PROJECT_VERSION}")

if(Faahhder_USE_SDL2)
    find_package(SDL2 CONFIG QUIET)
    if(SDL2_FOUND)
        target_link_libraries(FaahhderRuntime PUBLIC SDL2::SDL2)
        target_compile_definitions(FaahhderRuntime PUBLIC Faahhder_WITH_SDL2=1)
    endif()
endif()

if(Faahhder_USE_OPENGL)
    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        target_link_libraries(FaahhderRuntime PUBLIC OpenGL::GL)
        target_compile_definitions(FaahhderRuntime PUBLIC Faahhder_WITH_OPENGL=1)
    endif()
endif()

if(Faahhder_USE_LUA)
    find_package(Lua QUIET)
    if(Lua_FOUND)
        target_include_directories(FaahhderRuntime PUBLIC ${LUA_INCLUDE_DIR})
        target_link_libraries(FaahhderRuntime PUBLIC ${LUA_LIBRARIES})
        target_compile_definitions(FaahhderRuntime PUBLIC Faahhder_WITH_LUA=1)
    endif()
endif()

find_package(box2d CONFIG QUIET)
if(box2d_FOUND)
    target_link_libraries(FaahhderRuntime PUBLIC box2d::box2d)
    target_compile_definitions(FaahhderRuntime PUBLIC Faahhder_WITH_BOX2D=1)
endif()

if(MSVC)
    target_compile_options(FaahhderRuntime PRIVATE /W4)
else()
    target_compile_options(FaahhderRuntime PRIVATE -Wall -Wextra -Wpedantic)
endif()
