#include "Faahhder/Faahhder.hpp"

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr COLORREF ColorBg = RGB(12, 14, 18);
constexpr COLORREF ColorPanel = RGB(24, 27, 34);
constexpr COLORREF ColorPanelAlt = RGB(31, 36, 46);
constexpr COLORREF ColorField = RGB(18, 21, 28);
constexpr COLORREF ColorAccent = RGB(75, 170, 255);
constexpr COLORREF ColorText = RGB(232, 238, 245);
constexpr COLORREF ColorMuted = RGB(145, 154, 168);

enum ControlId {
    IdHierarchy = 100,
    IdAssets,
    IdCode,
    IdConsole,
    IdStatus,
    IdAddEntity,
    IdPlay,
    IdStop,
    IdSaveScene,
    IdNewProject,
    IdLoadScene,
    IdNewScript,
    IdSaveCode,
    IdBuildGame,
    IdRefresh,
    IdInspectName,
    IdInspectX,
    IdInspectY,
    IdInspectRotation,
    IdInspectScaleX,
    IdInspectScaleY,
    IdInspectScript,
    IdInspectorApply,
    IdAddSprite,
    IdAddBody,
    IdAddCollider,
    IdAddScript,
    IdAssetPath,
    IdAssetExtension,
    IdAssetApplyExtension,
    IdMenuNewLua = 500,
    IdMenuNewLogic,
    IdMenuNewConfig,
    IdMenuReveal,
    IdMenuDelete
};

struct EditorApp {
    std::filesystem::path engineRoot = ".";
    std::filesystem::path projectRoot = ".";
    faahhder::EditorModel editor{"."};
    faahhder::EntityId selectedEntity = faahhder::InvalidEntity;
    std::filesystem::path currentFile = "assets/snake.faahhder";
    std::filesystem::path selectedAsset;
    std::vector<faahhder::EntityId> hierarchyIds;
    std::vector<std::filesystem::path> assetRows;

    HFONT uiFont = nullptr;
    HFONT strongFont = nullptr;
    HFONT monoFont = nullptr;
    HBRUSH bgBrush = nullptr;
    HBRUSH panelBrush = nullptr;
    HBRUSH fieldBrush = nullptr;

    HWND title = nullptr;
    HWND hierarchyLabel = nullptr;
    HWND assetsLabel = nullptr;
    HWND codeLabel = nullptr;
    HWND inspectorLabel = nullptr;
    HWND consoleLabel = nullptr;
    HWND assetToolsLabel = nullptr;
    HWND status = nullptr;
    HWND hierarchy = nullptr;
    HWND assets = nullptr;
    HWND code = nullptr;
    HWND console = nullptr;
    HWND inspectName = nullptr;
    HWND inspectX = nullptr;
    HWND inspectY = nullptr;
    HWND inspectRotation = nullptr;
    HWND inspectScaleX = nullptr;
    HWND inspectScaleY = nullptr;
    HWND inspectScript = nullptr;
    HWND inspectLabels[7]{};
    HWND assetPath = nullptr;
    HWND assetExtension = nullptr;
};

std::string Trim(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) return {};
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& text) {
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file << text;
    return true;
}

std::unordered_map<std::string, std::string> ReadPairs(const std::filesystem::path& path) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        const auto eq = line.find('=');
        if (eq != std::string::npos) values[Trim(line.substr(0, eq))] = Trim(line.substr(eq + 1));
    }
    return values;
}

std::string PairValue(const std::unordered_map<std::string, std::string>& values, const std::string& key, const std::string& fallback) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

std::string SafeFileName(std::string value) {
    if (value.empty()) value = "FaahhderGame";
    for (char& c : value) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) c = '_';
    }
    return value;
}

bool IsProjectRoot(const std::filesystem::path& path) {
    return std::filesystem::exists(path / "CMakeLists.txt") && std::filesystem::exists(path / "src/FaahhderRuntime");
}

std::filesystem::path WalkForProjectRoot(std::filesystem::path start) {
    start = std::filesystem::absolute(start);
    if (std::filesystem::is_regular_file(start)) start = start.parent_path();
    for (auto cursor = start; !cursor.empty(); cursor = cursor.parent_path()) {
        if (IsProjectRoot(cursor)) return cursor;
        if (cursor == cursor.parent_path()) break;
    }
    return std::filesystem::absolute(".");
}

std::filesystem::path FindProjectRoot() {
    if (IsProjectRoot(std::filesystem::absolute("."))) return std::filesystem::absolute(".");
    char modulePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return WalkForProjectRoot(modulePath);
}

std::filesystem::path DefaultExampleProject(const std::filesystem::path& engineRoot) {
    const auto snake = engineRoot / "examples/snake";
    if (std::filesystem::exists(snake / "assets/snake.faahhder")) return snake;
    return engineRoot;
}

std::string WindowText(HWND hwnd) {
    const int length = GetWindowTextLengthA(hwnd);
    std::string text(static_cast<size_t>(length), '\0');
    if (length > 0) GetWindowTextA(hwnd, text.data(), length + 1);
    return text;
}

std::string NormalizeEditorNewlines(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 16);
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '\r') {
            out.push_back('\r');
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                out.push_back('\n');
                ++i;
            } else {
                out.push_back('\n');
            }
        } else if (c == '\n') {
            out.push_back('\r');
            out.push_back('\n');
        } else {
            out.push_back(c);
        }
    }
    return out;
}

bool IsWordChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

void ColorRange(HWND edit, long start, long end, COLORREF color) {
    if (!edit || start < 0 || end <= start) return;
    CHARRANGE previous{};
    SendMessageA(edit, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&previous));
    CHARRANGE range{start, end};
    SendMessageA(edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    CHARFORMAT2A format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = color;
    SendMessageA(edit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
    SendMessageA(edit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&previous));
}

void HighlightCode(EditorApp& app) {
    if (!app.code) return;
    const std::string text = WindowText(app.code);
    const auto ext = app.currentFile.extension().string();
    SendMessageA(app.code, WM_SETREDRAW, FALSE, 0);
    ColorRange(app.code, 0, static_cast<long>(text.size()), RGB(222, 228, 236));

    for (size_t i = 0; i < text.size();) {
        if (i + 1 < text.size() && text[i] == '-' && text[i + 1] == '-') {
            const size_t end = text.find('\n', i);
            ColorRange(app.code, static_cast<long>(i), static_cast<long>(end == std::string::npos ? text.size() : end), RGB(125, 140, 155));
            i = end == std::string::npos ? text.size() : end + 1;
            continue;
        }
        if (text[i] == '"' || text[i] == '\'') {
            const char quote = text[i];
            size_t end = i + 1;
            bool escaped = false;
            while (end < text.size()) {
                if (!escaped && text[end] == quote) {
                    ++end;
                    break;
                }
                escaped = !escaped && text[end] == '\\';
                if (text[end] != '\\') escaped = false;
                ++end;
            }
            ColorRange(app.code, static_cast<long>(i), static_cast<long>(end), RGB(255, 205, 130));
            i = end;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(text[i]))) {
            const size_t start = i;
            while (i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.')) ++i;
            ColorRange(app.code, static_cast<long>(start), static_cast<long>(i), RGB(130, 220, 160));
            continue;
        }
        ++i;
    }

    const std::vector<std::string> keywords = ext == ".lua"
        ? std::vector<std::string>{"function", "end", "local", "if", "then", "else", "elseif", "return", "true", "false", "nil"}
        : std::vector<std::string>{"name", "title", "controls", "restart", "goal", "columns", "rows", "cell", "wrap", "score", "speed"};

    for (const auto& keyword : keywords) {
        size_t pos = 0;
        while ((pos = text.find(keyword, pos)) != std::string::npos) {
            const bool leftOk = pos == 0 || !IsWordChar(text[pos - 1]);
            const size_t end = pos + keyword.size();
            const bool rightOk = end >= text.size() || !IsWordChar(text[end]);
            if (leftOk && rightOk) ColorRange(app.code, static_cast<long>(pos), static_cast<long>(end), ext == ".lua" ? RGB(105, 190, 255) : RGB(170, 210, 255));
            pos = end;
        }
    }

    if (ext != ".lua") {
        size_t lineStart = 0;
        while (lineStart < text.size()) {
            const size_t lineEnd = text.find('\n', lineStart);
            const size_t stop = lineEnd == std::string::npos ? text.size() : lineEnd;
            const size_t eq = text.find('=', lineStart);
            if (eq != std::string::npos && eq < stop) ColorRange(app.code, static_cast<long>(lineStart), static_cast<long>(eq), RGB(130, 220, 160));
            if (lineEnd == std::string::npos) break;
            lineStart = lineEnd + 1;
        }
    }

    SendMessageA(app.code, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(app.code, nullptr, TRUE);
}

void SetCodeText(EditorApp& app, const std::string& text) {
    SetWindowTextA(app.code, NormalizeEditorNewlines(text).c_str());
    HighlightCode(app);
}

float FloatFrom(HWND hwnd, float fallback) {
    try {
        const auto text = Trim(WindowText(hwnd));
        return text.empty() ? fallback : std::stof(text);
    } catch (...) {
        return fallback;
    }
}

void Set(HWND hwnd, const std::string& text) {
    SetWindowTextA(hwnd, text.c_str());
}

void Status(EditorApp& app, const std::string& text) {
    Set(app.status, "  " + text);
}

void AppendConsole(EditorApp& app, const std::string& message) {
    Set(app.console, WindowText(app.console) + message + "\r\n");
    SendMessageA(app.console, EM_SETSEL, -1, -1);
    SendMessageA(app.console, EM_SCROLLCARET, 0, 0);
    Status(app, message);
}

HWND MakeControl(HWND parent, const char* cls, const char* text, DWORD style, int id, DWORD exStyle = WS_EX_CLIENTEDGE) {
    return CreateWindowExA(
        exStyle,
        cls,
        text,
        WS_CHILD | WS_VISIBLE | style,
        0,
        0,
        100,
        100,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleA(nullptr),
        nullptr);
}

HWND Label(HWND parent, const char* text) {
    return MakeControl(parent, "STATIC", text, SS_LEFT, 0, 0);
}

HWND Button(HWND parent, const char* text, int id) {
    return MakeControl(parent, "BUTTON", text, BS_PUSHBUTTON | BS_FLAT, id, 0);
}

void ApplyFont(HWND hwnd, HFONT font) {
    SendMessageA(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

std::filesystem::path BuiltGameExecutable(const EditorApp& app) {
    const auto release = app.engineRoot / "build/dev/src/FaahhderGame/Release/FaahhderGame.exe";
    if (std::filesystem::exists(release)) return release;
    const auto debug = app.engineRoot / "build/dev/src/FaahhderGame/Debug/FaahhderGame.exe";
    if (std::filesystem::exists(debug)) return debug;
    const auto rootRelease = app.engineRoot / "build/src/FaahhderGame/Release/FaahhderGame.exe";
    if (std::filesystem::exists(rootRelease)) return rootRelease;
    return debug;
}

void EnsureExampleFiles(EditorApp& app) {
    std::filesystem::create_directories(app.projectRoot / "assets/scripts");
    std::filesystem::create_directories(app.projectRoot / "assets/scenes");
    std::filesystem::create_directories(app.projectRoot / "assets/textures");

    const auto projectFile = app.projectRoot / "project.faahhder";
    if (!std::filesystem::exists(projectFile)) {
        WriteTextFile(projectFile, "name=New Game\nruntime=FaahhderGame\nentry=assets/snake.faahhder\n");
    }

    const auto snakeConfig = app.projectRoot / "assets/snake.faahhder";
    if (!std::filesystem::exists(snakeConfig)) {
        WriteTextFile(snakeConfig,
            "title=Faahhder Snake\n"
            "columns=28\n"
            "rows=20\n"
            "cell=24\n"
            "start_length=3\n"
            "food_score=10\n"
            "base_speed_ms=130\n"
            "mid_speed_ms=105\n"
            "fast_speed_ms=80\n"
            "mid_score=60\n"
            "fast_score=120\n"
            "wrap=false\n"
            "bg=12,14,18\n"
            "panel=22,26,34\n"
            "grid=34,40,51\n"
            "snake_head=80,210,145\n"
            "snake_body=42,155,105\n"
            "food=255,90,105\n");
    }

    const auto logic = app.projectRoot / "assets/scripts/snake.logic";
    if (!std::filesystem::exists(logic)) {
        WriteTextFile(logic,
            "name=Faahhder Snake\n"
            "controls=Arrow keys or WASD\n"
            "restart=Space\n"
            "goal=Eat food, grow longer, avoid walls and your own body.\n");
    }

    const auto script = app.projectRoot / "assets/scripts/player.lua";
    if (!std::filesystem::exists(script)) {
        WriteTextFile(script,
            "function OnCreate(entity)\n"
            "    Log.Info(\"Player ready\")\n"
            "end\n\n"
            "function OnUpdate(entity, dt)\n"
            "    local speed = 120\n"
            "    if Input.IsKeyDown(\"A\") then Entity.SetPosition(entity, Entity.GetX(entity) - speed * dt, Entity.GetY(entity)) end\n"
            "    if Input.IsKeyDown(\"D\") then Entity.SetPosition(entity, Entity.GetX(entity) + speed * dt, Entity.GetY(entity)) end\n"
            "end\n");
    }
}

void EnsureStarterScene(EditorApp& app) {
    auto& scene = app.editor.GetScene();
    if (scene.FindEntityByName("Player") != faahhder::InvalidEntity) return;

    const auto player = scene.CreateEntity("Player");
    scene.GetTransform(player)->position = {80.0f, 120.0f};
    scene.AddSpriteRenderer(player, {"player", "characters", 0});
    scene.AddRigidbody(player, {faahhder::BodyType::Dynamic, {0.0f, 0.0f}, 0.0f});
    scene.AddCollider(player, {faahhder::ColliderShape::AABB, {}, {32.0f, 48.0f}, 0.0f, false});
    scene.AddLuaScript(player, {"assets/scripts/player.lua", true});

    const auto light = scene.CreateEntity("Example Light");
    scene.GetTransform(light)->position = {220.0f, 170.0f};
    scene.AddPointLight(light, {{0.5f, 0.8f, 1.0f, 1.0f}, 220.0f, 1.0f});

    const auto ground = scene.CreateEntity("Ground");
    scene.GetTransform(ground)->position = {0.0f, 24.0f};
    scene.AddCollider(ground, {faahhder::ColliderShape::AABB, {}, {640.0f, 48.0f}, 0.0f, false});

    app.selectedEntity = player;
    app.editor.Log("Example project loaded");
}

void RefreshHierarchy(EditorApp& app) {
    SendMessageA(app.hierarchy, LB_RESETCONTENT, 0, 0);
    app.hierarchyIds.clear();
    for (auto entity : app.editor.GetScene().Entities()) {
        const auto* info = app.editor.GetScene().GetInfo(entity);
        if (!info) continue;
        app.hierarchyIds.push_back(entity);
        const std::string row = std::to_string(entity) + "  " + info->name;
        SendMessageA(app.hierarchy, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
}

void RefreshAssets(EditorApp& app) {
    SendMessageA(app.assets, LB_RESETCONTENT, 0, 0);
    app.assetRows.clear();
    if (std::filesystem::exists(app.projectRoot / "project.faahhder")) {
        app.assetRows.push_back("project.faahhder");
        SendMessageA(app.assets, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>("project.faahhder  [.faahhder]"));
    }
    for (const auto& asset : app.editor.BuildAssetBrowser()) {
        auto relative = std::filesystem::relative(asset, app.projectRoot);
        app.assetRows.push_back(relative);
        const std::string ext = relative.extension().string();
        const std::string row = relative.generic_string() + "  [" + (ext.empty() ? "no extension" : ext) + "]";
        SendMessageA(app.assets, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
}

void RefreshInspector(EditorApp& app) {
    const auto* info = app.editor.GetScene().GetInfo(app.selectedEntity);
    const auto* transform = app.editor.GetScene().GetTransform(app.selectedEntity);
    const auto* script = app.editor.GetScene().GetLuaScript(app.selectedEntity);
    Set(app.inspectName, info ? info->name : "");
    Set(app.inspectX, transform ? std::to_string(transform->position.x) : "");
    Set(app.inspectY, transform ? std::to_string(transform->position.y) : "");
    Set(app.inspectRotation, transform ? std::to_string(transform->rotation) : "");
    Set(app.inspectScaleX, transform ? std::to_string(transform->scale.x) : "");
    Set(app.inspectScaleY, transform ? std::to_string(transform->scale.y) : "");
    Set(app.inspectScript, script ? script->path.generic_string() : "");
}

void SelectEntity(EditorApp& app, faahhder::EntityId entity) {
    app.selectedEntity = entity;
    if (const auto* script = app.editor.GetScene().GetLuaScript(entity)) app.currentFile = script->path;
    RefreshInspector(app);
    SetCodeText(app, ReadTextFile(app.projectRoot / app.currentFile));
    Status(app, "Selected entity. Ctrl+S saves code. F5 plays.");
}

void RefreshConsole(EditorApp& app) {
    std::ostringstream out;
    for (const auto& row : app.editor.BuildConsolePanel()) out << row << "\r\n";
    Set(app.console, out.str());
}

void RefreshAll(EditorApp& app) {
    RefreshHierarchy(app);
    RefreshAssets(app);
    RefreshConsole(app);
    RefreshInspector(app);
}

void SaveCurrentCode(EditorApp& app) {
    if (WriteTextFile(app.projectRoot / app.currentFile, WindowText(app.code))) {
        AppendConsole(app, "Saved " + app.currentFile.generic_string());
        HighlightCode(app);
        RefreshAssets(app);
    } else {
        AppendConsole(app, "Could not save " + app.currentFile.generic_string());
    }
}

void ApplyInspector(EditorApp& app) {
    auto* info = app.editor.GetScene().GetInfo(app.selectedEntity);
    auto* transform = app.editor.GetScene().GetTransform(app.selectedEntity);
    if (!info || !transform) {
        AppendConsole(app, "Select an entity before editing the inspector.");
        return;
    }
    info->name = Trim(WindowText(app.inspectName));
    transform->position.x = FloatFrom(app.inspectX, transform->position.x);
    transform->position.y = FloatFrom(app.inspectY, transform->position.y);
    transform->rotation = FloatFrom(app.inspectRotation, transform->rotation);
    transform->scale.x = FloatFrom(app.inspectScaleX, transform->scale.x);
    transform->scale.y = FloatFrom(app.inspectScaleY, transform->scale.y);
    const auto scriptPath = Trim(WindowText(app.inspectScript));
    if (!scriptPath.empty()) app.editor.GetScene().AddLuaScript(app.selectedEntity, {scriptPath, true});
    RefreshHierarchy(app);
    RefreshInspector(app);
    AppendConsole(app, "Applied inspector edits.");
}

void OpenAsset(EditorApp& app, int index) {
    if (index == LB_ERR || index >= static_cast<int>(app.assetRows.size())) return;
    app.selectedAsset = app.assetRows[static_cast<size_t>(index)];
    Set(app.assetPath, app.selectedAsset.generic_string());
    Set(app.assetExtension, app.selectedAsset.extension().string());
    const auto ext = app.selectedAsset.extension().string();
    if (ext == ".lua" || ext == ".logic" || ext == ".json" || ext == ".faahhder" || ext == ".txt" || ext == ".md") {
        app.currentFile = app.selectedAsset;
        SetCodeText(app, ReadTextFile(app.projectRoot / app.currentFile));
        AppendConsole(app, "Opened " + app.currentFile.generic_string());
    } else {
        AppendConsole(app, "Selected " + app.selectedAsset.generic_string());
    }
}

void CreateAsset(EditorApp& app, const std::filesystem::path& relative, const std::string& contents) {
    if (WriteTextFile(app.projectRoot / relative, contents)) {
        app.currentFile = relative;
        SetCodeText(app, contents);
        RefreshAssets(app);
        AppendConsole(app, "Created " + relative.generic_string());
    }
}

void ApplyAssetExtension(EditorApp& app) {
    const auto selected = Trim(WindowText(app.assetPath));
    std::string ext = Trim(WindowText(app.assetExtension));
    if (selected.empty()) {
        AppendConsole(app, "Select an asset before changing its extension.");
        return;
    }
    if (!ext.empty() && ext.front() != '.') ext = "." + ext;
    const auto oldPath = app.projectRoot / selected;
    auto newRel = std::filesystem::path(selected);
    newRel.replace_extension(ext);
    const auto newPath = app.projectRoot / newRel;
    if (!std::filesystem::exists(oldPath)) {
        AppendConsole(app, "Asset no longer exists: " + selected);
        return;
    }
    std::filesystem::rename(oldPath, newPath);
    app.selectedAsset = newRel;
    app.currentFile = newRel;
    RefreshAssets(app);
    Set(app.assetPath, newRel.generic_string());
    AppendConsole(app, "Renamed asset to " + newRel.generic_string());
}

bool CopyDirectory(const std::filesystem::path& from, const std::filesystem::path& to) {
    if (!std::filesystem::exists(from)) return false;
    std::filesystem::create_directories(to);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
        const auto relative = std::filesystem::relative(entry.path(), from);
        const auto target = to / relative;
        if (entry.is_directory()) {
            std::filesystem::create_directories(target);
        } else if (entry.is_regular_file()) {
            std::filesystem::create_directories(target.parent_path());
            std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing);
        }
    }
    return true;
}

bool EnsureGameBuilt(EditorApp& app) {
    auto gameExe = BuiltGameExecutable(app);
    if (std::filesystem::exists(gameExe)) return true;
    const auto buildScript = app.engineRoot / "scripts/build.ps1";
    AppendConsole(app, "Compiling FaahhderGame...");
    const std::string command = "powershell -ExecutionPolicy Bypass -File \"" + buildScript.string() + "\"";
    const int result = std::system(command.c_str());
    if (result != 0) {
        AppendConsole(app, "Build command failed.");
        return false;
    }
    return std::filesystem::exists(BuiltGameExecutable(app));
}

void PlayGame(EditorApp& app) {
    SaveCurrentCode(app);
    if (!EnsureGameBuilt(app)) {
        AppendConsole(app, "Play failed: FaahhderGame.exe was not produced.");
        return;
    }
    app.editor.Play();
    const auto exe = BuiltGameExecutable(app);
    const std::string args = "\"" + app.projectRoot.string() + "\"";
    const auto result = reinterpret_cast<intptr_t>(ShellExecuteA(nullptr, "open", exe.string().c_str(), args.c_str(), app.projectRoot.string().c_str(), SW_SHOWNORMAL));
    AppendConsole(app, result > 32 ? "Play launched a game window." : "Play failed to launch the game window.");
}

void StopPlay(EditorApp& app) {
    app.editor.Stop();
    AppendConsole(app, "Stopped editor play state.");
}

void BuildProjectExecutable(EditorApp& app) {
    SaveCurrentCode(app);
    if (!EnsureGameBuilt(app)) {
        AppendConsole(app, "Build failed: game executable missing.");
        return;
    }
    const auto project = ReadPairs(app.projectRoot / "project.faahhder");
    const std::string name = SafeFileName(PairValue(project, "name", app.projectRoot.filename().string()));
    const auto outDir = app.engineRoot / "dist" / name;
    std::filesystem::remove_all(outDir);
    std::filesystem::create_directories(outDir);
    std::filesystem::copy_file(BuiltGameExecutable(app), outDir / (name + ".exe"), std::filesystem::copy_options::overwrite_existing);
    if (std::filesystem::exists(app.projectRoot / "project.faahhder")) {
        std::filesystem::copy_file(app.projectRoot / "project.faahhder", outDir / "project.faahhder", std::filesystem::copy_options::overwrite_existing);
    }
    CopyDirectory(app.projectRoot / "assets", outDir / "assets");
    AppendConsole(app, "Built standalone game: " + outDir.string());
    ShellExecuteA(nullptr, "open", outDir.string().c_str(), nullptr, outDir.string().c_str(), SW_SHOWNORMAL);
}

void CreateProject(EditorApp& app) {
    const auto projectName = "Game_" + std::to_string(GetTickCount64());
    app.projectRoot = app.engineRoot / "projects" / projectName;
    app.editor = faahhder::EditorModel(app.projectRoot);
    app.selectedEntity = faahhder::InvalidEntity;
    app.currentFile = "assets/snake.faahhder";
    EnsureExampleFiles(app);
    EnsureStarterScene(app);
    RefreshAll(app);
    AppendConsole(app, "Created project: " + app.projectRoot.string());
}

void ShowAssetMenu(HWND hwnd, EditorApp& app, int x, int y) {
    HMENU menu = CreatePopupMenu();
    AppendMenuA(menu, MF_STRING, IdMenuNewLua, "Create Lua Script");
    AppendMenuA(menu, MF_STRING, IdMenuNewLogic, "Create Snake Logic");
    AppendMenuA(menu, MF_STRING, IdMenuNewConfig, "Create Config");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(menu, MF_STRING, IdMenuReveal, "Reveal In Explorer");
    AppendMenuA(menu, MF_STRING, IdMenuDelete, "Delete Selected Asset");
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, x, y, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

void Layout(HWND hwnd, EditorApp& app) {
    RECT r{};
    GetClientRect(hwnd, &r);
    const int w = r.right - r.left;
    const int h = r.bottom - r.top;
    const int gap = 10;
    const int toolbar = 52;
    const int status = 28;
    const int left = 285;
    const int right = 360;
    const int bottom = 150;
    const int label = 24;
    const int contentTop = toolbar + gap;
    const int contentBottom = h - bottom - status - gap;
    const int sideHeight = contentBottom - contentTop;
    const int centerX = left + gap;
    const int centerW = std::max(240, w - left - right - gap * 2);
    const int rightX = w - right + gap;
    const int rightW = right - gap * 2;

    MoveWindow(app.title, gap, 10, 230, 30, TRUE);
    int x = 250;
    for (int id : {IdAddEntity, IdPlay, IdStop, IdSaveScene, IdNewProject, IdLoadScene, IdNewScript, IdSaveCode, IdBuildGame, IdRefresh}) {
        MoveWindow(GetDlgItem(hwnd, id), x, 11, 94, 30, TRUE);
        x += 100;
    }

    MoveWindow(app.hierarchyLabel, gap, contentTop, left - gap * 2, label, TRUE);
    MoveWindow(app.hierarchy, gap, contentTop + label, left - gap * 2, sideHeight / 2 - label - 62, TRUE);
    MoveWindow(app.assetsLabel, gap, contentTop + sideHeight / 2 - 36, left - gap * 2, label, TRUE);
    MoveWindow(app.assets, gap, contentTop + sideHeight / 2 - 12, left - gap * 2, sideHeight / 2 - 104, TRUE);
    MoveWindow(app.assetToolsLabel, gap, contentTop + sideHeight - 112, left - gap * 2, label, TRUE);
    MoveWindow(app.assetPath, gap, contentTop + sideHeight - 86, left - gap * 2, 24, TRUE);
    MoveWindow(app.assetExtension, gap, contentTop + sideHeight - 56, 96, 24, TRUE);
    MoveWindow(GetDlgItem(hwnd, IdAssetApplyExtension), gap + 104, contentTop + sideHeight - 58, 150, 28, TRUE);

    MoveWindow(app.codeLabel, centerX, contentTop, centerW, label, TRUE);
    MoveWindow(app.code, centerX, contentTop + label, centerW, sideHeight - label, TRUE);

    MoveWindow(app.inspectorLabel, rightX, contentTop, rightW, label, TRUE);
    int iy = contentTop + label + 4;
    const int ih = 26;
    const int lw = 82;
    int labelIndex = 0;
    auto row = [&](HWND labelHwnd, HWND control) {
        MoveWindow(labelHwnd, rightX, iy + 4, lw, 20, TRUE);
        MoveWindow(control, rightX + lw, iy, rightW - lw, ih, TRUE);
        iy += ih + 7;
        ++labelIndex;
    };
    row(app.inspectLabels[labelIndex], app.inspectName);
    row(app.inspectLabels[labelIndex], app.inspectX);
    row(app.inspectLabels[labelIndex], app.inspectY);
    row(app.inspectLabels[labelIndex], app.inspectRotation);
    row(app.inspectLabels[labelIndex], app.inspectScaleX);
    row(app.inspectLabels[labelIndex], app.inspectScaleY);
    row(app.inspectLabels[labelIndex], app.inspectScript);
    MoveWindow(GetDlgItem(hwnd, IdInspectorApply), rightX, iy + 4, rightW, 30, TRUE);
    iy += 42;
    MoveWindow(GetDlgItem(hwnd, IdAddSprite), rightX, iy, rightW, 28, TRUE);
    MoveWindow(GetDlgItem(hwnd, IdAddBody), rightX, iy + 34, rightW, 28, TRUE);
    MoveWindow(GetDlgItem(hwnd, IdAddCollider), rightX, iy + 68, rightW, 28, TRUE);
    MoveWindow(GetDlgItem(hwnd, IdAddScript), rightX, iy + 102, rightW, 28, TRUE);

    MoveWindow(app.consoleLabel, gap, h - bottom - status, w - gap * 2, label, TRUE);
    MoveWindow(app.console, gap, h - bottom - status + label, w - gap * 2, bottom - label, TRUE);
    MoveWindow(app.status, 0, h - status, w, status, TRUE);
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT r{};
    GetClientRect(hwnd, &r);
    HBRUSH brush = CreateSolidBrush(ColorBg);
    FillRect(dc, &r, brush);
    DeleteObject(brush);
    EndPaint(hwnd, &ps);
}

void HandleCommand(HWND hwnd, EditorApp& app, int id) {
    switch (id) {
    case IdAddEntity: {
        const auto entity = app.editor.GetScene().CreateEntity("Entity");
        app.selectedEntity = entity;
        app.editor.Log("Created entity " + std::to_string(entity));
        RefreshAll(app);
        break;
    }
    case IdPlay:
        PlayGame(app);
        break;
    case IdStop:
        StopPlay(app);
        break;
    case IdSaveScene:
        if (app.editor.SaveScene(app.projectRoot / "assets/scenes/editor_scene.faahhder.json")) AppendConsole(app, "Saved scene.");
        break;
    case IdNewProject:
        CreateProject(app);
        break;
    case IdLoadScene:
        app.editor.LoadScene(app.projectRoot / "assets/scenes/sample_scene.faahhder.json");
        app.selectedEntity = app.editor.GetScene().FindEntityByName("Player");
        RefreshAll(app);
        AppendConsole(app, "Loaded scene.");
        break;
    case IdNewScript:
        CreateAsset(app, "assets/scripts/new_script.lua", "function OnCreate(entity)\n    Log.Info(\"Created in Faahhder\")\nend\n\nfunction OnUpdate(entity, dt)\nend\n");
        break;
    case IdSaveCode:
        SaveCurrentCode(app);
        break;
    case IdBuildGame:
        BuildProjectExecutable(app);
        break;
    case IdRefresh:
        RefreshAll(app);
        AppendConsole(app, "Refreshed.");
        break;
    case IdInspectorApply:
        ApplyInspector(app);
        break;
    case IdAddSprite:
        app.editor.GetScene().AddSpriteRenderer(app.selectedEntity, {"player", "characters", 0});
        AppendConsole(app, "Added SpriteRenderer.");
        break;
    case IdAddBody:
        app.editor.GetScene().AddRigidbody(app.selectedEntity, {faahhder::BodyType::Dynamic, {0.0f, 0.0f}, 1.0f});
        AppendConsole(app, "Added Rigidbody2D.");
        break;
    case IdAddCollider:
        app.editor.GetScene().AddCollider(app.selectedEntity, {faahhder::ColliderShape::AABB, {}, {32.0f, 32.0f}, 0.0f, false});
        AppendConsole(app, "Added Collider2D.");
        break;
    case IdAddScript:
        app.editor.GetScene().AddLuaScript(app.selectedEntity, {"assets/scripts/player.lua", true});
        RefreshInspector(app);
        AppendConsole(app, "Added LuaScript.");
        break;
    case IdAssetApplyExtension:
        ApplyAssetExtension(app);
        break;
    case IdMenuNewLua:
        CreateAsset(app, "assets/scripts/new_script.lua", "function OnCreate(entity)\nend\n\nfunction OnUpdate(entity, dt)\nend\n");
        break;
    case IdMenuNewLogic:
        CreateAsset(app, "assets/scripts/new.logic", "name=Faahhder Snake\ncontrols=Arrow keys or WASD\nrestart=Space\ngoal=Edit this rule file in the code editor.\n");
        break;
    case IdMenuNewConfig:
        CreateAsset(app, "assets/new_config.faahhder", "name=New Config\nvalue=1\n");
        break;
    case IdMenuReveal:
        ShellExecuteA(nullptr, "open", (app.projectRoot / app.selectedAsset).parent_path().string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case IdMenuDelete:
        if (!app.selectedAsset.empty() && std::filesystem::exists(app.projectRoot / app.selectedAsset)) {
            std::filesystem::remove(app.projectRoot / app.selectedAsset);
            RefreshAssets(app);
            AppendConsole(app, "Deleted " + app.selectedAsset.generic_string());
        }
        break;
    default:
        break;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* app = reinterpret_cast<EditorApp*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lp);
        app = reinterpret_cast<EditorApp*>(create->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->uiFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        app->strongFont = CreateFontA(21, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        app->monoFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
        app->bgBrush = CreateSolidBrush(ColorBg);
        app->panelBrush = CreateSolidBrush(ColorPanel);
        app->fieldBrush = CreateSolidBrush(ColorField);

        app->title = Label(hwnd, "Faahhder");
        app->hierarchyLabel = Label(hwnd, "Hierarchy");
        app->assetsLabel = Label(hwnd, "Assets");
        app->assetToolsLabel = Label(hwnd, "Asset Details");
        app->codeLabel = Label(hwnd, "Code Editor");
        app->inspectorLabel = Label(hwnd, "Inspector");
        app->consoleLabel = Label(hwnd, "Console");
        app->status = Label(hwnd, "  Ready");

        for (HWND label : {app->title, app->hierarchyLabel, app->assetsLabel, app->assetToolsLabel, app->codeLabel, app->inspectorLabel, app->consoleLabel, app->status}) ApplyFont(label, app->uiFont);
        ApplyFont(app->title, app->strongFont);

        Button(hwnd, "+ Entity", IdAddEntity);
        Button(hwnd, "Play", IdPlay);
        Button(hwnd, "Stop", IdStop);
        Button(hwnd, "Save Scene", IdSaveScene);
        Button(hwnd, "New Project", IdNewProject);
        Button(hwnd, "Load Scene", IdLoadScene);
        Button(hwnd, "New Script", IdNewScript);
        Button(hwnd, "Save Code", IdSaveCode);
        Button(hwnd, "Build EXE", IdBuildGame);
        Button(hwnd, "Refresh", IdRefresh);

        app->hierarchy = MakeControl(hwnd, "LISTBOX", "", LBS_NOTIFY | WS_VSCROLL, IdHierarchy);
        app->assets = MakeControl(hwnd, "LISTBOX", "", LBS_NOTIFY | WS_VSCROLL, IdAssets);
        app->code = MakeControl(hwnd, "RichEdit20A", "", ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL, IdCode);
        if (!app->code) {
            app->code = MakeControl(hwnd, "EDIT", "", ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL, IdCode);
        }
        app->console = MakeControl(hwnd, "EDIT", "", ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, IdConsole);
        app->inspectName = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectName);
        app->inspectX = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectX);
        app->inspectY = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectY);
        app->inspectRotation = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectRotation);
        app->inspectScaleX = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectScaleX);
        app->inspectScaleY = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectScaleY);
        app->inspectScript = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdInspectScript);
        const char* inspectorLabels[] = {"Name", "X", "Y", "Rotation", "Scale X", "Scale Y", "Script"};
        for (int i = 0; i < 7; ++i) {
            app->inspectLabels[i] = Label(hwnd, inspectorLabels[i]);
            ApplyFont(app->inspectLabels[i], app->uiFont);
        }
        app->assetPath = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL | ES_READONLY, IdAssetPath);
        app->assetExtension = MakeControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, IdAssetExtension);
        Button(hwnd, "Apply Extension", IdAssetApplyExtension);
        Button(hwnd, "Apply Inspector", IdInspectorApply);
        Button(hwnd, "Add SpriteRenderer", IdAddSprite);
        Button(hwnd, "Add Rigidbody2D", IdAddBody);
        Button(hwnd, "Add Collider2D", IdAddCollider);
        Button(hwnd, "Add LuaScript", IdAddScript);

        for (HWND control : {app->hierarchy, app->assets, app->inspectName, app->inspectX, app->inspectY, app->inspectRotation, app->inspectScaleX, app->inspectScaleY, app->inspectScript, app->assetPath, app->assetExtension}) ApplyFont(control, app->uiFont);
        ApplyFont(app->code, app->monoFont);
        ApplyFont(app->console, app->monoFont);

        SendMessageA(app->code, EM_SETLIMITTEXT, 0, 0);
        SendMessageA(app->code, EM_SETBKGNDCOLOR, 0, ColorField);
        RefreshAll(*app);
        SelectEntity(*app, app->selectedEntity);
        Layout(hwnd, *app);
        AppendConsole(*app, "Ready. F5 plays, Ctrl+S saves code, right-click assets for actions.");
        return 0;
    }
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_SIZE:
        if (app) Layout(hwnd, *app);
        return 0;
    case WM_CONTEXTMENU:
        if (app && reinterpret_cast<HWND>(wp) == app->assets) {
            ShowAssetMenu(hwnd, *app, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;
        }
        break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, ColorText);
        SetBkColor(dc, msg == WM_CTLCOLORSTATIC ? ColorBg : ColorField);
        return reinterpret_cast<LRESULT>(msg == WM_CTLCOLORSTATIC ? app->bgBrush : app->fieldBrush);
    }
    case WM_COMMAND:
        if (!app) return 0;
        if (LOWORD(wp) == IdHierarchy && HIWORD(wp) == LBN_SELCHANGE) {
            const int index = static_cast<int>(SendMessageA(app->hierarchy, LB_GETCURSEL, 0, 0));
            if (index != LB_ERR && index < static_cast<int>(app->hierarchyIds.size())) SelectEntity(*app, app->hierarchyIds[static_cast<size_t>(index)]);
            return 0;
        }
        if (LOWORD(wp) == IdAssets && (HIWORD(wp) == LBN_SELCHANGE || HIWORD(wp) == LBN_DBLCLK)) {
            const int index = static_cast<int>(SendMessageA(app->assets, LB_GETCURSEL, 0, 0));
            OpenAsset(*app, index);
            return 0;
        }
        HandleCommand(hwnd, *app, LOWORD(wp));
        return 0;
    case WM_DESTROY:
        if (app) {
            DeleteObject(app->uiFont);
            DeleteObject(app->strongFont);
            DeleteObject(app->monoFont);
            DeleteObject(app->bgBrush);
            DeleteObject(app->panelBrush);
            DeleteObject(app->fieldBrush);
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    LoadLibraryA("Riched20.dll");

    EditorApp app;
    app.engineRoot = FindProjectRoot();
    app.projectRoot = DefaultExampleProject(app.engineRoot);
    app.editor = faahhder::EditorModel(app.projectRoot);
    EnsureExampleFiles(app);
    EnsureStarterScene(app);

    WNDCLASSA wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "FaahhderEditorWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(ColorBg);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Faahhder",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1480,
        900,
        nullptr,
        nullptr,
        instance,
        &app);
    if (!hwnd) return 1;

    ACCEL accelerators[] = {
        {FVIRTKEY | FCONTROL, 'S', IdSaveCode},
        {FVIRTKEY | FCONTROL, 'N', IdNewProject},
        {FVIRTKEY, VK_F5, IdPlay},
        {FVIRTKEY | FSHIFT, VK_F5, IdStop}
    };
    HACCEL accel = CreateAcceleratorTableA(accelerators, static_cast<int>(std::size(accelerators)));

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        if (!TranslateAcceleratorA(hwnd, accel, &message)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    DestroyAcceleratorTable(accel);
    return 0;
}
