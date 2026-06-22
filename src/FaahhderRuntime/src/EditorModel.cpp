#include "Faahhder/EditorModel.hpp"

#include <sstream>
#include <utility>

namespace faahhder {

EditorModel::EditorModel(std::filesystem::path projectRoot)
    : m_projectRoot(std::move(projectRoot)) {
    NewScene();
}

void EditorModel::NewScene() {
    m_scene = Scene{};
    auto camera = m_scene.CreateEntity("Main Camera");
    m_scene.AddTransform(camera, {{0.0f, 0.0f}, 0.0f, {1.0f, 1.0f}});
    m_scene.AddCamera(camera, {1.0f, {1280.0f, 720.0f}, true});
    Log("Created new scene");
}

bool EditorModel::LoadScene(const std::filesystem::path& path) {
    const bool ok = m_scene.LoadJson(path);
    Log(ok ? "Loaded scene: " + path.string() : "Failed to load scene: " + path.string());
    return ok;
}

bool EditorModel::SaveScene(const std::filesystem::path& path) const {
    return m_scene.SaveJson(path);
}

void EditorModel::Play() {
    if (m_state == PlayState::Editing) {
        m_editSnapshot = m_scene;
    }
    m_state = PlayState::Playing;
    Log("Play mode");
}

void EditorModel::Pause() {
    if (m_state == PlayState::Playing) {
        m_state = PlayState::Paused;
        Log("Paused");
    }
}

void EditorModel::Stop() {
    if (m_state != PlayState::Editing) {
        m_scene = m_editSnapshot;
    }
    m_state = PlayState::Editing;
    Log("Stopped");
}

PlayState EditorModel::GetPlayState() const {
    return m_state;
}

Scene& EditorModel::GetScene() {
    return m_scene;
}

const Scene& EditorModel::GetScene() const {
    return m_scene;
}

std::vector<std::string> EditorModel::BuildHierarchyPanel() const {
    std::vector<std::string> rows;
    for (auto entity : m_scene.Entities()) {
        const auto* info = m_scene.GetInfo(entity);
        if (info) {
            rows.push_back(std::to_string(info->id) + " " + info->name);
        }
    }
    return rows;
}

std::vector<std::string> EditorModel::BuildInspectorPanel(EntityId selected) const {
    std::vector<std::string> rows;
    const auto* info = m_scene.GetInfo(selected);
    if (!info) {
        return {"No entity selected"};
    }
    rows.push_back("Name: " + info->name);
    if (m_scene.GetTransform(selected)) rows.push_back("Transform2D");
    if (m_scene.GetSpriteRenderer(selected)) rows.push_back("SpriteRenderer");
    if (m_scene.GetCamera(selected)) rows.push_back("Camera2D");
    if (m_scene.GetTilemap(selected)) rows.push_back("Tilemap");
    if (m_scene.GetParticleEmitter(selected)) rows.push_back("ParticleEmitter");
    if (m_scene.GetPointLight(selected)) rows.push_back("PointLight2D");
    if (m_scene.GetRigidbody(selected)) rows.push_back("Rigidbody2D");
    if (m_scene.GetCollider(selected)) rows.push_back("Collider2D");
    if (m_scene.GetLuaScript(selected)) rows.push_back("LuaScript");
    return rows;
}

std::vector<std::filesystem::path> EditorModel::BuildAssetBrowser() const {
    std::vector<std::filesystem::path> assets;
    const auto assetRoot = m_projectRoot / "assets";
    if (!std::filesystem::exists(assetRoot)) {
        return assets;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetRoot)) {
        if (entry.is_regular_file()) {
            assets.push_back(entry.path());
        }
    }
    return assets;
}

std::vector<std::string> EditorModel::BuildConsolePanel() const {
    return m_console;
}

std::vector<std::string> EditorModel::BuildTilemapPanel(EntityId selected) const {
    const auto* tilemap = m_scene.GetTilemap(selected);
    if (!tilemap) {
        return {"Select a Tilemap entity"};
    }
    return {"Size: " + std::to_string(tilemap->width) + "x" + std::to_string(tilemap->height),
            "Tile size: " + std::to_string(tilemap->tileSize)};
}

std::vector<std::string> EditorModel::BuildAnimationPanel(EntityId selected) const {
    const auto* sprite = m_scene.GetSpriteRenderer(selected);
    if (!sprite) {
        return {"Select a SpriteRenderer entity"};
    }
    return {"Spritesheet: " + sprite->spriteSheet, "Frame: " + std::to_string(sprite->frame)};
}

void EditorModel::Log(std::string message) {
    m_console.push_back(std::move(message));
}

}
