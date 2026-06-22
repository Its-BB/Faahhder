#pragma once

#include "Faahhder/Scene.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace faahhder {

class SceneManager {
public:
    std::shared_ptr<Scene> CreateScene(const std::string& name);
    std::shared_ptr<Scene> LoadScene(const std::string& name, const std::filesystem::path& path);
    bool SaveScene(const std::string& name, const std::filesystem::path& path) const;

    void SetActiveScene(const std::string& name);
    std::shared_ptr<Scene> ActiveScene() const;
    std::shared_ptr<Scene> GetScene(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::string m_activeScene;
};

}

