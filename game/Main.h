#pragma once

#include <windows.h>
// Every file of the original includes <string>: each one ends with the guarded initializer of the template static
// std::ctype<unsigned short>::id that <string> brings (docs/game-files.md)
#include <string>

#include <settings.h>

class IGraph;
class C_actor;

// The game itself: loads the settings setup.exe saved, starts the engine and runs the main menu until the player quits.
// VC6 keeps main __cdecl under /Gr.
int main(HINSTANCE hInstance);

// Runs the loaded mission until it ends
void GameLoopSingle();
// Closes what InitSystem started
void CloseSystem();

// The engine's graphics interface
extern IGraph *igraph;
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
// std::_Construct

// TEMPLATE: GAME 0x4d2040
// std::vector<IJoy *,std::allocator<IJoy *> >::_Destroy

// TEMPLATE: GAME 0x563cc0
// std::vector<IJoy *,std::allocator<IJoy *> >::_Ucopy

// TEMPLATE: GAME 0x59e6d0
// std::vector<IJoy *,std::allocator<IJoy *> >::_Ufill
