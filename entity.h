#pragma once

#include "vector.h"

#include <cstddef>
#include <cstdint>

namespace EntityOffsets {
inline constexpr std::ptrdiff_t kHealth = 0x90;
inline constexpr std::ptrdiff_t kTeam = 0x98;
inline constexpr std::ptrdiff_t kOrigin = 0x134;
inline constexpr std::ptrdiff_t kDormant = 0xE9;
} // namespace EntityOffsets

// Lightweight view over a client entity owned by the Source Engine.
// Instances of this class must never be constructed or deleted by the DLL.
class CBaseEntity {
public:
    [[nodiscard]] Vector GetOrigin() const noexcept {
        return Read<Vector>(EntityOffsets::kOrigin);
    }

    [[nodiscard]] int GetTeam() const noexcept {
        return Read<int>(EntityOffsets::kTeam);
    }

    [[nodiscard]] int GetHealth() const noexcept {
        return Read<int>(EntityOffsets::kHealth);
    }

    [[nodiscard]] bool IsDormant() const noexcept {
        return Read<std::uint8_t>(EntityOffsets::kDormant) != 0;
    }

private:
    template <typename T>
    [[nodiscard]] const T& Read(std::ptrdiff_t offset) const noexcept {
        const auto address = reinterpret_cast<std::uintptr_t>(this) +
                             static_cast<std::uintptr_t>(offset);
        return *reinterpret_cast<const T*>(address);
    }
};
