#include "Faahhder/Window.hpp"

#include "Faahhder/Input.hpp"

#include <utility>

#if Faahhder_WITH_SDL2
#include <SDL.h>
#endif

namespace faahhder {

Window::Window(WindowConfig config)
    : m_config(std::move(config)) {}

bool Window::Open() {
#if Faahhder_WITH_SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        Events::Emit({"WindowError", -1, SDL_GetError()});
        return false;
    }

#if Faahhder_WITH_OPENGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (m_config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
#if Faahhder_WITH_OPENGL
    flags |= SDL_WINDOW_OPENGL;
#endif

    auto* window = SDL_CreateWindow(
        m_config.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_config.width,
        m_config.height,
        flags);

    if (!window) {
        Events::Emit({"WindowError", -1, SDL_GetError()});
        SDL_Quit();
        return false;
    }

    m_nativeWindow = window;
#if Faahhder_WITH_OPENGL
    m_glContext = SDL_GL_CreateContext(window);
    if (!m_glContext) {
        Events::Emit({"WindowError", -1, SDL_GetError()});
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    SDL_GL_SetSwapInterval(m_config.vsync ? 1 : 0);
#endif
#endif

    m_open = true;
    m_shouldClose = false;
    Events::Emit({"WindowOpened", -1, m_config.title});
    return true;
}

void Window::Close() {
    if (m_open) {
        m_open = false;
        Events::Emit({"WindowClosed", -1, m_config.title});
    }

#if Faahhder_WITH_SDL2
#if Faahhder_WITH_OPENGL
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
#endif
    if (m_nativeWindow) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(m_nativeWindow));
        m_nativeWindow = nullptr;
    }
    SDL_Quit();
#endif
}

void Window::PollEvents() {
    if (m_open) {
#if Faahhder_WITH_SDL2
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                m_shouldClose = true;
                Events::Emit({"Quit", -1, {}});
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                Input::SetKeyDown(SDL_GetKeyName(event.key.keysym.sym), event.type == SDL_KEYDOWN);
                Events::Emit({"Key", -1, SDL_GetKeyName(event.key.keysym.sym)});
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                Input::SetMouseButtonDown(static_cast<int>(event.button.button), event.type == SDL_MOUSEBUTTONDOWN);
                break;
            case SDL_MOUSEMOTION:
                Input::SetMousePosition({static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)});
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    m_shouldClose = true;
                    Events::Emit({"Quit", -1, {}});
                } else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_config.width = event.window.data1;
                    m_config.height = event.window.data2;
                    Events::Emit({"WindowResized", -1, std::to_string(m_config.width) + "x" + std::to_string(m_config.height)});
                }
                break;
            default:
                break;
            }
        }
#endif
        Events::Emit({"WindowPoll", -1, {}});
    }
}

void Window::SwapBuffers() {
    if (m_open) {
#if Faahhder_WITH_SDL2 && Faahhder_WITH_OPENGL
        SDL_GL_SwapWindow(static_cast<SDL_Window*>(m_nativeWindow));
#endif
        Events::Emit({"WindowSwap", -1, {}});
    }
}

bool Window::IsOpen() const {
    return m_open;
}

bool Window::ShouldClose() const {
    return m_shouldClose;
}

int Window::Width() const {
    return m_config.width;
}

int Window::Height() const {
    return m_config.height;
}

const std::string& Window::Title() const {
    return m_config.title;
}

const WindowConfig& Window::Config() const {
    return m_config;
}

}
