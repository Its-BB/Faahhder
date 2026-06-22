#pragma once

#include "Faahhder/Assets.hpp"
#include "Faahhder/Math.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class BodyType {
    Static,
    Dynamic,
    Kinematic
};

enum class ColliderShape {
    AABB,
    Circle
};

struct Transform2D {
    Vec2 position{};
    float rotation = 0.0f;
    Vec2 scale{1.0f, 1.0f};
};

struct SpriteRenderer {
    AssetId texture;
    AssetId spriteSheet;
    int frame = 0;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    int sortingLayer = 0;
};

struct Camera2D {
    float zoom = 1.0f;
    Vec2 viewport{1280.0f, 720.0f};
    bool primary = false;
};

struct ParallaxLayer {
    AssetId texture;
    Vec2 factor{0.5f, 0.5f};
    Vec2 offset{};
};

struct Tilemap {
    int width = 0;
    int height = 0;
    int tileSize = 32;
    AssetId spriteSheet;
    std::vector<int> tiles;
    std::vector<int> solidTiles;
};

struct Particle {
    Vec2 position{};
    Vec2 velocity{};
    float lifetime = 0.0f;
    float age = 0.0f;
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct ParticleEmitter {
    AssetId texture;
    float emissionRate = 16.0f;
    float particleLifetime = 1.0f;
    Vec2 initialVelocity{0.0f, 40.0f};
    bool playing = true;
    std::vector<Particle> particles;
    float accumulator = 0.0f;
};

struct PointLight2D {
    Color color{1.0f, 0.92f, 0.75f, 1.0f};
    float radius = 160.0f;
    float intensity = 1.0f;
};

struct Rigidbody2D {
    BodyType type = BodyType::Dynamic;
    Vec2 velocity{};
    float gravityScale = 1.0f;
    float mass = 1.0f;
    bool fixedRotation = false;
};

struct Collider2D {
    ColliderShape shape = ColliderShape::AABB;
    Vec2 offset{};
    Vec2 size{1.0f, 1.0f};
    float radius = 0.5f;
    bool isTrigger = false;
};

struct LuaScript {
    std::filesystem::path path;
    bool hotReload = true;
    std::filesystem::file_time_type lastWriteTime{};
};

}

