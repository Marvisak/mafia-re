#pragma once

#include <windows.h>
// Every file of the original includes <string>: each one ends with the guarded initializer of the template static
// std::ctype<unsigned short>::id that <string> brings (docs/game-files.md)
#include <string>

// The static's destructor helper, which those initializers register with atexit: empty, so the linker folded it with
// the other empty functions of its kind, and the name our PDB keeps for the group changes with the build (foldfix)
// SYNTHETIC: GAME 0x404990
// ??1C_compass@@QAE@XZ

#include <settings.h>

class IGraph;
class I3D_driver;
class ISND_driver;
class C_actor;

// The game itself: loads the settings setup.exe saved, starts the engine and runs the main menu until the player quits.
// VC6 keeps main __cdecl under /Gr.
int main(HINSTANCE hInstance);

// Runs the loaded mission until it ends
void GameLoopSingle();
// Whether the main menu has Racing (PC only): it hasn't
bool RacingInMainMenu();
// Closes what InitSystem started
void CloseSystem();
// The W-buffer on or off (PS2 name)
void SetWBuffer(bool on);

// The engine's graphics interface
extern IGraph *igraph;
// The engine's 3D driver
extern I3D_driver *driver;
// The engine's sound driver, NULL if it didn't start
extern ISND_driver *isound;
// What setup.exe saved, read at the start and saved back with the game's options when the player quits. The name is ours,
// chosen for Main.cpp's .bss, where it follows igraph: its bucket has to come after igraph's (tools/bsshash.py).
extern S_settings setting;
// Cleared before each menu and mission
extern C_actor *NewMissionActors[2];
// Set while the intro mission runs
extern bool _is_intro_;

// The joysticks' vector (Main.cpp's joy): its inlined push_back calls these helpers, which the linker folded with the
// other vectors of pointers'; the original kept earlier files' copies
// TEMPLATE: GAME 0x499050
// ?_Construct@std@@YIXPAPAVGM_Component@@ABQAV2@@Z

// TEMPLATE: GAME 0x4d2040
// ?_Destroy@?$vector@PAVGM_Component@@V?$allocator@PAVGM_Component@@@std@@@std@@IAEXPAPAVGM_Component@@0@Z

// TEMPLATE: GAME 0x563cc0
// ?_Ucopy@?$vector@IV?$allocator@I@std@@@std@@IAEPAIPBI0PAI@Z

// TEMPLATE: GAME 0x59e6d0
// ?_Ufill@?$vector@PAVGM_Component@@V?$allocator@PAVGM_Component@@@std@@@std@@IAEXPAPAVGM_Component@@IABQAV3@@Z
