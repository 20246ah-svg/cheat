#pragma once

#include "entity.h"
#include "vector.h"

#include <cstddef>
#include <cstdint>

namespace VirtualMethod {
template <typename Function>
[[nodiscard]] Function Get(void* instance, std::size_t index) noexcept {
    const auto table = *static_cast<void***>(instance);
    return reinterpret_cast<Function>(table[index]);
}
} // namespace VirtualMethod

class IVEngineClient {
public:
    [[nodiscard]] int GetLocalPlayer() noexcept {
        using Function = int(__thiscall*)(void*);
        return VirtualMethod::Get<Function>(this, 12)(this);
    }

    [[nodiscard]] bool IsInGame() noexcept {
        using Function = bool(__thiscall*)(void*);
        return VirtualMethod::Get<Function>(this, 26)(this);
    }
};

class IBaseClientDLL final {};

class IClientEntityList {
public:
    [[nodiscard]] CBaseEntity* GetClientEntity(int index) noexcept {
        using Function = CBaseEntity*(__thiscall*)(void*, int);
        return VirtualMethod::Get<Function>(this, 3)(this, index);
    }

    [[nodiscard]] int GetHighestEntityIndex() noexcept {
        using Function = int(__thiscall*)(void*);
        return VirtualMethod::Get<Function>(this, 6)(this);
    }
};

class ISurface {
public:
    void DrawSetColor(int red, int green, int blue, int alpha) noexcept {
        using Function = void(__thiscall*)(void*, int, int, int, int);
        VirtualMethod::Get<Function>(this, 10)(this, red, green, blue, alpha);
    }

    void DrawFilledRect(int x0, int y0, int x1, int y1) noexcept {
        using Function = void(__thiscall*)(void*, int, int, int, int);
        VirtualMethod::Get<Function>(this, 12)(this, x0, y0, x1, y1);
    }

    void DrawOutlinedRect(int x0, int y0, int x1, int y1) noexcept {
        using Function = void(__thiscall*)(void*, int, int, int, int);
        VirtualMethod::Get<Function>(this, 14)(this, x0, y0, x1, y1);
    }

    void DrawLine(int x0, int y0, int x1, int y1) noexcept {
        using Function = void(__thiscall*)(void*, int, int, int, int);
        VirtualMethod::Get<Function>(this, 15)(this, x0, y0, x1, y1);
    }

    void GetScreenSize(int& width, int& height) noexcept {
        using Function = void(__thiscall*)(void*, int&, int&);
        VirtualMethod::Get<Function>(this, 37)(this, width, height);
    }
};

class IPanel {
public:
    [[nodiscard]] const char* GetName(unsigned int panel) noexcept {
        using Function = const char*(__thiscall*)(void*, unsigned int);
        return VirtualMethod::Get<Function>(this, 36)(this, panel);
    }
};

namespace Engine {
namespace AddressOffsets {
inline constexpr std::uintptr_t kLocalPlayer = 0x4C88EC;
inline constexpr std::uintptr_t kEntityList = 0x4C5C1C;
inline constexpr std::uintptr_t kViewMatrix = 0x4A3D64;
inline constexpr std::uintptr_t kEnginePointer = 0x3C5C64;
inline constexpr std::uintptr_t kEntityStride = 0x10;
} // namespace AddressOffsets

extern IVEngineClient* client_engine;
extern IBaseClientDLL* client;
extern IClientEntityList* entity_list;
extern ISurface* surface;
extern IPanel* panel;

extern std::uintptr_t client_base;
extern std::uintptr_t engine_base;

[[nodiscard]] bool Initialize() noexcept;
void Shutdown() noexcept;

[[nodiscard]] bool IsReady() noexcept;
[[nodiscard]] CBaseEntity* GetLocalPlayer() noexcept;
[[nodiscard]] CBaseEntity* GetEntityByIndex(int index) noexcept;
[[nodiscard]] const Matrix4x4* GetViewMatrix() noexcept;
} // namespace Engine
