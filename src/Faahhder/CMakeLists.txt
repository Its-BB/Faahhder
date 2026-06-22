add_executable(Faahhder WIN32
    src/main.cpp
)

target_link_libraries(Faahhder PRIVATE FaahhderRuntime)

if(Faahhder_USE_IMGUI)
    find_package(imgui CONFIG QUIET)
    if(imgui_FOUND)
        target_link_libraries(Faahhder PRIVATE imgui::imgui)
        target_compile_definitions(Faahhder PRIVATE Faahhder_WITH_IMGUI=1)
    endif()
endif()
