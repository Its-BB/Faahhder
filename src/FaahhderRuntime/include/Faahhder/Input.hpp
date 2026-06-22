#pragma once

#include "Faahhder/Math.hpp"

#include <string>
#include <unordered_set>

namespace faahhder {

class Input {
public:
    static void BeginFrame();
    static void SetKeyDown(const std::string& key, bool down);
    static void SetMouseButtonDown(int button, bool down);
    static void SetMousePosition(Vec2 position);

    static bool IsKeyDown(const std::string& key);
    static bool WasKeyPressed(const std::string& key);
    static bool IsMouseButtonDown(int button);
    static Vec2 GetMousePosition();
    static void Reset();

private:
    static std::unordered_set<std::string>& Keys();
    static std::unordered_set<std::string>& PreviousKeys();
    static std::unordered_set<int>& MouseButtons();
    static Vec2& MousePositionStorage();
};

}

