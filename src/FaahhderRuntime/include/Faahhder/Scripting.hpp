#pragma once

#include "Faahhder/Scene.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

class ScriptingEngine {
public:
    void BindRuntimeApi(Scene* scene);
    void OnCreate(Scene& scene, EntityId entity);
    void OnUpdate(Scene& scene, EntityId entity, float dt);
    void OnDestroy(Scene& scene, EntityId entity);
    void OnCollisionEnter(Scene& scene, EntityId entity, EntityId other);
    void OnTriggerEnter(Scene& scene, EntityId entity, EntityId other);
    void HotReload(Scene& scene);

    const std::vector<std::string>& Log() const;

private:
    void ExecuteLifecycle(Scene& scene, EntityId entity, const std::string& lifecycle, float dt = 0.0f, EntityId other = InvalidEntity);
    std::vector<std::string> m_log;
    Scene* m_scene = nullptr;
};

}

