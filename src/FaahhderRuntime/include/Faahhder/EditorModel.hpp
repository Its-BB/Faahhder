#pragma once

#include "Faahhder/Scene.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class PlayState {
    Editing,
    Playing,
    Paused
};

class EditorModel {
public:
    explicit EditorModel(std::filesystem::path projectRoot = ".");

    void NewScene();
    bool LoadScene(const std::filesystem::path& path);
    bool SaveScene(const std::filesystem::path& path) const;

    void Play();
    void Pause();
    void Stop();
    PlayState GetPlayState() const;

    Scene& GetScene();
    const Scene& GetScene() const;

    std::vector<std::string> BuildHierarchyPanel() const;
    std::vector<std::string> BuildInspectorPanel(EntityId selected) const;
    std::vector<std::filesystem::path> BuildAssetBrowser() const;
    std::vector<std::string> BuildConsolePanel() const;
    std::vector<std::string> BuildTilemapPanel(EntityId selected) const;
    std::vector<std::string> BuildAnimationPanel(EntityId selected) const;

    void Log(std::string message);

private:
    std::filesystem::path m_projectRoot;
    Scene m_scene;
    Scene m_editSnapshot;
    PlayState m_state = PlayState::Editing;
    std::vector<std::string> m_console;
};

}

