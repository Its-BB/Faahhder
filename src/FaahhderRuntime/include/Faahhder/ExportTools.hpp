#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class ExportItemKind {
    Executable,
    Asset,
    Manifest,
    Script,
    Scene,
    Texture,
    Bundle,
    Platform,
    Launcher,
    Report
};

struct ExportItem {
    int id = 0;
    ExportItemKind kind = ExportItemKind::Executable;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

struct ExportItemPatch {
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

class ExportPlan {
public:
    int Add(ExportItemKind kind, std::string name, std::filesystem::path path = {}, std::string tag = {});
    bool Upsert(int id, ExportItem value);
    bool Patch(int id, const ExportItemPatch& patch);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool Retag(int id, std::string tag);
    bool Enable(int id, bool enabled);
    bool MoveBefore(int id, int beforeId);
    bool MoveAfter(int id, int afterId);
    ExportItem* Find(int id);
    const ExportItem* Find(int id) const;
    const ExportItem* FindByName(const std::string& name) const;
    std::vector<ExportItem> FindByKind(ExportItemKind kind) const;
    std::vector<ExportItem> FindByTag(const std::string& tag) const;
    std::vector<ExportItem> Search(const std::string& text) const;
    std::vector<ExportItem> Enabled() const;
    std::vector<ExportItem> Sorted() const;
    std::vector<ExportItem> Entries() const;
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
    std::vector<ExportItem> entries_;
};

std::string ToString(ExportItemKind kind);
bool TryParseExportItemKind(const std::string& text, ExportItemKind& out);

}
