#include "Faahhder/GameplayTools.hpp"

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

std::string ToString(GameplayItemKind kind) {
    switch (kind) {
    case GameplayItemKind::Rule: return "Rule";
    case GameplayItemKind::Input: return "Input";
    case GameplayItemKind::Score: return "Score";
    case GameplayItemKind::Timer: return "Timer";
    case GameplayItemKind::State: return "State";
    case GameplayItemKind::Spawn: return "Spawn";
    case GameplayItemKind::Collision: return "Collision";
    case GameplayItemKind::Pickup: return "Pickup";
    case GameplayItemKind::Restart: return "Restart";
    case GameplayItemKind::Save: return "Save";
    }
    return "Rule";
}

bool TryParseGameplayItemKind(const std::string& text, GameplayItemKind& out) {
    if (text == "Rule") { out = GameplayItemKind::Rule; return true; }
    if (text == "Input") { out = GameplayItemKind::Input; return true; }
    if (text == "Score") { out = GameplayItemKind::Score; return true; }
    if (text == "Timer") { out = GameplayItemKind::Timer; return true; }
    if (text == "State") { out = GameplayItemKind::State; return true; }
    if (text == "Spawn") { out = GameplayItemKind::Spawn; return true; }
    if (text == "Collision") { out = GameplayItemKind::Collision; return true; }
    if (text == "Pickup") { out = GameplayItemKind::Pickup; return true; }
    if (text == "Restart") { out = GameplayItemKind::Restart; return true; }
    if (text == "Save") { out = GameplayItemKind::Save; return true; }
    return false;
}

int GameplayPlan::Add(GameplayItemKind kind, std::string name, std::filesystem::path path, std::string tag) {
    GameplayItem value;
    value.id = nextId_++;
    value.kind = kind;
    value.name = std::move(name);
    value.path = std::move(path);
    value.tag = std::move(tag);
    value.order = static_cast<int>(entries_.size());
    entries_.push_back(value);
    return value.id;
}

bool GameplayPlan::Upsert(int id, GameplayItem value) {
    value.id = id;
    for (auto& entry : entries_) {
        if (entry.id == id) { entry = std::move(value); return true; }
    }
    nextId_ = std::max(nextId_, id + 1);
    entries_.push_back(std::move(value));
    return true;
}

bool GameplayPlan::Patch(int id, const GameplayItemPatch& patch) {
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

bool GameplayPlan::Remove(int id) {
    const auto old = entries_.size();
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [id](const auto& entry) { return entry.id == id; }), entries_.end());
    for (std::size_t i = 0; i < entries_.size(); ++i) entries_[i].order = static_cast<int>(i);
    return old != entries_.size();
}

bool GameplayPlan::Rename(int id, std::string name) {
    auto* entry = Find(id);
    if (!entry) return false;
    entry->name = std::move(name);
    return true;
}

bool GameplayPlan::Retag(int id, std::string tag) {
    auto* entry = Find(id);
    if (!entry) return false;
    entry->tag = std::move(tag);
    return true;
}

bool GameplayPlan::Enable(int id, bool enabled) {
    auto* entry = Find(id);
    if (!entry) return false;
    entry->enabled = enabled;
    return true;
}

bool GameplayPlan::MoveBefore(int id, int beforeId) {
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

bool GameplayPlan::MoveAfter(int id, int afterId) {
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

GameplayItem* GameplayPlan::Find(int id) {
    for (auto& entry : entries_) if (entry.id == id) return &entry;
    return nullptr;
}

const GameplayItem* GameplayPlan::Find(int id) const {
    for (const auto& entry : entries_) if (entry.id == id) return &entry;
    return nullptr;
}

const GameplayItem* GameplayPlan::FindByName(const std::string& name) const {
    for (const auto& entry : entries_) if (entry.name == name) return &entry;
    return nullptr;
}

std::vector<GameplayItem> GameplayPlan::FindByKind(GameplayItemKind kind) const {
    std::vector<GameplayItem> out;
    for (const auto& entry : entries_) if (entry.kind == kind) out.push_back(entry);
    return out;
}

std::vector<GameplayItem> GameplayPlan::FindByTag(const std::string& tag) const {
    std::vector<GameplayItem> out;
    for (const auto& entry : entries_) if (entry.tag == tag) out.push_back(entry);
    return out;
}

std::vector<GameplayItem> GameplayPlan::Search(const std::string& text) const {
    std::vector<GameplayItem> out;
    for (const auto& entry : entries_) {
        if (ContainsInsensitive(entry.name, text) || ContainsInsensitive(entry.tag, text) || ContainsInsensitive(entry.path.generic_string(), text)) out.push_back(entry);
    }
    return out;
}

std::vector<GameplayItem> GameplayPlan::Enabled() const {
    std::vector<GameplayItem> out;
    for (const auto& entry : entries_) if (entry.enabled) out.push_back(entry);
    return out;
}

std::vector<GameplayItem> GameplayPlan::Sorted() const {
    auto out = entries_;
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        if (a.order != b.order) return a.order < b.order;
        return a.name < b.name;
    });
    return out;
}

std::vector<GameplayItem> GameplayPlan::Entries() const { return entries_; }
std::size_t GameplayPlan::Size() const { return entries_.size(); }
bool GameplayPlan::Empty() const { return entries_.empty(); }

int GameplayPlan::TotalWeight() const {
    int total = 0;
    for (const auto& entry : entries_) if (entry.enabled) total += entry.weight;
    return total;
}

int GameplayPlan::NextId() const { return nextId_; }
void GameplayPlan::Clear() { entries_.clear(); nextId_ = 1; }

std::string GameplayPlan::Serialize() const {
    std::ostringstream out;
    for (const auto& entry : entries_) {
        out << entry.id << "|" << ToString(entry.kind) << "|" << EscapeField(entry.name) << "|" << EscapeField(entry.path.generic_string()) << "|" << EscapeField(entry.tag) << "|" << entry.order << "|" << entry.weight << "|" << (entry.enabled ? 1 : 0) << "\n";
    }
    return out.str();
}

bool GameplayPlan::Deserialize(const std::string& text) {
    entries_.clear();
    nextId_ = 1;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (TrimValue(line).empty()) continue;
        const auto parts = SplitFields(line, '|');
        if (parts.size() != 8) return false;
        GameplayItem entry;
        entry.id = std::stoi(parts[0]);
        if (!TryParseGameplayItemKind(parts[1], entry.kind)) return false;
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

std::string GameplayPlan::Report() const {
    std::unordered_map<std::string, int> counts;
    for (const auto& entry : entries_) counts[ToString(entry.kind)]++;
    std::ostringstream out;
    out << "entries=" << entries_.size() << " enabled=" << Enabled().size() << " weight=" << TotalWeight();
    for (const auto& row : counts) out << "\n" << row.first << "=" << row.second;
    return out.str();
}

}
