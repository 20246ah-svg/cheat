#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "engine.h"
#include "hooks.h"

#include <cstdio>

static_assert(sizeof(void*) == 4,
              "This DLL targets the 32-bit ClientMod hl2.exe process");

namespace {
DWORD WINAPI InitializeThread(void*) noexcept {
    constexpr int kRenderInitializationAttempts = 40;
    bool render_ready = false;

    // Do not wait forever: a timeout now produces the exact missing module or
    // interface instead of silently leaving the user without a second dialog.
    for (int attempt = 0; attempt < kRenderInitializationAttempts; ++attempt) {
        if (Engine::Initialize()) {
            render_ready = true;
            break;
        }
        Sleep(250);
    }

    if (!render_ready) {
        Engine::BuildExtendedDiagnostics();
        OutputDebugStringA("[cheat] Render initialization failed: ");
        OutputDebugStringA(Engine::GetInitializationStatus());
        OutputDebugStringA("\n");
#if defined(CHEAT_DIAGNOSTICS)
        MessageBoxA(nullptr,
                    Engine::GetInitializationStatus(),
                    "cheat diagnostics - initialization failed",
                    MB_OK | MB_ICONERROR);
#endif
        return 0;
    }

    const bool hook_installed = Hooks::Initialize();

    // Rendering only needs IPanel and ISurface. Give the gameplay interfaces a
    // short additional window to appear, but never hold back the test square.
    for (int attempt = 0; attempt < 20 && !Engine::IsReady(); ++attempt) {
        Sleep(100);
        (void)Engine::Initialize();
    }

    char message[768]{};
    std::snprintf(
        message,
        sizeof(message),
        "%s\n\n%s",
        hook_installed
            ? "PaintTraverse hook installed. A red test square should be visible."
            : "PaintTraverse hook installation failed.",
        Engine::GetInitializationStatus());

    OutputDebugStringA("[cheat] ");
    OutputDebugStringA(message);
    OutputDebugStringA("\n");

#if defined(CHEAT_DIAGNOSTICS)
    MessageBoxA(nullptr,
                message,
                "cheat diagnostics",
                MB_OK | (hook_installed ? MB_ICONINFORMATION : MB_ICONERROR));
#endif

    return 0;
}
} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);

#if defined(CHEAT_DIAGNOSTICS)
        // Deliberately synchronous for the requested load-life check. Disable
        // CHEAT_DIAGNOSTICS after troubleshooting to avoid work under loader lock.
        MessageBoxA(nullptr,
                    "DLL loaded! Press OK to continue.",
                    "cheat diagnostics",
                    MB_OK | MB_ICONINFORMATION);
#endif

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
