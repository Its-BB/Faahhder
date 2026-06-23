#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class GameplayItemKind {
    Rule,
    Input,
    Score,
    Timer,
    State,
    Spawn,
    Collision,
    Pickup,
    Restart,
    Save
};

struct GameplayItem {
    int id = 0;
    GameplayItemKind kind = GameplayItemKind::Rule;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

struct GameplayItemPatch {
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

class GameplayPlan {
public:
    int Add(GameplayItemKind kind, std::string name, std::filesystem::path path = {}, std::string tag = {});
    bool Upsert(int id, GameplayItem value);
    bool Patch(int id, const GameplayItemPatch& patch);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool Retag(int id, std::string tag);
    bool Enable(int id, bool enabled);
    bool MoveBefore(int id, int beforeId);
    bool MoveAfter(int id, int afterId);
    GameplayItem* Find(int id);
    const GameplayItem* Find(int id) const;
    const GameplayItem* FindByName(const std::string& name) const;
    std::vector<GameplayItem> FindByKind(GameplayItemKind kind) const;
    std::vector<GameplayItem> FindByTag(const std::string& tag) const;
    std::vector<GameplayItem> Search(const std::string& text) const;
    std::vector<GameplayItem> Enabled() const;
    std::vector<GameplayItem> Sorted() const;
    std::vector<GameplayItem> Entries() const;
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
    std::vector<GameplayItem> entries_;
};

std::string ToString(GameplayItemKind kind);
bool TryParseGameplayItemKind(const std::string& text, GameplayItemKind& out);

}
