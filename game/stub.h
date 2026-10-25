#pragma once

// What the stubs log, the names of the functions not written yet that a run reaches. Not the original's: it goes with
// the last stub. The engine's dbgPrintf writes nothing once I3D_driver::Init has turned its log off (InitSystem doesn't
// ask IGraph for the log), so the stubs append their lines to the engine's log.txt, next to the executable, themselves.

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Writes "STUB: " and the formatted text as a line of its own
inline void StubLog(const char *format, ...)
{
    char name[MAX_PATH];
    GetModuleFileName(NULL, name, sizeof(name));
    char *cp = strrchr(name, '\\');
    strcpy(cp ? cp + 1 : name, "log.txt");
    FILE *f = fopen(name, "at");
    if (!f)
        return;
    fprintf(f, "STUB: ");
    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);
    fprintf(f, "\n");
    fclose(f);
}
