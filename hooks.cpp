#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "hooks.h"

#include "engine.h"
#include "esp.h"

#include <atomic>
#include <cstddef>
#include <cstring>

namespace Hooks {
namespace {
constexpr std::size_t kPaintTraverseIndex = 41;
constexpr const char* kOverlayPanelName = "MatSystemTopPanel";

using PaintTraverseFn = void(__thiscall*)(void*, unsigned int, bool, bool);

class VmtHook final {
public:
    VmtHook() = default;
    VmtHook(const VmtHook&) = delete;
    VmtHook& operator=(const VmtHook&) = delete;

    [[nodiscard]] bool Install(void* instance,
                               std::size_t index,
                               void* detour) noexcept {
        if (instance == nullptr || detour == nullptr || slot_ != nullptr) {
            return false;
        }

        void** const table = *static_cast<void***>(instance);
        if (table == nullptr || table[index] == nullptr) {
            return false;
        }

        void** const candidate_slot = &table[index];
        void* const candidate_original = *candidate_slot;

        DWORD old_protection = 0;
        if (!VirtualProtect(candidate_slot, sizeof(void*), PAGE_EXECUTE_READWRITE,
                            &old_protection)) {
            return false;
        }

        InterlockedExchangePointer(
            reinterpret_cast<PVOID volatile*>(candidate_slot), detour);

        DWORD ignored = 0;
        VirtualProtect(candidate_slot, sizeof(void*), old_protection, &ignored);
        FlushInstructionCache(GetCurrentProcess(), candidate_slot, sizeof(void*));

        slot_ = candidate_slot;
        original_ = candidate_original;
        detour_ = detour;
        return true;
    }

    void Remove() noexcept {
        if (slot_ == nullptr) {
            return;
        }

        DWORD old_protection = 0;
        if (VirtualProtect(slot_, sizeof(void*), PAGE_EXECUTE_READWRITE,
                           &old_protection)) {
            if (*slot_ == detour_) {
                InterlockedExchangePointer(
                    reinterpret_cast<PVOID volatile*>(slot_), original_);
            }

            DWORD ignored = 0;
            VirtualProtect(slot_, sizeof(void*), old_protection, &ignored);
            FlushInstructionCache(GetCurrentProcess(), slot_, sizeof(void*));
        }

        slot_ = nullptr;
        original_ = nullptr;
        detour_ = nullptr;
    }

    template <typename Function>
    [[nodiscard]] Function Original() const noexcept {
        return reinterpret_cast<Function>(original_);
    }

private:
    void** slot_ = nullptr;
    void* original_ = nullptr;
    void* detour_ = nullptr;
};

VmtHook g_panel_hook;
PaintTraverseFn g_original_paint_traverse = nullptr;
std::atomic_bool g_overlay_enabled{true};
unsigned int g_overlay_panel = 0;

void __fastcall HookedPaintTraverse(void* this_pointer,
                                    void*,
                                    unsigned int panel,
                                    bool force_repaint,
                                    bool allow_force) noexcept {
    if (g_original_paint_traverse != nullptr) {
        g_original_paint_traverse(
            this_pointer, panel, force_repaint, allow_force);
    }

    if ((GetAsyncKeyState(VK_INSERT) & 1) != 0) {
        g_overlay_enabled.store(!g_overlay_enabled.load(std::memory_order_relaxed),
                                std::memory_order_relaxed);
    }

    if (g_overlay_panel == 0 && Engine::panel != nullptr) {
        const char* const panel_name = Engine::panel->GetName(panel);
        if (panel_name != nullptr &&
            std::strcmp(panel_name, kOverlayPanelName) == 0) {
            g_overlay_panel = panel;
        }
    }

    if (panel == g_overlay_panel &&
        g_overlay_enabled.load(std::memory_order_relaxed)) {
        ESP::Render();
    }
}
} // namespace

bool Initialize() noexcept {
    if (!Engine::IsReady()) {
        return false;
    }

    void** const panel_table = *reinterpret_cast<void***>(Engine::panel);
    if (panel_table == nullptr || panel_table[kPaintTraverseIndex] == nullptr) {
        return false;
    }

    g_original_paint_traverse =
        reinterpret_cast<PaintTraverseFn>(panel_table[kPaintTraverseIndex]);

    if (!g_panel_hook.Install(
            Engine::panel,
            kPaintTraverseIndex,
            reinterpret_cast<void*>(&HookedPaintTraverse))) {
        g_original_paint_traverse = nullptr;
        return false;
    }

    return true;
}

void Shutdown() noexcept {
    g_panel_hook.Remove();
    g_original_paint_traverse = nullptr;
    g_overlay_panel = 0;
}

bool IsOverlayEnabled() noexcept {
    return g_overlay_enabled.load(std::memory_order_relaxed);
}
} // namespace Hooks
