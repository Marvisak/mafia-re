#include "version.h"

#include <windows.h>
#include <cstdio>

int EXPORT GetEngineVersion()
{
    return ENGINE_VERSION;
}

bool EXPORT_CDECL TEV(const char *title, int engine_version)
{
    if (engine_version == ENGINE_VERSION)
    {
        return true;
    }
    char text[128];
    sprintf(text, "Bad version - engine:%d  client:%d", ENGINE_VERSION, engine_version);
    MessageBoxA(0, text, title, MB_SYSTEMMODAL | MB_ICONSTOP);
    return false;
}