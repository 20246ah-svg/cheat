#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine.h"

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
} // namespace

bool Initialize() noexcept {
    if (IsReady()) {
        return true;
    }

    const HMODULE engine_module = GetModuleHandleA("engine.dll");
    const HMODULE client_module = GetModuleHandleA("client.dll");
    const HMODULE vgui2_module = GetModuleHandleA("vgui2.dll");
    const HMODULE surface_module = GetModuleHandleA("vguimatsurface.dll");

    if (engine_module == nullptr || client_module == nullptr ||
        vgui2_module == nullptr || surface_module == nullptr) {
        return false;
    }

    IVEngineClient* const captured_engine =
        CaptureInterface<IVEngineClient>(engine_module, "VEngineClient013");
    IBaseClientDLL* const captured_client =
        CaptureInterface<IBaseClientDLL>(client_module, "VClient017");
    IClientEntityList* const captured_entity_list =
        CaptureInterface<IClientEntityList>(client_module, "VClientEntityList003");
    ISurface* const captured_surface =
        CaptureInterface<ISurface>(surface_module, "VGUI_Surface030");
    IPanel* const captured_panel =
        CaptureInterface<IPanel>(vgui2_module, "VGUI_Panel009");

    if (captured_engine == nullptr || captured_client == nullptr ||
        captured_entity_list == nullptr || captured_surface == nullptr ||
        captured_panel == nullptr) {
        return false;
    }

    client_engine = captured_engine;
    client = captured_client;
    entity_list = captured_entity_list;
    surface = captured_surface;
    panel = captured_panel;
    client_base = reinterpret_cast<std::uintptr_t>(client_module);
    engine_base = reinterpret_cast<std::uintptr_t>(engine_module);
    return true;
}

void Shutdown() noexcept {
    panel = nullptr;
    surface = nullptr;
    entity_list = nullptr;
    client = nullptr;
    client_engine = nullptr;
    engine_base = 0;
    client_base = 0;
}

bool IsReady() noexcept {
    return client_engine != nullptr && client != nullptr && entity_list != nullptr &&
           surface != nullptr && panel != nullptr && client_base != 0 &&
           engine_base != 0;
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

const Matrix4x4* GetViewMatrix() noexcept {
    if (!IsReady()) {
        return nullptr;
    }

    return reinterpret_cast<const Matrix4x4*>(
        engine_base + AddressOffsets::kViewMatrix);
}
} // namespace Engine
