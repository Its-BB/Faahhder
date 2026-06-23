#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace faahhder {

enum class SnakeAuthoringKind {
    BoardRule,
    FoodRule,
    SnakePart,
    InputRule,
    ScoreRule,
    HazardRule,
    LevelPreset,
    SpeedCurve,
    SpawnRule,
    SkinRule,
    WinRule,
    ReplayMarker
};

struct SnakeAuthoringItem {
    int id = 0;
    SnakeAuthoringKind kind = SnakeAuthoringKind::BoardRule;
    std::string name;
    std::filesystem::path path;
    std::string group;
    std::string detail;
    int order = 0;
    int priority = 0;
    bool enabled = true;
    bool dirty = false;
};

struct SnakeAuthoringItemPatch {
    std::optional<SnakeAuthoringKind> kind;
    std::optional<std::string> name;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> group;
    std::optional<std::string> detail;
    std::optional<int> order;
    std::optional<int> priority;
    std::optional<bool> enabled;
    std::optional<bool> dirty;
};

struct SnakeAuthoringItemSummary {
    int total = 0;
    int enabled = 0;
    int dirty = 0;
    int priorityTotal = 0;
    std::map<std::string, int> byKind;
    std::map<std::string, int> byGroup;
};

std::string ToString(SnakeAuthoringKind kind);
bool TryParseSnakeAuthoringKind(const std::string& text, SnakeAuthoringKind& out);

class SnakeAuthoring {
public:
    int Add(SnakeAuthoringKind kind, std::string name, std::filesystem::path path = {}, std::string group = {}, std::string detail = {});
    int Add(SnakeAuthoringItem value);
    bool Upsert(SnakeAuthoringItem value);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool MoveToGroup(int id, std::string group);
    bool SetPath(int id, std::filesystem::path path);
    bool SetDetail(int id, std::string detail);
    bool SetPriority(int id, int priority);
    bool SetEnabled(int id, bool enabled);
    bool MarkDirty(int id, bool dirty = true);
    bool ApplyPatch(int id, const SnakeAuthoringItemPatch& patch);
    bool Reorder(int id, int order);
    bool MoveBefore(int movingId, int beforeId);
    bool MoveAfter(int movingId, int afterId);
    bool NormalizeOrder();
    bool Contains(int id) const;
    SnakeAuthoringItem* Find(int id);
    const SnakeAuthoringItem* Find(int id) const;
    const SnakeAuthoringItem* FindByName(const std::string& name) const;
    std::vector<SnakeAuthoringItem> All() const;
    std::vector<SnakeAuthoringItem> Enabled() const;
    std::vector<SnakeAuthoringItem> Dirty() const;
    std::vector<SnakeAuthoringItem> ByKind(SnakeAuthoringKind kind) const;
    std::vector<SnakeAuthoringItem> ByGroup(const std::string& group) const;
    std::vector<SnakeAuthoringItem> Search(const std::string& text) const;
    std::vector<SnakeAuthoringItem> TopPriority(int limit) const;
    std::set<std::string> Groups() const;
    SnakeAuthoringItemSummary Summarize() const;
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
    int NextId() const;
    int Size() const;
    void Clear();
private:
    int nextId_ = 1;
    std::vector<SnakeAuthoringItem> entries_;
};



std::vector<SnakeAuthoringItem> BuildSnakeRulePresetMap();

}
