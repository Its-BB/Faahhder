#include "Faahhder/Scene.hpp"

#include "Faahhder/Physics.hpp"
#include "Faahhder/Scripting.hpp"

#include <fstream>
#include <sstream>
#include <utility>

namespace faahhder {
namespace {

std::string Escape(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

std::string ExtractString(const std::string& source, const std::string& key, size_t start) {
    const auto keyPos = source.find("\"" + key + "\"", start);
    if (keyPos == std::string::npos) {
        return {};
    }
    const auto colon = source.find(':', keyPos);
    const auto firstQuote = source.find('"', colon + 1);
    const auto secondQuote = source.find('"', firstQuote + 1);
    if (firstQuote == std::string::npos || secondQuote == std::string::npos) {
        return {};
    }
    return source.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

float ExtractNumber(const std::string& source, const std::string& key, size_t start, float fallback = 0.0f) {
    const auto keyPos = source.find("\"" + key + "\"", start);
    if (keyPos == std::string::npos) {
        return fallback;
    }
    const auto colon = source.find(':', keyPos);
    if (colon == std::string::npos) {
        return fallback;
    }
    const auto end = source.find_first_of(",}\n", colon + 1);
    return std::stof(source.substr(colon + 1, end - colon - 1));
}

bool ExtractBool(const std::string& source, const std::string& key, size_t start, size_t end, bool fallback = false) {
    const auto keyPos = source.find("\"" + key + "\"", start);
    if (keyPos == std::string::npos || keyPos > end) {
        return fallback;
    }
    const auto colon = source.find(':', keyPos);
    if (colon == std::string::npos || colon > end) {
        return fallback;
    }
    const auto valuePos = source.find_first_not_of(" \t\r\n", colon + 1);
    return valuePos != std::string::npos && source.compare(valuePos, 4, "true") == 0;
}

}

EntityId Scene::CreateEntity(std::string name) {
    const EntityId id = m_nextEntity++;
    m_entities[id] = {id, std::move(name)};
    AddTransform(id);
    return id;
}

void Scene::DestroyEntity(EntityId entity) {
    m_entities.erase(entity);
    m_transforms.erase(entity);
    m_sprites.erase(entity);
    m_cameras.erase(entity);
    m_tilemaps.erase(entity);
    m_emitters.erase(entity);
    m_lights.erase(entity);
    m_rigidbodies.erase(entity);
    m_colliders.erase(entity);
    m_scripts.erase(entity);
}

EntityId Scene::FindEntityByName(const std::string& name) const {
    for (const auto& [id, info] : m_entities) {
        if (info.name == name) {
            return id;
        }
    }
    return InvalidEntity;
}

bool Scene::IsValid(EntityId entity) const {
    return m_entities.count(entity) > 0;
}

EntityInfo* Scene::GetInfo(EntityId entity) {
    auto it = m_entities.find(entity);
    return it == m_entities.end() ? nullptr : &it->second;
}

const EntityInfo* Scene::GetInfo(EntityId entity) const {
    auto it = m_entities.find(entity);
    return it == m_entities.end() ? nullptr : &it->second;
}

std::vector<EntityId> Scene::Entities() const {
    std::vector<EntityId> entities;
    for (const auto& [id, info] : m_entities) {
        if (info.active) {
            entities.push_back(id);
        }
    }
    return entities;
}

Transform2D& Scene::AddTransform(EntityId entity, Transform2D component) { return m_transforms[entity] = component; }
SpriteRenderer& Scene::AddSpriteRenderer(EntityId entity, SpriteRenderer component) { return m_sprites[entity] = std::move(component); }
Camera2D& Scene::AddCamera(EntityId entity, Camera2D component) { return m_cameras[entity] = component; }
Tilemap& Scene::AddTilemap(EntityId entity, Tilemap component) { return m_tilemaps[entity] = std::move(component); }
ParticleEmitter& Scene::AddParticleEmitter(EntityId entity, ParticleEmitter component) { return m_emitters[entity] = std::move(component); }
PointLight2D& Scene::AddPointLight(EntityId entity, PointLight2D component) { return m_lights[entity] = component; }
Rigidbody2D& Scene::AddRigidbody(EntityId entity, Rigidbody2D component) { return m_rigidbodies[entity] = component; }
Collider2D& Scene::AddCollider(EntityId entity, Collider2D component) { return m_colliders[entity] = component; }
LuaScript& Scene::AddLuaScript(EntityId entity, LuaScript component) { return m_scripts[entity] = std::move(component); }

Transform2D* Scene::GetTransform(EntityId entity) { auto it = m_transforms.find(entity); return it == m_transforms.end() ? nullptr : &it->second; }
SpriteRenderer* Scene::GetSpriteRenderer(EntityId entity) { auto it = m_sprites.find(entity); return it == m_sprites.end() ? nullptr : &it->second; }
Camera2D* Scene::GetCamera(EntityId entity) { auto it = m_cameras.find(entity); return it == m_cameras.end() ? nullptr : &it->second; }
Tilemap* Scene::GetTilemap(EntityId entity) { auto it = m_tilemaps.find(entity); return it == m_tilemaps.end() ? nullptr : &it->second; }
ParticleEmitter* Scene::GetParticleEmitter(EntityId entity) { auto it = m_emitters.find(entity); return it == m_emitters.end() ? nullptr : &it->second; }
PointLight2D* Scene::GetPointLight(EntityId entity) { auto it = m_lights.find(entity); return it == m_lights.end() ? nullptr : &it->second; }
Rigidbody2D* Scene::GetRigidbody(EntityId entity) { auto it = m_rigidbodies.find(entity); return it == m_rigidbodies.end() ? nullptr : &it->second; }
Collider2D* Scene::GetCollider(EntityId entity) { auto it = m_colliders.find(entity); return it == m_colliders.end() ? nullptr : &it->second; }
LuaScript* Scene::GetLuaScript(EntityId entity) { auto it = m_scripts.find(entity); return it == m_scripts.end() ? nullptr : &it->second; }

const Transform2D* Scene::GetTransform(EntityId entity) const { auto it = m_transforms.find(entity); return it == m_transforms.end() ? nullptr : &it->second; }
const SpriteRenderer* Scene::GetSpriteRenderer(EntityId entity) const { auto it = m_sprites.find(entity); return it == m_sprites.end() ? nullptr : &it->second; }
const Camera2D* Scene::GetCamera(EntityId entity) const { auto it = m_cameras.find(entity); return it == m_cameras.end() ? nullptr : &it->second; }
const Tilemap* Scene::GetTilemap(EntityId entity) const { auto it = m_tilemaps.find(entity); return it == m_tilemaps.end() ? nullptr : &it->second; }
const ParticleEmitter* Scene::GetParticleEmitter(EntityId entity) const { auto it = m_emitters.find(entity); return it == m_emitters.end() ? nullptr : &it->second; }
const PointLight2D* Scene::GetPointLight(EntityId entity) const { auto it = m_lights.find(entity); return it == m_lights.end() ? nullptr : &it->second; }
const Rigidbody2D* Scene::GetRigidbody(EntityId entity) const { auto it = m_rigidbodies.find(entity); return it == m_rigidbodies.end() ? nullptr : &it->second; }
const Collider2D* Scene::GetCollider(EntityId entity) const { auto it = m_colliders.find(entity); return it == m_colliders.end() ? nullptr : &it->second; }
const LuaScript* Scene::GetLuaScript(EntityId entity) const { auto it = m_scripts.find(entity); return it == m_scripts.end() ? nullptr : &it->second; }

Camera2D* Scene::PrimaryCamera() {
    for (auto& [id, camera] : m_cameras) {
        if (camera.primary) {
            return &camera;
        }
    }
    return nullptr;
}

Transform2D* Scene::PrimaryCameraTransform() {
    for (auto& [id, camera] : m_cameras) {
        if (camera.primary) {
            return GetTransform(id);
        }
    }
    return nullptr;
}

void Scene::Update(float dt) {
    static ScriptingEngine scripting;
    scripting.BindRuntimeApi(this);
    scripting.HotReload(*this);
    for (auto entity : Entities()) {
        if (GetLuaScript(entity)) {
            scripting.OnUpdate(*this, entity, dt);
        }
    }
    Physics2D::Step(*this, dt);
}

bool Scene::SaveJson(const std::filesystem::path& path) const {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
        return false;
    }
    file << SerializeJson();
    return true;
}

bool Scene::LoadJson(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return DeserializeJson(buffer.str());
}

std::string Scene::SerializeJson() const {
    std::ostringstream out;
    out << "{\n  \"entities\": [\n";
    bool first = true;
    for (const auto& [id, info] : m_entities) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        const auto* t = GetTransform(id);
        out << "    {\"id\":" << id << ",\"name\":\"" << Escape(info.name) << "\"";
        if (t) {
            out << ",\"x\":" << t->position.x << ",\"y\":" << t->position.y << ",\"rotation\":" << t->rotation
                << ",\"sx\":" << t->scale.x << ",\"sy\":" << t->scale.y;
        }
        if (const auto* sprite = GetSpriteRenderer(id)) {
            out << ",\"texture\":\"" << Escape(sprite->texture) << "\",\"sheet\":\"" << Escape(sprite->spriteSheet) << "\",\"frame\":" << sprite->frame;
        }
        if (const auto* camera = GetCamera(id)) {
            out << ",\"camera\":true,\"primary\":" << (camera->primary ? "true" : "false") << ",\"zoom\":" << camera->zoom;
        }
        if (const auto* body = GetRigidbody(id)) {
            out << ",\"rigidbody\":true,\"vx\":" << body->velocity.x << ",\"vy\":" << body->velocity.y
                << ",\"gravity\":" << body->gravityScale << ",\"mass\":" << body->mass;
        }
        if (const auto* collider = GetCollider(id)) {
            out << ",\"collider\":true,\"trigger\":" << (collider->isTrigger ? "true" : "false")
                << ",\"cw\":" << collider->size.x << ",\"ch\":" << collider->size.y << ",\"radius\":" << collider->radius;
        }
        if (const auto* light = GetPointLight(id)) {
            out << ",\"light\":true,\"lightRadius\":" << light->radius << ",\"intensity\":" << light->intensity;
        }
        if (const auto* script = GetLuaScript(id)) {
            out << ",\"script\":\"" << Escape(script->path.generic_string()) << "\"";
        }
        out << "}";
    }
    out << "\n  ]\n}\n";
    return out.str();
}

bool Scene::DeserializeJson(const std::string& json) {
    *this = Scene{};
    size_t pos = 0;
    while ((pos = json.find("{\"id\"", pos)) != std::string::npos) {
        const auto end = json.find('}', pos);
        const auto name = ExtractString(json, "name", pos);
        auto entity = CreateEntity(name.empty() ? "Entity" : name);
        if (auto* transform = GetTransform(entity)) {
            transform->position.x = ExtractNumber(json, "x", pos, 0.0f);
            transform->position.y = ExtractNumber(json, "y", pos, 0.0f);
            transform->rotation = ExtractNumber(json, "rotation", pos, 0.0f);
            transform->scale.x = ExtractNumber(json, "sx", pos, 1.0f);
            transform->scale.y = ExtractNumber(json, "sy", pos, 1.0f);
        }
        const auto texture = ExtractString(json, "texture", pos);
        const auto sheet = ExtractString(json, "sheet", pos);
        if (!texture.empty() || !sheet.empty()) {
            AddSpriteRenderer(entity, {texture, sheet, static_cast<int>(ExtractNumber(json, "frame", pos, 0.0f))});
        }
        if (json.find("\"camera\":true", pos) != std::string::npos && json.find("\"camera\":true", pos) < end) {
            AddCamera(entity, {ExtractNumber(json, "zoom", pos, 1.0f), {1280.0f, 720.0f}, ExtractBool(json, "primary", pos, end)});
        }
        if (json.find("\"rigidbody\":true", pos) != std::string::npos && json.find("\"rigidbody\":true", pos) < end) {
            AddRigidbody(entity, {BodyType::Dynamic, {ExtractNumber(json, "vx", pos, 0.0f), ExtractNumber(json, "vy", pos, 0.0f)}, ExtractNumber(json, "gravity", pos, 1.0f), ExtractNumber(json, "mass", pos, 1.0f)});
        }
        if (json.find("\"collider\":true", pos) != std::string::npos && json.find("\"collider\":true", pos) < end) {
            AddCollider(entity, {ColliderShape::AABB, {}, {ExtractNumber(json, "cw", pos, 1.0f), ExtractNumber(json, "ch", pos, 1.0f)}, ExtractNumber(json, "radius", pos, 0.5f), ExtractBool(json, "trigger", pos, end)});
        }
        if (json.find("\"light\":true", pos) != std::string::npos && json.find("\"light\":true", pos) < end) {
            AddPointLight(entity, {{1.0f, 0.92f, 0.75f, 1.0f}, ExtractNumber(json, "lightRadius", pos, 160.0f), ExtractNumber(json, "intensity", pos, 1.0f)});
        }
        const auto script = ExtractString(json, "script", pos);
        if (!script.empty()) {
            AddLuaScript(entity, {script, true});
        }
        pos = end == std::string::npos ? json.size() : end + 1;
    }
    return true;
}

}
