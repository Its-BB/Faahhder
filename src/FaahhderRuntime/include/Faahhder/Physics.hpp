#pragma once

#include "Faahhder/Components.hpp"
#include "Faahhder/Scene.hpp"

#include <optional>

namespace faahhder {

struct RaycastHit2D {
    EntityId entity = InvalidEntity;
    Vec2 point{};
    Vec2 normal{};
    float distance = 0.0f;
};

class Physics2D {
public:
    static bool IntersectsAABB(Rect a, Rect b);
    static bool IntersectsCircle(Vec2 centerA, float radiusA, Vec2 centerB, float radiusB);
    static Rect ColliderBounds(const Transform2D& transform, const Collider2D& collider);
    static void Step(Scene& scene, float dt);
    static std::optional<RaycastHit2D> Raycast(const Scene& scene, Vec2 origin, Vec2 direction, float distance);
    static void GenerateTileColliders(Scene& scene, EntityId tilemapEntity);
};

}

