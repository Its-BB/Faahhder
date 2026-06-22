#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

enum class EditorPanelKind {
    Viewport,
    Hierarchy,
    Inspector,
    Assets,
    Console,
    Code,
    Tilemap,
    Animation
};

struct PanelRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct PanelState {
    EditorPanelKind kind = EditorPanelKind::Viewport;
    std::string title;
    PanelRect rect;
    bool visible = true;
    bool focused = false;
};

struct ConsoleLine {
    std::string level;
    std::string text;
};

struct AssetNode {
    std::filesystem::path path;
    bool folder = false;
    std::vector<AssetNode> children;
};

struct InspectorField {
    std::string name;
    std::string value;
    bool editable = true;
};

class ConsoleBuffer {
public:
    void Push(std::string level, std::string text);
    void Info(std::string text);
    void Warn(std::string text);
    void Error(std::string text);
    void Clear();
    std::vector<ConsoleLine> Lines() const;
    std::vector<ConsoleLine> Filter(std::string level) const;
    std::string Flatten() const;
    std::size_t Count() const;
private:
    std::vector<ConsoleLine> lines_;
};

class AssetTreeBuilder {
public:
    explicit AssetTreeBuilder(std::filesystem::path root);
    AssetNode Build() const;
    std::vector<std::filesystem::path> FlattenFiles() const;
    std::vector<std::filesystem::path> FilesWithExtension(std::string extension) const;
private:
    AssetNode BuildNode(const std::filesystem::path& path) const;
    std::filesystem::path root_;
};

class InspectorDraft {
public:
    void SetTitle(std::string title);
    void AddField(std::string name, std::string value, bool editable = true);
    bool SetValue(const std::string& name, const std::string& value);
    std::string Title() const;
    std::vector<InspectorField> Fields() const;
    std::string ToText() const;
private:
    std::string title_;
    std::vector<InspectorField> fields_;
};

class EditorWorkspaceLayout {
public:
    void Reset(int width, int height);
    void Focus(EditorPanelKind kind);
    void SetVisible(EditorPanelKind kind, bool visible);
    bool Move(EditorPanelKind kind, PanelRect rect);
    PanelState* Find(EditorPanelKind kind);
    const PanelState* Find(EditorPanelKind kind) const;
    std::vector<PanelState> Panels() const;
    std::string Summary() const;
private:
    std::vector<PanelState> panels_;
};

std::string PanelName(EditorPanelKind kind);
bool Contains(const PanelRect& rect, int x, int y);
PanelRect ClampRect(PanelRect rect, int maxWidth, int maxHeight);

}
