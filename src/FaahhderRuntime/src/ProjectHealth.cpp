#include "Faahhder/ProjectHealth.hpp"

#include <sstream>

namespace faahhder {
namespace {

void Add(ProjectHealthReport& report, ProjectHealthLevel level, std::string title, std::string detail) {
    if (level == ProjectHealthLevel::Error) report.errors++;
    if (level == ProjectHealthLevel::Warning) report.warnings++;
    report.items.push_back({level, std::move(title), std::move(detail)});
}

const char* ToText(ProjectHealthLevel level) {
    switch (level) {
    case ProjectHealthLevel::Info: return "info";
    case ProjectHealthLevel::Warning: return "warning";
    case ProjectHealthLevel::Error: return "error";
    }
    return "info";
}

}

ProjectHealthReport InspectProjectHealth(const std::filesystem::path& projectRoot) {
    ProjectHealthReport report;
    report.projectRoot = projectRoot;
    if (!std::filesystem::exists(projectRoot)) {
        Add(report, ProjectHealthLevel::Error, "Project folder missing", projectRoot.generic_string());
        return report;
    }
    const auto projectFile = projectRoot / "project.faahhder";
    const auto assets = projectRoot / "assets";
    const auto scripts = assets / "scripts";
    if (std::filesystem::exists(projectFile)) Add(report, ProjectHealthLevel::Info, "Project file found", projectFile.generic_string());
    else Add(report, ProjectHealthLevel::Error, "Project file missing", projectFile.generic_string());
    if (std::filesystem::exists(assets) && std::filesystem::is_directory(assets)) Add(report, ProjectHealthLevel::Info, "Assets folder found", assets.generic_string());
    else Add(report, ProjectHealthLevel::Warning, "Assets folder missing", assets.generic_string());
    int scriptCount = 0;
    if (std::filesystem::exists(scripts)) {
        for (const auto& entry : std::filesystem::directory_iterator(scripts)) {
            if (entry.is_regular_file()) scriptCount++;
        }
    }
    if (scriptCount > 0) Add(report, ProjectHealthLevel::Info, "Scripts found", std::to_string(scriptCount));
    else Add(report, ProjectHealthLevel::Warning, "No scripts found", scripts.generic_string());
    return report;
}

std::string FormatProjectHealth(const ProjectHealthReport& report) {
    std::ostringstream out;
    out << "project=" << report.projectRoot.generic_string() << "\n";
    out << "errors=" << report.errors << "\n";
    out << "warnings=" << report.warnings;
    for (const auto& item : report.items) {
        out << "\n" << ToText(item.level) << ": " << item.title << " - " << item.detail;
    }
    return out.str();
}

}
