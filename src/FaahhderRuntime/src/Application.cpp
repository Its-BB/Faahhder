#include "Faahhder/Application.hpp"

#include "Faahhder/Input.hpp"
#include "Faahhder/Scene.hpp"
#include "Faahhder/Window.hpp"

#include <chrono>
#include <thread>
#include <utility>

namespace faahhder {

Application::Application(ApplicationConfig config)
    : m_config(std::move(config)) {}

Application::~Application() {
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        (*it)->OnDetach();
    }
}

void Application::PushLayer(std::unique_ptr<Layer> layer) {
    layer->OnAttach();
    m_layers.push_back(std::move(layer));
}

void Application::SetScene(std::shared_ptr<Scene> scene) {
    m_scene = std::move(scene);
}

std::shared_ptr<Scene> Application::GetScene() const {
    return m_scene;
}

void Application::Run() {
    using clock = std::chrono::steady_clock;
    m_running = true;
    Window window({m_config.title, m_config.width, m_config.height, m_config.vsync, true});
    if (!m_config.headless) {
        if (!window.Open()) {
            m_running = false;
            return;
        }
    }
    auto previous = clock::now();
    int frame = 0;

    while (m_running) {
        window.PollEvents();
        if (window.ShouldClose()) {
            Stop();
        }
        auto now = clock::now();
        m_deltaTime = std::chrono::duration<float>(now - previous).count();
        previous = now;
        if (m_deltaTime <= 0.0f) {
            m_deltaTime = 1.0f / 60.0f;
        }

        Input::BeginFrame();
        if (m_scene) {
            m_scene->Update(m_deltaTime);
        }
        for (auto& layer : m_layers) {
            layer->OnUpdate(m_deltaTime);
            layer->OnGui();
        }

        ++frame;
        if (m_config.maxFrames > 0 && frame >= m_config.maxFrames) {
            Stop();
        }

        if (m_config.headless) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            window.SwapBuffers();
        }
    }

    window.Close();
}

void Application::Stop() {
    m_running = false;
}

const ApplicationConfig& Application::GetConfig() const {
    return m_config;
}

float Application::GetDeltaTime() const {
    return m_deltaTime;
}

}
