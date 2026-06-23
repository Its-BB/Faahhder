#include "Faahhder/RuntimeQueries.hpp"

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

std::string ToString(RuntimeQueryKind kind) {
    switch (kind) {
        case RuntimeQueryKind::AabbProbe: return "AabbProbe";
        case RuntimeQueryKind::CircleProbe: return "CircleProbe";
        case RuntimeQueryKind::RaycastProbe: return "RaycastProbe";
        case RuntimeQueryKind::TileProbe: return "TileProbe";
        case RuntimeQueryKind::TriggerProbe: return "TriggerProbe";
        case RuntimeQueryKind::CameraProbe: return "CameraProbe";
        case RuntimeQueryKind::ParticleProbe: return "ParticleProbe";
        case RuntimeQueryKind::LightProbe: return "LightProbe";
        case RuntimeQueryKind::SpriteProbe: return "SpriteProbe";
        case RuntimeQueryKind::PathProbe: return "PathProbe";
        case RuntimeQueryKind::LayerProbe: return "LayerProbe";
        case RuntimeQueryKind::BatchProbe: return "BatchProbe";
    }
    return "AabbProbe";
}

bool TryParseRuntimeQueryKind(const std::string& text, RuntimeQueryKind& out) {
    const std::string lower = Lower(Trim(text));
    if (lower == "aabbprobe") { out = RuntimeQueryKind::AabbProbe; return true; }
    if (lower == "circleprobe") { out = RuntimeQueryKind::CircleProbe; return true; }
    if (lower == "raycastprobe") { out = RuntimeQueryKind::RaycastProbe; return true; }
    if (lower == "tileprobe") { out = RuntimeQueryKind::TileProbe; return true; }
    if (lower == "triggerprobe") { out = RuntimeQueryKind::TriggerProbe; return true; }
    if (lower == "cameraprobe") { out = RuntimeQueryKind::CameraProbe; return true; }
    if (lower == "particleprobe") { out = RuntimeQueryKind::ParticleProbe; return true; }
    if (lower == "lightprobe") { out = RuntimeQueryKind::LightProbe; return true; }
    if (lower == "spriteprobe") { out = RuntimeQueryKind::SpriteProbe; return true; }
    if (lower == "pathprobe") { out = RuntimeQueryKind::PathProbe; return true; }
    if (lower == "layerprobe") { out = RuntimeQueryKind::LayerProbe; return true; }
    if (lower == "batchprobe") { out = RuntimeQueryKind::BatchProbe; return true; }
    return false;
}

int RuntimeQueries::Add(RuntimeQueryKind kind, std::string name, std::filesystem::path path, std::string group, std::string detail) { RuntimeQueryItem value; value.id = nextId_++; value.kind = kind; value.name = std::move(name); value.path = std::move(path); value.group = std::move(group); value.detail = std::move(detail); value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); return entries_.back().id; }
int RuntimeQueries::Add(RuntimeQueryItem value) { if (value.id <= 0) value.id = nextId_++; nextId_ = std::max(nextId_, value.id + 1); if (value.order < 0) value.order = static_cast<int>(entries_.size()); entries_.push_back(std::move(value)); NormalizeOrder(); return entries_.back().id; }
bool RuntimeQueries::Upsert(RuntimeQueryItem value) { if (value.id > 0) { if (auto* existing = Find(value.id)) { *existing = std::move(value); NormalizeOrder(); return true; } } Add(std::move(value)); return true; }
bool RuntimeQueries::Remove(int id) { const auto it = std::remove_if(entries_.begin(), entries_.end(), [id](const RuntimeQueryItem& entry) { return entry.id == id; }); if (it == entries_.end()) return false; entries_.erase(it, entries_.end()); NormalizeOrder(); return true; }
bool RuntimeQueries::Rename(int id, std::string name) { if (auto* entry = Find(id)) { entry->name = std::move(name); entry->dirty = true; return true; } return false; }
bool RuntimeQueries::MoveToGroup(int id, std::string group) { if (auto* entry = Find(id)) { entry->group = std::move(group); entry->dirty = true; return true; } return false; }
bool RuntimeQueries::SetPath(int id, std::filesystem::path path) { if (auto* entry = Find(id)) { entry->path = std::move(path); entry->dirty = true; return true; } return false; }
bool RuntimeQueries::SetDetail(int id, std::string detail) { if (auto* entry = Find(id)) { entry->detail = std::move(detail); entry->dirty = true; return true; } return false; }
bool RuntimeQueries::SetPriority(int id, int priority) { if (auto* entry = Find(id)) { entry->priority = priority; entry->dirty = true; return true; } return false; }
bool RuntimeQueries::SetEnabled(int id, bool enabled) { if (auto* entry = Find(id)) { entry->enabled = enabled; entry->dirty = true; return true; } return false; }
bool RuntimeQueries::MarkDirty(int id, bool dirty) { if (auto* entry = Find(id)) { entry->dirty = dirty; return true; } return false; }
bool RuntimeQueries::ApplyPatch(int id, const RuntimeQueryItemPatch& patch) { auto* entry = Find(id); if (!entry) return false; if (patch.kind) entry->kind = *patch.kind; if (patch.name) entry->name = *patch.name; if (patch.path) entry->path = *patch.path; if (patch.group) entry->group = *patch.group; if (patch.detail) entry->detail = *patch.detail; if (patch.order) entry->order = *patch.order; if (patch.priority) entry->priority = *patch.priority; if (patch.enabled) entry->enabled = *patch.enabled; if (patch.dirty) entry->dirty = *patch.dirty; NormalizeOrder(); return true; }
bool RuntimeQueries::Reorder(int id, int order) { auto* entry = Find(id); if (!entry) return false; entry->order = std::max(0, order); entry->dirty = true; NormalizeOrder(); return true; }
bool RuntimeQueries::MoveBefore(int movingId, int beforeId) { const auto* before = Find(beforeId); if (!before || movingId == beforeId) return false; return Reorder(movingId, before->order); }
bool RuntimeQueries::MoveAfter(int movingId, int afterId) { const auto* after = Find(afterId); if (!after || movingId == afterId) return false; return Reorder(movingId, after->order + 1); }
bool RuntimeQueries::NormalizeOrder() { std::stable_sort(entries_.begin(), entries_.end(), [](const RuntimeQueryItem& a, const RuntimeQueryItem& b) { if (a.order != b.order) return a.order < b.order; return a.id < b.id; }); bool changed = false; for (int i = 0; i < static_cast<int>(entries_.size()); ++i) { if (entries_[i].order != i) changed = true; entries_[i].order = i; } return changed; }
bool RuntimeQueries::Contains(int id) const { return Find(id) != nullptr; }
RuntimeQueryItem* RuntimeQueries::Find(int id) { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const RuntimeQueryItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const RuntimeQueryItem* RuntimeQueries::Find(int id) const { auto it = std::find_if(entries_.begin(), entries_.end(), [id](const RuntimeQueryItem& entry) { return entry.id == id; }); return it == entries_.end() ? nullptr : &*it; }
const RuntimeQueryItem* RuntimeQueries::FindByName(const std::string& name) const { const std::string target = Lower(name); auto it = std::find_if(entries_.begin(), entries_.end(), [&target](const RuntimeQueryItem& entry) { return Lower(entry.name) == target; }); return it == entries_.end() ? nullptr : &*it; }
std::vector<RuntimeQueryItem> RuntimeQueries::All() const { return entries_; }
std::vector<RuntimeQueryItem> RuntimeQueries::Enabled() const { std::vector<RuntimeQueryItem> out; for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry); return out; }
std::vector<RuntimeQueryItem> RuntimeQueries::Dirty() const { std::vector<RuntimeQueryItem> out; for (const auto& entry : entries_) if (entry.dirty) out.push_back(entry); return out; }
std::vector<RuntimeQueryItem> RuntimeQueries::ByKind(RuntimeQueryKind kind) const { std::vector<RuntimeQueryItem> out; for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry); return out; }
std::vector<RuntimeQueryItem> RuntimeQueries::ByGroup(const std::string& group) const { std::vector<RuntimeQueryItem> out; const std::string target = Lower(group); for (const auto& entry : entries_) if (Lower(entry.group) == target) out.push_back(entry); return out; }
std::vector<RuntimeQueryItem> RuntimeQueries::Search(const std::string& text) const { std::vector<RuntimeQueryItem> out; for (const auto& entry : entries_) if (ContainsText(entry.name, text) || ContainsText(entry.group, text) || ContainsText(entry.detail, text) || ContainsText(entry.path.generic_string(), text)) out.push_back(entry); return out; }
std::vector<RuntimeQueryItem> RuntimeQueries::TopPriority(int limit) const { std::vector<RuntimeQueryItem> out = entries_; std::stable_sort(out.begin(), out.end(), [](const RuntimeQueryItem& a, const RuntimeQueryItem& b) { if (a.priority != b.priority) return a.priority > b.priority; return a.order < b.order; }); if (limit >= 0 && static_cast<int>(out.size()) > limit) out.resize(static_cast<size_t>(limit)); return out; }
std::set<std::string> RuntimeQueries::Groups() const { std::set<std::string> groups; for (const auto& entry : entries_) if (!entry.group.empty()) groups.insert(entry.group); return groups; }
RuntimeQueryItemSummary RuntimeQueries::Summarize() const { RuntimeQueryItemSummary summary; summary.total = static_cast<int>(entries_.size()); for (const auto& entry : entries_) { if (entry.enabled) summary.enabled++; if (entry.dirty) summary.dirty++; summary.priorityTotal += entry.priority; summary.byKind[ToString(entry.kind)]++; if (!entry.group.empty()) summary.byGroup[entry.group]++; } return summary; }
std::string RuntimeQueries::Serialize() const { std::ostringstream out; for (const auto& entry : entries_) out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.group) << "|" << EscapeField(entry.detail) << "|" << entry.order << "|" << entry.priority << "|" << (entry.enabled ? 1 : 0) << "|" << (entry.dirty ? 1 : 0) << "\n"; return out.str(); }
bool RuntimeQueries::Deserialize(const std::string& text) { entries_.clear(); nextId_ = 1; std::istringstream input(text); std::string line; while (std::getline(input, line)) { if (Trim(line).empty()) continue; const auto fields = SplitFields(line); if (fields.size() != 10) return false; RuntimeQueryItem entry; entry.id = std::stoi(fields[0]); if (!TryParseRuntimeQueryKind(fields[1], entry.kind)) return false; entry.name = fields[2]; entry.path = fields[3]; entry.group = fields[4]; entry.detail = fields[5]; entry.order = std::stoi(fields[6]); entry.priority = std::stoi(fields[7]); entry.enabled = fields[8] == "1"; entry.dirty = fields[9] == "1"; nextId_ = std::max(nextId_, entry.id + 1); entries_.push_back(std::move(entry)); } NormalizeOrder(); return true; }
std::string RuntimeQueries::Report() const { const auto summary = Summarize(); std::ostringstream out; out << "total=" << summary.total << " enabled=" << summary.enabled << " dirty=" << summary.dirty << " priority=" << summary.priorityTotal; for (const auto& row : summary.byKind) out << "\n" << row.first << "=" << row.second; for (const auto& row : summary.byGroup) out << "\ngroup:" << row.first << "=" << row.second; return out.str(); }
int RuntimeQueries::NextId() const { return nextId_; }
int RuntimeQueries::Size() const { return static_cast<int>(entries_.size()); }
void RuntimeQueries::Clear() { entries_.clear(); nextId_ = 1; }

std::vector<RuntimeQueryItem> BuildRuntimeQueryChecklist() {
    std::vector<RuntimeQueryItem> items;
    const RuntimeQueryKind kinds[] = {RuntimeQueryKind::AabbProbe, RuntimeQueryKind::CircleProbe, RuntimeQueryKind::RaycastProbe, RuntimeQueryKind::TileProbe, RuntimeQueryKind::TriggerProbe, RuntimeQueryKind::CameraProbe, RuntimeQueryKind::ParticleProbe, RuntimeQueryKind::LightProbe, RuntimeQueryKind::SpriteProbe, RuntimeQueryKind::BatchProbe};
    int id = 1;
    for (RuntimeQueryKind kind : kinds) {
        RuntimeQueryItem item;
        item.id = id++;
        item.kind = kind;
        item.name = ToString(kind);
        item.group = "runtime-query";
        item.detail = "Tracked query path for editor previews and runtime debugging";
        item.order = static_cast<int>(items.size());
        item.priority = item.id % 4;
        items.push_back(std::move(item));
    }
    return items;
}


std::vector<RuntimeQueryItem> BuildRuntimeQueryPresetMap() {
    std::vector<RuntimeQueryItem> items;
    items.reserve(30);

    {
        RuntimeQueryItem item;
        item.id = 1;
        item.kind = RuntimeQueryKind::AabbProbe;
        item.name = "AabbProbe preset 1";
        item.group = "runtime-probe";
        item.detail = "Preset 1 used by runtime-probe workflows";
        item.order = 0;
        item.priority = 1;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 2;
        item.kind = RuntimeQueryKind::CircleProbe;
        item.name = "CircleProbe preset 2";
        item.group = "runtime-probe";
        item.detail = "Preset 2 used by runtime-probe workflows";
        item.order = 1;
        item.priority = 2;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 3;
        item.kind = RuntimeQueryKind::RaycastProbe;
        item.name = "RaycastProbe preset 3";
        item.group = "runtime-probe";
        item.detail = "Preset 3 used by runtime-probe workflows";
        item.order = 2;
        item.priority = 3;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 4;
        item.kind = RuntimeQueryKind::TileProbe;
        item.name = "TileProbe preset 4";
        item.group = "runtime-probe";
        item.detail = "Preset 4 used by runtime-probe workflows";
        item.order = 3;
        item.priority = 4;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 5;
        item.kind = RuntimeQueryKind::TriggerProbe;
        item.name = "TriggerProbe preset 5";
        item.group = "runtime-probe";
        item.detail = "Preset 5 used by runtime-probe workflows";
        item.order = 4;
        item.priority = 5;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 6;
        item.kind = RuntimeQueryKind::CameraProbe;
        item.name = "CameraProbe preset 6";
        item.group = "runtime-probe";
        item.detail = "Preset 6 used by runtime-probe workflows";
        item.order = 5;
        item.priority = 6;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 7;
        item.kind = RuntimeQueryKind::ParticleProbe;
        item.name = "ParticleProbe preset 7";
        item.group = "runtime-probe";
        item.detail = "Preset 7 used by runtime-probe workflows";
        item.order = 6;
        item.priority = 7;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 8;
        item.kind = RuntimeQueryKind::LightProbe;
        item.name = "LightProbe preset 8";
        item.group = "runtime-probe";
        item.detail = "Preset 8 used by runtime-probe workflows";
        item.order = 7;
        item.priority = 8;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 9;
        item.kind = RuntimeQueryKind::SpriteProbe;
        item.name = "SpriteProbe preset 9";
        item.group = "runtime-probe";
        item.detail = "Preset 9 used by runtime-probe workflows";
        item.order = 8;
        item.priority = 9;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 10;
        item.kind = RuntimeQueryKind::PathProbe;
        item.name = "PathProbe preset 10";
        item.group = "runtime-probe";
        item.detail = "Preset 10 used by runtime-probe workflows";
        item.order = 9;
        item.priority = 10;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 11;
        item.kind = RuntimeQueryKind::LayerProbe;
        item.name = "LayerProbe preset 11";
        item.group = "runtime-probe";
        item.detail = "Preset 11 used by runtime-probe workflows";
        item.order = 10;
        item.priority = 1;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 12;
        item.kind = RuntimeQueryKind::BatchProbe;
        item.name = "BatchProbe preset 12";
        item.group = "runtime-probe";
        item.detail = "Preset 12 used by runtime-probe workflows";
        item.order = 11;
        item.priority = 2;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 13;
        item.kind = RuntimeQueryKind::AabbProbe;
        item.name = "AabbProbe preset 13";
        item.group = "runtime-probe";
        item.detail = "Preset 13 used by runtime-probe workflows";
        item.order = 12;
        item.priority = 3;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 14;
        item.kind = RuntimeQueryKind::CircleProbe;
        item.name = "CircleProbe preset 14";
        item.group = "runtime-probe";
        item.detail = "Preset 14 used by runtime-probe workflows";
        item.order = 13;
        item.priority = 4;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 15;
        item.kind = RuntimeQueryKind::RaycastProbe;
        item.name = "RaycastProbe preset 15";
        item.group = "runtime-probe";
        item.detail = "Preset 15 used by runtime-probe workflows";
        item.order = 14;
        item.priority = 5;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 16;
        item.kind = RuntimeQueryKind::TileProbe;
        item.name = "TileProbe preset 16";
        item.group = "runtime-probe";
        item.detail = "Preset 16 used by runtime-probe workflows";
        item.order = 15;
        item.priority = 6;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 17;
        item.kind = RuntimeQueryKind::TriggerProbe;
        item.name = "TriggerProbe preset 17";
        item.group = "runtime-probe";
        item.detail = "Preset 17 used by runtime-probe workflows";
        item.order = 16;
        item.priority = 7;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 18;
        item.kind = RuntimeQueryKind::CameraProbe;
        item.name = "CameraProbe preset 18";
        item.group = "runtime-probe";
        item.detail = "Preset 18 used by runtime-probe workflows";
        item.order = 17;
        item.priority = 8;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 19;
        item.kind = RuntimeQueryKind::ParticleProbe;
        item.name = "ParticleProbe preset 19";
        item.group = "runtime-probe";
        item.detail = "Preset 19 used by runtime-probe workflows";
        item.order = 18;
        item.priority = 9;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 20;
        item.kind = RuntimeQueryKind::LightProbe;
        item.name = "LightProbe preset 20";
        item.group = "runtime-probe";
        item.detail = "Preset 20 used by runtime-probe workflows";
        item.order = 19;
        item.priority = 10;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 21;
        item.kind = RuntimeQueryKind::SpriteProbe;
        item.name = "SpriteProbe preset 21";
        item.group = "runtime-probe";
        item.detail = "Preset 21 used by runtime-probe workflows";
        item.order = 20;
        item.priority = 1;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 22;
        item.kind = RuntimeQueryKind::PathProbe;
        item.name = "PathProbe preset 22";
        item.group = "runtime-probe";
        item.detail = "Preset 22 used by runtime-probe workflows";
        item.order = 21;
        item.priority = 2;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 23;
        item.kind = RuntimeQueryKind::LayerProbe;
        item.name = "LayerProbe preset 23";
        item.group = "runtime-probe";
        item.detail = "Preset 23 used by runtime-probe workflows";
        item.order = 22;
        item.priority = 3;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 24;
        item.kind = RuntimeQueryKind::BatchProbe;
        item.name = "BatchProbe preset 24";
        item.group = "runtime-probe";
        item.detail = "Preset 24 used by runtime-probe workflows";
        item.order = 23;
        item.priority = 4;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 25;
        item.kind = RuntimeQueryKind::AabbProbe;
        item.name = "AabbProbe preset 25";
        item.group = "runtime-probe";
        item.detail = "Preset 25 used by runtime-probe workflows";
        item.order = 24;
        item.priority = 5;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 26;
        item.kind = RuntimeQueryKind::CircleProbe;
        item.name = "CircleProbe preset 26";
        item.group = "runtime-probe";
        item.detail = "Preset 26 used by runtime-probe workflows";
        item.order = 25;
        item.priority = 6;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 27;
        item.kind = RuntimeQueryKind::RaycastProbe;
        item.name = "RaycastProbe preset 27";
        item.group = "runtime-probe";
        item.detail = "Preset 27 used by runtime-probe workflows";
        item.order = 26;
        item.priority = 7;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 28;
        item.kind = RuntimeQueryKind::TileProbe;
        item.name = "TileProbe preset 28";
        item.group = "runtime-probe";
        item.detail = "Preset 28 used by runtime-probe workflows";
        item.order = 27;
        item.priority = 8;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 29;
        item.kind = RuntimeQueryKind::TriggerProbe;
        item.name = "TriggerProbe preset 29";
        item.group = "runtime-probe";
        item.detail = "Preset 29 used by runtime-probe workflows";
        item.order = 28;
        item.priority = 9;
        items.push_back(item);
    }

    {
        RuntimeQueryItem item;
        item.id = 30;
        item.kind = RuntimeQueryKind::CameraProbe;
        item.name = "CameraProbe preset 30";
        item.group = "runtime-probe";
        item.detail = "Preset 30 used by runtime-probe workflows";
        item.order = 29;
        item.priority = 10;
        items.push_back(item);
    }

    return items;
}

}
