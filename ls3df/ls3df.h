#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <types.h>

// What GetEngineVersion returns and TEV checks the game's version against
#define ENGINE_VERSION 395

class IGraph;
class I3D_driver;
class ISND_driver;

// The engine's three drivers, which GetIGraph, I3DGetDriver and ISndGetDriver return
extern bool disableLog;
extern IGraph *pGlobalIGraph;
extern I3D_driver *pI3DDriver;
extern ISND_driver *pISndDriver;

// The undecorated exports are __cdecl, where the engine's default is __fastcall (/Gr). __declspec(dllexport) has to
// come before the return type: VC6 ignores it after a '*' (C4518).
#define LS3D_EXPORT extern "C" __declspec(dllexport)

LS3D_EXPORT int __cdecl GetEngineVersion();
LS3D_EXPORT bool __cdecl TEV(const char *title, int engine_version);
LS3D_EXPORT IGraph *__cdecl GetIGraph();
LS3D_EXPORT ISND_driver *__cdecl ISndGetDriver();
LS3D_EXPORT I3D_driver *__cdecl I3DGetDriver();
