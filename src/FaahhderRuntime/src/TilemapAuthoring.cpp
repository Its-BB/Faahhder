#include "Faahhder/TilemapAuthoring.hpp"

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

std::string ToString(TilemapAuthoringKind kind) {
    switch (kind) {
        case TilemapAuthoringKind::Brush: return "Brush";
        case TilemapAuthoringKind::Stamp: return "Stamp";
        case TilemapAuthoringKind::CollisionRule: return "CollisionRule";
        case TilemapAuthoringKind::LayerRule: return "LayerRule";
        case TilemapAuthoringKind::PaletteEntry: return "PaletteEntry";
        case TilemapAuthoringKind::AutotileRule: return "AutotileRule";
        case TilemapAuthoringKind::Selection: return "Selection";
        case TilemapAuthoringKind::FillJob: return "FillJob";
        case TilemapAuthoringKind::EraseJob: return "EraseJob";
        case TilemapAuthoringKind::PreviewTile: return "PreviewTile";
        case TilemapAuthoringKind::GridGuide: return "GridGuide";
        case TilemapAuthoringKind::HistoryPoint: return "HistoryPoint";
    }
    return "Brush";
}

bool TryParseTilemapAuthoringKind(const std::string& text, TilemapAuthoringKind& out) {
    const std::string lower = Lower(Trim(text));
    if (lower == "brush") { out = TilemapAuthoringKind::Brush; return true; }
    if (lower == "stamp") { out = TilemapAuthoringKind::Stamp; return true; }
    if (lower == "collisionrule") { out = TilemapAuthoringKind::CollisionRule; return true; }
    if (lower == "layerrule") { out = TilemapAuthoringKind::LayerRule; return true; }
    if (lower == "paletteentry") { out = TilemapAuthoringKind::PaletteEntry; return true; }
    if (lower == "autotilerule") { out = TilemapAuthoringKind::AutotileRule; return true; }
    if (lower == "selection") { out = TilemapAuthoringKind::Selection; return true; }
    if (lower == "filljob") { out = TilemapAuthoringKind::FillJob; return true; }
    if (lower == "erasejob") { out = TilemapAuthoringKind::EraseJob; return true; }
    if (lower == "previewtile") { out = TilemapAuthoringKind::PreviewTile; return true; }
    if (lower == "gridguide") { out = TilemapAuthoringKind::GridGuide; return true; }
    if (lower == "historypoint") { out = TilemapAuthoringKind::HistoryPoint; return true; }
    return false;
}

int TilemapAuthoring::Add(TilemapAuthoringKind kind, std::string name, std::filesystem::path path, std::string group, std::string detail) { TilemapAuthoringItem value; value.id = nextId_++; value.kind = kind; value.name = std::move(name); value.path = std::move(path); value.group = std::move(group); value.detail = std::move(detail); value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); return entries_.back().id; }
int TilemapAuthoring::Add(TilemapAuthoringItem value) { if (value.id <= 0) value.id = nextId_++; nextId_ = std::max(nextId_, value.id + 1); if (value.order < 0) value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); NormalizeOrder(); return entries_.back().id; }
bool TilemapAuthoring::Upsert(TilemapAuthoringItem value) { if (value.id > 0) { if (auto* existing = Find(value.id)) { *existing = std::move(value); NormalizeOrder(); return true; } } Add(std::move(value)); return true; }
bool TilemapAuthoring::Remove(int id) { const auto it = std::remove_if(entries_.begin(), entries_.end(), [id](const TilemapAuthoringItem& entry) { return entry.id == id; }); if (it == entries_.end()) return false; entries_.erase(it, entries_.end()); NormalizeOrder(); return true; }
bool TilemapAuthoring::Rename(int id, std::string name) { if (auto* entry = Find(id)) { entry->name = std::move(name); entry->dirty = true; return true; } return false; }
bool TilemapAuthoring::MoveToGroup(int id, std::string group) { if (auto* entry = Find(id)) { entry->group = std::move(group); entry->dirty = true; return true; } return false; }
bool TilemapAuthoring::SetPath(int id, std::filesystem::path path) { if (auto* entry = Find(id)) { entry->path = std::move(path); entry->dirty = true; return true; } return false; }
bool TilemapAuthoring::SetDetail(int id, std::string detail) { if (auto* entry = Find(id)) { entry->detail = std::move(detail); entry->dirty = true; return true; } return false; }
bool TilemapAuthoring::SetPriority(int id, int priority) { if (auto* entry = Find(id)) { entry->priority = priority; entry->dirty = true; return true; } return false; }
bool TilemapAuthoring::SetEnabled(int id, bool enabled) { if (auto* entry = Find(id)) { entry->enabled = enabled; entry->dirty = true; return true; } return false; }
bool TilemapAuthoring::MarkDirty(int id, bool dirty) { if (auto* entry = Find(id)) { entry->dirty = dirty; return true; } return false; }
bool TilemapAuthoring::ApplyPatch(int id, const TilemapAuthoringItemPatch& patch) { auto* entry = Find(id); if (!entry) return false; if (patch.kind) entry->kind = *patch.kind; if (patch.name) entry->name = *patch.name; if (patch.path) entry->path = *patch.path; if (patch.group) entry->group = *patch.group; if (patch.detail) entry->detail = *patch.detail; if (patch.order) entry->order = *patch.order; if (patch.priority) entry->priority = *patch.priority; if (patch.enabled) entry->enabled = *patch.enabled; if (patch.dirty) entry->dirty = *patch.dirty; NormalizeOrder(); return true; }
bool TilemapAuthoring::Reorder(int id, int order) { auto* entry = Find(id); if (!entry) return false; entry->order = std::max(0, order); entry->dirty = true; NormalizeOrder(); return true; }
bool TilemapAuthoring::MoveBefore(int movingId, int beforeId) { const auto* before = Find(beforeId); if (!before || movingId == beforeId) return false; return Reorder(movingId, before->order); }
bool TilemapAuthoring::MoveAfter(int movingId, int afterId) { const auto* after = Find(afterId); if (!after || movingId == afterId) return false; return Reorder(movingId, after->order + 1); }
bool TilemapAuthoring::NormalizeOrder() { std::stable_sort(entries_.begin(), entries_.end(), [](const TilemapAuthoringItem& a, const TilemapAuthoringItem& b) { if (a.order != b.order) return a.order < b.order; return a.id < b.id; }); bool changed = false; for (int i = 0; i < static_cast<int>(entries_.size()); ++i) { if (entries_[i].order != i) changed = true; entries_[i].order = i; } return changed; }
bool TilemapAuthoring::Contains(int id) const { return Find(id) != nullptr; }
TilemapAuthoringItem* TilemapAuthoring::Find(int id) { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const TilemapAuthoringItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const TilemapAuthoringItem* TilemapAuthoring::Find(int id) const { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const TilemapAuthoringItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const TilemapAuthoringItem* TilemapAuthoring::FindByName(const std::string& name) const { const std::string target = Lower(name); auto it = std::find_if(entries_.begin(), entries_.end(), [&target](const TilemapAuthoringItem& entry) { return Lower(entry.name) == target; }); return it == entries_.end() ? nullptr : &*it; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::All() const { return entries_; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::Enabled() const { std::vector<TilemapAuthoringItem> out; for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry); return out; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::Dirty() const { std::vector<TilemapAuthoringItem> out; for (const auto& entry : entries_) if (entry.dirty) out.push_back(entry); return out; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::ByKind(TilemapAuthoringKind kind) const { std::vector<TilemapAuthoringItem> out; for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry); return out; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::ByGroup(const std::string& group) const { std::vector<TilemapAuthoringItem> out; const std::string target = Lower(group); for (const auto& entry : entries_) if (Lower(entry.group) == target) out.push_back(entry); return out; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::Search(const std::string& text) const { std::vector<TilemapAuthoringItem> out; for (const auto& entry : entries_) if (ContainsText(entry.name, text) || ContainsText(entry.group, text) || ContainsText(entry.detail, text) || ContainsText(entry.path.generic_string(), text)) out.push_back(entry); return out; }
std::vector<TilemapAuthoringItem> TilemapAuthoring::TopPriority(int limit) const { std::vector<TilemapAuthoringItem> out = entries_; std::stable_sort(out.begin(), out.end(), [](const TilemapAuthoringItem& a, const TilemapAuthoringItem& b) { if (a.priority != b.priority) return a.priority > b.priority; return a.order < b.order; }); if (limit >= 0 && static_cast<int>(out.size()) > limit) out.resize(static_cast<size_t>(limit)); return out; }
std::set<std::string> TilemapAuthoring::Groups() const { std::set<std::string> groups; for (const auto& entry : entries_) if (!entry.group.empty()) groups.insert(entry.group); return groups; }
TilemapAuthoringItemSummary TilemapAuthoring::Summarize() const { TilemapAuthoringItemSummary summary; summary.total = static_cast<int>(entries_.size()); for (const auto& entry : entries_) { if (entry.enabled) summary.enabled++; if (entry.dirty) summary.dirty++; summary.priorityTotal += entry.priority; summary.byKind[ToString(entry.kind)]++; if (!entry.group.empty()) summary.byGroup[entry.group]++; } return summary; }
std::string TilemapAuthoring::Serialize() const { std::ostringstream out; for (const auto& entry : entries_) out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.group) << "|" << EscapeField(entry.detail) << "|" << entry.order << "|" << entry.priority << "|" << (entry.enabled ? 1 : 0) << "|" << (entry.dirty ? 1 : 0) << "\n"; return out.str(); }
bool TilemapAuthoring::Deserialize(const std::string& text) { entries_.clear(); nextId_ = 1; std::istringstream input(text); std::string line; while (std::getline(input, line)) { if (Trim(line).empty()) continue; const auto fields = SplitFields(line); if (fields.size() != 10) return false; TilemapAuthoringItem entry; entry.id = std::stoi(fields[0]); if (!TryParseTilemapAuthoringKind(fields[1], entry.kind)) return false; entry.name = fields[2]; entry.path = fields[3]; entry.group = fields[4]; entry.detail = fields[5]; entry.order = std::stoi(fields[6]); entry.priority = std::stoi(fields[7]); entry.enabled = fields[8] == "1"; entry.dirty = fields[9] == "1"; nextId_ = std::max(nextId_, entry.id + 1); entries_.push_back(std::move(entry)); } NormalizeOrder(); return true; }
std::string TilemapAuthoring::Report() const { const auto summary = Summarize(); std::ostringstream out; out << "total=" << summary.total << " enabled=" << summary.enabled << " dirty=" << summary.dirty << " priority=" << summary.priorityTotal; for (const auto& row : summary.byKind) out << "\n" << row.first << "=" << row.second; for (const auto& row : summary.byGroup) out << "\ngroup:" << row.first << "=" << row.second; return out.str(); }
int TilemapAuthoring::NextId() const { return nextId_; }
int TilemapAuthoring::Size() const { return static_cast<int>(entries_.size()); }
void TilemapAuthoring::Clear() { entries_.clear(); nextId_ = 1; }

std::vector<TilemapAuthoringItem> BuildTilemapAuthoringDefaults(const std::filesystem::path& tilesetPath) {
    std::vector<TilemapAuthoringItem> items;
    const struct Row { TilemapAuthoringKind kind; const char* name; const char* group; const char* detail; int priority; } rows[] = {
        {TilemapAuthoringKind::Brush, "Pencil brush", "paint", "Paints one tile at the cursor", 8},
        {TilemapAuthoringKind::Brush, "Rectangle brush", "paint", "Paints a filled rectangular area", 7},
        {TilemapAuthoringKind::Stamp, "Room stamp", "stamp", "Places a saved arrangement of tiles", 6},
        {TilemapAuthoringKind::CollisionRule, "Solid tile rule", "collision", "Generates static tile colliders", 9},
        {TilemapAuthoringKind::AutotileRule, "Edge autotile", "autotile", "Chooses border tiles from neighbors", 8},
        {TilemapAuthoringKind::PreviewTile, "Hovered tile", "preview", "Stores the tile currently under the cursor", 4},
        {TilemapAuthoringKind::HistoryPoint, "Tile edit history", "history", "Restores tile edits through undo", 5},
    };
    int id = 1;
    for (const auto& row : rows) {
        TilemapAuthoringItem item;
        item.id = id++;
        item.kind = row.kind;
        item.name = row.name;
        item.path = tilesetPath;
        item.group = row.group;
        item.detail = row.detail;
        item.order = static_cast<int>(items.size());
        item.priority = row.priority;
        items.push_back(std::move(item));
    }
    return items;
}


std::vector<TilemapAuthoringItem> BuildTilemapToolPresetMap() {
    std::vector<TilemapAuthoringItem> items;
    items.reserve(34);

    {
        TilemapAuthoringItem item;
        item.id = 1;
        item.kind = TilemapAuthoringKind::Brush;
        item.name = "Brush preset 1";
        item.group = "tilemap-tool";
        item.detail = "Preset 1 used by tilemap-tool workflows";
        item.order = 0;
        item.priority = 1;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 2;
        item.kind = TilemapAuthoringKind::Stamp;
        item.name = "Stamp preset 2";
        item.group = "tilemap-tool";
        item.detail = "Preset 2 used by tilemap-tool workflows";
        item.order = 1;
        item.priority = 2;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 3;
        item.kind = TilemapAuthoringKind::CollisionRule;
        item.name = "CollisionRule preset 3";
        item.group = "tilemap-tool";
        item.detail = "Preset 3 used by tilemap-tool workflows";
        item.order = 2;
        item.priority = 3;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 4;
        item.kind = TilemapAuthoringKind::LayerRule;
        item.name = "LayerRule preset 4";
        item.group = "tilemap-tool";
        item.detail = "Preset 4 used by tilemap-tool workflows";
        item.order = 3;
        item.priority = 4;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 5;
        item.kind = TilemapAuthoringKind::PaletteEntry;
        item.name = "PaletteEntry preset 5";
        item.group = "tilemap-tool";
        item.detail = "Preset 5 used by tilemap-tool workflows";
        item.order = 4;
        item.priority = 5;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 6;
        item.kind = TilemapAuthoringKind::AutotileRule;
        item.name = "AutotileRule preset 6";
        item.group = "tilemap-tool";
        item.detail = "Preset 6 used by tilemap-tool workflows";
        item.order = 5;
        item.priority = 6;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 7;
        item.kind = TilemapAuthoringKind::Selection;
        item.name = "Selection preset 7";
        item.group = "tilemap-tool";
        item.detail = "Preset 7 used by tilemap-tool workflows";
        item.order = 6;
        item.priority = 7;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 8;
        item.kind = TilemapAuthoringKind::FillJob;
        item.name = "FillJob preset 8";
        item.group = "tilemap-tool";
        item.detail = "Preset 8 used by tilemap-tool workflows";
        item.order = 7;
        item.priority = 8;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 9;
        item.kind = TilemapAuthoringKind::EraseJob;
        item.name = "EraseJob preset 9";
        item.group = "tilemap-tool";
        item.detail = "Preset 9 used by tilemap-tool workflows";
        item.order = 8;
        item.priority = 9;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 10;
        item.kind = TilemapAuthoringKind::PreviewTile;
        item.name = "PreviewTile preset 10";
        item.group = "tilemap-tool";
        item.detail = "Preset 10 used by tilemap-tool workflows";
        item.order = 9;
        item.priority = 10;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 11;
        item.kind = TilemapAuthoringKind::GridGuide;
        item.name = "GridGuide preset 11";
        item.group = "tilemap-tool";
        item.detail = "Preset 11 used by tilemap-tool workflows";
        item.order = 10;
        item.priority = 1;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 12;
        item.kind = TilemapAuthoringKind::HistoryPoint;
        item.name = "HistoryPoint preset 12";
        item.group = "tilemap-tool";
        item.detail = "Preset 12 used by tilemap-tool workflows";
        item.order = 11;
        item.priority = 2;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 13;
        item.kind = TilemapAuthoringKind::Brush;
        item.name = "Brush preset 13";
        item.group = "tilemap-tool";
        item.detail = "Preset 13 used by tilemap-tool workflows";
        item.order = 12;
        item.priority = 3;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 14;
        item.kind = TilemapAuthoringKind::Stamp;
        item.name = "Stamp preset 14";
        item.group = "tilemap-tool";
        item.detail = "Preset 14 used by tilemap-tool workflows";
        item.order = 13;
        item.priority = 4;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 15;
        item.kind = TilemapAuthoringKind::CollisionRule;
        item.name = "CollisionRule preset 15";
        item.group = "tilemap-tool";
        item.detail = "Preset 15 used by tilemap-tool workflows";
        item.order = 14;
        item.priority = 5;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 16;
        item.kind = TilemapAuthoringKind::LayerRule;
        item.name = "LayerRule preset 16";
        item.group = "tilemap-tool";
        item.detail = "Preset 16 used by tilemap-tool workflows";
        item.order = 15;
        item.priority = 6;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 17;
        item.kind = TilemapAuthoringKind::PaletteEntry;
        item.name = "PaletteEntry preset 17";
        item.group = "tilemap-tool";
        item.detail = "Preset 17 used by tilemap-tool workflows";
        item.order = 16;
        item.priority = 7;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 18;
        item.kind = TilemapAuthoringKind::AutotileRule;
        item.name = "AutotileRule preset 18";
        item.group = "tilemap-tool";
        item.detail = "Preset 18 used by tilemap-tool workflows";
        item.order = 17;
        item.priority = 8;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 19;
        item.kind = TilemapAuthoringKind::Selection;
        item.name = "Selection preset 19";
        item.group = "tilemap-tool";
        item.detail = "Preset 19 used by tilemap-tool workflows";
        item.order = 18;
        item.priority = 9;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 20;
        item.kind = TilemapAuthoringKind::FillJob;
        item.name = "FillJob preset 20";
        item.group = "tilemap-tool";
        item.detail = "Preset 20 used by tilemap-tool workflows";
        item.order = 19;
        item.priority = 10;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 21;
        item.kind = TilemapAuthoringKind::EraseJob;
        item.name = "EraseJob preset 21";
        item.group = "tilemap-tool";
        item.detail = "Preset 21 used by tilemap-tool workflows";
        item.order = 20;
        item.priority = 1;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 22;
        item.kind = TilemapAuthoringKind::PreviewTile;
        item.name = "PreviewTile preset 22";
        item.group = "tilemap-tool";
        item.detail = "Preset 22 used by tilemap-tool workflows";
        item.order = 21;
        item.priority = 2;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 23;
        item.kind = TilemapAuthoringKind::GridGuide;
        item.name = "GridGuide preset 23";
        item.group = "tilemap-tool";
        item.detail = "Preset 23 used by tilemap-tool workflows";
        item.order = 22;
        item.priority = 3;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 24;
        item.kind = TilemapAuthoringKind::HistoryPoint;
        item.name = "HistoryPoint preset 24";
        item.group = "tilemap-tool";
        item.detail = "Preset 24 used by tilemap-tool workflows";
        item.order = 23;
        item.priority = 4;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 25;
        item.kind = TilemapAuthoringKind::Brush;
        item.name = "Brush preset 25";
        item.group = "tilemap-tool";
        item.detail = "Preset 25 used by tilemap-tool workflows";
        item.order = 24;
        item.priority = 5;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 26;
        item.kind = TilemapAuthoringKind::Stamp;
        item.name = "Stamp preset 26";
        item.group = "tilemap-tool";
        item.detail = "Preset 26 used by tilemap-tool workflows";
        item.order = 25;
        item.priority = 6;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 27;
        item.kind = TilemapAuthoringKind::CollisionRule;
        item.name = "CollisionRule preset 27";
        item.group = "tilemap-tool";
        item.detail = "Preset 27 used by tilemap-tool workflows";
        item.order = 26;
        item.priority = 7;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 28;
        item.kind = TilemapAuthoringKind::LayerRule;
        item.name = "LayerRule preset 28";
        item.group = "tilemap-tool";
        item.detail = "Preset 28 used by tilemap-tool workflows";
        item.order = 27;
        item.priority = 8;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 29;
        item.kind = TilemapAuthoringKind::PaletteEntry;
        item.name = "PaletteEntry preset 29";
        item.group = "tilemap-tool";
        item.detail = "Preset 29 used by tilemap-tool workflows";
        item.order = 28;
        item.priority = 9;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 30;
        item.kind = TilemapAuthoringKind::AutotileRule;
        item.name = "AutotileRule preset 30";
        item.group = "tilemap-tool";
        item.detail = "Preset 30 used by tilemap-tool workflows";
        item.order = 29;
        item.priority = 10;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 31;
        item.kind = TilemapAuthoringKind::Selection;
        item.name = "Selection preset 31";
        item.group = "tilemap-tool";
        item.detail = "Preset 31 used by tilemap-tool workflows";
        item.order = 30;
        item.priority = 1;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 32;
        item.kind = TilemapAuthoringKind::FillJob;
        item.name = "FillJob preset 32";
        item.group = "tilemap-tool";
        item.detail = "Preset 32 used by tilemap-tool workflows";
        item.order = 31;
        item.priority = 2;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 33;
        item.kind = TilemapAuthoringKind::EraseJob;
        item.name = "EraseJob preset 33";
        item.group = "tilemap-tool";
        item.detail = "Preset 33 used by tilemap-tool workflows";
        item.order = 32;
        item.priority = 3;
        items.push_back(item);
    }

    {
        TilemapAuthoringItem item;
        item.id = 34;
        item.kind = TilemapAuthoringKind::PreviewTile;
        item.name = "PreviewTile preset 34";
        item.group = "tilemap-tool";
        item.detail = "Preset 34 used by tilemap-tool workflows";
        item.order = 33;
        item.priority = 4;
        items.push_back(item);
    }

    return items;
}

}
