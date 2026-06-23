#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class RenderPhysicsItemKind {
    Sprite,
    Tile,
    Particle,
    Light,
    Camera,
    Collider,
    Rigidbody,
    Trigger,
    Raycast,
    Layer
};

struct RenderPhysicsItem {
    int id = 0;
    RenderPhysicsItemKind kind = RenderPhysicsItemKind::Sprite;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

struct RenderPhysicsItemPatch {
    bool hasName = false;
    bool hasPath = false;
    bool hasTag = false;
    bool hasOrder = false;
    bool hasWeight = false;
    bool hasEnabled = false;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

class RenderPhysicsPlan {
public:
    int Add(RenderPhysicsItemKind kind, std::string name, std::filesystem::path path = {}, std::string tag = {});
    bool Upsert(int id, RenderPhysicsItem value);
    bool Patch(int id, const RenderPhysicsItemPatch& patch);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool Retag(int id, std::string tag);
    bool Enable(int id, bool enabled);
    bool MoveBefore(int id, int beforeId);
    bool MoveAfter(int id, int afterId);
    RenderPhysicsItem* Find(int id);
    const RenderPhysicsItem* Find(int id) const;
    const RenderPhysicsItem* FindByName(const std::string& name) const;
    std::vector<RenderPhysicsItem> FindByKind(RenderPhysicsItemKind kind) const;
    std::vector<RenderPhysicsItem> FindByTag(const std::string& tag) const;
    std::vector<RenderPhysicsItem> Search(const std::string& text) const;
    std::vector<RenderPhysicsItem> Enabled() const;
    std::vector<RenderPhysicsItem> Sorted() const;
    std::vector<RenderPhysicsItem> Entries() const;
    std::size_t Size() const;
    bool Empty() const;
    int TotalWeight() const;
    int NextId() const;
    void Clear();
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
private:
    int nextId_ = 1;
    std::vector<RenderPhysicsItem> entries_;
};

std::string ToString(RenderPhysicsItemKind kind);
bool TryParseRenderPhysicsItemKind(const std::string& text, RenderPhysicsItemKind& out);

}
