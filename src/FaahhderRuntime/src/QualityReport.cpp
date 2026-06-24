#include "Faahhder/QualityReport.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace faahhder {
namespace {

std::string Lower(std::string text) { for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return text; }
std::string Trim(std::string value) { while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin()); while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back(); return value; }
bool ContainsText(const std::string& text, const std::string& needle) { if (needle.empty()) return true; return Lower(text).find(Lower(needle)) != std::string::npos; }
std::vector<std::string> SplitFields(const std::string& line) { std::vector<std::string> fields; std::string current; bool escaped = false; for (char c : line) { if (escaped) { current.push_back(c); escaped = false; continue; } if (c == '\\') { escaped = true; continue; } if (c == '|') { fields.push_back(current); current.clear(); continue; } current.push_back(c); } fields.push_back(current); return fields; }
std::string EscapeField(const std::string& text) { std::string out; for (char c : text) { if (c == '|' || c == '\\') out.push_back('\\'); out.push_back(c); } return out; }

}

std::string ToString(QualityReportKind kind) {
    switch (kind) {
        case QualityReportKind::FrameSample: return "FrameSample";
        case QualityReportKind::MemorySample: return "MemorySample";
        case QualityReportKind::AssetIssue: return "AssetIssue";
        case QualityReportKind::SceneIssue: return "SceneIssue";
        case QualityReportKind::ScriptIssue: return "ScriptIssue";
        case QualityReportKind::ExportIssue: return "ExportIssue";
        case QualityReportKind::UiIssue: return "UiIssue";
        case QualityReportKind::PhysicsIssue: return "PhysicsIssue";
        case QualityReportKind::RenderIssue: return "RenderIssue";
        case QualityReportKind::ProjectIssue: return "ProjectIssue";
        case QualityReportKind::BuildIssue: return "BuildIssue";
        case QualityReportKind::Suggestion: return "Suggestion";
    }
    return "FrameSample";
}

bool TryParseQualityReportKind(const std::string& text, QualityReportKind& out) {
    const std::string lower = Lower(Trim(text));
    if (lower == "framesample") { out = QualityReportKind::FrameSample; return true; }
    if (lower == "memorysample") { out = QualityReportKind::MemorySample; return true; }
    if (lower == "assetissue") { out = QualityReportKind::AssetIssue; return true; }
    if (lower == "sceneissue") { out = QualityReportKind::SceneIssue; return true; }
    if (lower == "scriptissue") { out = QualityReportKind::ScriptIssue; return true; }
    if (lower == "exportissue") { out = QualityReportKind::ExportIssue; return true; }
    if (lower == "uiissue") { out = QualityReportKind::UiIssue; return true; }
    if (lower == "physicsissue") { out = QualityReportKind::PhysicsIssue; return true; }
    if (lower == "renderissue") { out = QualityReportKind::RenderIssue; return true; }
    if (lower == "projectissue") { out = QualityReportKind::ProjectIssue; return true; }
    if (lower == "buildissue") { out = QualityReportKind::BuildIssue; return true; }
    if (lower == "suggestion") { out = QualityReportKind::Suggestion; return true; }
    return false;
}

int QualityReport::Add(QualityReportKind kind, std::string name, std::filesystem::path path, std::string group, std::string detail) { QualityReportItem value; value.id = nextId_++; value.kind = kind; value.name = std::move(name); value.path = std::move(path); value.group = std::move(group); value.detail = std::move(detail); value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); return entries_.back().id; }
int QualityReport::Add(QualityReportItem value) { if (value.id <= 0) value.id = nextId_++; nextId_ = std::max(nextId_, value.id + 1); if (value.order < 0) value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); NormalizeOrder(); return entries_.back().id; }
bool QualityReport::Upsert(QualityReportItem value) { if (value.id > 0) { if (auto* existing = Find(value.id)) { *existing = std::move(value); NormalizeOrder(); return true; } } Add(std::move(value)); return true; }
bool QualityReport::Remove(int id) { const auto it = std::remove_if(entries_.begin(), entries_.end(), [id](const QualityReportItem& entry) { return entry.id == id; }); if (it == entries_.end()) return false; entries_.erase(it, entries_.end()); NormalizeOrder(); return true; }
bool QualityReport::Rename(int id, std::string name) { if (auto* entry = Find(id)) { entry->name = std::move(name); entry->dirty = true; return true; } return false; }
bool QualityReport::MoveToGroup(int id, std::string group) { if (auto* entry = Find(id)) { entry->group = std::move(group); entry->dirty = true; return true; } return false; }
bool QualityReport::SetPath(int id, std::filesystem::path path) { if (auto* entry = Find(id)) { entry->path = std::move(path); entry->dirty = true; return true; } return false; }
bool QualityReport::SetDetail(int id, std::string detail) { if (auto* entry = Find(id)) { entry->detail = std::move(detail); entry->dirty = true; return true; } return false; }
bool QualityReport::SetPriority(int id, int priority) { if (auto* entry = Find(id)) { entry->priority = priority; entry->dirty = true; return true; } return false; }
bool QualityReport::SetEnabled(int id, bool enabled) { if (auto* entry = Find(id)) { entry->enabled = enabled; entry->dirty = true; return true; } return false; }
bool QualityReport::MarkDirty(int id, bool dirty) { if (auto* entry = Find(id)) { entry->dirty = dirty; return true; } return false; }
bool QualityReport::ApplyPatch(int id, const QualityReportItemPatch& patch) { auto* entry = Find(id); if (!entry) return false; if (patch.kind) entry->kind = *patch.kind; if (patch.name) entry->name = *patch.name; if (patch.path) entry->path = *patch.path; if (patch.group) entry->group = *patch.group; if (patch.detail) entry->detail = *patch.detail; if (patch.order) entry->order = *patch.order; if (patch.priority) entry->priority = *patch.priority; if (patch.enabled) entry->enabled = *patch.enabled; if (patch.dirty) entry->dirty = *patch.dirty; NormalizeOrder(); return true; }
bool QualityReport::Reorder(int id, int order) { auto* entry = Find(id); if (!entry) return false; entry->order = std::max(0, order); entry->dirty = true; NormalizeOrder(); return true; }
bool QualityReport::MoveBefore(int movingId, int beforeId) { const auto* before = Find(beforeId); if (!before || movingId == beforeId) return false; return Reorder(movingId, before->order); }
bool QualityReport::MoveAfter(int movingId, int afterId) { const auto* after = Find(afterId); if (!after || movingId == afterId) return false; return Reorder(movingId, after->order + 1); }
bool QualityReport::NormalizeOrder() { std::stable_sort(entries_.begin(), entries_.end(), [](const QualityReportItem& a, const QualityReportItem& b) { if (a.order != b.order) return a.order < b.order; return a.id < b.id; }); bool changed = false; for (int i = 0; i < static_cast<int>(entries_.size()); ++i) { if (entries_[i].order != i) changed = true; entries_[i].order = i; } return changed; }
bool QualityReport::Contains(int id) const { return Find(id) != nullptr; }
QualityReportItem* QualityReport::Find(int id) { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const QualityReportItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const QualityReportItem* QualityReport::Find(int id) const { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const QualityReportItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const QualityReportItem* QualityReport::FindByName(const std::string& name) const { const std::string target = Lower(name); auto it = std::find_if(entries_.begin(), entries_.end(), [&target](const QualityReportItem& entry) { return Lower(entry.name) == target; }); return it == entries_.end() ? nullptr : &*it; }
std::vector<QualityReportItem> QualityReport::All() const { return entries_; }
std::vector<QualityReportItem> QualityReport::Enabled() const { std::vector<QualityReportItem> out; for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry); return out; }
std::vector<QualityReportItem> QualityReport::Dirty() const { std::vector<QualityReportItem> out; for (const auto& entry : entries_) if (entry.dirty) out.push_back(entry); return out; }
std::vector<QualityReportItem> QualityReport::ByKind(QualityReportKind kind) const { std::vector<QualityReportItem> out; for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry); return out; }
std::vector<QualityReportItem> QualityReport::ByGroup(const std::string& group) const { std::vector<QualityReportItem> out; const std::string target = Lower(group); for (const auto& entry : entries_) if (Lower(entry.group) == target) out.push_back(entry); return out; }
std::vector<QualityReportItem> QualityReport::Search(const std::string& text) const { std::vector<QualityReportItem> out; for (const auto& entry : entries_) if (ContainsText(entry.name, text) || ContainsText(entry.group, text) || ContainsText(entry.detail, text) || ContainsText(entry.path.generic_string(), text)) out.push_back(entry); return out; }
std::vector<QualityReportItem> QualityReport::TopPriority(int limit) const { std::vector<QualityReportItem> out = entries_; std::stable_sort(out.begin(), out.end(), [](const QualityReportItem& a, const QualityReportItem& b) { if (a.priority != b.priority) return a.priority > b.priority; return a.order < b.order; }); if (limit >= 0 && static_cast<int>(out.size()) > limit) out.resize(static_cast<size_t>(limit)); return out; }
std::set<std::string> QualityReport::Groups() const { std::set<std::string> groups; for (const auto& entry : entries_) if (!entry.group.empty()) groups.insert(entry.group); return groups; }
QualityReportItemSummary QualityReport::Summarize() const { QualityReportItemSummary summary; summary.total = static_cast<int>(entries_.size()); for (const auto& entry : entries_) { if (entry.enabled) summary.enabled++; if (entry.dirty) summary.dirty++; summary.priorityTotal += entry.priority; summary.byKind[ToString(entry.kind)]++; if (!entry.group.empty()) summary.byGroup[entry.group]++; } return summary; }
std::string QualityReport::Serialize() const { std::ostringstream out; for (const auto& entry : entries_) out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.group) << "|" << EscapeField(entry.detail) << "|" << entry.order << "|" << entry.priority << "|" << (entry.enabled ? 1 : 0) << "|" << (entry.dirty ? 1 : 0) << "\n"; return out.str(); }
bool QualityReport::Deserialize(const std::string& text) { entries_.clear(); nextId_ = 1; std::istringstream input(text); std::string line; while (std::getline(input, line)) { if (Trim(line).empty()) continue; const auto fields = SplitFields(line); if (fields.size() != 10) return false; QualityReportItem entry; entry.id = std::stoi(fields[0]); if (!TryParseQualityReportKind(fields[1], entry.kind)) return false; entry.name = fields[2]; entry.path = fields[3]; entry.group = fields[4]; entry.detail = fields[5]; entry.order = std::stoi(fields[6]); entry.priority = std::stoi(fields[7]); entry.enabled = fields[8] == "1"; entry.dirty = fields[9] == "1"; nextId_ = std::max(nextId_, entry.id + 1); entries_.push_back(std::move(entry)); } NormalizeOrder(); return true; }
std::string QualityReport::Report() const { const auto summary = Summarize(); std::ostringstream out; out << "total=" << summary.total << " enabled=" << summary.enabled << " dirty=" << summary.dirty << " priority=" << summary.priorityTotal; for (const auto& row : summary.byKind) out << "\n" << row.first << "=" << row.second; for (const auto& row : summary.byGroup) out << "\ngroup:" << row.first << "=" << row.second; return out.str(); }
int QualityReport::NextId() const { return nextId_; }
int QualityReport::Size() const { return static_cast<int>(entries_.size()); }
void QualityReport::Clear() { entries_.clear(); nextId_ = 1; }

std::vector<QualityReportItem> BuildDefaultQualityGates() {
    std::vector<QualityReportItem> items;
    const struct Row { QualityReportKind kind; const char* name; const char* group; const char* detail; int priority; } rows[] = {
        {QualityReportKind::FrameSample, "Frame pacing", "runtime", "Tracks slow frames during play mode", 8},
        {QualityReportKind::MemorySample, "Memory growth", "runtime", "Watches allocation growth", 7},
        {QualityReportKind::AssetIssue, "Missing asset", "assets", "Reports manifest files that cannot be opened", 10},
        {QualityReportKind::SceneIssue, "Scene load", "scene", "Verifies entity and component data can reload", 10},
        {QualityReportKind::ScriptIssue, "Lua reload", "scripts", "Checks script reload errors", 9},
        {QualityReportKind::ExportIssue, "Export launch", "export", "Confirms exported executable starts", 10},
        {QualityReportKind::UiIssue, "Editor layout", "editor", "Confirms panels fit inside the window", 6},
        {QualityReportKind::PhysicsIssue, "Collision query", "physics", "Checks AABB, circle, and raycast query paths", 8},
        {QualityReportKind::RenderIssue, "Render batch", "render", "Checks sprite, tile, particle, and light command flow", 8},
        {QualityReportKind::BuildIssue, "Compiler result", "build", "Stores the latest build result per target", 9},
    };
    int id = 1;
    for (const auto& row : rows) {
        QualityReportItem item;
        item.id = id++;
        item.kind = row.kind;
        item.name = row.name;
        item.group = row.group;
        item.detail = row.detail;
        item.order = static_cast<int>(items.size());
        item.priority = row.priority;
        items.push_back(std::move(item));
    }
    return items;
}

std::string BuildQualityGateSummary(const std::vector<QualityReportItem>& items) {
    int enabled = 0;
    int highPriority = 0;
    std::map<std::string, int> groups;
    for (const auto& item : items) {
        if (item.enabled) enabled++;
        if (item.priority >= 8) highPriority++;
        groups[item.group]++;
    }
    std::ostringstream out;
    out << "quality-gates=" << items.size() << " enabled=" << enabled << " high-priority=" << highPriority;
    for (const auto& row : groups) out << "\n" << row.first << "=" << row.second;
    return out.str();
}


std::vector<QualityReportItem> BuildQualityReportPresetMap() {
    std::vector<QualityReportItem> items;
    items.reserve(42);

    {
        QualityReportItem item;
        item.id = 1;
        item.kind = QualityReportKind::FrameSample;
        item.name = "FrameSample preset 1";
        item.group = "quality-gate";
        item.detail = "Preset 1 used by quality-gate workflows";
        item.order = 0;
        item.priority = 1;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 2;
        item.kind = QualityReportKind::MemorySample;
        item.name = "MemorySample preset 2";
        item.group = "quality-gate";
        item.detail = "Preset 2 used by quality-gate workflows";
        item.order = 1;
        item.priority = 2;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 3;
        item.kind = QualityReportKind::AssetIssue;
        item.name = "AssetIssue preset 3";
        item.group = "quality-gate";
        item.detail = "Preset 3 used by quality-gate workflows";
        item.order = 2;
        item.priority = 3;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 4;
        item.kind = QualityReportKind::SceneIssue;
        item.name = "SceneIssue preset 4";
        item.group = "quality-gate";
        item.detail = "Preset 4 used by quality-gate workflows";
        item.order = 3;
        item.priority = 4;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 5;
        item.kind = QualityReportKind::ScriptIssue;
        item.name = "ScriptIssue preset 5";
        item.group = "quality-gate";
        item.detail = "Preset 5 used by quality-gate workflows";
        item.order = 4;
        item.priority = 5;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 6;
        item.kind = QualityReportKind::ExportIssue;
        item.name = "ExportIssue preset 6";
        item.group = "quality-gate";
        item.detail = "Preset 6 used by quality-gate workflows";
        item.order = 5;
        item.priority = 6;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 7;
        item.kind = QualityReportKind::UiIssue;
        item.name = "UiIssue preset 7";
        item.group = "quality-gate";
        item.detail = "Preset 7 used by quality-gate workflows";
        item.order = 6;
        item.priority = 7;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 8;
        item.kind = QualityReportKind::PhysicsIssue;
        item.name = "PhysicsIssue preset 8";
        item.group = "quality-gate";
        item.detail = "Preset 8 used by quality-gate workflows";
        item.order = 7;
        item.priority = 8;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 9;
        item.kind = QualityReportKind::RenderIssue;
        item.name = "RenderIssue preset 9";
        item.group = "quality-gate";
        item.detail = "Preset 9 used by quality-gate workflows";
        item.order = 8;
        item.priority = 9;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 10;
        item.kind = QualityReportKind::ProjectIssue;
        item.name = "ProjectIssue preset 10";
        item.group = "quality-gate";
        item.detail = "Preset 10 used by quality-gate workflows";
        item.order = 9;
        item.priority = 10;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 11;
        item.kind = QualityReportKind::BuildIssue;
        item.name = "BuildIssue preset 11";
        item.group = "quality-gate";
        item.detail = "Preset 11 used by quality-gate workflows";
        item.order = 10;
        item.priority = 1;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 12;
        item.kind = QualityReportKind::Suggestion;
        item.name = "Suggestion preset 12";
        item.group = "quality-gate";
        item.detail = "Preset 12 used by quality-gate workflows";
        item.order = 11;
        item.priority = 2;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 13;
        item.kind = QualityReportKind::FrameSample;
        item.name = "FrameSample preset 13";
        item.group = "quality-gate";
        item.detail = "Preset 13 used by quality-gate workflows";
        item.order = 12;
        item.priority = 3;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 14;
        item.kind = QualityReportKind::MemorySample;
        item.name = "MemorySample preset 14";
        item.group = "quality-gate";
        item.detail = "Preset 14 used by quality-gate workflows";
        item.order = 13;
        item.priority = 4;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 15;
        item.kind = QualityReportKind::AssetIssue;
        item.name = "AssetIssue preset 15";
        item.group = "quality-gate";
        item.detail = "Preset 15 used by quality-gate workflows";
        item.order = 14;
        item.priority = 5;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 16;
        item.kind = QualityReportKind::SceneIssue;
        item.name = "SceneIssue preset 16";
        item.group = "quality-gate";
        item.detail = "Preset 16 used by quality-gate workflows";
        item.order = 15;
        item.priority = 6;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 17;
        item.kind = QualityReportKind::ScriptIssue;
        item.name = "ScriptIssue preset 17";
        item.group = "quality-gate";
        item.detail = "Preset 17 used by quality-gate workflows";
        item.order = 16;
        item.priority = 7;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 18;
        item.kind = QualityReportKind::ExportIssue;
        item.name = "ExportIssue preset 18";
        item.group = "quality-gate";
        item.detail = "Preset 18 used by quality-gate workflows";
        item.order = 17;
        item.priority = 8;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 19;
        item.kind = QualityReportKind::UiIssue;
        item.name = "UiIssue preset 19";
        item.group = "quality-gate";
        item.detail = "Preset 19 used by quality-gate workflows";
        item.order = 18;
        item.priority = 9;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 20;
        item.kind = QualityReportKind::PhysicsIssue;
        item.name = "PhysicsIssue preset 20";
        item.group = "quality-gate";
        item.detail = "Preset 20 used by quality-gate workflows";
        item.order = 19;
        item.priority = 10;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 21;
        item.kind = QualityReportKind::RenderIssue;
        item.name = "RenderIssue preset 21";
        item.group = "quality-gate";
        item.detail = "Preset 21 used by quality-gate workflows";
        item.order = 20;
        item.priority = 1;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 22;
        item.kind = QualityReportKind::ProjectIssue;
        item.name = "ProjectIssue preset 22";
        item.group = "quality-gate";
        item.detail = "Preset 22 used by quality-gate workflows";
        item.order = 21;
        item.priority = 2;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 23;
        item.kind = QualityReportKind::BuildIssue;
        item.name = "BuildIssue preset 23";
        item.group = "quality-gate";
        item.detail = "Preset 23 used by quality-gate workflows";
        item.order = 22;
        item.priority = 3;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 24;
        item.kind = QualityReportKind::Suggestion;
        item.name = "Suggestion preset 24";
        item.group = "quality-gate";
        item.detail = "Preset 24 used by quality-gate workflows";
        item.order = 23;
        item.priority = 4;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 25;
        item.kind = QualityReportKind::FrameSample;
        item.name = "FrameSample preset 25";
        item.group = "quality-gate";
        item.detail = "Preset 25 used by quality-gate workflows";
        item.order = 24;
        item.priority = 5;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 26;
        item.kind = QualityReportKind::MemorySample;
        item.name = "MemorySample preset 26";
        item.group = "quality-gate";
        item.detail = "Preset 26 used by quality-gate workflows";
        item.order = 25;
        item.priority = 6;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 27;
        item.kind = QualityReportKind::AssetIssue;
        item.name = "AssetIssue preset 27";
        item.group = "quality-gate";
        item.detail = "Preset 27 used by quality-gate workflows";
        item.order = 26;
        item.priority = 7;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 28;
        item.kind = QualityReportKind::SceneIssue;
        item.name = "SceneIssue preset 28";
        item.group = "quality-gate";
        item.detail = "Preset 28 used by quality-gate workflows";
        item.order = 27;
        item.priority = 8;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 29;
        item.kind = QualityReportKind::ScriptIssue;
        item.name = "ScriptIssue preset 29";
        item.group = "quality-gate";
        item.detail = "Preset 29 used by quality-gate workflows";
        item.order = 28;
        item.priority = 9;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 30;
        item.kind = QualityReportKind::ExportIssue;
        item.name = "ExportIssue preset 30";
        item.group = "quality-gate";
        item.detail = "Preset 30 used by quality-gate workflows";
        item.order = 29;
        item.priority = 10;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 31;
        item.kind = QualityReportKind::UiIssue;
        item.name = "UiIssue preset 31";
        item.group = "quality-gate";
        item.detail = "Preset 31 used by quality-gate workflows";
        item.order = 30;
        item.priority = 1;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 32;
        item.kind = QualityReportKind::PhysicsIssue;
        item.name = "PhysicsIssue preset 32";
        item.group = "quality-gate";
        item.detail = "Preset 32 used by quality-gate workflows";
        item.order = 31;
        item.priority = 2;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 33;
        item.kind = QualityReportKind::RenderIssue;
        item.name = "RenderIssue preset 33";
        item.group = "quality-gate";
        item.detail = "Preset 33 used by quality-gate workflows";
        item.order = 32;
        item.priority = 3;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 34;
        item.kind = QualityReportKind::ProjectIssue;
        item.name = "ProjectIssue preset 34";
        item.group = "quality-gate";
        item.detail = "Preset 34 used by quality-gate workflows";
        item.order = 33;
        item.priority = 4;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 35;
        item.kind = QualityReportKind::BuildIssue;
        item.name = "BuildIssue preset 35";
        item.group = "quality-gate";
        item.detail = "Preset 35 used by quality-gate workflows";
        item.order = 34;
        item.priority = 5;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 36;
        item.kind = QualityReportKind::Suggestion;
        item.name = "Suggestion preset 36";
        item.group = "quality-gate";
        item.detail = "Preset 36 used by quality-gate workflows";
        item.order = 35;
        item.priority = 6;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 37;
        item.kind = QualityReportKind::FrameSample;
        item.name = "FrameSample preset 37";
        item.group = "quality-gate";
        item.detail = "Preset 37 used by quality-gate workflows";
        item.order = 36;
        item.priority = 7;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 38;
        item.kind = QualityReportKind::MemorySample;
        item.name = "MemorySample preset 38";
        item.group = "quality-gate";
        item.detail = "Preset 38 used by quality-gate workflows";
        item.order = 37;
        item.priority = 8;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 39;
        item.kind = QualityReportKind::AssetIssue;
        item.name = "AssetIssue preset 39";
        item.group = "quality-gate";
        item.detail = "Preset 39 used by quality-gate workflows";
        item.order = 38;
        item.priority = 9;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 40;
        item.kind = QualityReportKind::SceneIssue;
        item.name = "SceneIssue preset 40";
        item.group = "quality-gate";
        item.detail = "Preset 40 used by quality-gate workflows";
        item.order = 39;
        item.priority = 10;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 41;
        item.kind = QualityReportKind::ScriptIssue;
        item.name = "ScriptIssue preset 41";
        item.group = "quality-gate";
        item.detail = "Preset 41 used by quality-gate workflows";
        item.order = 40;
        item.priority = 1;
        items.push_back(item);
    }

    {
        QualityReportItem item;
        item.id = 42;
        item.kind = QualityReportKind::ExportIssue;
        item.name = "ExportIssue preset 42";
        item.group = "quality-gate";
        item.detail = "Preset 42 used by quality-gate workflows";
        item.order = 41;
        item.priority = 2;
        items.push_back(item);
    }

    return items;
}

}
