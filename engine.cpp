#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>

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

char g_initialization_status[768] = "Initialization has not started.";
char g_engine_owner[MAX_PATH] = "not found";
char g_client_owner[MAX_PATH] = "not found";
char g_surface_owner[MAX_PATH] = "not found";
char g_panel_owner[MAX_PATH] = "not found";

template <typename Interface>
struct InterfaceCapture {
    Interface* instance = nullptr;
    HMODULE owner = nullptr;
};

template <std::size_t Size>
void StoreModuleName(HMODULE module, char (&destination)[Size]) noexcept {
    char path[MAX_PATH]{};
    if (module == nullptr ||
        GetModuleFileNameA(module, path, static_cast<DWORD>(sizeof(path))) == 0) {
        std::snprintf(destination, Size, "%s", "unknown");
        return;
    }

    const char* name = std::strrchr(path, '\\');
    name = name == nullptr ? path : name + 1;
    std::snprintf(destination, Size, "%s", name);
}

template <typename Interface>
[[nodiscard]] InterfaceCapture<Interface> CaptureFromLoadedModules(
    const char* interface_name) noexcept {
    InterfaceCapture<Interface> result{};
    HANDLE const snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        return result;
    }

    MODULEENTRY32 module_entry{};
    module_entry.dwSize = sizeof(module_entry);
    if (Module32First(snapshot, &module_entry)) {
        do {
            const auto factory = reinterpret_cast<CreateInterfaceFn>(
                GetProcAddress(module_entry.hModule, "CreateInterface"));
            if (factory == nullptr) {
                continue;
            }

            Interface* const instance =
                static_cast<Interface*>(factory(interface_name, nullptr));
            if (instance != nullptr) {
                result.instance = instance;
                result.owner = module_entry.hModule;
                break;
            }
        } while (Module32Next(snapshot, &module_entry));
    }

    CloseHandle(snapshot);
    return result;
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

void UpdateInterfaceStatus() noexcept {
    if (IsReady()) {
        std::snprintf(
            g_initialization_status,
            sizeof(g_initialization_status),
            "Interfaces ready. engine=%s; entities=%s; surface=%s; panel=%s.%s",
            g_engine_owner,
            g_client_owner,
            g_surface_owner,
            g_panel_owner,
            client == nullptr ? " Optional VClient017 is missing." : "");
        return;
    }

    ResetStatus("Missing interfaces after scanning every loaded module:");
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
    // ClientMod runs several CMLauncher.exe processes and uses renamed modules
    // such as clientmod_client.dll. Search every loaded module for the Source
    // CreateInterface export instead of assuming engine.dll/client.dll names.
    if (client_engine == nullptr) {
        const auto captured =
            CaptureFromLoadedModules<IVEngineClient>("VEngineClient013");
        client_engine = captured.instance;
        if (captured.instance != nullptr) {
            engine_base = reinterpret_cast<std::uintptr_t>(captured.owner);
            StoreModuleName(captured.owner, g_engine_owner);
        }
    }

    if (client == nullptr) {
        const auto captured =
            CaptureFromLoadedModules<IBaseClientDLL>("VClient017");
        client = captured.instance;
        if (captured.instance != nullptr) {
            if (client_base == 0) {
                client_base = reinterpret_cast<std::uintptr_t>(captured.owner);
            }
            StoreModuleName(captured.owner, g_client_owner);
        }
    }

    if (entity_list == nullptr) {
        const auto captured = CaptureFromLoadedModules<IClientEntityList>(
            "VClientEntityList003");
        entity_list = captured.instance;
        if (captured.instance != nullptr) {
            // The entity-list owner is the correct base for client offsets.
            client_base = reinterpret_cast<std::uintptr_t>(captured.owner);
            StoreModuleName(captured.owner, g_client_owner);
        }
    }

    if (surface == nullptr) {
        const auto captured =
            CaptureFromLoadedModules<ISurface>("VGUI_Surface030");
        surface = captured.instance;
        if (captured.instance != nullptr) {
            StoreModuleName(captured.owner, g_surface_owner);
        }
    }

    if (panel == nullptr) {
        const auto captured =
            CaptureFromLoadedModules<IPanel>("VGUI_Panel009");
        panel = captured.instance;
        if (captured.instance != nullptr) {
            StoreModuleName(captured.owner, g_panel_owner);
        }
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
