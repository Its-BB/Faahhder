#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class PackEntryType {
    Texture,
    Script,
    Scene,
    Data,
    Unknown
};

struct PackEntryPlan {
    PackEntryType type = PackEntryType::Unknown;
    std::filesystem::path source;
    std::filesystem::path packedPath;
    std::uintmax_t size = 0;
};

struct ExportProfile {
    std::string projectName = "Game";
    std::filesystem::path projectRoot;
    std::filesystem::path outputRoot;
    bool copyLooseAssets = true;
    bool writeManifest = true;
};

class AssetPackPlan {
public:
    void Scan(const std::filesystem::path& assetRoot);
    void Add(PackEntryPlan entry);
    std::vector<PackEntryPlan> Entries() const;
    std::string ManifestText() const;
    std::uintmax_t TotalBytes() const;
private:
    std::vector<PackEntryPlan> entries_;
};

class ExportPlanner {
public:
    explicit ExportPlanner(ExportProfile profile);
    std::vector<std::filesystem::path> OutputFiles() const;
    std::filesystem::path ExecutablePath() const;
    std::filesystem::path ManifestPath() const;
    std::string Report(const AssetPackPlan& plan) const;
private:
    ExportProfile profile_;
};

std::string PackEntryTypeName(PackEntryType type);
PackEntryType PackEntryTypeFromExtension(const std::filesystem::path& path);

}
