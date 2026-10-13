// The engine's globals and its plain C exports, at 0x1006b430-0x1006bd11 in the original. The PS2 build names the
// file (__sinit_Globals.cpp): its functions TEV to ErrorHandler come right before IGraph.cpp's there, as they do here.
// Which folder of C:\MyProjects\engine8 it was in is unknown.
// Not complete yet. Still missing, in the original's order: the $E pair of the BreBre object StaticInstance and its
// constructor, which opens log.txt (0x1006b430-0x1006b6c6), I3DGetResultMessage, dbgPrintf, dbgPrintf1,
// dbgPrintGuid, the $E of a global bounding box, BBoxVertices and ErrorHandler (0x1006b770-0x1006bd11).
#include "ls3df.h"

#include <stdio.h>

// VC6 lays out zero-initialized globals in declaration order, after the ones its .bss hash places (StaticInstance).
// Hashed, pGlobalIGraph would come first.

// GLOBAL: LS3DF 0x101c5293
bool disableLog = false;

// GLOBAL: LS3DF 0x101c5294
IGraph *pGlobalIGraph = NULL;

// GLOBAL: LS3DF 0x101c5298
I3D_driver *pI3DDriver = NULL;

// GLOBAL: LS3DF 0x101c529c
ISND_driver *pISndDriver = NULL;

// FUNCTION: LS3DF 0x1006b6d0
int __cdecl GetEngineVersion()
{
    return ENGINE_VERSION;
}

// FUNCTION: LS3DF 0x1006b6e0
bool __cdecl TEV(const char *title, int engine_version)
{
    if (engine_version == ENGINE_VERSION)
        return true;
    char text[128];
    sprintf(text, "Bad version - engine:%d  client:%d", ENGINE_VERSION, engine_version);
    MessageBoxA(NULL, text, title, MB_SYSTEMMODAL | MB_ICONSTOP);
    return false;
}

// FUNCTION: LS3DF 0x1006b740
IGraph *__cdecl GetIGraph()
{
    return pGlobalIGraph;
}

// FUNCTION: LS3DF 0x1006b750
ISND_driver *__cdecl ISndGetDriver()
{
    return pISndDriver;
}

// FUNCTION: LS3DF 0x1006b760
I3D_driver *__cdecl I3DGetDriver()
{
    return pI3DDriver;
}
