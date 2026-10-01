#pragma once

namespace Hooks {
[[nodiscard]] bool Initialize() noexcept;
void Shutdown() noexcept;
[[nodiscard]] bool IsOverlayEnabled() noexcept;
} // namespace Hooks
