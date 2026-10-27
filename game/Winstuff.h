#pragma once

#include "Main.h"

// The startup's Windows helpers (docs/game-files.md, file 77; the name is a guess, PS2's Winstuff.cpp). The names are
// ours.

// MsgBox's buttons, and its answers but -1 for any other (the names are ours)
#define MSGBOX_OK 0
#define MSGBOX_YESNO 1
#define MSGBOX_YESNOCANCEL 2
#define MSGBOX_RETRYCANCEL 3
#define MSGBOX_OKCANCEL 4
#define MSGBOX_RETURN_NO 0
#define MSGBOX_RETURN_YES 1             // yes or retry
#define MSGBOX_RETURN_CANCEL 2

// A message box over the game's window, MSGBOX_* buttons, and the answer, MSGBOX_RETURN_*
int MsgBox(const char *text, const char *caption, int type);
// LoadString
int LoadResString(HINSTANCE hinstance, uint id, char *buffer, int size);
// Gives the window the executable's icon
void SetWindowIcon(HINSTANCE hinstance, HWND hwnd);
// Copies the name into path if the file opens, else looks for it in \MafiaGame on each drive C: to Z: that has a volume
// and copies the path it found there; false if it found none
bool FindGameFile(const char *name, char *path);
// Asks for the first CD until CheckCD finds it; false if the player cancels. main calls it before the engine starts,
// C_mission::Open with it running, which presents its frame before each message.
bool WaitForCD();
// Whether the file exists (FindFirstFile)
bool FileExists(const char *name);
// Looks for a1.dta in \MafiaDE, \MafiaEN, \MafiaSP, \MafiaFR, \MafiaIT and the root's \MafiaGame on each drive C: to Z:
// that has a volume
bool CheckCD();
