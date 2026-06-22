#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class EditorItemKind {
    Viewport,
    Hierarchy,
    Inspector,
    Asset,
    Console,
    Code,
    Command,
    Shortcut,
    Panel,
    Theme
};

struct EditorItem {
    int id = 0;
    EditorItemKind kind = EditorItemKind::Viewport;
    std::string name;
    std::filesystem::path path;
    std::string tag;
    int order = 0;
    int weight = 1;
    bool enabled = true;
};

struct EditorItemPatch {
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

class EditorState {
public:
    int Add(EditorItemKind kind, std::string name, std::filesystem::path path = {}, std::string tag = {});
    bool Upsert(int id, EditorItem value);
    bool Patch(int id, const EditorItemPatch& patch);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool Retag(int id, std::string tag);
    bool Enable(int id, bool enabled);
    bool MoveBefore(int id, int beforeId);
    bool MoveAfter(int id, int afterId);
    EditorItem* Find(int id);
    const EditorItem* Find(int id) const;
    const EditorItem* FindByName(const std::string& name) const;
    std::vector<EditorItem> FindByKind(EditorItemKind kind) const;
    std::vector<EditorItem> FindByTag(const std::string& tag) const;
    std::vector<EditorItem> Search(const std::string& text) const;
    std::vector<EditorItem> Enabled() const;
    std::vector<EditorItem> Sorted() const;
    std::vector<EditorItem> Entries() const;
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
    std::vector<EditorItem> entries_;
};

std::string ToString(EditorItemKind kind);
bool TryParseEditorItemKind(const std::string& text, EditorItemKind& out);

}
