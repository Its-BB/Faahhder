#include "Faahhder/Faahhder.hpp"

#define NOMINMAX
#include <windows.h>

#include <deque>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

constexpr int BoardX = 36;
constexpr int BoardY = 78;
constexpr COLORREF TextColor = RGB(230, 236, 245);
constexpr COLORREF MutedText = RGB(145, 154, 168);

struct CellPos {
    int x = 0;
    int y = 0;
};

bool operator==(CellPos a, CellPos b) {
    return a.x == b.x && a.y == b.y;
}

struct SnakeProject {
    std::string title = "Faahhder Snake";
    std::string controls = "Arrow keys or WASD";
    std::string restart = "Space";
    std::string goal = "Eat food, grow longer, avoid walls and your own body.";
    int columns = 28;
    int rows = 20;
    int cell = 24;
    int startLength = 3;
    int foodScore = 10;
    int baseSpeedMs = 130;
    int midSpeedMs = 105;
    int fastSpeedMs = 80;
    int midScore = 60;
    int fastScore = 120;
    bool wrap = false;
    COLORREF bg = RGB(12, 14, 18);
    COLORREF panel = RGB(22, 26, 34);
    COLORREF grid = RGB(34, 40, 51);
    COLORREF snakeHead = RGB(80, 210, 145);
    COLORREF snakeBody = RGB(42, 155, 105);
    COLORREF food = RGB(255, 90, 105);
};

struct GameApp {
    std::filesystem::path projectRoot;
    SnakeProject project;
    std::deque<CellPos> snake;
    CellPos food;
    CellPos dir{1, 0};
    CellPos queuedDir{1, 0};
    int score = 0;
    int best = 0;
    bool gameOver = false;
    DWORD lastStep = 0;
    HFONT titleFont = nullptr;
    HFONT uiFont = nullptr;
    std::mt19937 rng{std::random_device{}()};
};

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

int IntValue(const std::unordered_map<std::string, std::string>& values, const std::string& key, int fallback) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : std::stoi(it->second);
}

bool BoolValue(const std::unordered_map<std::string, std::string>& values, const std::string& key, bool fallback) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second == "true" || it->second == "1" || it->second == "yes";
}

std::string StringValue(const std::unordered_map<std::string, std::string>& values, const std::string& key, const std::string& fallback) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

COLORREF ColorValue(const std::unordered_map<std::string, std::string>& values, const std::string& key, COLORREF fallback) {
    const auto it = values.find(key);
    if (it == values.end()) return fallback;
    std::stringstream stream(it->second);
    std::string part;
    int rgb[3]{0, 0, 0};
    for (int i = 0; i < 3 && std::getline(stream, part, ','); ++i) rgb[i] = std::stoi(Trim(part));
    return RGB(rgb[0], rgb[1], rgb[2]);
}

std::filesystem::path FindProjectRoot() {
    auto cursor = std::filesystem::absolute(".");
    for (;;) {
        if (std::filesystem::exists(cursor / "assets/snake.faahhder")) return cursor;
        if (cursor == cursor.parent_path()) break;
        cursor = cursor.parent_path();
    }

    char modulePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    cursor = std::filesystem::absolute(modulePath).parent_path();
    for (;;) {
        if (std::filesystem::exists(cursor / "assets/snake.faahhder")) return cursor;
        if (cursor == cursor.parent_path()) break;
        cursor = cursor.parent_path();
    }
    return std::filesystem::absolute(".");
}

std::filesystem::path ProjectRootFromCommandLine(LPSTR commandLine) {
    std::string text = commandLine ? commandLine : "";
    while (!text.empty() && text.front() == ' ') text.erase(text.begin());
    while (!text.empty() && text.back() == ' ') text.pop_back();
    if (text.empty()) return {};
    if (text.front() == '"' && text.back() == '"' && text.size() >= 2) text = text.substr(1, text.size() - 2);
    const auto path = std::filesystem::absolute(text);
    if (std::filesystem::exists(path / "assets/snake.faahhder")) return path;
    return {};
}

void LoadProject(GameApp& app) {
    const auto root = app.projectRoot.empty() ? FindProjectRoot() : app.projectRoot;
    const auto config = ReadPairs(root / "assets/snake.faahhder");
    const auto logic = ReadPairs(root / "assets/scripts/snake.logic");

    app.project.title = StringValue(config, "title", app.project.title);
    app.project.columns = IntValue(config, "columns", app.project.columns);
    app.project.rows = IntValue(config, "rows", app.project.rows);
    app.project.cell = IntValue(config, "cell", app.project.cell);
    app.project.startLength = IntValue(config, "start_length", app.project.startLength);
    app.project.foodScore = IntValue(config, "food_score", app.project.foodScore);
    app.project.baseSpeedMs = IntValue(config, "base_speed_ms", app.project.baseSpeedMs);
    app.project.midSpeedMs = IntValue(config, "mid_speed_ms", app.project.midSpeedMs);
    app.project.fastSpeedMs = IntValue(config, "fast_speed_ms", app.project.fastSpeedMs);
    app.project.midScore = IntValue(config, "mid_score", app.project.midScore);
    app.project.fastScore = IntValue(config, "fast_score", app.project.fastScore);
    app.project.wrap = BoolValue(config, "wrap", app.project.wrap);
    app.project.bg = ColorValue(config, "bg", app.project.bg);
    app.project.panel = ColorValue(config, "panel", app.project.panel);
    app.project.grid = ColorValue(config, "grid", app.project.grid);
    app.project.snakeHead = ColorValue(config, "snake_head", app.project.snakeHead);
    app.project.snakeBody = ColorValue(config, "snake_body", app.project.snakeBody);
    app.project.food = ColorValue(config, "food", app.project.food);
    app.project.title = StringValue(logic, "name", app.project.title);
    app.project.controls = StringValue(logic, "controls", app.project.controls);
    app.project.restart = StringValue(logic, "restart", app.project.restart);
    app.project.goal = StringValue(logic, "goal", app.project.goal);
}

void DrawRect(HDC dc, int x, int y, int w, int h, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    RECT rect{x, y, x + w, y + h};
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void DrawTextAt(HDC dc, HFONT font, int x, int y, const std::string& text, COLORREF color = TextColor) {
    SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, x, y, text.c_str(), static_cast<int>(text.size()));
}

bool Contains(const std::deque<CellPos>& cells, CellPos value) {
    for (auto cell : cells) if (cell == value) return true;
    return false;
}

CellPos RandomFood(GameApp& app) {
    std::uniform_int_distribution<int> xDist(0, app.project.columns - 1);
    std::uniform_int_distribution<int> yDist(0, app.project.rows - 1);
    CellPos pos;
    do {
        pos = {xDist(app.rng), yDist(app.rng)};
    } while (Contains(app.snake, pos));
    return pos;
}

void Reset(GameApp& app) {
    LoadProject(app);
    app.snake.clear();
    const int startX = app.project.columns / 2;
    const int startY = app.project.rows / 2;
    for (int i = 0; i < app.project.startLength; ++i) app.snake.push_back({startX - i, startY});
    app.dir = {1, 0};
    app.queuedDir = {1, 0};
    app.score = 0;
    app.gameOver = false;
    app.lastStep = GetTickCount();
    app.food = RandomFood(app);
}

bool Opposite(CellPos a, CellPos b) {
    return a.x + b.x == 0 && a.y + b.y == 0;
}

void Step(GameApp& app) {
    if (app.gameOver) return;
    if (!Opposite(app.dir, app.queuedDir)) app.dir = app.queuedDir;

    CellPos next = app.snake.front();
    next.x += app.dir.x;
    next.y += app.dir.y;

    if (app.project.wrap) {
        if (next.x < 0) next.x = app.project.columns - 1;
        if (next.x >= app.project.columns) next.x = 0;
        if (next.y < 0) next.y = app.project.rows - 1;
        if (next.y >= app.project.rows) next.y = 0;
    }

    if (next.x < 0 || next.x >= app.project.columns || next.y < 0 || next.y >= app.project.rows || Contains(app.snake, next)) {
        app.gameOver = true;
        app.best = std::max(app.best, app.score);
        return;
    }

    app.snake.push_front(next);
    if (next == app.food) {
        app.score += app.project.foodScore;
        app.food = RandomFood(app);
    } else {
        app.snake.pop_back();
    }
}

void Update(GameApp& app, HWND hwnd) {
    const DWORD now = GetTickCount();
    const DWORD interval = app.score >= app.project.fastScore ? app.project.fastSpeedMs : app.score >= app.project.midScore ? app.project.midSpeedMs : app.project.baseSpeedMs;
    if (now - app.lastStep >= interval) {
        app.lastStep = now;
        Step(app);
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void Render(GameApp& app, HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);
    DrawRect(dc, 0, 0, client.right, client.bottom, app.project.bg);
    DrawTextAt(dc, app.titleFont, BoardX, 22, app.project.title);
    DrawTextAt(dc, app.uiFont, BoardX + 280, 30, "Score " + std::to_string(app.score) + "    Best " + std::to_string(app.best), MutedText);
    DrawRect(dc, BoardX - 8, BoardY - 8, app.project.columns * app.project.cell + 16, app.project.rows * app.project.cell + 16, app.project.panel);

    for (int y = 0; y < app.project.rows; ++y) {
        for (int x = 0; x < app.project.columns; ++x) {
            DrawRect(dc, BoardX + x * app.project.cell, BoardY + y * app.project.cell, app.project.cell - 2, app.project.cell - 2, app.project.grid);
        }
    }

    DrawRect(dc, BoardX + app.food.x * app.project.cell + 3, BoardY + app.food.y * app.project.cell + 3, app.project.cell - 8, app.project.cell - 8, app.project.food);

    bool head = true;
    for (auto cell : app.snake) {
        DrawRect(dc, BoardX + cell.x * app.project.cell + 2, BoardY + cell.y * app.project.cell + 2, app.project.cell - 6, app.project.cell - 6, head ? app.project.snakeHead : app.project.snakeBody);
        head = false;
    }

    const int footerY = BoardY + app.project.rows * app.project.cell + 26;
    DrawTextAt(dc, app.uiFont, BoardX, footerY, app.project.controls + " to move. " + app.project.restart + " restarts.");
    DrawTextAt(dc, app.uiFont, BoardX, footerY + 24, app.project.goal, MutedText);

    if (app.gameOver) {
        const int overlayX = BoardX + (app.project.columns * app.project.cell - 370) / 2;
        const int overlayY = BoardY + (app.project.rows * app.project.cell - 120) / 2;
        DrawRect(dc, overlayX, overlayY, 370, 120, RGB(18, 22, 29));
        DrawTextAt(dc, app.titleFont, overlayX + 92, overlayY + 22, "Game Over");
        DrawTextAt(dc, app.uiFont, overlayX + 76, overlayY + 72, "Press " + app.project.restart + " to restart", MutedText);
    }

    EndPaint(hwnd, &ps);
}

void QueueDirection(GameApp& app, CellPos dir) {
    if (!Opposite(app.dir, dir)) app.queuedDir = dir;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* app = reinterpret_cast<GameApp*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_CREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lp);
        app = reinterpret_cast<GameApp*>(create->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->titleFont = CreateFontA(30, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        app->uiFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        Reset(*app);
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }
    case WM_KEYDOWN:
        if (!app) return 0;
        if (wp == VK_LEFT || wp == 'A') QueueDirection(*app, {-1, 0});
        if (wp == VK_RIGHT || wp == 'D') QueueDirection(*app, {1, 0});
        if (wp == VK_UP || wp == 'W') QueueDirection(*app, {0, -1});
        if (wp == VK_DOWN || wp == 'S') QueueDirection(*app, {0, 1});
        if (wp == VK_SPACE && app->gameOver) Reset(*app);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_TIMER:
        if (app) Update(*app, hwnd);
        return 0;
    case WM_PAINT:
        if (app) Render(*app, hwnd);
        return 0;
    case WM_DESTROY:
        if (app) {
            DeleteObject(app->titleFont);
            DeleteObject(app->uiFont);
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
}

}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR commandLine, int show) {
    GameApp app;
    app.projectRoot = ProjectRootFromCommandLine(commandLine);
    LoadProject(app);

    WNDCLASSA wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "FaahhderSnakeWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(app.project.bg);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        app.project.title.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        BoardX * 2 + app.project.columns * app.project.cell + 16,
        BoardY + app.project.rows * app.project.cell + 76,
        nullptr,
        nullptr,
        instance,
        &app);

    if (!hwnd) return 1;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}
