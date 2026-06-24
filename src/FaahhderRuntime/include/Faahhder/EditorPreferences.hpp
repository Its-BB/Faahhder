#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

struct EditorPreferences {
    std::string theme = "dark";
    bool openLastProject = true;
    bool autosaveCode = true;
    int autosaveSeconds = 30;
    int recentProjectLimit = 8;
    std::vector<std::filesystem::path> recentProjects;
};

class EditorPreferenceStore {
public:
    explicit EditorPreferenceStore(std::filesystem::path path);

    EditorPreferences Load() const;
    bool Save(const EditorPreferences& preferences) const;
    bool AddRecentProject(const std::filesystem::path& project);
    const std::filesystem::path& Path() const;

private:
    std::filesystem::path path_;
};

std::string SerializeEditorPreferences(const EditorPreferences& preferences);
EditorPreferences ParseEditorPreferences(const std::string& text);

}
