#include "Faahhder/Input.hpp"

namespace faahhder {

void Input::BeginFrame() {
    PreviousKeys() = Keys();
}

void Input::SetKeyDown(const std::string& key, bool down) {
    if (down) {
        Keys().insert(key);
    } else {
        Keys().erase(key);
    }
}

void Input::SetMouseButtonDown(int button, bool down) {
    if (down) {
        MouseButtons().insert(button);
    } else {
        MouseButtons().erase(button);
    }
}

void Input::SetMousePosition(Vec2 position) {
    MousePositionStorage() = position;
}

bool Input::IsKeyDown(const std::string& key) {
    return Keys().count(key) > 0;
}

bool Input::WasKeyPressed(const std::string& key) {
    return Keys().count(key) > 0 && PreviousKeys().count(key) == 0;
}

bool Input::IsMouseButtonDown(int button) {
    return MouseButtons().count(button) > 0;
}

Vec2 Input::GetMousePosition() {
    return MousePositionStorage();
}

void Input::Reset() {
    Keys().clear();
    PreviousKeys().clear();
    MouseButtons().clear();
    MousePositionStorage() = {};
}

std::unordered_set<std::string>& Input::Keys() {
    static std::unordered_set<std::string> keys;
    return keys;
}

std::unordered_set<std::string>& Input::PreviousKeys() {
    static std::unordered_set<std::string> keys;
    return keys;
}

std::unordered_set<int>& Input::MouseButtons() {
    static std::unordered_set<int> buttons;
    return buttons;
}

Vec2& Input::MousePositionStorage() {
    static Vec2 position;
    return position;
}

}

