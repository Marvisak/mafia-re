// The startup's Windows helpers, the file at 0x5661f0-0x566f3c (docs/game-files.md); the name is a guess, PS2's
// Winstuff.cpp, which has no functions left. Written so far: the CD check (WaitForCD, FileExists, CheckCD), whose
// names are ours. CheckCD's "X:\" is the literal of the drive search before it (0x5662e0), not written yet.

#include "Winstuff.h"

#include <stdio.h>

#include "IGraph/IGraph.h"

// The call at the loop's top: VC6 keeps a while (true) that leaves through a return unrotated, where a
// while (!CheckCD()) gets a second call at the bottom
// FUNCTION: GAME 0x5663d0
bool WaitForCD()
{
    while (true)
    {
        if (CheckCD())
            return true;
        if (igraph)
            igraph->Present();
        // STRING: GAME 0x63669c
        if (MessageBox(NULL, "Please insert MAFIA CD 1", NULL, MB_RETRYCANCEL | MB_ICONERROR) == IDCANCEL)
            return false;
    }
}

// FUNCTION: GAME 0x566410
bool FileExists(const char *name)
{
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFile(name, &data);
    if (find == INVALID_HANDLE_VALUE)
        return false;
    FindClose(find);
    return true;
}

// Every folder is tested, also after a hit. The drive's type is asked for and not used. The loop ends below 'Z' + 1
// (jl), as the drive search's does.
// FUNCTION: GAME 0x566440
bool CheckCD()
{
    for (int drive = 'C'; drive < 'Z' + 1; drive++)
    {
        char path[128];
        char file_system[128];
        char volume[128];
        DWORD serial, max_length, flags;
        // STRING: GAME 0x636698
        strcpy(path, "X:\\");
        path[0] = drive;
        GetDriveType(path);
        if (GetVolumeInformation(path, volume, sizeof(volume), &serial, &max_length, &flags, file_system,
                sizeof(file_system)))
        {
            // STRING: GAME 0x636750
            sprintf(path, "%c:\\MafiaDE\\MafiaGame\\a1.dta", (char)drive);
            bool found = FileExists(path);
            // STRING: GAME 0x636730
            sprintf(path, "%c:\\MafiaEN\\MafiaGame\\a1.dta", (char)drive);
            found |= FileExists(path);
            // STRING: GAME 0x636710
            sprintf(path, "%c:\\MafiaSP\\MafiaGame\\a1.dta", (char)drive);
            found |= FileExists(path);
            // STRING: GAME 0x6366f0
            sprintf(path, "%c:\\MafiaFR\\MafiaGame\\a1.dta", (char)drive);
            found |= FileExists(path);
            // STRING: GAME 0x6366d0
            sprintf(path, "%c:\\MafiaIT\\MafiaGame\\a1.dta", (char)drive);
            found |= FileExists(path);
            // STRING: GAME 0x6366b8
            sprintf(path, "%c:\\MafiaGame\\a1.dta", (char)drive);
            found |= FileExists(path);
            if (found)
                return true;
        }
    }
    return false;
}
