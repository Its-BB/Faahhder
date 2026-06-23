#include "Faahhder/Faahhder.hpp"

#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr COLORREF Bg = RGB(18, 20, 24);
constexpr COLORREF Panel2 = RGB(35, 39, 48);
constexpr COLORREF Text = RGB(232, 237, 243);

enum ControlId {
    IdHierarchy = 100,
    IdAssets,
    IdInspector,
    IdCode,
    IdConsole,
    IdStatus,
    IdAddEntity,
    IdPlay,
    IdPause,
    IdStop,
    IdSaveScene,
    IdNewProject,
    IdLoadSample,
    IdNewScript,
    IdSaveCode,
    IdRefresh
};

struct EditorApp {
    std::filesystem::path engineRoot = ".";
    std::filesystem::path projectRoot = ".";
    faahhder::EditorModel editor{"."};
    faahhder::EntityId selectedEntity = faahhder::InvalidEntity;
    std::filesystem::path currentFile = "assets/project.faahhder";
    std::vector<faahhder::EntityId> hierarchyIds;

    HFONT uiFont = nullptr;
    HFONT monoFont = nullptr;
    HBRUSH bgBrush = nullptr;
    HBRUSH editBrush = nullptr;

    HWND title = nullptr;
    HWND hierarchyLabel = nullptr;
    HWND assetsLabel = nullptr;
    HWND inspectorLabel = nullptr;
    HWND codeLabel = nullptr;
    HWND consoleLabel = nullptr;
    HWND status = nullptr;
    HWND hierarchy = nullptr;
    HWND assets = nullptr;
    HWND inspector = nullptr;
    HWND code = nullptr;
    HWND console = nullptr;
};

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) return {};
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}

std::string Trim(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.pop_back();
    return value;
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
    if (value.empty()) value = "FaahhderProject";
    for (char& c : value) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) c = '_';
    }
    return value;
}

bool IsProjectRoot(const std::filesystem::path& path) {
    return std::filesystem::exists(path / "CMakeLists.txt") &&
           std::filesystem::exists(path / "src/FaahhderRuntime");
}

std::filesystem::path WalkForProjectRoot(std::filesystem::path start) {
    start = std::filesystem::absolute(start);
    if (std::filesystem::is_regular_file(start)) {
        start = start.parent_path();
    }

    for (auto cursor = start; !cursor.empty(); cursor = cursor.parent_path()) {
        if (IsProjectRoot(cursor)) {
            return cursor;
        }
        if (cursor == cursor.parent_path()) {
            break;
        }
    }
    return std::filesystem::absolute(".");
}

std::filesystem::path FindProjectRoot() {
    if (IsProjectRoot(std::filesystem::absolute("."))) {
        return std::filesystem::absolute(".");
    }

    char modulePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return WalkForProjectRoot(modulePath);
}

std::filesystem::path DefaultExampleProject(const std::filesystem::path& engineRoot) {
    const auto starter = engineRoot / "examples/starter";
    if (std::filesystem::exists(starter / "assets/project.faahhder")) {
        return starter;
    }
    return engineRoot;
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& text) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    if (!file) return false;
    file << text;
    return true;
}

std::string WindowText(HWND hwnd) {
    const int length = GetWindowTextLengthA(hwnd);
    std::string text(static_cast<size_t>(length), '\0');
    if (length > 0) {
        GetWindowTextA(hwnd, text.data(), length + 1);
    }
    return text;
}

void Set(HWND hwnd, const std::string& text) {
    SetWindowTextA(hwnd, text.c_str());
}

void Status(EditorApp& app, const std::string& text) {
    Set(app.status, "  " + text);
}

void AppendConsole(EditorApp& app, const std::string& message) {
    Set(app.console, WindowText(app.console) + message + "\r\n");
    Status(app, message);
}

HWND Control(HWND parent, const char* cls, const char* text, DWORD style, int id) {
    return CreateWindowExA(
        WS_EX_CLIENTEDGE,
        cls,
        text,
        WS_CHILD | WS_VISIBLE | style,
        0, 0, 100, 100,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleA(nullptr),
        nullptr);
}

HWND Label(HWND parent, const char* text) {
    return CreateWindowExA(
        0,
        "STATIC",
        text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 100, 22,
        parent,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr);
}

HWND Button(HWND parent, const char* text, int id) {
    return CreateWindowExA(
        0,
        "BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 90, 28,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleA(nullptr),
        nullptr);
}

void ApplyFont(HWND hwnd, HFONT font) {
    SendMessageA(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void EnsureExampleFiles(EditorApp& app) {
    std::filesystem::create_directories(app.projectRoot / "assets/scripts");
    std::filesystem::create_directories(app.projectRoot / "assets/scenes");
    std::filesystem::create_directories(app.projectRoot / "assets/textures");

    const auto projectFile = app.projectRoot / "project.faahhder";
    if (!std::filesystem::exists(projectFile)) {
        WriteTextFile(projectFile,
            "name=New Game\n"
            "runtime=EditorOnly\n"
            "entry=assets/project.faahhder\n");
    }

    const auto starterConfig = app.projectRoot / "assets/project.faahhder";
    if (!std::filesystem::exists(starterConfig)) {
        WriteTextFile(starterConfig,
            "title=Faahhder Starter\n"
            "width=960\n"
            "height=540\n"
            "clear=12,14,18\n"
            "scene=assets/scenes/sample_scene.faahhder.json\n"
            "script=assets/scripts/starter.logic\n"
            "primary=80,210,145\n"
            "accent=255,90,105\n");
    }

    const auto starterLogic = app.projectRoot / "assets/scripts/starter.logic";
    if (!std::filesystem::exists(starterLogic)) {
        WriteTextFile(starterLogic,
            "name=Faahhder Starter\n"
            "controls=Arrow keys or WASD\n"
            "action=Space\n"
            "goal=Move the player, edit this file, and save changes from the editor.\n"
            "on_update=read_input\n"
            "on_action=log_message\n");
    }

    const auto script = app.projectRoot / "assets/scripts/player.lua";
    if (!std::filesystem::exists(script)) {
        WriteTextFile(script,
            "function OnCreate(entity)\n"
            "    Log.Info(\"Player ready\")\n"
            "end\n\n"
            "function OnUpdate(entity, dt)\n"
            "    local speed = 120\n"
            "    if Input.IsKeyDown(\"A\") then\n"
            "        Entity.SetPosition(entity, Entity.GetX(entity) - speed * dt, Entity.GetY(entity))\n"
            "    end\n"
            "    if Input.IsKeyDown(\"D\") then\n"
            "        Entity.SetPosition(entity, Entity.GetX(entity) + speed * dt, Entity.GetY(entity))\n"
            "    end\n"
            "end\n");
    }
}

void EnsureStarterScene(EditorApp& app) {
    auto& scene = app.editor.GetScene();
    if (scene.FindEntityByName("Player") != faahhder::InvalidEntity) {
        return;
    }

    const auto camera = scene.FindEntityByName("Main Camera");
    if (camera != faahhder::InvalidEntity) {
        if (auto* transform = scene.GetTransform(camera)) transform->position = {0.0f, 0.0f};
    }

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
    app.editor.Log("Example project loaded: Player, Example Light, Ground");
}

std::string Inspector(EditorApp& app, faahhder::EntityId entity) {
    std::ostringstream out;
    for (const auto& row : app.editor.BuildInspectorPanel(entity)) {
        out << row << "\r\n";
    }
    if (const auto* transform = app.editor.GetScene().GetTransform(entity)) {
        out << "\r\nTransform\r\n";
        out << "  Position: " << transform->position.x << ", " << transform->position.y << "\r\n";
        out << "  Rotation: " << transform->rotation << "\r\n";
        out << "  Scale: " << transform->scale.x << ", " << transform->scale.y << "\r\n";
    }
    if (const auto* script = app.editor.GetScene().GetLuaScript(entity)) {
        out << "\r\nScript\r\n";
        out << "  File: " << script->path.generic_string() << "\r\n";
        out << "  Edit it in the center Code Editor panel.\r\n";
    }
    return out.str();
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
    if (std::filesystem::exists(app.projectRoot / "project.faahhder")) {
        SendMessageA(app.assets, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>("project.faahhder"));
    }
    for (const auto& asset : app.editor.BuildAssetBrowser()) {
        const auto relative = std::filesystem::relative(asset, app.projectRoot).generic_string();
        SendMessageA(app.assets, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(relative.c_str()));
    }
}

void SelectEntity(EditorApp& app, faahhder::EntityId entity) {
    app.selectedEntity = entity;
    Set(app.inspector, Inspector(app, entity));
    if (const auto* script = app.editor.GetScene().GetLuaScript(entity)) {
        app.currentFile = script->path;
    }
    Set(app.code, ReadTextFile(app.projectRoot / app.currentFile));
    Status(app, "Editing code: " + app.currentFile.generic_string() + "  |  Double-click assets to open files.");
}

void RefreshConsole(EditorApp& app) {
    std::ostringstream out;
    for (const auto& row : app.editor.BuildConsolePanel()) {
        out << row << "\r\n";
    }
    Set(app.console, out.str());
}

void RefreshAll(EditorApp& app) {
    RefreshHierarchy(app);
    RefreshAssets(app);
    RefreshConsole(app);
    SelectEntity(app, app.selectedEntity);
}

void CreateProject(EditorApp& app) {
    const auto projectName = "Game_" + std::to_string(GetTickCount());
    app.projectRoot = app.engineRoot / "projects" / projectName;
    app.editor = faahhder::EditorModel(app.projectRoot);
    app.selectedEntity = faahhder::InvalidEntity;
    app.currentFile = "assets/project.faahhder";
    EnsureExampleFiles(app);
    EnsureStarterScene(app);
    RefreshAll(app);
    AppendConsole(app, "Created project: " + app.projectRoot.string());
}

void OpenAsset(EditorApp& app) {
    const int index = static_cast<int>(SendMessageA(app.assets, LB_GETCURSEL, 0, 0));
    if (index == LB_ERR) return;

    char buffer[MAX_PATH]{};
    SendMessageA(app.assets, LB_GETTEXT, index, reinterpret_cast<LPARAM>(buffer));
    const std::filesystem::path asset = buffer;
    const auto ext = asset.extension().string();
    if (ext == ".lua" || ext == ".logic" || ext == ".json" || ext == ".faahhder" || ext == ".txt" || ext == ".md") {
        app.currentFile = asset;
        Set(app.code, ReadTextFile(app.projectRoot / asset));
        AppendConsole(app, "Opened " + asset.generic_string() + " in Code Editor");
    } else {
        AppendConsole(app, "Asset selected: " + asset.generic_string());
    }
}

void Layout(HWND hwnd, EditorApp& app) {
    RECT r{};
    GetClientRect(hwnd, &r);
    const int w = r.right - r.left;
    const int h = r.bottom - r.top;
    const int gap = 12;
    const int toolbar = 54;
    const int status = 28;
    const int left = 260;
    const int right = 330;
    const int bottom = 150;
    const int label = 24;
    const int contentTop = toolbar + gap;
    const int contentBottom = h - bottom - status - gap;
    const int sideHeight = contentBottom - contentTop;

    MoveWindow(app.title, gap, 10, 250, 28, TRUE);

    int x = 275;
    const int y = 12;
    const int bw = 78;
    for (int id : {IdAddEntity, IdPlay, IdPause, IdStop, IdSaveScene, IdNewProject, IdLoadSample, IdNewScript, IdSaveCode, IdRefresh}) {
        MoveWindow(GetDlgItem(hwnd, id), x, y, bw, 28, TRUE);
        x += bw + 6;
    }

    MoveWindow(app.hierarchyLabel, gap, contentTop, left - gap * 2, label, TRUE);
    MoveWindow(app.hierarchy, gap, contentTop + label, left - gap * 2, sideHeight / 2 - label - gap / 2, TRUE);
    MoveWindow(app.assetsLabel, gap, contentTop + sideHeight / 2 + gap / 2, left - gap * 2, label, TRUE);
    MoveWindow(app.assets, gap, contentTop + sideHeight / 2 + label + gap / 2, left - gap * 2, sideHeight / 2 - label - gap / 2, TRUE);

    MoveWindow(app.codeLabel, left + gap, contentTop, w - left - right - gap * 2, label, TRUE);
    MoveWindow(app.code, left + gap, contentTop + label, w - left - right - gap * 2, sideHeight - label, TRUE);

    MoveWindow(app.inspectorLabel, w - right + gap, contentTop, right - gap * 2, label, TRUE);
    MoveWindow(app.inspector, w - right + gap, contentTop + label, right - gap * 2, sideHeight - label, TRUE);

    MoveWindow(app.consoleLabel, gap, h - bottom - status, w - gap * 2, label, TRUE);
    MoveWindow(app.console, gap, h - bottom - status + label, w - gap * 2, bottom - label, TRUE);
    MoveWindow(app.status, 0, h - status, w, status, TRUE);
}

void PaintBackground(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT r{};
    GetClientRect(hwnd, &r);
    HBRUSH brush = CreateSolidBrush(Bg);
    FillRect(dc, &r, brush);
    DeleteObject(brush);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* app = reinterpret_cast<EditorApp*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lp);
        app = reinterpret_cast<EditorApp*>(create->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

        app->uiFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        app->monoFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
        app->bgBrush = CreateSolidBrush(Bg);
        app->editBrush = CreateSolidBrush(Panel2);

        app->title = Label(hwnd, "Faahhder Editor");
        app->hierarchyLabel = Label(hwnd, "Hierarchy");
        app->assetsLabel = Label(hwnd, "Assets  (double-click to open code)");
        app->codeLabel = Label(hwnd, "Code Editor  - write Lua/JSON here, then click Save Code");
        app->inspectorLabel = Label(hwnd, "Inspector");
        app->consoleLabel = Label(hwnd, "Console");
        app->status = Label(hwnd, "  Ready");

        for (HWND labelHwnd : {app->title, app->hierarchyLabel, app->assetsLabel, app->codeLabel, app->inspectorLabel, app->consoleLabel, app->status}) {
            ApplyFont(labelHwnd, app->uiFont);
        }

        Button(hwnd, "+ Entity", IdAddEntity);
        Button(hwnd, "Play", IdPlay);
        Button(hwnd, "Pause", IdPause);
        Button(hwnd, "Stop", IdStop);
        Button(hwnd, "Save Scene", IdSaveScene);
        Button(hwnd, "New Proj", IdNewProject);
        Button(hwnd, "Load Scene", IdLoadSample);
        Button(hwnd, "New Script", IdNewScript);
        Button(hwnd, "Save Code", IdSaveCode);
        Button(hwnd, "Refresh", IdRefresh);

        app->hierarchy = Control(hwnd, "LISTBOX", "", LBS_NOTIFY | WS_VSCROLL, IdHierarchy);
        app->assets = Control(hwnd, "LISTBOX", "", LBS_NOTIFY | WS_VSCROLL, IdAssets);
        app->inspector = Control(hwnd, "EDIT", "", ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, IdInspector);
        app->code = Control(hwnd, "EDIT", "", ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL, IdCode);
        app->console = Control(hwnd, "EDIT", "", ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, IdConsole);

        for (HWND control : {app->hierarchy, app->assets, app->inspector}) ApplyFont(control, app->uiFont);
        ApplyFont(app->code, app->monoFont);
        ApplyFont(app->console, app->monoFont);

        RefreshAll(*app);
        Layout(hwnd, *app);
        AppendConsole(*app, "Edit project files in the Code Editor, then click Save Code.");
        return 0;
    }
    case WM_PAINT:
        PaintBackground(hwnd);
        return 0;
    case WM_SIZE:
        if (app) Layout(hwnd, *app);
        return 0;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, Text);
        SetBkColor(dc, msg == WM_CTLCOLORSTATIC ? Bg : Panel2);
        return reinterpret_cast<LRESULT>(msg == WM_CTLCOLORSTATIC ? app->bgBrush : app->editBrush);
    }
    case WM_COMMAND:
        if (!app) return 0;
        if (LOWORD(wp) == IdHierarchy && HIWORD(wp) == LBN_SELCHANGE) {
            const int index = static_cast<int>(SendMessageA(app->hierarchy, LB_GETCURSEL, 0, 0));
            if (index != LB_ERR && index < static_cast<int>(app->hierarchyIds.size())) {
                SelectEntity(*app, app->hierarchyIds[static_cast<size_t>(index)]);
            }
            return 0;
        }
        if (LOWORD(wp) == IdAssets && HIWORD(wp) == LBN_DBLCLK) {
            OpenAsset(*app);
            return 0;
        }

        switch (LOWORD(wp)) {
        case IdAddEntity: {
            const auto entity = app->editor.GetScene().CreateEntity("Entity");
            app->editor.Log("Created Entity " + std::to_string(entity));
            app->selectedEntity = entity;
            RefreshAll(*app);
            break;
        }
        case IdPlay:
            app->editor.Play();
            AppendConsole(*app, "Play mode started");
            break;
        case IdPause:
            app->editor.Pause();
            AppendConsole(*app, "Play mode paused");
            break;
        case IdStop:
            app->editor.Stop();
            RefreshAll(*app);
            AppendConsole(*app, "Play mode stopped");
            break;
        case IdSaveScene:
            if (app->editor.SaveScene(app->projectRoot / "assets/scenes/editor_scene.faahhder.json")) {
                AppendConsole(*app, "Saved assets/scenes/editor_scene.faahhder.json");
            }
            break;
        case IdNewProject:
            CreateProject(*app);
            break;
        case IdLoadSample:
            app->editor.LoadScene(app->projectRoot / "assets/scenes/sample_scene.faahhder.json");
            app->selectedEntity = app->editor.GetScene().FindEntityByName("Player");
            RefreshAll(*app);
            AppendConsole(*app, "Loaded example scene");
            break;
        case IdNewScript:
            app->currentFile = "assets/scripts/starter_custom.logic";
            Set(app->code,
                "name=Faahhder Starter\r\n"
                "controls=Arrow keys or WASD\r\n"
                "action=Space\r\n"
                "goal=Move the player, edit this file, and save changes from the editor.\r\n"
                "on_update=read_input\r\n"
                "on_action=log_message\r\n");
            AppendConsole(*app, "Created new editable logic buffer: assets/scripts/starter_custom.logic");
            break;
        case IdSaveCode:
            if (WriteTextFile(app->projectRoot / app->currentFile, WindowText(app->code))) {
                AppendConsole(*app, "Saved code: " + app->currentFile.generic_string());
                RefreshAssets(*app);
            } else {
                AppendConsole(*app, "Failed to save code");
            }
            break;
        case IdRefresh:
            RefreshAll(*app);
            AppendConsole(*app, "Refreshed");
            break;
        default:
            break;
        }
        return 0;
    case WM_DESTROY:
        if (app) {
            DeleteObject(app->uiFont);
            DeleteObject(app->monoFont);
            DeleteObject(app->bgBrush);
            DeleteObject(app->editBrush);
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
}

}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
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
    wc.hbrBackground = CreateSolidBrush(Bg);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Faahhder Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1420,
        860,
        nullptr,
        nullptr,
        instance,
        &app);

    if (!hwnd) return 1;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return 0;
}
