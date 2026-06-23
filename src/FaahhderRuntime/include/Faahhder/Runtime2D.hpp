#pragma once

#include "Faahhder/Math.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace faahhder {

struct Camera2DState {
    Vec2 position;
    float zoom = 1.0f;
    float viewportWidth = 1280.0f;
    float viewportHeight = 720.0f;
    Vec2 ScreenToWorld(Vec2 screen) const;
    Vec2 WorldToScreen(Vec2 world) const;
    Rect ViewRect() const;
};

struct SpriteBatchCommand {
    std::string texture;
    Vec2 position;
    Vec2 size = {32.0f, 32.0f};
    float rotation = 0.0f;
    std::uint32_t tint = 0xffffffffu;
    int layer = 0;
};

struct TileCell {
    int tile = -1;
    bool solid = false;
};

class TileLayerData {
public:
    TileLayerData() = default;
    TileLayerData(int width, int height, int tileSize);
    bool Resize(int width, int height, int tileSize);
    bool Set(int x, int y, TileCell cell);
    TileCell Get(int x, int y) const;
    std::vector<SpriteBatchCommand> BuildRenderCommands(const std::string& texture) const;
    std::vector<Rect> SolidRects() const;
    bool IntersectsSolid(Rect rect) const;
    int Width() const;
    int Height() const;
    int TileSize() const;
private:
    int width_ = 0;
    int height_ = 0;
    int tileSize_ = 32;
    std::vector<TileCell> cells_;
};

struct ParallaxLayerData {
    std::string texture;
    Vec2 offset;
    Vec2 factor = {1.0f, 1.0f};
    int layer = -10;
};

struct ParticleData {
    Vec2 position;
    Vec2 velocity;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 4.0f;
    std::uint32_t color = 0xffffffffu;
};

class ParticleEmitter2D {
public:
    void Emit(Vec2 position, Vec2 velocity, float life, float size, std::uint32_t color);
    void Burst(Vec2 center, int count, float speed, float life, float size, std::uint32_t color);
    void Update(float dt);
    std::vector<SpriteBatchCommand> BuildRenderCommands(const std::string& texture) const;
    std::size_t Count() const;
    void Clear();
private:
    std::vector<ParticleData> particles_;
};

struct PointLight2DData {
    Vec2 position;
    float radius = 128.0f;
    float intensity = 1.0f;
    std::uint32_t color = 0xffffffffu;
};

class Runtime2DWorld {
public:
    Camera2DState& Camera();
    const Camera2DState& Camera() const;
    TileLayerData& Tiles();
    ParticleEmitter2D& Particles();
    void AddSprite(SpriteBatchCommand command);
    void AddParallax(ParallaxLayerData layer);
    void AddLight(PointLight2DData light);
    std::vector<SpriteBatchCommand> BuildFrame() const;
    std::vector<PointLight2DData> Lights() const;
    void ClearFrameCommands();
private:
    Camera2DState camera_;
    TileLayerData tiles_;
    ParticleEmitter2D particles_;
    std::vector<SpriteBatchCommand> sprites_;
    std::vector<ParallaxLayerData> parallax_;
    std::vector<PointLight2DData> lights_;
};

}
