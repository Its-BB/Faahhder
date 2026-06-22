#include "Faahhder/EditorPanels.hpp"

#include <algorithm>
#include <sstream>

namespace faahhder {

std::string PanelName(EditorPanelKind kind) {
    switch (kind) {
    case EditorPanelKind::Viewport: return "Viewport";
    case EditorPanelKind::Hierarchy: return "Hierarchy";
    case EditorPanelKind::Inspector: return "Inspector";
    case EditorPanelKind::Assets: return "Assets";
    case EditorPanelKind::Console: return "Console";
    case EditorPanelKind::Code: return "Code";
    case EditorPanelKind::Tilemap: return "Tilemap";
    case EditorPanelKind::Animation: return "Animation";
    }
    return "Panel";
}

bool Contains(const PanelRect& rect, int x, int y) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
}

PanelRect ClampRect(PanelRect rect, int maxWidth, int maxHeight) {
    rect.width = std::max(0, std::min(rect.width, maxWidth));
    rect.height = std::max(0, std::min(rect.height, maxHeight));
    rect.x = std::max(0, std::min(rect.x, std::max(0, maxWidth - rect.width)));
    rect.y = std::max(0, std::min(rect.y, std::max(0, maxHeight - rect.height)));
    return rect;
}

void ConsoleBuffer::Push(std::string level, std::string text) {
    lines_.push_back({std::move(level), std::move(text)});
    if (lines_.size() > 512) {
        lines_.erase(lines_.begin(), lines_.begin() + static_cast<std::ptrdiff_t>(lines_.size() - 512));
    }
}

void ConsoleBuffer::Info(std::string text) {
    Push("info", std::move(text));
}

void ConsoleBuffer::Warn(std::string text) {
    Push("warn", std::move(text));
}

void ConsoleBuffer::Error(std::string text) {
    Push("error", std::move(text));
}

void ConsoleBuffer::Clear() {
    lines_.clear();
}

std::vector<ConsoleLine> ConsoleBuffer::Lines() const {
    return lines_;
}

std::vector<ConsoleLine> ConsoleBuffer::Filter(std::string level) const {
    std::vector<ConsoleLine> out;
    for (const auto& line : lines_) {
        if (line.level == level) out.push_back(line);
    }
    return out;
}

std::string ConsoleBuffer::Flatten() const {
    std::ostringstream out;
    for (const auto& line : lines_) {
        out << "[" << line.level << "] " << line.text << "\n";
    }
    return out.str();
}

std::size_t ConsoleBuffer::Count() const {
    return lines_.size();
}

AssetTreeBuilder::AssetTreeBuilder(std::filesystem::path root) : root_(std::move(root)) {}

AssetNode AssetTreeBuilder::Build() const {
    return BuildNode(root_);
}

AssetNode AssetTreeBuilder::BuildNode(const std::filesystem::path& path) const {
    AssetNode node;
    node.path = path;
    node.folder = std::filesystem::is_directory(path);
    if (!node.folder || !std::filesystem::exists(path)) return node;

    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        paths.push_back(entry.path());
    }

    std::sort(paths.begin(), paths.end(), [](const auto& a, const auto& b) {
        const bool ad = std::filesystem::is_directory(a);
        const bool bd = std::filesystem::is_directory(b);
        if (ad != bd) return ad > bd;
        return a.filename().string() < b.filename().string();
    });

    for (const auto& child : paths) {
        node.children.push_back(BuildNode(child));
    }
    return node;
}

std::vector<std::filesystem::path> AssetTreeBuilder::FlattenFiles() const {
    std::vector<std::filesystem::path> out;
    if (!std::filesystem::exists(root_)) return out;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root_)) {
        if (entry.is_regular_file()) out.push_back(entry.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::filesystem::path> AssetTreeBuilder::FilesWithExtension(std::string extension) const {
    std::vector<std::filesystem::path> out;
    for (const auto& file : FlattenFiles()) {
        if (file.extension().string() == extension) out.push_back(file);
    }
    return out;
}

void InspectorDraft::SetTitle(std::string title) {
    title_ = std::move(title);
}

void InspectorDraft::AddField(std::string name, std::string value, bool editable) {
    fields_.push_back({std::move(name), std::move(value), editable});
}

bool InspectorDraft::SetValue(const std::string& name, const std::string& value) {
    for (auto& field : fields_) {
        if (field.name == name && field.editable) {
            field.value = value;
            return true;
        }
    }
    return false;
}

std::string InspectorDraft::Title() const {
    return title_;
}

std::vector<InspectorField> InspectorDraft::Fields() const {
    return fields_;
}

std::string InspectorDraft::ToText() const {
    std::ostringstream out;
    out << title_ << "\n";
    for (const auto& field : fields_) {
        out << field.name << ": " << field.value;
        if (!field.editable) out << " (locked)";
        out << "\n";
    }
    return out.str();
}

void EditorWorkspaceLayout::Reset(int width, int height) {
    panels_.clear();
    const int toolbar = 42;
    const int bottom = std::max(120, height / 4);
    const int left = std::max(180, width / 5);
    const int right = std::max(220, width / 4);
    const int centerWidth = std::max(320, width - left - right);
    panels_.push_back({EditorPanelKind::Hierarchy, PanelName(EditorPanelKind::Hierarchy), {0, toolbar, left, height - toolbar - bottom}, true, false});
    panels_.push_back({EditorPanelKind::Assets, PanelName(EditorPanelKind::Assets), {0, height - bottom, left, bottom}, true, false});
    panels_.push_back({EditorPanelKind::Viewport, PanelName(EditorPanelKind::Viewport), {left, toolbar, centerWidth, (height - toolbar - bottom) / 2}, true, true});
    panels_.push_back({EditorPanelKind::Code, PanelName(EditorPanelKind::Code), {left, toolbar + (height - toolbar - bottom) / 2, centerWidth, (height - toolbar - bottom) / 2}, true, false});
    panels_.push_back({EditorPanelKind::Inspector, PanelName(EditorPanelKind::Inspector), {left + centerWidth, toolbar, right, height - toolbar - bottom}, true, false});
    panels_.push_back({EditorPanelKind::Console, PanelName(EditorPanelKind::Console), {left, height - bottom, width - left, bottom}, true, false});
}

void EditorWorkspaceLayout::Focus(EditorPanelKind kind) {
    for (auto& panel : panels_) panel.focused = panel.kind == kind;
}

void EditorWorkspaceLayout::SetVisible(EditorPanelKind kind, bool visible) {
    if (auto* panel = Find(kind)) panel->visible = visible;
}

bool EditorWorkspaceLayout::Move(EditorPanelKind kind, PanelRect rect) {
    if (auto* panel = Find(kind)) {
        panel->rect = rect;
        return true;
    }
    return false;
}

PanelState* EditorWorkspaceLayout::Find(EditorPanelKind kind) {
    for (auto& panel : panels_) if (panel.kind == kind) return &panel;
    return nullptr;
}

const PanelState* EditorWorkspaceLayout::Find(EditorPanelKind kind) const {
    for (const auto& panel : panels_) if (panel.kind == kind) return &panel;
    return nullptr;
}

std::vector<PanelState> EditorWorkspaceLayout::Panels() const {
    return panels_;
}

std::string EditorWorkspaceLayout::Summary() const {
    std::ostringstream out;
    for (const auto& panel : panels_) {
        out << panel.title << " " << panel.rect.x << "," << panel.rect.y << " " << panel.rect.width << "x" << panel.rect.height;
        if (panel.focused) out << " focused";
        if (!panel.visible) out << " hidden";
        out << "\n";
    }
    return out.str();
}

}
