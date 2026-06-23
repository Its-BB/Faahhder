#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace faahhder {

enum class RuntimeQueryKind {
    AabbProbe,
    CircleProbe,
    RaycastProbe,
    TileProbe,
    TriggerProbe,
    CameraProbe,
    ParticleProbe,
    LightProbe,
    SpriteProbe,
    PathProbe,
    LayerProbe,
    BatchProbe
};

struct RuntimeQueryItem {
    int id = 0;
    RuntimeQueryKind kind = RuntimeQueryKind::AabbProbe;
    std::string name;
    std::filesystem::path path;
    std::string group;
    std::string detail;
    int order = 0;
    int priority = 0;
    bool enabled = true;
    bool dirty = false;
};

struct RuntimeQueryItemPatch {
    std::optional<RuntimeQueryKind> kind;
    std::optional<std::string> name;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> group;
    std::optional<std::string> detail;
    std::optional<int> order;
    std::optional<int> priority;
    std::optional<bool> enabled;
    std::optional<bool> dirty;
};

struct RuntimeQueryItemSummary {
    int total = 0;
    int enabled = 0;
    int dirty = 0;
    int priorityTotal = 0;
    std::map<std::string, int> byKind;
    std::map<std::string, int> byGroup;
};

std::string ToString(RuntimeQueryKind kind);
bool TryParseRuntimeQueryKind(const std::string& text, RuntimeQueryKind& out);

class RuntimeQueries {
public:
    int Add(RuntimeQueryKind kind, std::string name, std::filesystem::path path = {}, std::string group = {}, std::string detail = {});
    int Add(RuntimeQueryItem value);
    bool Upsert(RuntimeQueryItem value);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool MoveToGroup(int id, std::string group);
    bool SetPath(int id, std::filesystem::path path);
    bool SetDetail(int id, std::string detail);
    bool SetPriority(int id, int priority);
    bool SetEnabled(int id, bool enabled);
    bool MarkDirty(int id, bool dirty = true);
    bool ApplyPatch(int id, const RuntimeQueryItemPatch& patch);
    bool Reorder(int id, int order);
    bool MoveBefore(int movingId, int beforeId);
    bool MoveAfter(int movingId, int afterId);
    bool NormalizeOrder();
    bool Contains(int id) const;
    RuntimeQueryItem* Find(int id);
    const RuntimeQueryItem* Find(int id) const;
    const RuntimeQueryItem* FindByName(const std::string& name) const;
    std::vector<RuntimeQueryItem> All() const;
    std::vector<RuntimeQueryItem> Enabled() const;
    std::vector<RuntimeQueryItem> Dirty() const;
    std::vector<RuntimeQueryItem> ByKind(RuntimeQueryKind kind) const;
    std::vector<RuntimeQueryItem> ByGroup(const std::string& group) const;
    std::vector<RuntimeQueryItem> Search(const std::string& text) const;
    std::vector<RuntimeQueryItem> TopPriority(int limit) const;
    std::set<std::string> Groups() const;
    RuntimeQueryItemSummary Summarize() const;
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
    int NextId() const;
    int Size() const;
    void Clear();
private:
    int nextId_ = 1;
    std::vector<RuntimeQueryItem> entries_;
};

std::vector<RuntimeQueryItem> BuildRuntimeQueryChecklist();

std::vector<RuntimeQueryItem> BuildRuntimeQueryPresetMap();

}
