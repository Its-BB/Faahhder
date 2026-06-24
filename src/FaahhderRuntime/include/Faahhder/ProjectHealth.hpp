#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class ProjectHealthLevel {
    Info,
    Warning,
    Error
};

struct ProjectHealthItem {
    ProjectHealthLevel level = ProjectHealthLevel::Info;
    std::string title;
    std::string detail;
};

struct ProjectHealthReport {
    std::filesystem::path projectRoot;
    std::vector<ProjectHealthItem> items;
    int errors = 0;
    int warnings = 0;
};

ProjectHealthReport InspectProjectHealth(const std::filesystem::path& projectRoot);
std::string FormatProjectHealth(const ProjectHealthReport& report);

}
