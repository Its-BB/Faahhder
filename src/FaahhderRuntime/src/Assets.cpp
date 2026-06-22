#include "Faahhder/Assets.hpp"

#include <fstream>
#include <sstream>
#include <utility>

namespace faahhder {
namespace {

std::unordered_map<AssetId, Texture> g_textures;
std::unordered_map<AssetId, SpriteSheet> g_spriteSheets;

std::vector<std::string> Split(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(line);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

void TryReadPpmSize(const std::filesystem::path& path, int& width, int& height) {
    if (width > 0 && height > 0) {
        return;
    }
    std::ifstream file(path);
    std::string magic;
    if (file >> magic && magic == "P3") {
        file >> width >> height;
    }
}

}

Texture& Assets::LoadTexture(const AssetId& id, const std::filesystem::path& path, int width, int height) {
    TryReadPpmSize(path, width, height);
    auto [it, inserted] = g_textures.emplace(id, Texture{});
    it->second = Texture{id, path, width, height};
    return it->second;
}

SpriteSheet& Assets::LoadSpriteSheet(const AssetId& id, const AssetId& texture, int frameWidth, int frameHeight, int columns, int rows) {
    SpriteSheet sheet;
    sheet.id = id;
    sheet.texture = texture;
    sheet.frameWidth = frameWidth;
    sheet.frameHeight = frameHeight;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < columns; ++x) {
            sheet.frames.push_back({x * frameWidth, y * frameHeight, frameWidth, frameHeight});
        }
    }

    auto [it, inserted] = g_spriteSheets.emplace(id, SpriteSheet{});
    it->second = std::move(sheet);
    return it->second;
}

bool Assets::LoadManifest(const std::filesystem::path& manifestPath) {
    std::ifstream file(manifestPath);
    if (!file) {
        return false;
    }
    const auto root = manifestPath.parent_path();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto parts = Split(line, '|');
        if (parts.size() >= 3 && parts[0] == "texture") {
            LoadTexture(parts[1], root / parts[2]);
        } else if (parts.size() >= 7 && parts[0] == "spritesheet") {
            LoadSpriteSheet(parts[1], parts[2], std::stoi(parts[3]), std::stoi(parts[4]), std::stoi(parts[5]), std::stoi(parts[6]));
        }
    }
    return true;
}

const Texture* Assets::GetTexture(const AssetId& id) {
    auto it = g_textures.find(id);
    return it == g_textures.end() ? nullptr : &it->second;
}

std::optional<AssetRef> Assets::Get(const AssetId& id) {
    if (const auto* texture = GetTexture(id)) {
        return AssetRef{AssetKind::Texture, texture->id, texture->path};
    }
    if (const auto* sheet = GetSpriteSheet(id)) {
        return AssetRef{AssetKind::SpriteSheet, sheet->id, {}};
    }
    return std::nullopt;
}

const SpriteSheet* Assets::GetSpriteSheet(const AssetId& id) {
    auto it = g_spriteSheets.find(id);
    return it == g_spriteSheets.end() ? nullptr : &it->second;
}

std::vector<AssetId> Assets::ListTextures() {
    std::vector<AssetId> ids;
    for (const auto& [id, texture] : g_textures) {
        ids.push_back(id);
    }
    return ids;
}

void Assets::Clear() {
    g_textures.clear();
    g_spriteSheets.clear();
}

}
