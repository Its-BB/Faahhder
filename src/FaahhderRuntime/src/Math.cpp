#include "Faahhder/Math.hpp"

#include <sstream>

namespace faahhder {

Vec2 operator+(Vec2 lhs, Vec2 rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

Vec2 operator-(Vec2 lhs, Vec2 rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

Vec2 operator*(Vec2 lhs, float rhs) {
    return {lhs.x * rhs, lhs.y * rhs};
}

bool NearlyEqual(float lhs, float rhs, float epsilon) {
    return std::fabs(lhs - rhs) <= epsilon;
}

float Length(Vec2 value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

float Dot(Vec2 lhs, Vec2 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

std::string ToString(Vec2 value) {
    std::ostringstream out;
    out << "(" << value.x << ", " << value.y << ")";
    return out.str();
}

}

