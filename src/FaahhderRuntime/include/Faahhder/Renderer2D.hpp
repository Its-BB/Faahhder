#pragma once

#include "Faahhder/Components.hpp"
#include "Faahhder/Scene.hpp"

#include <string>
#include <vector>

namespace faahhder {

struct RenderCommand {
    std::string kind;
    EntityId entity = InvalidEntity;
    AssetId texture;
    Vec2 position{};
    Vec2 size{1.0f, 1.0f};
    Color color{};
    int sortingLayer = 0;
};

class Renderer2D {
public:
    void BeginScene(const Camera2D& camera, Transform2D cameraTransform);
    void DrawScene(const Scene& scene);
    void DrawSprite(EntityId entity, const Transform2D& transform, const SpriteRenderer& sprite);
    void DrawTilemap(EntityId entity, const Transform2D& transform, const Tilemap& tilemap);
    void UpdateParticles(Scene& scene, float dt);
    void EndScene();
    void Clear();

    const std::vector<RenderCommand>& Commands() const;
    Color ambientLight{0.18f, 0.18f, 0.22f, 1.0f};

private:
    Camera2D m_camera;
    Transform2D m_cameraTransform;
    std::vector<RenderCommand> m_commands;
};

}

