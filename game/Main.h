#pragma once

#include <windows.h>
// Every file of the original includes <string>: each one ends with the guarded initializer of the template static
// std::ctype<unsigned short>::id that <string> brings (docs/game-files.md)
#include <string>

// The static's destructor helper, which those initializers register with atexit: empty, so the linker folded it with
// the other empty functions of its kind, and the name our PDB keeps for the group changes with the build (foldfix)
// SYNTHETIC: GAME 0x404990
// ?vfunc_27@C_actor@@UAEXXZ

#include <settings.h>

class IGraph;
class I3D_driver;
class ISND_driver;
class C_actor;
struct S_vector;

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
// The next mission the scripts' CHANGE_MISSION asks for (PS2 names): its name and the second name, empty where no
// actors are kept, the kept actor's position and its z and y axes in the space of the script actor's frame, and the
// command's value
extern char NewMission[32];
extern bool ActiveNewMission;
extern char NewMission_ENTRY[32];
extern S_vector NewMission_POS;
extern S_vector NewMission_ROT[2];
extern float NewMissionSpeed;
// Set while the intro mission runs
extern bool _is_intro_;

// The joysticks' vector (Main.cpp's joy): its inlined push_back calls these helpers, which the linker folded with the
// other vectors of pointers'; the original kept earlier files' copies
// TEMPLATE: GAME 0x499050
// ?_Construct@std@@YIXPAPAVC_actor@@ABQAV2@@Z

// TEMPLATE: GAME 0x4d2040
// ?_Destroy@?$vector@PAUS_shvestky@C_shvestky@@V?$allocator@PAUS_shvestky@C_shvestky@@@std@@@std@@IAEXPAPAUS_shvestky@C_shvestky@@0@Z

// TEMPLATE: GAME 0x563cc0
// ?_Ucopy@?$vector@PAVC_program@@V?$allocator@PAVC_program@@@std@@@std@@IAEPAPAVC_program@@PBQAV3@0PAPAV3@@Z

// TEMPLATE: GAME 0x59e6d0
// ?_Ufill@?$vector@PAVIJoy@@V?$allocator@PAVIJoy@@@std@@@std@@IAEXPAPAVIJoy@@IABQAV3@@Z
