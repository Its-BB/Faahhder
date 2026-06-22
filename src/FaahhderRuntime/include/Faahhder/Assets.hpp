#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace faahhder {

using AssetId = std::string;

struct Texture {
    AssetId id;
    std::filesystem::path path;
    int width = 0;
    int height = 0;
};

struct SpriteFrame {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct SpriteSheet {
    AssetId id;
    AssetId texture;
    int frameWidth = 0;
    int frameHeight = 0;
    std::vector<SpriteFrame> frames;
};

enum class AssetKind {
    Unknown,
    Texture,
    SpriteSheet
};

struct AssetRef {
    AssetKind kind = AssetKind::Unknown;
    AssetId id;
    std::filesystem::path path;
};

class Assets {
public:
    static Texture& LoadTexture(const AssetId& id, const std::filesystem::path& path, int width = 0, int height = 0);
    static SpriteSheet& LoadSpriteSheet(const AssetId& id, const AssetId& texture, int frameWidth, int frameHeight, int columns, int rows);
    static bool LoadManifest(const std::filesystem::path& manifestPath);

    static std::optional<AssetRef> Get(const AssetId& id);
    static const Texture* GetTexture(const AssetId& id);
    static const SpriteSheet* GetSpriteSheet(const AssetId& id);
    static std::vector<AssetId> ListTextures();
    static void Clear();
};

}
