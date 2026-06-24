#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace faahhder {

enum class QualityReportKind {
    FrameSample,
    MemorySample,
    AssetIssue,
    SceneIssue,
    ScriptIssue,
    ExportIssue,
    UiIssue,
    PhysicsIssue,
    RenderIssue,
    ProjectIssue,
    BuildIssue,
    Suggestion
};

struct QualityReportItem {
    int id = 0;
    QualityReportKind kind = QualityReportKind::FrameSample;
    std::string name;
    std::filesystem::path path;
    std::string group;
    std::string detail;
    int order = 0;
    int priority = 0;
    bool enabled = true;
    bool dirty = false;
};

struct QualityReportItemPatch {
    std::optional<QualityReportKind> kind;
    std::optional<std::string> name;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> group;
    std::optional<std::string> detail;
    std::optional<int> order;
    std::optional<int> priority;
    std::optional<bool> enabled;
    std::optional<bool> dirty;
};

struct QualityReportItemSummary {
    int total = 0;
    int enabled = 0;
    int dirty = 0;
    int priorityTotal = 0;
    std::map<std::string, int> byKind;
    std::map<std::string, int> byGroup;
};

std::string ToString(QualityReportKind kind);
bool TryParseQualityReportKind(const std::string& text, QualityReportKind& out);

class QualityReport {
public:
    int Add(QualityReportKind kind, std::string name, std::filesystem::path path = {}, std::string group = {}, std::string detail = {});
    int Add(QualityReportItem value);
    bool Upsert(QualityReportItem value);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool MoveToGroup(int id, std::string group);
    bool SetPath(int id, std::filesystem::path path);
    bool SetDetail(int id, std::string detail);
    bool SetPriority(int id, int priority);
    bool SetEnabled(int id, bool enabled);
    bool MarkDirty(int id, bool dirty = true);
    bool ApplyPatch(int id, const QualityReportItemPatch& patch);
    bool Reorder(int id, int order);
    bool MoveBefore(int movingId, int beforeId);
    bool MoveAfter(int movingId, int afterId);
    bool NormalizeOrder();
    bool Contains(int id) const;
    QualityReportItem* Find(int id);
    const QualityReportItem* Find(int id) const;
    const QualityReportItem* FindByName(const std::string& name) const;
    std::vector<QualityReportItem> All() const;
    std::vector<QualityReportItem> Enabled() const;
    std::vector<QualityReportItem> Dirty() const;
    std::vector<QualityReportItem> ByKind(QualityReportKind kind) const;
    std::vector<QualityReportItem> ByGroup(const std::string& group) const;
    std::vector<QualityReportItem> Search(const std::string& text) const;
    std::vector<QualityReportItem> TopPriority(int limit) const;
    std::set<std::string> Groups() const;
    QualityReportItemSummary Summarize() const;
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
    int NextId() const;
    int Size() const;
    void Clear();
private:
    int nextId_ = 1;
    std::vector<QualityReportItem> entries_;
};

std::vector<QualityReportItem> BuildDefaultQualityGates();
std::string BuildQualityGateSummary(const std::vector<QualityReportItem>& items);

std::vector<QualityReportItem> BuildQualityReportPresetMap();

}
