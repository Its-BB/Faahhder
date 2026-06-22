#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>

namespace {

std::vector<std::filesystem::path> CollectFiles(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(root)) {
        return files;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    return files;
}

}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: FaahhderPacker <asset-root> <output-pack>\n";
        return 1;
    }

    const std::filesystem::path root = argv[1];
    const std::filesystem::path output = argv[2];
    if (!output.parent_path().empty()) {
        std::filesystem::create_directories(output.parent_path());
    }

    std::ofstream pack(output, std::ios::binary);
    std::ofstream manifest(output.string() + ".manifest");
    if (!pack || !manifest) {
        std::cerr << "Failed to open output files\n";
        return 2;
    }

    const auto files = CollectFiles(root);
    for (const auto& file : files) {
        const auto relative = std::filesystem::relative(file, root).generic_string();
        std::ifstream input(file, std::ios::binary);
        const auto offset = pack.tellp();
        pack << input.rdbuf();
        const auto size = static_cast<std::uintmax_t>(std::filesystem::file_size(file));
        manifest << relative << "|" << offset << "|" << size << "\n";
    }

    std::cout << "Packed " << files.size() << " assets into " << output << "\n";
    return 0;
}
