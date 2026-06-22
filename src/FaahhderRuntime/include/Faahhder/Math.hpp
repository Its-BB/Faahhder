#pragma once

#include <cmath>
#include <string>

namespace faahhder {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct Rect {
    Vec2 min{};
    Vec2 max{};
};

Vec2 operator+(Vec2 lhs, Vec2 rhs);
Vec2 operator-(Vec2 lhs, Vec2 rhs);
Vec2 operator*(Vec2 lhs, float rhs);
bool NearlyEqual(float lhs, float rhs, float epsilon = 0.0001f);
float Length(Vec2 value);
float Dot(Vec2 lhs, Vec2 rhs);
std::string ToString(Vec2 value);

}

