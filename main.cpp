#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine.h"
#include "hooks.h"

static_assert(sizeof(void*) == 4,
              "This DLL targets the 32-bit ClientMod hl2.exe process");

namespace {
DWORD WINAPI InitializeThread(void*) noexcept {
    // The DLL can be loaded before all engine modules have completed startup.
    while (!Engine::Initialize()) {
        Sleep(250);
    }

    Hooks::Initialize();
    return 0;
}
} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);

        HANDLE const thread = CreateThread(
            nullptr, 0, &InitializeThread, nullptr, 0, nullptr);
        if (thread == nullptr) {
            return FALSE;
        }

        CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
        // reserved != nullptr means process termination; engine modules may have
        // already gone away, so no explicit cleanup is safe in that case.
        Hooks::Shutdown();
        Engine::Shutdown();
    }

    return TRUE;
}
