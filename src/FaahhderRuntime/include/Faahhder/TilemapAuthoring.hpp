#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace faahhder {

enum class TilemapAuthoringKind {
    Brush,
    Stamp,
    CollisionRule,
    LayerRule,
    PaletteEntry,
    AutotileRule,
    Selection,
    FillJob,
    EraseJob,
    PreviewTile,
    GridGuide,
    HistoryPoint
};

struct TilemapAuthoringItem {
    int id = 0;
    TilemapAuthoringKind kind = TilemapAuthoringKind::Brush;
    std::string name;
    std::filesystem::path path;
    std::string group;
    std::string detail;
    int order = 0;
    int priority = 0;
    bool enabled = true;
    bool dirty = false;
};

struct TilemapAuthoringItemPatch {
    std::optional<TilemapAuthoringKind> kind;
    std::optional<std::string> name;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> group;
    std::optional<std::string> detail;
    std::optional<int> order;
    std::optional<int> priority;
    std::optional<bool> enabled;
    std::optional<bool> dirty;
};

struct TilemapAuthoringItemSummary {
    int total = 0;
    int enabled = 0;
    int dirty = 0;
    int priorityTotal = 0;
    std::map<std::string, int> byKind;
    std::map<std::string, int> byGroup;
};

std::string ToString(TilemapAuthoringKind kind);
bool TryParseTilemapAuthoringKind(const std::string& text, TilemapAuthoringKind& out);

class TilemapAuthoring {
public:
    int Add(TilemapAuthoringKind kind, std::string name, std::filesystem::path path = {}, std::string group = {}, std::string detail = {});
    int Add(TilemapAuthoringItem value);
    bool Upsert(TilemapAuthoringItem value);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool MoveToGroup(int id, std::string group);
    bool SetPath(int id, std::filesystem::path path);
    bool SetDetail(int id, std::string detail);
    bool SetPriority(int id, int priority);
    bool SetEnabled(int id, bool enabled);
    bool MarkDirty(int id, bool dirty = true);
    bool ApplyPatch(int id, const TilemapAuthoringItemPatch& patch);
    bool Reorder(int id, int order);
    bool MoveBefore(int movingId, int beforeId);
    bool MoveAfter(int movingId, int afterId);
    bool NormalizeOrder();
    bool Contains(int id) const;
    TilemapAuthoringItem* Find(int id);
    const TilemapAuthoringItem* Find(int id) const;
    const TilemapAuthoringItem* FindByName(const std::string& name) const;
    std::vector<TilemapAuthoringItem> All() const;
    std::vector<TilemapAuthoringItem> Enabled() const;
    std::vector<TilemapAuthoringItem> Dirty() const;
    std::vector<TilemapAuthoringItem> ByKind(TilemapAuthoringKind kind) const;
    std::vector<TilemapAuthoringItem> ByGroup(const std::string& group) const;
    std::vector<TilemapAuthoringItem> Search(const std::string& text) const;
    std::vector<TilemapAuthoringItem> TopPriority(int limit) const;
    std::set<std::string> Groups() const;
    TilemapAuthoringItemSummary Summarize() const;
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
    int NextId() const;
    int Size() const;
    void Clear();
private:
    int nextId_ = 1;
    std::vector<TilemapAuthoringItem> entries_;
};

std::vector<TilemapAuthoringItem> BuildTilemapAuthoringDefaults(const std::filesystem::path& tilesetPath);

std::vector<TilemapAuthoringItem> BuildTilemapToolPresetMap();

}
