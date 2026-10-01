#pragma once

#include <cmath>
#include <cstddef>

struct Vector2 {
    float x{};
    float y{};
};

struct Vector {
    float x{};
    float y{};
    float z{};

    constexpr Vector() noexcept = default;
    constexpr Vector(float xValue, float yValue, float zValue) noexcept
        : x(xValue), y(yValue), z(zValue) {}

    [[nodiscard]] constexpr Vector operator+(const Vector& other) const noexcept {
        return {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] constexpr Vector operator-(const Vector& other) const noexcept {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] constexpr Vector operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar};
    }

    [[nodiscard]] constexpr Vector operator/(float scalar) const noexcept {
        return {x / scalar, y / scalar, z / scalar};
    }

    constexpr Vector& operator+=(const Vector& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    [[nodiscard]] constexpr float Dot(const Vector& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr float LengthSquared() const noexcept {
        return Dot(*this);
    }

    [[nodiscard]] float Length() const noexcept {
        return std::sqrt(LengthSquared());
    }
};

[[nodiscard]] constexpr Vector operator*(float scalar, const Vector& vector) noexcept {
    return vector * scalar;
}

struct Vector4 {
    float x{};
    float y{};
    float z{};
    float w{};
};

struct Matrix4x4 {
    float values[4][4]{};

    [[nodiscard]] constexpr const float* operator[](std::size_t row) const noexcept {
        return values[row];
    }

    [[nodiscard]] constexpr float* operator[](std::size_t row) noexcept {
        return values[row];
    }

    [[nodiscard]] constexpr Vector4 TransformPoint(const Vector& point) const noexcept {
        return {
            values[0][0] * point.x + values[0][1] * point.y +
                values[0][2] * point.z + values[0][3],
            values[1][0] * point.x + values[1][1] * point.y +
                values[1][2] * point.z + values[1][3],
            values[2][0] * point.x + values[2][1] * point.y +
                values[2][2] * point.z + values[2][3],
            values[3][0] * point.x + values[3][1] * point.y +
                values[3][2] * point.z + values[3][3]
        };
    }
};

static_assert(sizeof(Vector) == sizeof(float) * 3, "Vector must contain exactly three floats");
static_assert(sizeof(Matrix4x4) == sizeof(float) * 16,
              "Matrix4x4 must contain exactly sixteen floats");
