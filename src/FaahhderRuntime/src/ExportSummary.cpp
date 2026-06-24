#include "Faahhder/ExportSummary.hpp"

#include <algorithm>
#include <sstream>

namespace faahhder {

ExportSummary BuildExportSummary(const std::filesystem::path& outputRoot) {
    ExportSummary summary;
    summary.outputRoot = outputRoot;
    if (!std::filesystem::exists(outputRoot)) return summary;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(outputRoot)) {
        ExportSummaryItem item;
        item.path = entry.path().lexically_relative(outputRoot);
        item.directory = entry.is_directory();
        if (!item.directory) {
            item.bytes = entry.file_size();
            summary.totalBytes += item.bytes;
            const auto extension = entry.path().extension().string();
            if (extension == ".exe") summary.executableCount++;
            if (entry.path().string().find("assets") != std::string::npos) summary.assetCount++;
        }
        summary.items.push_back(std::move(item));
    }
    std::sort(summary.items.begin(), summary.items.end(), [](const ExportSummaryItem& a, const ExportSummaryItem& b) {
        return a.path.generic_string() < b.path.generic_string();
    });
    return summary;
}

std::string FormatExportSummary(const ExportSummary& summary) {
    std::ostringstream out;
    out << "output=" << summary.outputRoot.generic_string() << "\n";
    out << "files=" << summary.items.size() << "\n";
    out << "bytes=" << summary.totalBytes << "\n";
    out << "executables=" << summary.executableCount << "\n";
    out << "assets=" << summary.assetCount;
    for (const auto& item : summary.items) {
        out << "\n" << (item.directory ? "dir " : "file ") << item.path.generic_string();
        if (!item.directory) out << " " << item.bytes;
    }
    return out.str();
}

}
