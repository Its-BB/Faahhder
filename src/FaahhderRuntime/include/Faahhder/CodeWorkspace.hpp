#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace faahhder {

enum class CodeWorkspaceKind {
    SourceFile,
    ScriptFile,
    OpenTab,
    Breakpoint,
    Diagnostic,
    SearchResult,
    Command,
    Snippet,
    WatchValue,
    BuildMessage,
    Task,
    Bookmark
};

struct CodeWorkspaceItem {
    int id = 0;
    CodeWorkspaceKind kind = CodeWorkspaceKind::SourceFile;
    std::string name;
    std::filesystem::path path;
    std::string group;
    std::string detail;
    int order = 0;
    int priority = 0;
    bool enabled = true;
    bool dirty = false;
};

struct CodeWorkspaceItemPatch {
    std::optional<CodeWorkspaceKind> kind;
    std::optional<std::string> name;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> group;
    std::optional<std::string> detail;
    std::optional<int> order;
    std::optional<int> priority;
    std::optional<bool> enabled;
    std::optional<bool> dirty;
};

struct CodeWorkspaceItemSummary {
    int total = 0;
    int enabled = 0;
    int dirty = 0;
    int priorityTotal = 0;
    std::map<std::string, int> byKind;
    std::map<std::string, int> byGroup;
};

std::string ToString(CodeWorkspaceKind kind);
bool TryParseCodeWorkspaceKind(const std::string& text, CodeWorkspaceKind& out);

class CodeWorkspace {
public:
    int Add(CodeWorkspaceKind kind, std::string name, std::filesystem::path path = {}, std::string group = {}, std::string detail = {});
    int Add(CodeWorkspaceItem value);
    bool Upsert(CodeWorkspaceItem value);
    bool Remove(int id);
    bool Rename(int id, std::string name);
    bool MoveToGroup(int id, std::string group);
    bool SetPath(int id, std::filesystem::path path);
    bool SetDetail(int id, std::string detail);
    bool SetPriority(int id, int priority);
    bool SetEnabled(int id, bool enabled);
    bool MarkDirty(int id, bool dirty = true);
    bool ApplyPatch(int id, const CodeWorkspaceItemPatch& patch);
    bool Reorder(int id, int order);
    bool MoveBefore(int movingId, int beforeId);
    bool MoveAfter(int movingId, int afterId);
    bool NormalizeOrder();
    bool Contains(int id) const;
    CodeWorkspaceItem* Find(int id);
    const CodeWorkspaceItem* Find(int id) const;
    const CodeWorkspaceItem* FindByName(const std::string& name) const;
    std::vector<CodeWorkspaceItem> All() const;
    std::vector<CodeWorkspaceItem> Enabled() const;
    std::vector<CodeWorkspaceItem> Dirty() const;
    std::vector<CodeWorkspaceItem> ByKind(CodeWorkspaceKind kind) const;
    std::vector<CodeWorkspaceItem> ByGroup(const std::string& group) const;
    std::vector<CodeWorkspaceItem> Search(const std::string& text) const;
    std::vector<CodeWorkspaceItem> TopPriority(int limit) const;
    std::set<std::string> Groups() const;
    CodeWorkspaceItemSummary Summarize() const;
    std::string Serialize() const;
    bool Deserialize(const std::string& text);
    std::string Report() const;
    int NextId() const;
    int Size() const;
    void Clear();
private:
    int nextId_ = 1;
    std::vector<CodeWorkspaceItem> entries_;
};

std::vector<CodeWorkspaceItem> BuildDefaultCodeWorkspace(const std::filesystem::path& projectRoot);

std::vector<CodeWorkspaceItem> BuildCodeWorkspaceActionMap();

}
