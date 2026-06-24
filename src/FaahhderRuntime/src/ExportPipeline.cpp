#include "Faahhder/ExportPipeline.hpp"

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

std::string ToString(ExportPipelineKind kind) {
    switch (kind) {
        case ExportPipelineKind::AssetEntry: return "AssetEntry";
        case ExportPipelineKind::ScriptEntry: return "ScriptEntry";
        case ExportPipelineKind::SceneEntry: return "SceneEntry";
        case ExportPipelineKind::ManifestEntry: return "ManifestEntry";
        case ExportPipelineKind::CopyStep: return "CopyStep";
        case ExportPipelineKind::CompileStep: return "CompileStep";
        case ExportPipelineKind::BundleStep: return "BundleStep";
        case ExportPipelineKind::VerifyStep: return "VerifyStep";
        case ExportPipelineKind::LaunchStep: return "LaunchStep";
        case ExportPipelineKind::TemplateFile: return "TemplateFile";
        case ExportPipelineKind::PlatformRule: return "PlatformRule";
        case ExportPipelineKind::ReportLine: return "ReportLine";
    }
    return "AssetEntry";
}

bool TryParseExportPipelineKind(const std::string& text, ExportPipelineKind& out) {
    const std::string lower = Lower(Trim(text));
    if (lower == "assetentry") { out = ExportPipelineKind::AssetEntry; return true; }
    if (lower == "scriptentry") { out = ExportPipelineKind::ScriptEntry; return true; }
    if (lower == "sceneentry") { out = ExportPipelineKind::SceneEntry; return true; }
    if (lower == "manifestentry") { out = ExportPipelineKind::ManifestEntry; return true; }
    if (lower == "copystep") { out = ExportPipelineKind::CopyStep; return true; }
    if (lower == "compilestep") { out = ExportPipelineKind::CompileStep; return true; }
    if (lower == "bundlestep") { out = ExportPipelineKind::BundleStep; return true; }
    if (lower == "verifystep") { out = ExportPipelineKind::VerifyStep; return true; }
    if (lower == "launchstep") { out = ExportPipelineKind::LaunchStep; return true; }
    if (lower == "templatefile") { out = ExportPipelineKind::TemplateFile; return true; }
    if (lower == "platformrule") { out = ExportPipelineKind::PlatformRule; return true; }
    if (lower == "reportline") { out = ExportPipelineKind::ReportLine; return true; }
    return false;
}

int ExportPipeline::Add(ExportPipelineKind kind, std::string name, std::filesystem::path path, std::string group, std::string detail) { ExportPipelineItem value; value.id = nextId_++; value.kind = kind; value.name = std::move(name); value.path = std::move(path); value.group = std::move(group); value.detail = std::move(detail); value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); return entries_.back().id; }
int ExportPipeline::Add(ExportPipelineItem value) { if (value.id <= 0) value.id = nextId_++; nextId_ = std::max(nextId_, value.id + 1); if (value.order < 0) value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); NormalizeOrder(); return entries_.back().id; }
bool ExportPipeline::Upsert(ExportPipelineItem value) { if (value.id > 0) { if (auto* existing = Find(value.id)) { *existing = std::move(value); NormalizeOrder(); return true; } } Add(std::move(value)); return true; }
bool ExportPipeline::Remove(int id) { const auto it = std::remove_if(entries_.begin(), entries_.end(), [id](const ExportPipelineItem& entry) { return entry.id == id; }); if (it == entries_.end()) return false; entries_.erase(it, entries_.end()); NormalizeOrder(); return true; }
bool ExportPipeline::Rename(int id, std::string name) { if (auto* entry = Find(id)) { entry->name = std::move(name); entry->dirty = true; return true; } return false; }
bool ExportPipeline::MoveToGroup(int id, std::string group) { if (auto* entry = Find(id)) { entry->group = std::move(group); entry->dirty = true; return true; } return false; }
bool ExportPipeline::SetPath(int id, std::filesystem::path path) { if (auto* entry = Find(id)) { entry->path = std::move(path); entry->dirty = true; return true; } return false; }
bool ExportPipeline::SetDetail(int id, std::string detail) { if (auto* entry = Find(id)) { entry->detail = std::move(detail); entry->dirty = true; return true; } return false; }
bool ExportPipeline::SetPriority(int id, int priority) { if (auto* entry = Find(id)) { entry->priority = priority; entry->dirty = true; return true; } return false; }
bool ExportPipeline::SetEnabled(int id, bool enabled) { if (auto* entry = Find(id)) { entry->enabled = enabled; entry->dirty = true; return true; } return false; }
bool ExportPipeline::MarkDirty(int id, bool dirty) { if (auto* entry = Find(id)) { entry->dirty = dirty; return true; } return false; }
bool ExportPipeline::ApplyPatch(int id, const ExportPipelineItemPatch& patch) { auto* entry = Find(id); if (!entry) return false; if (patch.kind) entry->kind = *patch.kind; if (patch.name) entry->name = *patch.name; if (patch.path) entry->path = *patch.path; if (patch.group) entry->group = *patch.group; if (patch.detail) entry->detail = *patch.detail; if (patch.order) entry->order = *patch.order; if (patch.priority) entry->priority = *patch.priority; if (patch.enabled) entry->enabled = *patch.enabled; if (patch.dirty) entry->dirty = *patch.dirty; NormalizeOrder(); return true; }
bool ExportPipeline::Reorder(int id, int order) { auto* entry = Find(id); if (!entry) return false; entry->order = std::max(0, order); entry->dirty = true; NormalizeOrder(); return true; }
bool ExportPipeline::MoveBefore(int movingId, int beforeId) { const auto* before = Find(beforeId); if (!before || movingId == beforeId) return false; return Reorder(movingId, before->order); }
bool ExportPipeline::MoveAfter(int movingId, int afterId) { const auto* after = Find(afterId); if (!after || movingId == afterId) return false; return Reorder(movingId, after->order + 1); }
bool ExportPipeline::NormalizeOrder() { std::stable_sort(entries_.begin(), entries_.end(), [](const ExportPipelineItem& a, const ExportPipelineItem& b) { if (a.order != b.order) return a.order < b.order; return a.id < b.id; }); bool changed = false; for (int i = 0; i < static_cast<int>(entries_.size()); ++i) { if (entries_[i].order != i) changed = true; entries_[i].order = i; } return changed; }
bool ExportPipeline::Contains(int id) const { return Find(id) != nullptr; }
ExportPipelineItem* ExportPipeline::Find(int id) { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const ExportPipelineItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const ExportPipelineItem* ExportPipeline::Find(int id) const { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const ExportPipelineItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const ExportPipelineItem* ExportPipeline::FindByName(const std::string& name) const { const std::string target = Lower(name); auto it = std::find_if(entries_.begin(), entries_.end(), [&target](const ExportPipelineItem& entry) { return Lower(entry.name) == target; }); return it == entries_.end() ? nullptr : &*it; }
std::vector<ExportPipelineItem> ExportPipeline::All() const { return entries_; }
std::vector<ExportPipelineItem> ExportPipeline::Enabled() const { std::vector<ExportPipelineItem> out; for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry); return out; }
std::vector<ExportPipelineItem> ExportPipeline::Dirty() const { std::vector<ExportPipelineItem> out; for (const auto& entry : entries_) if (entry.dirty) out.push_back(entry); return out; }
std::vector<ExportPipelineItem> ExportPipeline::ByKind(ExportPipelineKind kind) const { std::vector<ExportPipelineItem> out; for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry); return out; }
std::vector<ExportPipelineItem> ExportPipeline::ByGroup(const std::string& group) const { std::vector<ExportPipelineItem> out; const std::string target = Lower(group); for (const auto& entry : entries_) if (Lower(entry.group) == target) out.push_back(entry); return out; }
std::vector<ExportPipelineItem> ExportPipeline::Search(const std::string& text) const { std::vector<ExportPipelineItem> out; for (const auto& entry : entries_) if (ContainsText(entry.name, text) || ContainsText(entry.group, text) || ContainsText(entry.detail, text) || ContainsText(entry.path.generic_string(), text)) out.push_back(entry); return out; }
std::vector<ExportPipelineItem> ExportPipeline::TopPriority(int limit) const { std::vector<ExportPipelineItem> out = entries_; std::stable_sort(out.begin(), out.end(), [](const ExportPipelineItem& a, const ExportPipelineItem& b) { if (a.priority != b.priority) return a.priority > b.priority; return a.order < b.order; }); if (limit >= 0 && static_cast<int>(out.size()) > limit) out.resize(static_cast<size_t>(limit)); return out; }
std::set<std::string> ExportPipeline::Groups() const { std::set<std::string> groups; for (const auto& entry : entries_) if (!entry.group.empty()) groups.insert(entry.group); return groups; }
ExportPipelineItemSummary ExportPipeline::Summarize() const { ExportPipelineItemSummary summary; summary.total = static_cast<int>(entries_.size()); for (const auto& entry : entries_) { if (entry.enabled) summary.enabled++; if (entry.dirty) summary.dirty++; summary.priorityTotal += entry.priority; summary.byKind[ToString(entry.kind)]++; if (!entry.group.empty()) summary.byGroup[entry.group]++; } return summary; }
std::string ExportPipeline::Serialize() const { std::ostringstream out; for (const auto& entry : entries_) out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.group) << "|" << EscapeField(entry.detail) << "|" << entry.order << "|" << entry.priority << "|" << (entry.enabled ? 1 : 0) << "|" << (entry.dirty ? 1 : 0) << "\n"; return out.str(); }
bool ExportPipeline::Deserialize(const std::string& text) { entries_.clear(); nextId_ = 1; std::istringstream input(text); std::string line; while (std::getline(input, line)) { if (Trim(line).empty()) continue; const auto fields = SplitFields(line); if (fields.size() != 10) return false; ExportPipelineItem entry; entry.id = std::stoi(fields[0]); if (!TryParseExportPipelineKind(fields[1], entry.kind)) return false; entry.name = fields[2]; entry.path = fields[3]; entry.group = fields[4]; entry.detail = fields[5]; entry.order = std::stoi(fields[6]); entry.priority = std::stoi(fields[7]); entry.enabled = fields[8] == "1"; entry.dirty = fields[9] == "1"; nextId_ = std::max(nextId_, entry.id + 1); entries_.push_back(std::move(entry)); } NormalizeOrder(); return true; }
std::string ExportPipeline::Report() const { const auto summary = Summarize(); std::ostringstream out; out << "total=" << summary.total << " enabled=" << summary.enabled << " dirty=" << summary.dirty << " priority=" << summary.priorityTotal; for (const auto& row : summary.byKind) out << "\n" << row.first << "=" << row.second; for (const auto& row : summary.byGroup) out << "\ngroup:" << row.first << "=" << row.second; return out.str(); }
int ExportPipeline::NextId() const { return nextId_; }
int ExportPipeline::Size() const { return static_cast<int>(entries_.size()); }
void ExportPipeline::Clear() { entries_.clear(); nextId_ = 1; }

std::vector<ExportPipelineItem> BuildWindowsExportPipeline(const std::filesystem::path& projectRoot, const std::filesystem::path& outputRoot) {
    std::vector<ExportPipelineItem> items;
    const struct Row { ExportPipelineKind kind; const char* name; const char* group; const char* detail; int priority; } rows[] = {
        {ExportPipelineKind::VerifyStep, "Validate project file", "verify", "Checks project metadata before export", 10},
        {ExportPipelineKind::SceneEntry, "Collect scenes", "collect", "Adds scene documents to the export manifest", 9},
        {ExportPipelineKind::AssetEntry, "Collect assets", "collect", "Adds textures, scripts, and data files", 9},
        {ExportPipelineKind::BundleStep, "Pack assets", "pack", "Writes the packed asset folder", 8},
        {ExportPipelineKind::CompileStep, "Build game target", "build", "Compiles the standalone game executable", 8},
        {ExportPipelineKind::CopyStep, "Copy runtime files", "copy", "Copies files needed beside the executable", 7},
        {ExportPipelineKind::LaunchStep, "Smoke launch", "verify", "Runs the exported game once from output", 6},
    };
    int id = 1;
    for (const auto& row : rows) {
        ExportPipelineItem item;
        item.id = id++;
        item.kind = row.kind;
        item.name = row.name;
        item.path = row.kind == ExportPipelineKind::CopyStep ? outputRoot : projectRoot;
        item.group = row.group;
        item.detail = row.detail;
        item.order = static_cast<int>(items.size());
        item.priority = row.priority;
        items.push_back(std::move(item));
    }
    return items;
}


std::vector<ExportPipelineItem> BuildExportPipelinePresetMap() {
    std::vector<ExportPipelineItem> items;
    items.reserve(30);

    {
        ExportPipelineItem item;
        item.id = 1;
        item.kind = ExportPipelineKind::AssetEntry;
        item.name = "AssetEntry preset 1";
        item.group = "export-step";
        item.detail = "Preset 1 used by export-step workflows";
        item.order = 0;
        item.priority = 1;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 2;
        item.kind = ExportPipelineKind::ScriptEntry;
        item.name = "ScriptEntry preset 2";
        item.group = "export-step";
        item.detail = "Preset 2 used by export-step workflows";
        item.order = 1;
        item.priority = 2;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 3;
        item.kind = ExportPipelineKind::SceneEntry;
        item.name = "SceneEntry preset 3";
        item.group = "export-step";
        item.detail = "Preset 3 used by export-step workflows";
        item.order = 2;
        item.priority = 3;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 4;
        item.kind = ExportPipelineKind::ManifestEntry;
        item.name = "ManifestEntry preset 4";
        item.group = "export-step";
        item.detail = "Preset 4 used by export-step workflows";
        item.order = 3;
        item.priority = 4;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 5;
        item.kind = ExportPipelineKind::CopyStep;
        item.name = "CopyStep preset 5";
        item.group = "export-step";
        item.detail = "Preset 5 used by export-step workflows";
        item.order = 4;
        item.priority = 5;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 6;
        item.kind = ExportPipelineKind::CompileStep;
        item.name = "CompileStep preset 6";
        item.group = "export-step";
        item.detail = "Preset 6 used by export-step workflows";
        item.order = 5;
        item.priority = 6;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 7;
        item.kind = ExportPipelineKind::BundleStep;
        item.name = "BundleStep preset 7";
        item.group = "export-step";
        item.detail = "Preset 7 used by export-step workflows";
        item.order = 6;
        item.priority = 7;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 8;
        item.kind = ExportPipelineKind::VerifyStep;
        item.name = "VerifyStep preset 8";
        item.group = "export-step";
        item.detail = "Preset 8 used by export-step workflows";
        item.order = 7;
        item.priority = 8;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 9;
        item.kind = ExportPipelineKind::LaunchStep;
        item.name = "LaunchStep preset 9";
        item.group = "export-step";
        item.detail = "Preset 9 used by export-step workflows";
        item.order = 8;
        item.priority = 9;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 10;
        item.kind = ExportPipelineKind::TemplateFile;
        item.name = "TemplateFile preset 10";
        item.group = "export-step";
        item.detail = "Preset 10 used by export-step workflows";
        item.order = 9;
        item.priority = 10;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 11;
        item.kind = ExportPipelineKind::PlatformRule;
        item.name = "PlatformRule preset 11";
        item.group = "export-step";
        item.detail = "Preset 11 used by export-step workflows";
        item.order = 10;
        item.priority = 1;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 12;
        item.kind = ExportPipelineKind::ReportLine;
        item.name = "ReportLine preset 12";
        item.group = "export-step";
        item.detail = "Preset 12 used by export-step workflows";
        item.order = 11;
        item.priority = 2;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 13;
        item.kind = ExportPipelineKind::AssetEntry;
        item.name = "AssetEntry preset 13";
        item.group = "export-step";
        item.detail = "Preset 13 used by export-step workflows";
        item.order = 12;
        item.priority = 3;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 14;
        item.kind = ExportPipelineKind::ScriptEntry;
        item.name = "ScriptEntry preset 14";
        item.group = "export-step";
        item.detail = "Preset 14 used by export-step workflows";
        item.order = 13;
        item.priority = 4;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 15;
        item.kind = ExportPipelineKind::SceneEntry;
        item.name = "SceneEntry preset 15";
        item.group = "export-step";
        item.detail = "Preset 15 used by export-step workflows";
        item.order = 14;
        item.priority = 5;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 16;
        item.kind = ExportPipelineKind::ManifestEntry;
        item.name = "ManifestEntry preset 16";
        item.group = "export-step";
        item.detail = "Preset 16 used by export-step workflows";
        item.order = 15;
        item.priority = 6;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 17;
        item.kind = ExportPipelineKind::CopyStep;
        item.name = "CopyStep preset 17";
        item.group = "export-step";
        item.detail = "Preset 17 used by export-step workflows";
        item.order = 16;
        item.priority = 7;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 18;
        item.kind = ExportPipelineKind::CompileStep;
        item.name = "CompileStep preset 18";
        item.group = "export-step";
        item.detail = "Preset 18 used by export-step workflows";
        item.order = 17;
        item.priority = 8;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 19;
        item.kind = ExportPipelineKind::BundleStep;
        item.name = "BundleStep preset 19";
        item.group = "export-step";
        item.detail = "Preset 19 used by export-step workflows";
        item.order = 18;
        item.priority = 9;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 20;
        item.kind = ExportPipelineKind::VerifyStep;
        item.name = "VerifyStep preset 20";
        item.group = "export-step";
        item.detail = "Preset 20 used by export-step workflows";
        item.order = 19;
        item.priority = 10;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 21;
        item.kind = ExportPipelineKind::LaunchStep;
        item.name = "LaunchStep preset 21";
        item.group = "export-step";
        item.detail = "Preset 21 used by export-step workflows";
        item.order = 20;
        item.priority = 1;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 22;
        item.kind = ExportPipelineKind::TemplateFile;
        item.name = "TemplateFile preset 22";
        item.group = "export-step";
        item.detail = "Preset 22 used by export-step workflows";
        item.order = 21;
        item.priority = 2;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 23;
        item.kind = ExportPipelineKind::PlatformRule;
        item.name = "PlatformRule preset 23";
        item.group = "export-step";
        item.detail = "Preset 23 used by export-step workflows";
        item.order = 22;
        item.priority = 3;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 24;
        item.kind = ExportPipelineKind::ReportLine;
        item.name = "ReportLine preset 24";
        item.group = "export-step";
        item.detail = "Preset 24 used by export-step workflows";
        item.order = 23;
        item.priority = 4;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 25;
        item.kind = ExportPipelineKind::AssetEntry;
        item.name = "AssetEntry preset 25";
        item.group = "export-step";
        item.detail = "Preset 25 used by export-step workflows";
        item.order = 24;
        item.priority = 5;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 26;
        item.kind = ExportPipelineKind::ScriptEntry;
        item.name = "ScriptEntry preset 26";
        item.group = "export-step";
        item.detail = "Preset 26 used by export-step workflows";
        item.order = 25;
        item.priority = 6;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 27;
        item.kind = ExportPipelineKind::SceneEntry;
        item.name = "SceneEntry preset 27";
        item.group = "export-step";
        item.detail = "Preset 27 used by export-step workflows";
        item.order = 26;
        item.priority = 7;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 28;
        item.kind = ExportPipelineKind::ManifestEntry;
        item.name = "ManifestEntry preset 28";
        item.group = "export-step";
        item.detail = "Preset 28 used by export-step workflows";
        item.order = 27;
        item.priority = 8;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 29;
        item.kind = ExportPipelineKind::CopyStep;
        item.name = "CopyStep preset 29";
        item.group = "export-step";
        item.detail = "Preset 29 used by export-step workflows";
        item.order = 28;
        item.priority = 9;
        items.push_back(item);
    }

    {
        ExportPipelineItem item;
        item.id = 30;
        item.kind = ExportPipelineKind::CompileStep;
        item.name = "CompileStep preset 30";
        item.group = "export-step";
        item.detail = "Preset 30 used by export-step workflows";
        item.order = 29;
        item.priority = 10;
        items.push_back(item);
    }

    return items;
}

}
