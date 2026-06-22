#pragma once

#include "Faahhder/Event.hpp"

#include <memory>
#include <string>
#include <vector>

namespace faahhder {

class Scene;

struct ApplicationConfig {
    std::string title = "Faahhder";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool headless = false;
    int maxFrames = 0;
};

class Layer {
public:
    virtual ~Layer() = default;
    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnEvent(const Event&) {}
    virtual void OnUpdate(float) {}
    virtual void OnGui() {}
};

class Application {
public:
    explicit Application(ApplicationConfig config = {});
    ~Application();

    void PushLayer(std::unique_ptr<Layer> layer);
    void SetScene(std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> GetScene() const;

    void Run();
    void Stop();

    const ApplicationConfig& GetConfig() const;
    float GetDeltaTime() const;

private:
    ApplicationConfig m_config;
    std::vector<std::unique_ptr<Layer>> m_layers;
    std::shared_ptr<Scene> m_scene;
    bool m_running = false;
    float m_deltaTime = 0.0f;
};

}

