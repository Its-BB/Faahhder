#include "Faahhder/Packaging.hpp"

#include <algorithm>
#include <sstream>

namespace faahhder {

std::string PackEntryTypeName(PackEntryType type) {
    switch (type) {
    case PackEntryType::Texture: return "texture";
    case PackEntryType::Script: return "script";
    case PackEntryType::Scene: return "scene";
    case PackEntryType::Data: return "data";
    case PackEntryType::Unknown: return "unknown";
    }
    return "unknown";
}

PackEntryType PackEntryTypeFromExtension(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    if (ext == ".ppm" || ext == ".png" || ext == ".jpg") return PackEntryType::Texture;
    if (ext == ".lua" || ext == ".logic") return PackEntryType::Script;
    if (ext == ".json") return PackEntryType::Scene;
    if (ext == ".faahhder") return PackEntryType::Data;
    return PackEntryType::Unknown;
}

void AssetPackPlan::Scan(const std::filesystem::path& assetRoot) {
    entries_.clear();
    if (!std::filesystem::exists(assetRoot)) return;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetRoot)) {
        if (!entry.is_regular_file()) continue;
        PackEntryPlan plan;
        plan.type = PackEntryTypeFromExtension(entry.path());
        plan.source = entry.path();
        plan.packedPath = std::filesystem::relative(entry.path(), assetRoot);
        plan.size = entry.file_size();
        entries_.push_back(plan);
    }
    std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
        return a.packedPath.generic_string() < b.packedPath.generic_string();
    });
}

void AssetPackPlan::Add(PackEntryPlan entry) {
    entries_.push_back(std::move(entry));
}

std::vector<PackEntryPlan> AssetPackPlan::Entries() const {
    return entries_;
}

std::string AssetPackPlan::ManifestText() const {
    std::ostringstream out;
    for (const auto& entry : entries_) {
        out << PackEntryTypeName(entry.type) << "|" << entry.packedPath.generic_string() << "|" << entry.size << "\n";
    }
    return out.str();
}

std::uintmax_t AssetPackPlan::TotalBytes() const {
    std::uintmax_t total = 0;
    for (const auto& entry : entries_) total += entry.size;
    return total;
}

ExportPlanner::ExportPlanner(ExportProfile profile) : profile_(std::move(profile)) {}

std::vector<std::filesystem::path> ExportPlanner::OutputFiles() const {
    return {ExecutablePath(), ManifestPath(), profile_.outputRoot / "assets"};
}

std::filesystem::path ExportPlanner::ExecutablePath() const {
    return profile_.outputRoot / (profile_.projectName + ".exe");
}

std::filesystem::path ExportPlanner::ManifestPath() const {
    return profile_.outputRoot / "assets" / "manifest.faahhder";
}

std::string ExportPlanner::Report(const AssetPackPlan& plan) const {
    std::ostringstream out;
    out << "Project: " << profile_.projectName << "\n";
    out << "Output: " << profile_.outputRoot.string() << "\n";
    out << "Executable: " << ExecutablePath().filename().string() << "\n";
    out << "Entries: " << plan.Entries().size() << "\n";
    out << "Bytes: " << plan.TotalBytes() << "\n";
    out << "Loose assets: " << (profile_.copyLooseAssets ? "yes" : "no") << "\n";
    return out.str();
}

}
