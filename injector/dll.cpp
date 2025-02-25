#include <windows.h>

#include <subhook.h>

#include "../engine/hooks.h"

DWORD WINAPI ReplacerThread(HMODULE hModule)
{
    auto engine_base_address = (uintptr_t)GetModuleHandleA("LS3DF.dll");
    auto engine_hooks = RegisterEngineHooks();
    for (auto &[address, hook_func] : engine_hooks)
    {
        auto hook = subhook_new(reinterpret_cast<void *>(engine_base_address + address), hook_func, static_cast<subhook_flags_t>(0));
        subhook_install(hook);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        HANDLE thread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ReplacerThread, hModule, 0, nullptr);
        if (thread != INVALID_HANDLE_VALUE)
        {
            CloseHandle(thread);
        }
    }
    return TRUE;
}