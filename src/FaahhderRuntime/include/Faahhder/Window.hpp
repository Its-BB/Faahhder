#pragma once

#include "Faahhder/Event.hpp"

#include <string>

namespace faahhder {

struct WindowConfig {
    std::string title = "Faahhder";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool resizable = true;
};

class Window {
public:
    explicit Window(WindowConfig config = {});

    bool Open();
    void Close();
    void PollEvents();
    void SwapBuffers();

    bool IsOpen() const;
    bool ShouldClose() const;
    int Width() const;
    int Height() const;
    const std::string& Title() const;
    const WindowConfig& Config() const;

private:
    WindowConfig m_config;
    bool m_open = false;
    bool m_shouldClose = false;
    void* m_nativeWindow = nullptr;
    void* m_glContext = nullptr;
};

}
