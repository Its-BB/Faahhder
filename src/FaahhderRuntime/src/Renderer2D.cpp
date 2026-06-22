#include "Faahhder/Renderer2D.hpp"

#include <algorithm>

namespace faahhder {

void Renderer2D::BeginScene(const Camera2D& camera, Transform2D cameraTransform) {
    m_camera = camera;
    m_cameraTransform = cameraTransform;
    m_commands.clear();
}

void Renderer2D::DrawScene(const Scene& scene) {
    for (auto entity : scene.Entities()) {
        const auto* transform = scene.GetTransform(entity);
        if (!transform) {
            continue;
        }
        if (const auto* sprite = scene.GetSpriteRenderer(entity)) {
            DrawSprite(entity, *transform, *sprite);
        }
        if (const auto* tilemap = scene.GetTilemap(entity)) {
            DrawTilemap(entity, *transform, *tilemap);
        }
        if (const auto* emitter = scene.GetParticleEmitter(entity)) {
            for (const auto& particle : emitter->particles) {
                m_commands.push_back({"particle", entity, emitter->texture, particle.position, {8.0f, 8.0f}, particle.color, 100});
            }
        }
        if (const auto* light = scene.GetPointLight(entity)) {
            m_commands.push_back({"light", entity, {}, transform->position, {light->radius, light->radius}, light->color, 1000});
        }
    }
}

void Renderer2D::DrawSprite(EntityId entity, const Transform2D& transform, const SpriteRenderer& sprite) {
    m_commands.push_back({"sprite", entity, sprite.texture, transform.position, transform.scale, sprite.color, sprite.sortingLayer});
}

void Renderer2D::DrawTilemap(EntityId entity, const Transform2D& transform, const Tilemap& tilemap) {
    for (int y = 0; y < tilemap.height; ++y) {
        for (int x = 0; x < tilemap.width; ++x) {
            const int index = y * tilemap.width + x;
            if (index >= static_cast<int>(tilemap.tiles.size()) || tilemap.tiles[index] < 0) {
                continue;
            }
            m_commands.push_back({"tile", entity, tilemap.spriteSheet, {transform.position.x + x * tilemap.tileSize, transform.position.y + y * tilemap.tileSize}, {static_cast<float>(tilemap.tileSize), static_cast<float>(tilemap.tileSize)}, {}, 0});
        }
    }
}

void Renderer2D::UpdateParticles(Scene& scene, float dt) {
    for (auto entity : scene.Entities()) {
        auto* emitter = scene.GetParticleEmitter(entity);
        auto* transform = scene.GetTransform(entity);
        if (!emitter || !transform || !emitter->playing) {
            continue;
        }

        emitter->accumulator += emitter->emissionRate * dt;
        while (emitter->accumulator >= 1.0f) {
            emitter->particles.push_back({transform->position, emitter->initialVelocity, emitter->particleLifetime, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}});
            emitter->accumulator -= 1.0f;
        }

        for (auto& particle : emitter->particles) {
            particle.age += dt;
            particle.position = particle.position + particle.velocity * dt;
            particle.color.a = std::max(0.0f, 1.0f - particle.age / particle.lifetime);
        }
        emitter->particles.erase(std::remove_if(emitter->particles.begin(), emitter->particles.end(), [](const Particle& particle) {
            return particle.age >= particle.lifetime;
        }), emitter->particles.end());
    }
}

void Renderer2D::EndScene() {
    std::sort(m_commands.begin(), m_commands.end(), [](const RenderCommand& lhs, const RenderCommand& rhs) {
        return lhs.sortingLayer < rhs.sortingLayer;
    });
}

void Renderer2D::Clear() {
    m_commands.clear();
}

const std::vector<RenderCommand>& Renderer2D::Commands() const {
    return m_commands;
}

}

