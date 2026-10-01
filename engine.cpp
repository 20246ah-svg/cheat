#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine.h"

#include <cstdio>
#include <cstring>

namespace Engine {
IVEngineClient* client_engine = nullptr;
IBaseClientDLL* client = nullptr;
IClientEntityList* entity_list = nullptr;
ISurface* surface = nullptr;
IPanel* panel = nullptr;

std::uintptr_t client_base = 0;
std::uintptr_t engine_base = 0;

namespace {
using CreateInterfaceFn = void*(__cdecl*)(const char*, int*);

char g_initialization_status[512] = "Initialization has not started.";

template <typename Interface>
[[nodiscard]] Interface* CaptureInterface(HMODULE module, const char* name) noexcept {
    if (module == nullptr) {
        return nullptr;
    }

    const auto factory = reinterpret_cast<CreateInterfaceFn>(
        GetProcAddress(module, "CreateInterface"));
    if (factory == nullptr) {
        return nullptr;
    }

    return static_cast<Interface*>(factory(name, nullptr));
}

void ResetStatus(const char* text) noexcept {
    std::snprintf(g_initialization_status,
                  sizeof(g_initialization_status),
                  "%s",
                  text);
}

void AppendStatus(const char* text) noexcept {
    const std::size_t used = std::strlen(g_initialization_status);
    if (used >= sizeof(g_initialization_status) - 1) {
        return;
    }

    std::snprintf(g_initialization_status + used,
                  sizeof(g_initialization_status) - used,
                  " %s",
                  text);
}

void UpdateModuleStatus(HMODULE engine_module,
                        HMODULE client_module,
                        HMODULE vgui2_module,
                        HMODULE surface_module) noexcept {
    ResetStatus("Waiting for modules:");
    if (engine_module == nullptr) {
        AppendStatus("engine.dll");
    }
    if (client_module == nullptr) {
        AppendStatus("client.dll");
    }
    if (vgui2_module == nullptr) {
        AppendStatus("vgui2.dll");
    }
    if (surface_module == nullptr) {
        AppendStatus("vguimatsurface.dll");
    }
}

void UpdateInterfaceStatus() noexcept {
    if (IsReady()) {
        if (client == nullptr) {
            ResetStatus("ESP interfaces are ready. Optional VClient017 is missing.");
        } else {
            ResetStatus("All ClientMod interfaces are ready.");
        }
        return;
    }

    ResetStatus("Missing interfaces:");
    if (client_engine == nullptr) {
        AppendStatus("VEngineClient013");
    }
    if (entity_list == nullptr) {
        AppendStatus("VClientEntityList003");
    }
    if (surface == nullptr) {
        AppendStatus("VGUI_Surface030");
    }
    if (panel == nullptr) {
        AppendStatus("VGUI_Panel009");
    }
    if (client == nullptr) {
        AppendStatus("(optional VClient017)");
    }
}
} // namespace

bool Initialize() noexcept {
    const HMODULE engine_module = GetModuleHandleA("engine.dll");
    const HMODULE client_module = GetModuleHandleA("client.dll");
    const HMODULE vgui2_module = GetModuleHandleA("vgui2.dll");
    const HMODULE surface_module = GetModuleHandleA("vguimatsurface.dll");

    if (engine_module == nullptr || client_module == nullptr ||
        vgui2_module == nullptr || surface_module == nullptr) {
        UpdateModuleStatus(
            engine_module, client_module, vgui2_module, surface_module);
        return false;
    }

    engine_base = reinterpret_cast<std::uintptr_t>(engine_module);
    client_base = reinterpret_cast<std::uintptr_t>(client_module);

    if (client_engine == nullptr) {
        client_engine =
            CaptureInterface<IVEngineClient>(engine_module, "VEngineClient013");
    }
    if (client == nullptr) {
        client = CaptureInterface<IBaseClientDLL>(client_module, "VClient017");
    }
    if (entity_list == nullptr) {
        entity_list = CaptureInterface<IClientEntityList>(
            client_module, "VClientEntityList003");
    }
    if (surface == nullptr) {
        surface =
            CaptureInterface<ISurface>(surface_module, "VGUI_Surface030");
    }
    if (panel == nullptr) {
        panel = CaptureInterface<IPanel>(vgui2_module, "VGUI_Panel009");
    }

    UpdateInterfaceStatus();

    // Rendering diagnostics can run with only IPanel and ISurface. Gameplay
    // interfaces are checked separately by IsReady(), so one missing interface
    // no longer prevents the PaintTraverse test from being installed.
    return IsRenderReady();
}

void Shutdown() noexcept {
    panel = nullptr;
    surface = nullptr;
    entity_list = nullptr;
    client = nullptr;
    client_engine = nullptr;
    engine_base = 0;
    client_base = 0;
    ResetStatus("Engine interfaces have been released.");
}

bool IsRenderReady() noexcept {
    return surface != nullptr && panel != nullptr;
}

bool IsReady() noexcept {
    return IsRenderReady() && client_engine != nullptr && entity_list != nullptr &&
           client_base != 0 && engine_base != 0;
}

const char* GetInitializationStatus() noexcept {
    return g_initialization_status;
}

CBaseEntity* GetLocalPlayer() noexcept {
    if (!IsReady()) {
        return nullptr;
    }

    // ClientMod v34 keeps a pointer to the local entity at this module-relative
    // address. If it has not been populated yet, use the engine/entity-list path.
    auto* const local = *reinterpret_cast<CBaseEntity**>(
        client_base + AddressOffsets::kLocalPlayer);
    if (local != nullptr) {
        return local;
    }

    const int local_index = client_engine->GetLocalPlayer();
    return local_index > 0 ? entity_list->GetClientEntity(local_index) : nullptr;
}

CBaseEntity* GetEntityByIndex(int index) noexcept {
    if (!IsReady() || index < 0 || index > 64) {
        return nullptr;
    }

    // CreateInterface remains the preferred path. The module-relative entity
    // list is kept as a fallback for ClientMod builds where the interface
    // temporarily returns no entity during level transitions.
    if (CBaseEntity* const entity = entity_list->GetClientEntity(index);
        entity != nullptr) {
        return entity;
    }

    const std::uintptr_t slot = client_base + AddressOffsets::kEntityList +
                                static_cast<std::uintptr_t>(index) *
                                    AddressOffsets::kEntityStride;
    return *reinterpret_cast<CBaseEntity**>(slot);
}

const Matrix4x4* GetViewMatrix() noexcept {
    if (!IsReady()) {
        return nullptr;
    }

    return reinterpret_cast<const Matrix4x4*>(
        engine_base + AddressOffsets::kViewMatrix);
}
} // namespace Engine
