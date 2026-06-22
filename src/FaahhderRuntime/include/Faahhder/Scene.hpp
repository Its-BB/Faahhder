#pragma once

#include "Faahhder/Components.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace faahhder {

using EntityId = std::uint32_t;
constexpr EntityId InvalidEntity = 0;

struct EntityInfo {
    EntityId id = InvalidEntity;
    std::string name;
    EntityId parent = InvalidEntity;
    std::vector<EntityId> children;
    bool active = true;
};

class Scene {
public:
    EntityId CreateEntity(std::string name = "Entity");
    void DestroyEntity(EntityId entity);
    EntityId FindEntityByName(const std::string& name) const;
    bool IsValid(EntityId entity) const;

    EntityInfo* GetInfo(EntityId entity);
    const EntityInfo* GetInfo(EntityId entity) const;
    std::vector<EntityId> Entities() const;

    Transform2D& AddTransform(EntityId entity, Transform2D component = {});
    SpriteRenderer& AddSpriteRenderer(EntityId entity, SpriteRenderer component = {});
    Camera2D& AddCamera(EntityId entity, Camera2D component = {});
    Tilemap& AddTilemap(EntityId entity, Tilemap component = {});
    ParticleEmitter& AddParticleEmitter(EntityId entity, ParticleEmitter component = {});
    PointLight2D& AddPointLight(EntityId entity, PointLight2D component = {});
    Rigidbody2D& AddRigidbody(EntityId entity, Rigidbody2D component = {});
    Collider2D& AddCollider(EntityId entity, Collider2D component = {});
    LuaScript& AddLuaScript(EntityId entity, LuaScript component = {});

    Transform2D* GetTransform(EntityId entity);
    SpriteRenderer* GetSpriteRenderer(EntityId entity);
    Camera2D* GetCamera(EntityId entity);
    Tilemap* GetTilemap(EntityId entity);
    ParticleEmitter* GetParticleEmitter(EntityId entity);
    PointLight2D* GetPointLight(EntityId entity);
    Rigidbody2D* GetRigidbody(EntityId entity);
    Collider2D* GetCollider(EntityId entity);
    LuaScript* GetLuaScript(EntityId entity);

    const Transform2D* GetTransform(EntityId entity) const;
    const SpriteRenderer* GetSpriteRenderer(EntityId entity) const;
    const Camera2D* GetCamera(EntityId entity) const;
    const Tilemap* GetTilemap(EntityId entity) const;
    const ParticleEmitter* GetParticleEmitter(EntityId entity) const;
    const PointLight2D* GetPointLight(EntityId entity) const;
    const Rigidbody2D* GetRigidbody(EntityId entity) const;
    const Collider2D* GetCollider(EntityId entity) const;
    const LuaScript* GetLuaScript(EntityId entity) const;

    Camera2D* PrimaryCamera();
    Transform2D* PrimaryCameraTransform();
    void Update(float dt);

    bool SaveJson(const std::filesystem::path& path) const;
    bool LoadJson(const std::filesystem::path& path);
    std::string SerializeJson() const;
    bool DeserializeJson(const std::string& json);

private:
    EntityId m_nextEntity = 1;
    std::unordered_map<EntityId, EntityInfo> m_entities;
    std::unordered_map<EntityId, Transform2D> m_transforms;
    std::unordered_map<EntityId, SpriteRenderer> m_sprites;
    std::unordered_map<EntityId, Camera2D> m_cameras;
    std::unordered_map<EntityId, Tilemap> m_tilemaps;
    std::unordered_map<EntityId, ParticleEmitter> m_emitters;
    std::unordered_map<EntityId, PointLight2D> m_lights;
    std::unordered_map<EntityId, Rigidbody2D> m_rigidbodies;
    std::unordered_map<EntityId, Collider2D> m_colliders;
    std::unordered_map<EntityId, LuaScript> m_scripts;
};

}

