#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

struct ProjectMetaField {
    std::string key;
    std::string value;
};

class ProjectSettingsDocument {
public:
    bool Load(const std::filesystem::path& path);
    bool Save(const std::filesystem::path& path) const;
    void Set(std::string key, std::string value);
    std::string Get(const std::string& key, const std::string& fallback = {}) const;
    std::vector<ProjectMetaField> Fields() const;
private:
    std::vector<ProjectMetaField> fields_;
};

struct SceneEntityDraft {
    int id = 0;
    std::string name;
    float x = 0.0f;
    float y = 0.0f;
    std::string archetype;
};

class SceneDraftDocument {
public:
    void Add(SceneEntityDraft entity);
    bool Remove(int id);
    SceneEntityDraft* Find(int id);
    std::vector<SceneEntityDraft> Entities() const;
    std::string ToJson() const;
    bool Save(const std::filesystem::path& path) const;
private:
    std::vector<SceneEntityDraft> entities_;
};

class UndoHistory {
public:
    void Push(std::string label, std::string snapshot);
    bool CanUndo() const;
    bool CanRedo() const;
    std::string Undo(const std::string& current);
    std::string Redo(const std::string& current);
    std::vector<std::string> Labels() const;
private:
    std::vector<std::pair<std::string, std::string>> undo_;
    std::vector<std::pair<std::string, std::string>> redo_;
};

class DirtyFileTracker {
public:
    void MarkDirty(std::filesystem::path path);
    void MarkClean(std::filesystem::path path);
    bool IsDirty(const std::filesystem::path& path) const;
    std::vector<std::filesystem::path> DirtyFiles() const;
private:
    std::vector<std::filesystem::path> dirty_;
};

}
