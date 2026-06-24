#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class QualityItemKind {
    Frame,
    Memory,
    AssetCheck,
    SceneCheck,
    ScriptCheck,
    ExportCheck,
    Warning,
    Error,
    Profile,
    Summary
};

struct QualityItem {
    int id = 0;
    QualityItemKind kind = QualityItemKind::Frame;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

struct QualityItemPatch {
    bool hasName = false;
    bool hasPath = false;
    bool hasTag = false;
    bool hasOrder = false;
    bool hasWeight = false;
    bool hasEnabled = false;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

class QualityPlan {
public:
    int Add(QualityItemKind kind, std::string name, std::filesystem::path path = {}, std::string tag = {});
    bool Upsert(int id, QualityItem value);
    bool Patch(int id, const QualityItemPatch& patch);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool Retag(int id, std::string tag);
    bool Enable(int id, bool enabled);
    bool MoveBefore(int id, int beforeId);
    bool MoveAfter(int id, int afterId);
    QualityItem* Find(int id);
    const QualityItem* Find(int id) const;
    const QualityItem* FindByName(const std::string& name) const;
    std::vector<QualityItem> FindByKind(QualityItemKind kind) const;
    std::vector<QualityItem> FindByTag(const std::string& tag) const;
    std::vector<QualityItem> Search(const std::string& text) const;
    std::vector<QualityItem> Enabled() const;
    std::vector<QualityItem> Sorted() const;
    std::vector<QualityItem> Entries() const;
    std::size_t Size() const;
    bool Empty() const;
    int TotalWeight() const;
    int NextId() const;
    void Clear();
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
private:
    int nextId_ = 1;
    std::vector<QualityItem> entries_;
};

std::string ToString(QualityItemKind kind);
bool TryParseQualityItemKind(const std::string& text, QualityItemKind& out);

}
