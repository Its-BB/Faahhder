#include "Faahhder/ProjectEditing.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace faahhder {
namespace {

std::string TrimLine(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
}

}

bool ProjectSettingsDocument::Load(const std::filesystem::path& path) {
    fields_.clear();
    std::ifstream file(path);
    if (!file) return false;
    std::string line;
    while (std::getline(file, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        Set(TrimLine(line.substr(0, eq)), TrimLine(line.substr(eq + 1)));
    }
    return true;
}

bool ProjectSettingsDocument::Save(const std::filesystem::path& path) const {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) return false;
    for (const auto& field : fields_) file << field.key << "=" << field.value << "\n";
    return true;
}

void ProjectSettingsDocument::Set(std::string key, std::string value) {
    for (auto& field : fields_) {
        if (field.key == key) {
            field.value = std::move(value);
            return;
        }
    }
    fields_.push_back({std::move(key), std::move(value)});
}

std::string ProjectSettingsDocument::Get(const std::string& key, const std::string& fallback) const {
    for (const auto& field : fields_) if (field.key == key) return field.value;
    return fallback;
}

std::vector<ProjectMetaField> ProjectSettingsDocument::Fields() const {
    return fields_;
}

void SceneDraftDocument::Add(SceneEntityDraft entity) {
    if (entity.id == 0) entity.id = static_cast<int>(entities_.size()) + 1;
    entities_.push_back(std::move(entity));
}

bool SceneDraftDocument::Remove(int id) {
    const auto old = entities_.size();
    entities_.erase(std::remove_if(entities_.begin(), entities_.end(), [id](const auto& e) { return e.id == id; }), entities_.end());
    return entities_.size() != old;
}

SceneEntityDraft* SceneDraftDocument::Find(int id) {
    for (auto& entity : entities_) if (entity.id == id) return &entity;
    return nullptr;
}

std::vector<SceneEntityDraft> SceneDraftDocument::Entities() const {
    return entities_;
}

std::string SceneDraftDocument::ToJson() const {
    std::ostringstream out;
    out << "{\n  \"entities\": [\n";
    for (std::size_t i = 0; i < entities_.size(); ++i) {
        const auto& e = entities_[i];
        out << "    {\"id\":" << e.id << ",\"name\":\"" << e.name << "\",\"x\":" << e.x << ",\"y\":" << e.y << ",\"archetype\":\"" << e.archetype << "\"}";
        if (i + 1 < entities_.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    return out.str();
}

bool SceneDraftDocument::Save(const std::filesystem::path& path) const {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) return false;
    file << ToJson();
    return true;
}

void UndoHistory::Push(std::string label, std::string snapshot) {
    undo_.push_back({std::move(label), std::move(snapshot)});
    redo_.clear();
    if (undo_.size() > 128) undo_.erase(undo_.begin());
}

bool UndoHistory::CanUndo() const { return !undo_.empty(); }
bool UndoHistory::CanRedo() const { return !redo_.empty(); }

std::string UndoHistory::Undo(const std::string& current) {
    if (undo_.empty()) return current;
    auto item = undo_.back();
    undo_.pop_back();
    redo_.push_back({item.first, current});
    return item.second;
}

std::string UndoHistory::Redo(const std::string& current) {
    if (redo_.empty()) return current;
    auto item = redo_.back();
    redo_.pop_back();
    undo_.push_back({item.first, current});
    return item.second;
}

std::vector<std::string> UndoHistory::Labels() const {
    std::vector<std::string> labels;
    for (const auto& entry : undo_) labels.push_back(entry.first);
    return labels;
}

void DirtyFileTracker::MarkDirty(std::filesystem::path path) {
    path = path.lexically_normal();
    if (!IsDirty(path)) dirty_.push_back(std::move(path));
}

void DirtyFileTracker::MarkClean(std::filesystem::path path) {
    path = path.lexically_normal();
    dirty_.erase(std::remove(dirty_.begin(), dirty_.end(), path), dirty_.end());
}

bool DirtyFileTracker::IsDirty(const std::filesystem::path& path) const {
    const auto normalized = path.lexically_normal();
    return std::find(dirty_.begin(), dirty_.end(), normalized) != dirty_.end();
}

std::vector<std::filesystem::path> DirtyFileTracker::DirtyFiles() const {
    return dirty_;
}

}
