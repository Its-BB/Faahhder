#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace faahhder {

enum class ExportPipelineKind {
    AssetEntry,
    ScriptEntry,
    SceneEntry,
    ManifestEntry,
    CopyStep,
    CompileStep,
    BundleStep,
    VerifyStep,
    LaunchStep,
    TemplateFile,
    PlatformRule,
    ReportLine
};

struct ExportPipelineItem {
    int id = 0;
    ExportPipelineKind kind = ExportPipelineKind::AssetEntry;
    std::string name;
    std::filesystem::path path;
    std::string group;
    std::string detail;
    int order = 0;
    int priority = 0;
    bool enabled = true;
    bool dirty = false;
};

struct ExportPipelineItemPatch {
    std::optional<ExportPipelineKind> kind;
    std::optional<std::string> name;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> group;
    std::optional<std::string> detail;
    std::optional<int> order;
    std::optional<int> priority;
    std::optional<bool> enabled;
    std::optional<bool> dirty;
};

struct ExportPipelineItemSummary {
    int total = 0;
    int enabled = 0;
    int dirty = 0;
    int priorityTotal = 0;
    std::map<std::string, int> byKind;
    std::map<std::string, int> byGroup;
};

std::string ToString(ExportPipelineKind kind);
bool TryParseExportPipelineKind(const std::string& text, ExportPipelineKind& out);

class ExportPipeline {
public:
    int Add(ExportPipelineKind kind, std::string name, std::filesystem::path path = {}, std::string group = {}, std::string detail = {});
    int Add(ExportPipelineItem value);
    bool Upsert(ExportPipelineItem value);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool MoveToGroup(int id, std::string group);
    bool SetPath(int id, std::filesystem::path path);
    bool SetDetail(int id, std::string detail);
    bool SetPriority(int id, int priority);
    bool SetEnabled(int id, bool enabled);
    bool MarkDirty(int id, bool dirty = true);
    bool ApplyPatch(int id, const ExportPipelineItemPatch& patch);
    bool Reorder(int id, int order);
    bool MoveBefore(int movingId, int beforeId);
    bool MoveAfter(int movingId, int afterId);
    bool NormalizeOrder();
    bool Contains(int id) const;
    ExportPipelineItem* Find(int id);
    const ExportPipelineItem* Find(int id) const;
    const ExportPipelineItem* FindByName(const std::string& name) const;
    std::vector<ExportPipelineItem> All() const;
    std::vector<ExportPipelineItem> Enabled() const;
    std::vector<ExportPipelineItem> Dirty() const;
    std::vector<ExportPipelineItem> ByKind(ExportPipelineKind kind) const;
    std::vector<ExportPipelineItem> ByGroup(const std::string& group) const;
    std::vector<ExportPipelineItem> Search(const std::string& text) const;
    std::vector<ExportPipelineItem> TopPriority(int limit) const;
    std::set<std::string> Groups() const;
    ExportPipelineItemSummary Summarize() const;
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
    int NextId() const;
    int Size() const;
    void Clear();
private:
    int nextId_ = 1;
    std::vector<ExportPipelineItem> entries_;
};

std::vector<ExportPipelineItem> BuildWindowsExportPipeline(const std::filesystem::path& projectRoot, const std::filesystem::path& outputRoot);

std::vector<ExportPipelineItem> BuildExportPipelinePresetMap();

}
