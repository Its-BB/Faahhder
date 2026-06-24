#include "Faahhder/EditorPreferences.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace faahhder {
namespace {

std::string Trim(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
}

bool ToBool(const std::string& value, bool fallback) {
    if (value == "true" || value == "1" || value == "yes") return true;
    if (value == "false" || value == "0" || value == "no") return false;
    return fallback;
}

int ToInt(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

}

EditorPreferenceStore::EditorPreferenceStore(std::filesystem::path path) : path_(std::move(path)) {}

EditorPreferences EditorPreferenceStore::Load() const {
    std::ifstream file(path_);
    if (!file) return {};
    std::ostringstream out;
    out << file.rdbuf();
    return ParseEditorPreferences(out.str());
}

bool EditorPreferenceStore::Save(const EditorPreferences& preferences) const {
    if (path_.has_parent_path()) std::filesystem::create_directories(path_.parent_path());
    std::ofstream file(path_, std::ios::trunc);
    if (!file) return false;
    file << SerializeEditorPreferences(preferences);
    return true;
}

bool EditorPreferenceStore::AddRecentProject(const std::filesystem::path& project) {
    auto preferences = Load();
    const auto normalized = project.lexically_normal();
    preferences.recentProjects.erase(std::remove(preferences.recentProjects.begin(), preferences.recentProjects.end(), normalized), preferences.recentProjects.end());
    preferences.recentProjects.insert(preferences.recentProjects.begin(), normalized);
    if (preferences.recentProjectLimit < 1) preferences.recentProjectLimit = 1;
    if (static_cast<int>(preferences.recentProjects.size()) > preferences.recentProjectLimit) {
        preferences.recentProjects.resize(static_cast<std::size_t>(preferences.recentProjectLimit));
    }
    return Save(preferences);
}

const std::filesystem::path& EditorPreferenceStore::Path() const {
    return path_;
}

std::string SerializeEditorPreferences(const EditorPreferences& preferences) {
    std::ostringstream out;
    out << "theme=" << preferences.theme << "\n";
    out << "openLastProject=" << (preferences.openLastProject ? "true" : "false") << "\n";
    out << "autosaveCode=" << (preferences.autosaveCode ? "true" : "false") << "\n";
    out << "autosaveSeconds=" << preferences.autosaveSeconds << "\n";
    out << "recentProjectLimit=" << preferences.recentProjectLimit << "\n";
    for (const auto& project : preferences.recentProjects) out << "recent=" << project.generic_string() << "\n";
    return out.str();
}

EditorPreferences ParseEditorPreferences(const std::string& text) {
    EditorPreferences preferences;
    preferences.recentProjects.clear();
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        const auto split = line.find('=');
        if (split == std::string::npos) continue;
        const auto key = Trim(line.substr(0, split));
        const auto value = Trim(line.substr(split + 1));
        if (key == "theme") preferences.theme = value;
        else if (key == "openLastProject") preferences.openLastProject = ToBool(value, preferences.openLastProject);
        else if (key == "autosaveCode") preferences.autosaveCode = ToBool(value, preferences.autosaveCode);
        else if (key == "autosaveSeconds") preferences.autosaveSeconds = std::max(5, ToInt(value, preferences.autosaveSeconds));
        else if (key == "recentProjectLimit") preferences.recentProjectLimit = std::max(1, ToInt(value, preferences.recentProjectLimit));
        else if (key == "recent" && !value.empty()) preferences.recentProjects.push_back(std::filesystem::path(value).lexically_normal());
    }
    if (static_cast<int>(preferences.recentProjects.size()) > preferences.recentProjectLimit) {
        preferences.recentProjects.resize(static_cast<std::size_t>(preferences.recentProjectLimit));
    }
    return preferences;
}

}
