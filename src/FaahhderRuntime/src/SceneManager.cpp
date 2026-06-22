#include "Faahhder/SceneManager.hpp"

namespace faahhder {

std::shared_ptr<Scene> SceneManager::CreateScene(const std::string& name) {
    auto scene = std::make_shared<Scene>();
    m_scenes[name] = scene;
    if (m_activeScene.empty()) {
        m_activeScene = name;
    }
    return scene;
}

std::shared_ptr<Scene> SceneManager::LoadScene(const std::string& name, const std::filesystem::path& path) {
    auto scene = std::make_shared<Scene>();
    if (!scene->LoadJson(path)) {
        return nullptr;
    }
    m_scenes[name] = scene;
    if (m_activeScene.empty()) {
        m_activeScene = name;
    }
    return scene;
}

bool SceneManager::SaveScene(const std::string& name, const std::filesystem::path& path) const {
    const auto scene = GetScene(name);
    return scene ? scene->SaveJson(path) : false;
}

void SceneManager::SetActiveScene(const std::string& name) {
    if (m_scenes.count(name) > 0) {
        m_activeScene = name;
    }
}

std::shared_ptr<Scene> SceneManager::ActiveScene() const {
    return GetScene(m_activeScene);
}

std::shared_ptr<Scene> SceneManager::GetScene(const std::string& name) const {
    const auto it = m_scenes.find(name);
    return it == m_scenes.end() ? nullptr : it->second;
}

}

