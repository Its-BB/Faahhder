#include "Faahhder/Physics.hpp"

#include "Faahhder/Event.hpp"

#include <algorithm>
#include <limits>

namespace faahhder {

bool Physics2D::IntersectsAABB(Rect a, Rect b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y;
}

bool Physics2D::IntersectsCircle(Vec2 centerA, float radiusA, Vec2 centerB, float radiusB) {
    const auto delta = centerA - centerB;
    const float radius = radiusA + radiusB;
    return Dot(delta, delta) <= radius * radius;
}

Rect Physics2D::ColliderBounds(const Transform2D& transform, const Collider2D& collider) {
    const Vec2 center = transform.position + collider.offset;
    const Vec2 half = collider.size * 0.5f;
    return {center - half, center + half};
}

void Physics2D::Step(Scene& scene, float dt) {
    constexpr Vec2 gravity{0.0f, -980.0f};
    for (auto entity : scene.Entities()) {
        auto* body = scene.GetRigidbody(entity);
        auto* transform = scene.GetTransform(entity);
        if (!body || !transform || body->type != BodyType::Dynamic) {
            continue;
        }
        body->velocity = body->velocity + gravity * (body->gravityScale * dt);
        transform->position = transform->position + body->velocity * dt;
    }

    const auto entities = scene.Entities();
    for (size_t i = 0; i < entities.size(); ++i) {
        for (size_t j = i + 1; j < entities.size(); ++j) {
            auto a = entities[i];
            auto b = entities[j];
            const auto* ta = scene.GetTransform(a);
            const auto* tb = scene.GetTransform(b);
            const auto* ca = scene.GetCollider(a);
            const auto* cb = scene.GetCollider(b);
            if (!ta || !tb || !ca || !cb) {
                continue;
            }

            bool intersects = false;
            if (ca->shape == ColliderShape::Circle && cb->shape == ColliderShape::Circle) {
                intersects = IntersectsCircle(ta->position + ca->offset, ca->radius, tb->position + cb->offset, cb->radius);
            } else {
                intersects = IntersectsAABB(ColliderBounds(*ta, *ca), ColliderBounds(*tb, *cb));
            }

            if (intersects) {
                const bool trigger = ca->isTrigger || cb->isTrigger;
                Events::Emit({trigger ? "TriggerEnter" : "CollisionEnter", static_cast<int>(a), std::to_string(b)});
                Events::Emit({trigger ? "TriggerEnter" : "CollisionEnter", static_cast<int>(b), std::to_string(a)});
            }
        }
    }
}

std::optional<RaycastHit2D> Physics2D::Raycast(const Scene& scene, Vec2 origin, Vec2 direction, float distance) {
    const float length = Length(direction);
    if (length <= 0.0001f || distance <= 0.0f) {
        return std::nullopt;
    }

    direction = direction * (1.0f / length);
    std::optional<RaycastHit2D> best;

    for (auto entity : scene.Entities()) {
        const auto* transform = scene.GetTransform(entity);
        const auto* collider = scene.GetCollider(entity);
        if (!transform || !collider) {
            continue;
        }

        const Rect bounds = ColliderBounds(*transform, *collider);
        float tMin = 0.0f;
        float tMax = distance;

        const auto testAxis = [&](float originValue, float directionValue, float minValue, float maxValue, float& inOutMin, float& inOutMax) {
            if (std::fabs(directionValue) < 0.0001f) {
                return originValue >= minValue && originValue <= maxValue;
            }
            const float inv = 1.0f / directionValue;
            float t1 = (minValue - originValue) * inv;
            float t2 = (maxValue - originValue) * inv;
            if (t1 > t2) {
                std::swap(t1, t2);
            }
            inOutMin = std::max(inOutMin, t1);
            inOutMax = std::min(inOutMax, t2);
            return inOutMin <= inOutMax;
        };

        if (!testAxis(origin.x, direction.x, bounds.min.x, bounds.max.x, tMin, tMax)) {
            continue;
        }
        if (!testAxis(origin.y, direction.y, bounds.min.y, bounds.max.y, tMin, tMax)) {
            continue;
        }
        if (tMin < 0.0f || tMin > distance) {
            continue;
        }
        if (!best || tMin < best->distance) {
            best = RaycastHit2D{entity, origin + direction * tMin, {}, tMin};
        }
    }

    return best;
}

void Physics2D::GenerateTileColliders(Scene& scene, EntityId tilemapEntity) {
    const auto* tilemap = scene.GetTilemap(tilemapEntity);
    const auto* transform = scene.GetTransform(tilemapEntity);
    if (!tilemap || !transform) {
        return;
    }

    for (int y = 0; y < tilemap->height; ++y) {
        for (int x = 0; x < tilemap->width; ++x) {
            const int index = y * tilemap->width + x;
            if (index >= static_cast<int>(tilemap->tiles.size())) {
                continue;
            }
            const int tile = tilemap->tiles[index];
            if (std::find(tilemap->solidTiles.begin(), tilemap->solidTiles.end(), tile) == tilemap->solidTiles.end()) {
                continue;
            }
            auto colliderEntity = scene.CreateEntity("Tile Collider");
            scene.AddTransform(colliderEntity, {{transform->position.x + x * tilemap->tileSize, transform->position.y + y * tilemap->tileSize}, 0.0f, {1.0f, 1.0f}});
            scene.AddCollider(colliderEntity, {ColliderShape::AABB, {}, {static_cast<float>(tilemap->tileSize), static_cast<float>(tilemap->tileSize)}, 0.0f, false});
        }
    }
}

}

