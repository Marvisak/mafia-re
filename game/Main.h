#pragma once

#include <windows.h>
// Every file of the original includes <string>: each one ends with the guarded initializer of the template static
// std::ctype<unsigned short>::id that <string> brings (docs/game-files.md)
#include <string>

// The static's destructor helper, which those initializers register with atexit: empty, so the linker folded it with
// the other empty functions of its kind, and the name our PDB keeps for the group changes with the build (foldfix)
// SYNTHETIC: GAME 0x404990
// ??1S_unknown_26c@C_game@@QAE@XZ

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
// ?_Construct@std@@YIXPAPAVIJoy@@ABQAV2@@Z

// TEMPLATE: GAME 0x4d2040
// ?_Destroy@?$vector@US_frame_actor_buffer@C_mission@@V?$allocator@US_frame_actor_buffer@C_mission@@@std@@@std@@IAEXPAUS_frame_actor_buffer@C_mission@@0@Z

// TEMPLATE: GAME 0x563cc0
// ?_Ucopy@?$vector@PAVC_effect_subpart@@V?$allocator@PAVC_effect_subpart@@@std@@@std@@IAEPAPAVC_effect_subpart@@PBQAV3@0PAPAV3@@Z

// TEMPLATE: GAME 0x59e6d0
// ?_Ufill@?$vector@PAVC_effect_subpart@@V?$allocator@PAVC_effect_subpart@@@std@@@std@@IAEXPAPAVC_effect_subpart@@IABQAV3@@Z
