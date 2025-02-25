#include <map>

#include "main.h"
#include "version.h"

#define RegisterHook(func, address) \
    hooks[address] = func

std::map<uintptr_t, void *> RegisterEngineHooks()
{
    std::map<uintptr_t, void *> hooks;

    RegisterHook(RandFloat, 0x2CDA0);

    RegisterHook(GetEngineVersion, 0x6B6D0);
    RegisterHook(TEV, 0x6B6E0);

    return hooks;
}