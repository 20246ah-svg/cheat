#pragma once

#include "vector.h"

namespace ESP {
[[nodiscard]] bool WorldToScreen(const Vector& world, Vector2& screen) noexcept;
void Render() noexcept;
} // namespace ESP
