#include "Faahhder/SnakeAuthoring.hpp"

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

std::string ToString(SnakeAuthoringKind kind) {
    switch (kind) {
        case SnakeAuthoringKind::BoardRule: return "BoardRule";
        case SnakeAuthoringKind::FoodRule: return "FoodRule";
        case SnakeAuthoringKind::SnakePart: return "SnakePart";
        case SnakeAuthoringKind::InputRule: return "InputRule";
        case SnakeAuthoringKind::ScoreRule: return "ScoreRule";
        case SnakeAuthoringKind::HazardRule: return "HazardRule";
        case SnakeAuthoringKind::LevelPreset: return "LevelPreset";
        case SnakeAuthoringKind::SpeedCurve: return "SpeedCurve";
        case SnakeAuthoringKind::SpawnRule: return "SpawnRule";
        case SnakeAuthoringKind::SkinRule: return "SkinRule";
        case SnakeAuthoringKind::WinRule: return "WinRule";
        case SnakeAuthoringKind::ReplayMarker: return "ReplayMarker";
    }
    return "BoardRule";
}

bool TryParseSnakeAuthoringKind(const std::string& text, SnakeAuthoringKind& out) {
    const std::string lower = Lower(Trim(text));
    if (lower == "boardrule") { out = SnakeAuthoringKind::BoardRule; return true; }
    if (lower == "foodrule") { out = SnakeAuthoringKind::FoodRule; return true; }
    if (lower == "snakepart") { out = SnakeAuthoringKind::SnakePart; return true; }
    if (lower == "inputrule") { out = SnakeAuthoringKind::InputRule; return true; }
    if (lower == "scorerule") { out = SnakeAuthoringKind::ScoreRule; return true; }
    if (lower == "hazardrule") { out = SnakeAuthoringKind::HazardRule; return true; }
    if (lower == "levelpreset") { out = SnakeAuthoringKind::LevelPreset; return true; }
    if (lower == "speedcurve") { out = SnakeAuthoringKind::SpeedCurve; return true; }
    if (lower == "spawnrule") { out = SnakeAuthoringKind::SpawnRule; return true; }
    if (lower == "skinrule") { out = SnakeAuthoringKind::SkinRule; return true; }
    if (lower == "winrule") { out = SnakeAuthoringKind::WinRule; return true; }
    if (lower == "replaymarker") { out = SnakeAuthoringKind::ReplayMarker; return true; }
    return false;
}

int SnakeAuthoring::Add(SnakeAuthoringKind kind, std::string name, std::filesystem::path path, std::string group, std::string detail) { SnakeAuthoringItem value; value.id = nextId_++; value.kind = kind; value.name = std::move(name); value.path = std::move(path); value.group = std::move(group); value.detail = std::move(detail); value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); return entries_.back().id; }
int SnakeAuthoring::Add(SnakeAuthoringItem value) { if (value.id <= 0) value.id = nextId_++; nextId_ = std::max(nextId_, value.id + 1); if (value.order < 0) value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); NormalizeOrder(); return entries_.back().id; }
bool SnakeAuthoring::Upsert(SnakeAuthoringItem value) { if (value.id > 0) { if (auto* existing = Find(value.id)) { *existing = std::move(value); NormalizeOrder(); return true; } } Add(std::move(value)); return true; }
bool SnakeAuthoring::Remove(int id) { const auto it = std::remove_if(entries_.begin(), entries_.end(), [id](const SnakeAuthoringItem& entry) { return entry.id == id; }); if (it == entries_.end()) return false; entries_.erase(it, entries_.end()); NormalizeOrder(); return true; }
bool SnakeAuthoring::Rename(int id, std::string name) { if (auto* entry = Find(id)) { entry->name = std::move(name); entry->dirty = true; return true; } return false; }
bool SnakeAuthoring::MoveToGroup(int id, std::string group) { if (auto* entry = Find(id)) { entry->group = std::move(group); entry->dirty = true; return true; } return false; }
bool SnakeAuthoring::SetPath(int id, std::filesystem::path path) { if (auto* entry = Find(id)) { entry->path = std::move(path); entry->dirty = true; return true; } return false; }
bool SnakeAuthoring::SetDetail(int id, std::string detail) { if (auto* entry = Find(id)) { entry->detail = std::move(detail); entry->dirty = true; return true; } return false; }
bool SnakeAuthoring::SetPriority(int id, int priority) { if (auto* entry = Find(id)) { entry->priority = priority; entry->dirty = true; return true; } return false; }
bool SnakeAuthoring::SetEnabled(int id, bool enabled) { if (auto* entry = Find(id)) { entry->enabled = enabled; entry->dirty = true; return true; } return false; }
bool SnakeAuthoring::MarkDirty(int id, bool dirty) { if (auto* entry = Find(id)) { entry->dirty = dirty; return true; } return false; }
bool SnakeAuthoring::ApplyPatch(int id, const SnakeAuthoringItemPatch& patch) { auto* entry = Find(id); if (!entry) return false; if (patch.kind) entry->kind = *patch.kind; if (patch.name) entry->name = *patch.name; if (patch.path) entry->path = *patch.path; if (patch.group) entry->group = *patch.group; if (patch.detail) entry->detail = *patch.detail; if (patch.order) entry->order = *patch.order; if (patch.priority) entry->priority = *patch.priority; if (patch.enabled) entry->enabled = *patch.enabled; if (patch.dirty) entry->dirty = *patch.dirty; NormalizeOrder(); return true; }
bool SnakeAuthoring::Reorder(int id, int order) { auto* entry = Find(id); if (!entry) return false; entry->order = std::max(0, order); entry->dirty = true; NormalizeOrder(); return true; }
bool SnakeAuthoring::MoveBefore(int movingId, int beforeId) { const auto* before = Find(beforeId); if (!before || movingId == beforeId) return false; return Reorder(movingId, before->order); }
bool SnakeAuthoring::MoveAfter(int movingId, int afterId) { const auto* after = Find(afterId); if (!after || movingId == afterId) return false; return Reorder(movingId, after->order + 1); }
bool SnakeAuthoring::NormalizeOrder() { std::stable_sort(entries_.begin(), entries_.end(), [](const SnakeAuthoringItem& a, const SnakeAuthoringItem& b) { if (a.order != b.order) return a.order < b.order; return a.id < b.id; }); bool changed = false; for (int i = 0; i < static_cast<int>(entries_.size()); ++i) { if (entries_[i].order != i) changed = true; entries_[i].order = i; } return changed; }
bool SnakeAuthoring::Contains(int id) const { return Find(id) != nullptr; }
SnakeAuthoringItem* SnakeAuthoring::Find(int id) { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const SnakeAuthoringItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const SnakeAuthoringItem* SnakeAuthoring::Find(int id) const { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const SnakeAuthoringItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const SnakeAuthoringItem* SnakeAuthoring::FindByName(const std::string& name) const { const std::string target = Lower(name); auto it = std::find_if(entries_.begin(), entries_.end(), [&target](const SnakeAuthoringItem& entry) { return Lower(entry.name) == target; }); return it == entries_.end() ? nullptr : &*it; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::All() const { return entries_; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::Enabled() const { std::vector<SnakeAuthoringItem> out; for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry); return out; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::Dirty() const { std::vector<SnakeAuthoringItem> out; for (const auto& entry : entries_) if (entry.dirty) out.push_back(entry); return out; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::ByKind(SnakeAuthoringKind kind) const { std::vector<SnakeAuthoringItem> out; for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry); return out; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::ByGroup(const std::string& group) const { std::vector<SnakeAuthoringItem> out; const std::string target = Lower(group); for (const auto& entry : entries_) if (Lower(entry.group) == target) out.push_back(entry); return out; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::Search(const std::string& text) const { std::vector<SnakeAuthoringItem> out; for (const auto& entry : entries_) if (ContainsText(entry.name, text) || ContainsText(entry.group, text) || ContainsText(entry.detail, text) || ContainsText(entry.path.generic_string(), text)) out.push_back(entry); return out; }
std::vector<SnakeAuthoringItem> SnakeAuthoring::TopPriority(int limit) const { std::vector<SnakeAuthoringItem> out = entries_; std::stable_sort(out.begin(), out.end(), [](const SnakeAuthoringItem& a, const SnakeAuthoringItem& b) { if (a.priority != b.priority) return a.priority > b.priority; return a.order < b.order; }); if (limit >= 0 && static_cast<int>(out.size()) > limit) out.resize(static_cast<size_t>(limit)); return out; }
std::set<std::string> SnakeAuthoring::Groups() const { std::set<std::string> groups; for (const auto& entry : entries_) if (!entry.group.empty()) groups.insert(entry.group); return groups; }
SnakeAuthoringItemSummary SnakeAuthoring::Summarize() const { SnakeAuthoringItemSummary summary; summary.total = static_cast<int>(entries_.size()); for (const auto& entry : entries_) { if (entry.enabled) summary.enabled++; if (entry.dirty) summary.dirty++; summary.priorityTotal += entry.priority; summary.byKind[ToString(entry.kind)]++; if (!entry.group.empty()) summary.byGroup[entry.group]++; } return summary; }
std::string SnakeAuthoring::Serialize() const { std::ostringstream out; for (const auto& entry : entries_) out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.group) << "|" << EscapeField(entry.detail) << "|" << entry.order << "|" << entry.priority << "|" << (entry.enabled ? 1 : 0) << "|" << (entry.dirty ? 1 : 0) << "\n"; return out.str(); }
bool SnakeAuthoring::Deserialize(const std::string& text) { entries_.clear(); nextId_ = 1; std::istringstream input(text); std::string line; while (std::getline(input, line)) { if (Trim(line).empty()) continue; const auto fields = SplitFields(line); if (fields.size() != 10) return false; SnakeAuthoringItem entry; entry.id = std::stoi(fields[0]); if (!TryParseSnakeAuthoringKind(fields[1], entry.kind)) return false; entry.name = fields[2]; entry.path = fields[3]; entry.group = fields[4]; entry.detail = fields[5]; entry.order = std::stoi(fields[6]); entry.priority = std::stoi(fields[7]); entry.enabled = fields[8] == "1"; entry.dirty = fields[9] == "1"; nextId_ = std::max(nextId_, entry.id + 1); entries_.push_back(std::move(entry)); } NormalizeOrder(); return true; }
std::string SnakeAuthoring::Report() const { const auto summary = Summarize(); std::ostringstream out; out << "total=" << summary.total << " enabled=" << summary.enabled << " dirty=" << summary.dirty << " priority=" << summary.priorityTotal; for (const auto& row : summary.byKind) out << "\n" << row.first << "=" << row.second; for (const auto& row : summary.byGroup) out << "\ngroup:" << row.first << "=" << row.second; return out.str(); }
int SnakeAuthoring::NextId() const { return nextId_; }
int SnakeAuthoring::Size() const { return static_cast<int>(entries_.size()); }
void SnakeAuthoring::Clear() { entries_.clear(); nextId_ = 1; }




std::vector<SnakeAuthoringItem> BuildSnakeRulePresetMap() {
    std::vector<SnakeAuthoringItem> items;
    items.reserve(6);

    {
        SnakeAuthoringItem item;
        item.id = 1;
        item.kind = SnakeAuthoringKind::BoardRule;
        item.name = "BoardRule preset 1";
        item.group = "snake-rule";
        item.detail = "Preset 1 used by snake-rule workflows";
        item.order = 0;
        item.priority = 1;
        items.push_back(item);
    }

    {
        SnakeAuthoringItem item;
        item.id = 2;
        item.kind = SnakeAuthoringKind::FoodRule;
        item.name = "FoodRule preset 2";
        item.group = "snake-rule";
        item.detail = "Preset 2 used by snake-rule workflows";
        item.order = 1;
        item.priority = 2;
        items.push_back(item);
    }

    {
        SnakeAuthoringItem item;
        item.id = 3;
        item.kind = SnakeAuthoringKind::SnakePart;
        item.name = "SnakePart preset 3";
        item.group = "snake-rule";
        item.detail = "Preset 3 used by snake-rule workflows";
        item.order = 2;
        item.priority = 3;
        items.push_back(item);
    }

    {
        SnakeAuthoringItem item;
        item.id = 4;
        item.kind = SnakeAuthoringKind::InputRule;
        item.name = "InputRule preset 4";
        item.group = "snake-rule";
        item.detail = "Preset 4 used by snake-rule workflows";
        item.order = 3;
        item.priority = 4;
        items.push_back(item);
    }

    {
        SnakeAuthoringItem item;
        item.id = 5;
        item.kind = SnakeAuthoringKind::ScoreRule;
        item.name = "ScoreRule preset 5";
        item.group = "snake-rule";
        item.detail = "Preset 5 used by snake-rule workflows";
        item.order = 4;
        item.priority = 5;
        items.push_back(item);
    }

    {
        SnakeAuthoringItem item;
        item.id = 6;
        item.kind = SnakeAuthoringKind::HazardRule;
        item.name = "HazardRule preset 6";
        item.group = "snake-rule";
        item.detail = "Preset 6 used by snake-rule workflows";
        item.order = 5;
        item.priority = 6;
        items.push_back(item);
    }

    return items;
}

}
