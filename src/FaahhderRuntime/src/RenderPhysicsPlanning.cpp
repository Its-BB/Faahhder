#include "Faahhder/RenderPhysicsPlanning.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace faahhder {
namespace {

std::string TrimValue(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
}

std::vector<std::string> SplitFields(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    bool escaped = false;
    for (char c : line) {
        if (escaped) { current.push_back(c); escaped = false; continue; }
        if (c == '\\') { escaped = true; continue; }
        if (c == delimiter) { parts.push_back(current); current.clear(); continue; }
        current.push_back(c);
    }
    parts.push_back(current);
    return parts;
}

std::string EscapeField(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (c == '|' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

bool ContainsInsensitive(std::string haystack, std::string needle) {
    std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return haystack.find(needle) != std::string::npos;
}

}

std::string ToString(RenderPhysicsItemKind kind) {
    switch (kind) {
    case RenderPhysicsItemKind::Sprite: return "Sprite";
    case RenderPhysicsItemKind::Tile: return "Tile";
    case RenderPhysicsItemKind::Particle: return "Particle";
    case RenderPhysicsItemKind::Light: return "Light";
    case RenderPhysicsItemKind::Camera: return "Camera";
    case RenderPhysicsItemKind::Collider: return "Collider";
    case RenderPhysicsItemKind::Rigidbody: return "Rigidbody";
    case RenderPhysicsItemKind::Trigger: return "Trigger";
    case RenderPhysicsItemKind::Raycast: return "Raycast";
    case RenderPhysicsItemKind::Layer: return "Layer";
    }
    return "Sprite";
}

bool TryParseRenderPhysicsItemKind(const std::string& text, RenderPhysicsItemKind& out) {
    if (text == "Sprite") { out = RenderPhysicsItemKind::Sprite; return true; }
    if (text == "Tile") { out = RenderPhysicsItemKind::Tile; return true; }
    if (text == "Particle") { out = RenderPhysicsItemKind::Particle; return true; }
    if (text == "Light") { out = RenderPhysicsItemKind::Light; return true; }
    if (text == "Camera") { out = RenderPhysicsItemKind::Camera; return true; }
    if (text == "Collider") { out = RenderPhysicsItemKind::Collider; return true; }
    if (text == "Rigidbody") { out = RenderPhysicsItemKind::Rigidbody; return true; }
    if (text == "Trigger") { out = RenderPhysicsItemKind::Trigger; return true; }
    if (text == "Raycast") { out = RenderPhysicsItemKind::Raycast; return true; }
    if (text == "Layer") { out = RenderPhysicsItemKind::Layer; return true; }
    return false;
}

int RenderPhysicsPlan::Add(RenderPhysicsItemKind kind, std::string name, std::filesystem::path path, std::string tag) {
    RenderPhysicsItem value;
    value.id = nextId_++;
    value.kind = kind;
    value.name = std::move(name);
    value.path = std::move(path);
    value.tag = std::move(tag);
    value.order = static_cast<int>(entries_.size());
    entries_.push_back(value);
    return value.id;
}

bool RenderPhysicsPlan::Upsert(int id, RenderPhysicsItem value) {
    value.id = id;
    for (auto& entry : entries_) {
        if (entry.id == id) { entry = std::move(value); return true; }
    }
    nextId_ = std::max(nextId_, id + 1);
    entries_.push_back(std::move(value));
    return true;
}

bool RenderPhysicsPlan::Patch(int id, const RenderPhysicsItemPatch& patch) {
    auto* entry = Find(id);
    if (!entry) return false;
    if (patch.hasName) entry->name = patch.name;
    if (patch.hasPath) entry->path = patch.path;
    if (patch.hasTag) entry->tag = patch.tag;
    if (patch.hasOrder) entry->order = patch.order;
    if (patch.hasWeight) entry->weight = patch.weight;
    if (patch.hasEnabled) entry->enabled = patch.enabled;
    return true;
}

bool RenderPhysicsPlan::Remove(int id) {
    const auto old = entries_.size();
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [id](const auto& entry) { return entry.id == id; }), entries_.end());
    for (std::size_t i = 0; i < entries_.size(); ++i) entries_[i].order = static_cast<int>(i);
    return old != entries_.size();
}

bool RenderPhysicsPlan::Rename(int id, std::string name) {
    auto* entry = Find(id);
    if (!entry) return false;
    entry->name = std::move(name);
    return true;
}

bool RenderPhysicsPlan::Retag(int id, std::string tag) {
    auto* entry = Find(id);
    if (!entry) return false;
    entry->tag = std::move(tag);
    return true;
}

bool RenderPhysicsPlan::Enable(int id, bool enabled) {
    auto* entry = Find(id);
    if (!entry) return false;
    entry->enabled = enabled;
    return true;
}

bool RenderPhysicsPlan::MoveBefore(int id, int beforeId) {
    auto it = std::find_if(entries_.begin(), entries_.end(), [id](const auto& entry) { return entry.id == id; });
    auto before = std::find_if(entries_.begin(), entries_.end(), [beforeId](const auto& entry) { return entry.id == beforeId; });
    if (it == entries_.end() || before == entries_.end() || it == before) return false;
    auto value = *it;
    entries_.erase(it);
    before = std::find_if(entries_.begin(), entries_.end(), [beforeId](const auto& entry) { return entry.id == beforeId; });
    entries_.insert(before, value);
    for (std::size_t i = 0; i < entries_.size(); ++i) entries_[i].order = static_cast<int>(i);
    return true;
}

bool RenderPhysicsPlan::MoveAfter(int id, int afterId) {
    auto it = std::find_if(entries_.begin(), entries_.end(), [id](const auto& entry) { return entry.id == id; });
    auto after = std::find_if(entries_.begin(), entries_.end(), [afterId](const auto& entry) { return entry.id == afterId; });
    if (it == entries_.end() || after == entries_.end() || it == after) return false;
    auto value = *it;
    entries_.erase(it);
    after = std::find_if(entries_.begin(), entries_.end(), [afterId](const auto& entry) { return entry.id == afterId; });
    entries_.insert(after + 1, value);
    for (std::size_t i = 0; i < entries_.size(); ++i) entries_[i].order = static_cast<int>(i);
    return true;
}

RenderPhysicsItem* RenderPhysicsPlan::Find(int id) {
    for (auto& entry : entries_) if (entry.id == id) return &entry;
    return nullptr;
}

const RenderPhysicsItem* RenderPhysicsPlan::Find(int id) const {
    for (const auto& entry : entries_) if (entry.id == id) return &entry;
    return nullptr;
}

const RenderPhysicsItem* RenderPhysicsPlan::FindByName(const std::string& name) const {
    for (const auto& entry : entries_) if (entry.name == name) return &entry;
    return nullptr;
}

std::vector<RenderPhysicsItem> RenderPhysicsPlan::FindByKind(RenderPhysicsItemKind kind) const {
    std::vector<RenderPhysicsItem> out;
    for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry);
    return out;
}

std::vector<RenderPhysicsItem> RenderPhysicsPlan::FindByTag(const std::string& tag) const {
    std::vector<RenderPhysicsItem> out;
    for (const auto& entry : entries_) if (entry.tag == tag) out.push_back(entry);
    return out;
}

std::vector<RenderPhysicsItem> RenderPhysicsPlan::Search(const std::string& text) const {
    std::vector<RenderPhysicsItem> out;
    for (const auto& entry : entries_) {
        if (ContainsInsensitive(entry.name, text) || ContainsInsensitive(entry.tag, text) || ContainsInsensitive(entry.path.generic_string(), text)) out.push_back(entry);
    }
    return out;
}

std::vector<RenderPhysicsItem> RenderPhysicsPlan::Enabled() const {
    std::vector<RenderPhysicsItem> out;
    for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry);
    return out;
}

std::vector<RenderPhysicsItem> RenderPhysicsPlan::Sorted() const {
    auto out = entries_;
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        if (a.order != b.order) return a.order < b.order;
        return a.name < b.name;
    });
    return out;
}

std::vector<RenderPhysicsItem> RenderPhysicsPlan::Entries() const { return entries_; }
std::size_t RenderPhysicsPlan::Size() const { return entries_.size(); }
bool RenderPhysicsPlan::Empty() const { return entries_.empty(); }

int RenderPhysicsPlan::TotalWeight() const {
    int total = 0;
    for (const auto& entry : entries_) if (entry.enabled) total += entry.weight;
    return total;
}

int RenderPhysicsPlan::NextId() const { return nextId_; }
void RenderPhysicsPlan::Clear() { entries_.clear(); nextId_ = 1; }

std::string RenderPhysicsPlan::Serialize() const {
    std::ostringstream out;
    for (const auto& entry : entries_) {
        out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.tag) << "|" << entry.order << "|" << entry.weight << "|" << (entry.enabled ? 1 : 0) << "\n";
    }
    return out.str();
}

bool RenderPhysicsPlan::Deserialize(const std::string& text) {
    entries_.clear();
    nextId_ = 1;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (TrimValue(line).empty()) continue;
        const auto parts = SplitFields(line, '|');
        if (parts.size() != 8) return false;
        RenderPhysicsItem entry;
        entry.id = std::stoi(parts[0]);
        if (!TryParseRenderPhysicsItemKind(parts[1], entry.kind)) return false;
        entry.name = parts[2];
        entry.path = parts[3];
        entry.tag = parts[4];
        entry.order = std::stoi(parts[5]);
        entry.weight = std::stoi(parts[6]);
        entry.enabled = parts[7] == "1";
        nextId_ = std::max(nextId_, entry.id + 1);
        entries_.push_back(std::move(entry));
    }
    return true;
}

std::string RenderPhysicsPlan::Report() const {
    std::unordered_map<std::string, int> counts;
    for (const auto& entry : entries_) counts[ToString(entry.kind)]++;
    std::ostringstream out;
    out << "entries=" << entries_.size() << " enabled=" << Enabled().size() << " weight=" << TotalWeight();
    for (const auto& row : counts) out << "\n" << row.first << "=" << row.second;
    return out.str();
}

}
