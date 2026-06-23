#include "Faahhder/Runtime2D.hpp"

#include <algorithm>
#include <cmath>

namespace faahhder {

Vec2 Camera2DState::ScreenToWorld(Vec2 screen) const {
    const float halfW = viewportWidth * 0.5f;
    const float halfH = viewportHeight * 0.5f;
    return {position.x + (screen.x - halfW) / zoom, position.y + (screen.y - halfH) / zoom};
}

Vec2 Camera2DState::WorldToScreen(Vec2 world) const {
    const float halfW = viewportWidth * 0.5f;
    const float halfH = viewportHeight * 0.5f;
    return {(world.x - position.x) * zoom + halfW, (world.y - position.y) * zoom + halfH};
}

Rect Camera2DState::ViewRect() const {
    const float w = viewportWidth / zoom;
    const float h = viewportHeight / zoom;
    return {{position.x - w * 0.5f, position.y - h * 0.5f}, {position.x + w * 0.5f, position.y + h * 0.5f}};
}

TileLayerData::TileLayerData(int width, int height, int tileSize) {
    Resize(width, height, tileSize);
}

bool TileLayerData::Resize(int width, int height, int tileSize) {
    if (width < 0 || height < 0 || tileSize <= 0) return false;
    width_ = width;
    height_ = height;
    tileSize_ = tileSize;
    cells_.assign(static_cast<std::size_t>(width_ * height_), {});
    return true;
}

bool TileLayerData::Set(int x, int y, TileCell cell) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return false;
    cells_[static_cast<std::size_t>(y * width_ + x)] = cell;
    return true;
}

TileCell TileLayerData::Get(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return {};
    return cells_[static_cast<std::size_t>(y * width_ + x)];
}

std::vector<SpriteBatchCommand> TileLayerData::BuildRenderCommands(const std::string& texture) const {
    std::vector<SpriteBatchCommand> out;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const auto cell = Get(x, y);
            if (cell.tile < 0) continue;
            SpriteBatchCommand cmd;
            cmd.texture = texture;
            cmd.position = {static_cast<float>(x * tileSize_), static_cast<float>(y * tileSize_)};
            cmd.size = {static_cast<float>(tileSize_), static_cast<float>(tileSize_)};
            cmd.layer = 0;
            out.push_back(cmd);
        }
    }
    return out;
}

std::vector<Rect> TileLayerData::SolidRects() const {
    std::vector<Rect> out;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (!Get(x, y).solid) continue;
            const float left = static_cast<float>(x * tileSize_);
            const float top = static_cast<float>(y * tileSize_);
            out.push_back({{left, top}, {left + static_cast<float>(tileSize_), top + static_cast<float>(tileSize_)}});
        }
    }
    return out;
}

bool TileLayerData::IntersectsSolid(Rect rect) const {
    for (const auto& solid : SolidRects()) {
        const bool hit = rect.min.x < solid.max.x && rect.max.x > solid.min.x && rect.min.y < solid.max.y && rect.max.y > solid.min.y;
        if (hit) return true;
    }
    return false;
}

int TileLayerData::Width() const { return width_; }
int TileLayerData::Height() const { return height_; }
int TileLayerData::TileSize() const { return tileSize_; }

void ParticleEmitter2D::Emit(Vec2 position, Vec2 velocity, float life, float size, std::uint32_t color) {
    if (life <= 0.0f || size <= 0.0f) return;
    particles_.push_back({position, velocity, life, life, size, color});
}

void ParticleEmitter2D::Burst(Vec2 center, int count, float speed, float life, float size, std::uint32_t color) {
    if (count <= 0) return;
    constexpr float pi = 3.1415926535f;
    for (int i = 0; i < count; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(count)) * pi * 2.0f;
        Emit(center, {std::cos(angle) * speed, std::sin(angle) * speed}, life, size, color);
    }
}

void ParticleEmitter2D::Update(float dt) {
    for (auto& particle : particles_) {
        particle.position = particle.position + particle.velocity * dt;
        particle.life -= dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const ParticleData& p) {
        return p.life <= 0.0f;
    }), particles_.end());
}

std::vector<SpriteBatchCommand> ParticleEmitter2D::BuildRenderCommands(const std::string& texture) const {
    std::vector<SpriteBatchCommand> out;
    out.reserve(particles_.size());
    for (const auto& particle : particles_) {
        const float t = particle.maxLife <= 0.0f ? 0.0f : particle.life / particle.maxLife;
        SpriteBatchCommand cmd;
        cmd.texture = texture;
        cmd.position = particle.position;
        cmd.size = {particle.size * std::max(0.0f, t), particle.size * std::max(0.0f, t)};
        cmd.tint = particle.color;
        cmd.layer = 100;
        out.push_back(cmd);
    }
    return out;
}

std::size_t ParticleEmitter2D::Count() const { return particles_.size(); }
void ParticleEmitter2D::Clear() { particles_.clear(); }

Camera2DState& Runtime2DWorld::Camera() { return camera_; }
const Camera2DState& Runtime2DWorld::Camera() const { return camera_; }
TileLayerData& Runtime2DWorld::Tiles() { return tiles_; }
ParticleEmitter2D& Runtime2DWorld::Particles() { return particles_; }

void Runtime2DWorld::AddSprite(SpriteBatchCommand command) { sprites_.push_back(std::move(command)); }
void Runtime2DWorld::AddParallax(ParallaxLayerData layer) { parallax_.push_back(std::move(layer)); }
void Runtime2DWorld::AddLight(PointLight2DData light) { lights_.push_back(light); }

std::vector<SpriteBatchCommand> Runtime2DWorld::BuildFrame() const {
    std::vector<SpriteBatchCommand> out;
    for (const auto& layer : parallax_) {
        SpriteBatchCommand cmd;
        cmd.texture = layer.texture;
        cmd.position = layer.offset - Vec2{camera_.position.x * layer.factor.x, camera_.position.y * layer.factor.y};
        cmd.size = {camera_.viewportWidth, camera_.viewportHeight};
        cmd.layer = layer.layer;
        out.push_back(cmd);
    }
    auto tileCommands = tiles_.BuildRenderCommands("tiles");
    out.insert(out.end(), tileCommands.begin(), tileCommands.end());
    out.insert(out.end(), sprites_.begin(), sprites_.end());
    auto particleCommands = particles_.BuildRenderCommands("particle");
    out.insert(out.end(), particleCommands.begin(), particleCommands.end());
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.layer < b.layer; });
    return out;
}

std::vector<PointLight2DData> Runtime2DWorld::Lights() const { return lights_; }

void Runtime2DWorld::ClearFrameCommands() {
    sprites_.clear();
    parallax_.clear();
    lights_.clear();
}

}
