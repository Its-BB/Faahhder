#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

namespace faahhder {

struct ExportSummaryItem {
    std::filesystem::path path;
    std::uintmax_t bytes = 0;
    bool directory = false;
};

struct ExportSummary {
    std::filesystem::path outputRoot;
    std::vector<ExportSummaryItem> items;
    std::uintmax_t totalBytes = 0;
    int executableCount = 0;
    int assetCount = 0;
};

ExportSummary BuildExportSummary(const std::filesystem::path& outputRoot);
std::string FormatExportSummary(const ExportSummary& summary);

}
