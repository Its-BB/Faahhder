#include "Faahhder/CodeWorkspace.hpp"

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

std::string ToString(CodeWorkspaceKind kind) {
    switch (kind) {
        case CodeWorkspaceKind::SourceFile: return "SourceFile";
        case CodeWorkspaceKind::ScriptFile: return "ScriptFile";
        case CodeWorkspaceKind::OpenTab: return "OpenTab";
        case CodeWorkspaceKind::Breakpoint: return "Breakpoint";
        case CodeWorkspaceKind::Diagnostic: return "Diagnostic";
        case CodeWorkspaceKind::SearchResult: return "SearchResult";
        case CodeWorkspaceKind::Command: return "Command";
        case CodeWorkspaceKind::Snippet: return "Snippet";
        case CodeWorkspaceKind::WatchValue: return "WatchValue";
        case CodeWorkspaceKind::BuildMessage: return "BuildMessage";
        case CodeWorkspaceKind::Task: return "Task";
        case CodeWorkspaceKind::Bookmark: return "Bookmark";
    }
    return "SourceFile";
}

bool TryParseCodeWorkspaceKind(const std::string& text, CodeWorkspaceKind& out) {
    const std::string lower = Lower(Trim(text));
    if (lower == "sourcefile") { out = CodeWorkspaceKind::SourceFile; return true; }
    if (lower == "scriptfile") { out = CodeWorkspaceKind::ScriptFile; return true; }
    if (lower == "opentab") { out = CodeWorkspaceKind::OpenTab; return true; }
    if (lower == "breakpoint") { out = CodeWorkspaceKind::Breakpoint; return true; }
    if (lower == "diagnostic") { out = CodeWorkspaceKind::Diagnostic; return true; }
    if (lower == "searchresult") { out = CodeWorkspaceKind::SearchResult; return true; }
    if (lower == "command") { out = CodeWorkspaceKind::Command; return true; }
    if (lower == "snippet") { out = CodeWorkspaceKind::Snippet; return true; }
    if (lower == "watchvalue") { out = CodeWorkspaceKind::WatchValue; return true; }
    if (lower == "buildmessage") { out = CodeWorkspaceKind::BuildMessage; return true; }
    if (lower == "task") { out = CodeWorkspaceKind::Task; return true; }
    if (lower == "bookmark") { out = CodeWorkspaceKind::Bookmark; return true; }
    return false;
}

int CodeWorkspace::Add(CodeWorkspaceKind kind, std::string name, std::filesystem::path path, std::string group, std::string detail) { CodeWorkspaceItem value; value.id = nextId_++; value.kind = kind; value.name = std::move(name); value.path = std::move(path); value.group = std::move(group); value.detail = std::move(detail); value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); return entries_.back().id; }
int CodeWorkspace::Add(CodeWorkspaceItem value) { if (value.id <= 0) value.id = nextId_++; nextId_ = std::max(nextId_, value.id + 1); if (value.order < 0) value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); NormalizeOrder(); return entries_.back().id; }
bool CodeWorkspace::Upsert(CodeWorkspaceItem value) { if (value.id > 0) { if (auto* existing = Find(value.id)) { *existing = std::move(value); NormalizeOrder(); return true; } } Add(std::move(value)); return true; }
bool CodeWorkspace::Remove(int id) { const auto it = std::remove_if(entries_.begin(), entries_.end(), [id](const CodeWorkspaceItem& entry) { return entry.id == id; }); if (it == entries_.end()) return false; entries_.erase(it, entries_.end()); NormalizeOrder(); return true; }
bool CodeWorkspace::Rename(int id, std::string name) { if (auto* entry = Find(id)) { entry->name = std::move(name); entry->dirty = true; return true; } return false; }
bool CodeWorkspace::MoveToGroup(int id, std::string group) { if (auto* entry = Find(id)) { entry->group = std::move(group); entry->dirty = true; return true; } return false; }
bool CodeWorkspace::SetPath(int id, std::filesystem::path path) { if (auto* entry = Find(id)) { entry->path = std::move(path); entry->dirty = true; return true; } return false; }
bool CodeWorkspace::SetDetail(int id, std::string detail) { if (auto* entry = Find(id)) { entry->detail = std::move(detail); entry->dirty = true; return true; } return false; }
bool CodeWorkspace::SetPriority(int id, int priority) { if (auto* entry = Find(id)) { entry->priority = priority; entry->dirty = true; return true; } return false; }
bool CodeWorkspace::SetEnabled(int id, bool enabled) { if (auto* entry = Find(id)) { entry->enabled = enabled; entry->dirty = true; return true; } return false; }
bool CodeWorkspace::MarkDirty(int id, bool dirty) { if (auto* entry = Find(id)) { entry->dirty = dirty; return true; } return false; }
bool CodeWorkspace::ApplyPatch(int id, const CodeWorkspaceItemPatch& patch) { auto* entry = Find(id); if (!entry) return false; if (patch.kind) entry->kind = *patch.kind; if (patch.name) entry->name = *patch.name; if (patch.path) entry->path = *patch.path; if (patch.group) entry->group = *patch.group; if (patch.detail) entry->detail = *patch.detail; if (patch.order) entry->order = *patch.order; if (patch.priority) entry->priority = *patch.priority; if (patch.enabled) entry->enabled = *patch.enabled; if (patch.dirty) entry->dirty = *patch.dirty; NormalizeOrder(); return true; }
bool CodeWorkspace::Reorder(int id, int order) { auto* entry = Find(id); if (!entry) return false; entry->order = std::max(0, order); entry->dirty = true; NormalizeOrder(); return true; }
bool CodeWorkspace::MoveBefore(int movingId, int beforeId) { const auto* before = Find(beforeId); if (!before || movingId == beforeId) return false; return Reorder(movingId, before->order); }
bool CodeWorkspace::MoveAfter(int movingId, int afterId) { const auto* after = Find(afterId); if (!after || movingId == afterId) return false; return Reorder(movingId, after->order + 1); }
bool CodeWorkspace::NormalizeOrder() { std::stable_sort(entries_.begin(), entries_.end(), [](const CodeWorkspaceItem& a, const CodeWorkspaceItem& b) { if (a.order != b.order) return a.order < b.order; return a.id < b.id; }); bool changed = false; for (int i = 0; i < static_cast<int>(entries_.size()); ++i) { if (entries_[i].order != i) changed = true; entries_[i].order = i; } return changed; }
bool CodeWorkspace::Contains(int id) const { return Find(id) != nullptr; }
CodeWorkspaceItem* CodeWorkspace::Find(int id) { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const CodeWorkspaceItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const CodeWorkspaceItem* CodeWorkspace::Find(int id) const { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const CodeWorkspaceItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const CodeWorkspaceItem* CodeWorkspace::FindByName(const std::string& name) const { const std::string target = Lower(name); auto it = std::find_if(entries_.begin(), entries_.end(), [&target](const CodeWorkspaceItem& entry) { return Lower(entry.name) == target; }); return it == entries_.end() ? nullptr : &*it; }
std::vector<CodeWorkspaceItem> CodeWorkspace::All() const { return entries_; }
std::vector<CodeWorkspaceItem> CodeWorkspace::Enabled() const { std::vector<CodeWorkspaceItem> out; for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry); return out; }
std::vector<CodeWorkspaceItem> CodeWorkspace::Dirty() const { std::vector<CodeWorkspaceItem> out; for (const auto& entry : entries_) if (entry.dirty) out.push_back(entry); return out; }
std::vector<CodeWorkspaceItem> CodeWorkspace::ByKind(CodeWorkspaceKind kind) const { std::vector<CodeWorkspaceItem> out; for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry); return out; }
std::vector<CodeWorkspaceItem> CodeWorkspace::ByGroup(const std::string& group) const { std::vector<CodeWorkspaceItem> out; const std::string target = Lower(group); for (const auto& entry : entries_) if (Lower(entry.group) == target) out.push_back(entry); return out; }
std::vector<CodeWorkspaceItem> CodeWorkspace::Search(const std::string& text) const { std::vector<CodeWorkspaceItem> out; for (const auto& entry : entries_) if (ContainsText(entry.name, text) || ContainsText(entry.group, text) || ContainsText(entry.detail, text) || ContainsText(entry.path.generic_string(), text)) out.push_back(entry); return out; }
std::vector<CodeWorkspaceItem> CodeWorkspace::TopPriority(int limit) const { std::vector<CodeWorkspaceItem> out = entries_; std::stable_sort(out.begin(), out.end(), [](const CodeWorkspaceItem& a, const CodeWorkspaceItem& b) { if (a.priority != b.priority) return a.priority > b.priority; return a.order < b.order; }); if (limit >= 0 && static_cast<int>(out.size()) > limit) out.resize(static_cast<size_t>(limit)); return out; }
std::set<std::string> CodeWorkspace::Groups() const { std::set<std::string> groups; for (const auto& entry : entries_) if (!entry.group.empty()) groups.insert(entry.group); return groups; }
CodeWorkspaceItemSummary CodeWorkspace::Summarize() const { CodeWorkspaceItemSummary summary; summary.total = static_cast<int>(entries_.size()); for (const auto& entry : entries_) { if (entry.enabled) summary.enabled++; if (entry.dirty) summary.dirty++; summary.priorityTotal += entry.priority; summary.byKind[ToString(entry.kind)]++; if (!entry.group.empty()) summary.byGroup[entry.group]++; } return summary; }
std::string CodeWorkspace::Serialize() const { std::ostringstream out; for (const auto& entry : entries_) out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.group) << "|" << EscapeField(entry.detail) << "|" << entry.order << "|" << entry.priority << "|" << (entry.enabled ? 1 : 0) << "|" << (entry.dirty ? 1 : 0) << "\n"; return out.str(); }
bool CodeWorkspace::Deserialize(const std::string& text) { entries_.clear(); nextId_ = 1; std::istringstream input(text); std::string line; while (std::getline(input, line)) { if (Trim(line).empty()) continue; const auto fields = SplitFields(line); if (fields.size() != 10) return false; CodeWorkspaceItem entry; entry.id = std::stoi(fields[0]); if (!TryParseCodeWorkspaceKind(fields[1], entry.kind)) return false; entry.name = fields[2]; entry.path = fields[3]; entry.group = fields[4]; entry.detail = fields[5]; entry.order = std::stoi(fields[6]); entry.priority = std::stoi(fields[7]); entry.enabled = fields[8] == "1"; entry.dirty = fields[9] == "1"; nextId_ = std::max(nextId_, entry.id + 1); entries_.push_back(std::move(entry)); } NormalizeOrder(); return true; }
std::string CodeWorkspace::Report() const { const auto summary = Summarize(); std::ostringstream out; out << "total=" << summary.total << " enabled=" << summary.enabled << " dirty=" << summary.dirty << " priority=" << summary.priorityTotal; for (const auto& row : summary.byKind) out << "\n" << row.first << "=" << row.second; for (const auto& row : summary.byGroup) out << "\ngroup:" << row.first << "=" << row.second; return out.str(); }
int CodeWorkspace::NextId() const { return nextId_; }
int CodeWorkspace::Size() const { return static_cast<int>(entries_.size()); }
void CodeWorkspace::Clear() { entries_.clear(); nextId_ = 1; }

std::vector<CodeWorkspaceItem> BuildDefaultCodeWorkspace(const std::filesystem::path& projectRoot) {
    std::vector<CodeWorkspaceItem> items;
    const struct Row { CodeWorkspaceKind kind; const char* name; const char* path; const char* group; const char* detail; int priority; } rows[] = {
        {CodeWorkspaceKind::SourceFile, "Editor main", "src/Faahhder/src/main.cpp", "editor", "Window loop and editor controls", 9},
        {CodeWorkspaceKind::SourceFile, "Runtime umbrella", "src/FaahhderRuntime/include/Faahhder/Faahhder.hpp", "runtime", "Public engine API includes", 8},
        {CodeWorkspaceKind::ScriptFile, "Project scripts", "projects", "scripts", "Lua files owned by the active project", 7},
        {CodeWorkspaceKind::Diagnostic, "Build diagnostics", "build", "console", "Compiler output grouped by file", 6},
        {CodeWorkspaceKind::Snippet, "Entity movement", "snippets/movement.lua", "scripts", "Reusable Lua movement starting point", 5},
    };
    int id = 1;
    for (const auto& row : rows) {
        CodeWorkspaceItem item;
        item.id = id++;
        item.kind = row.kind;
        item.name = row.name;
        item.path = projectRoot / row.path;
        item.group = row.group;
        item.detail = row.detail;
        item.order = static_cast<int>(items.size());
        item.priority = row.priority;
        items.push_back(std::move(item));
    }
    return items;
}


std::vector<CodeWorkspaceItem> BuildCodeWorkspaceActionMap() {
    std::vector<CodeWorkspaceItem> items;
    items.reserve(24);

    {
        CodeWorkspaceItem item;
        item.id = 1;
        item.kind = CodeWorkspaceKind::SourceFile;
        item.name = "SourceFile preset 1";
        item.group = "editor-action";
        item.detail = "Preset 1 used by editor-action workflows";
        item.order = 0;
        item.priority = 1;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 2;
        item.kind = CodeWorkspaceKind::ScriptFile;
        item.name = "ScriptFile preset 2";
        item.group = "editor-action";
        item.detail = "Preset 2 used by editor-action workflows";
        item.order = 1;
        item.priority = 2;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 3;
        item.kind = CodeWorkspaceKind::OpenTab;
        item.name = "OpenTab preset 3";
        item.group = "editor-action";
        item.detail = "Preset 3 used by editor-action workflows";
        item.order = 2;
        item.priority = 3;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 4;
        item.kind = CodeWorkspaceKind::Breakpoint;
        item.name = "Breakpoint preset 4";
        item.group = "editor-action";
        item.detail = "Preset 4 used by editor-action workflows";
        item.order = 3;
        item.priority = 4;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 5;
        item.kind = CodeWorkspaceKind::Diagnostic;
        item.name = "Diagnostic preset 5";
        item.group = "editor-action";
        item.detail = "Preset 5 used by editor-action workflows";
        item.order = 4;
        item.priority = 5;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 6;
        item.kind = CodeWorkspaceKind::SearchResult;
        item.name = "SearchResult preset 6";
        item.group = "editor-action";
        item.detail = "Preset 6 used by editor-action workflows";
        item.order = 5;
        item.priority = 6;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 7;
        item.kind = CodeWorkspaceKind::Command;
        item.name = "Command preset 7";
        item.group = "editor-action";
        item.detail = "Preset 7 used by editor-action workflows";
        item.order = 6;
        item.priority = 7;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 8;
        item.kind = CodeWorkspaceKind::Snippet;
        item.name = "Snippet preset 8";
        item.group = "editor-action";
        item.detail = "Preset 8 used by editor-action workflows";
        item.order = 7;
        item.priority = 8;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 9;
        item.kind = CodeWorkspaceKind::WatchValue;
        item.name = "WatchValue preset 9";
        item.group = "editor-action";
        item.detail = "Preset 9 used by editor-action workflows";
        item.order = 8;
        item.priority = 9;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 10;
        item.kind = CodeWorkspaceKind::BuildMessage;
        item.name = "BuildMessage preset 10";
        item.group = "editor-action";
        item.detail = "Preset 10 used by editor-action workflows";
        item.order = 9;
        item.priority = 10;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 11;
        item.kind = CodeWorkspaceKind::Task;
        item.name = "Task preset 11";
        item.group = "editor-action";
        item.detail = "Preset 11 used by editor-action workflows";
        item.order = 10;
        item.priority = 1;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 12;
        item.kind = CodeWorkspaceKind::Bookmark;
        item.name = "Bookmark preset 12";
        item.group = "editor-action";
        item.detail = "Preset 12 used by editor-action workflows";
        item.order = 11;
        item.priority = 2;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 13;
        item.kind = CodeWorkspaceKind::SourceFile;
        item.name = "SourceFile preset 13";
        item.group = "editor-action";
        item.detail = "Preset 13 used by editor-action workflows";
        item.order = 12;
        item.priority = 3;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 14;
        item.kind = CodeWorkspaceKind::ScriptFile;
        item.name = "ScriptFile preset 14";
        item.group = "editor-action";
        item.detail = "Preset 14 used by editor-action workflows";
        item.order = 13;
        item.priority = 4;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 15;
        item.kind = CodeWorkspaceKind::OpenTab;
        item.name = "OpenTab preset 15";
        item.group = "editor-action";
        item.detail = "Preset 15 used by editor-action workflows";
        item.order = 14;
        item.priority = 5;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 16;
        item.kind = CodeWorkspaceKind::Breakpoint;
        item.name = "Breakpoint preset 16";
        item.group = "editor-action";
        item.detail = "Preset 16 used by editor-action workflows";
        item.order = 15;
        item.priority = 6;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 17;
        item.kind = CodeWorkspaceKind::Diagnostic;
        item.name = "Diagnostic preset 17";
        item.group = "editor-action";
        item.detail = "Preset 17 used by editor-action workflows";
        item.order = 16;
        item.priority = 7;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 18;
        item.kind = CodeWorkspaceKind::SearchResult;
        item.name = "SearchResult preset 18";
        item.group = "editor-action";
        item.detail = "Preset 18 used by editor-action workflows";
        item.order = 17;
        item.priority = 8;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 19;
        item.kind = CodeWorkspaceKind::Command;
        item.name = "Command preset 19";
        item.group = "editor-action";
        item.detail = "Preset 19 used by editor-action workflows";
        item.order = 18;
        item.priority = 9;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 20;
        item.kind = CodeWorkspaceKind::Snippet;
        item.name = "Snippet preset 20";
        item.group = "editor-action";
        item.detail = "Preset 20 used by editor-action workflows";
        item.order = 19;
        item.priority = 10;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 21;
        item.kind = CodeWorkspaceKind::WatchValue;
        item.name = "WatchValue preset 21";
        item.group = "editor-action";
        item.detail = "Preset 21 used by editor-action workflows";
        item.order = 20;
        item.priority = 1;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 22;
        item.kind = CodeWorkspaceKind::BuildMessage;
        item.name = "BuildMessage preset 22";
        item.group = "editor-action";
        item.detail = "Preset 22 used by editor-action workflows";
        item.order = 21;
        item.priority = 2;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 23;
        item.kind = CodeWorkspaceKind::Task;
        item.name = "Task preset 23";
        item.group = "editor-action";
        item.detail = "Preset 23 used by editor-action workflows";
        item.order = 22;
        item.priority = 3;
        items.push_back(item);
    }

    {
        CodeWorkspaceItem item;
        item.id = 24;
        item.kind = CodeWorkspaceKind::Bookmark;
        item.name = "Bookmark preset 24";
        item.group = "editor-action";
        item.detail = "Preset 24 used by editor-action workflows";
        item.order = 23;
        item.priority = 4;
        items.push_back(item);
    }

    return items;
}

}
