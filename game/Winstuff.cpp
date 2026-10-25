// The startup's Windows helpers, the file at 0x5661f0-0x566f3c (docs/game-files.md); the name is a guess, PS2's
// Winstuff.cpp, which has no functions left. Written so far: what InitSystem calls (LoadResString, SetWindowIcon,
// FindGameFile) and the CD check (WaitForCD, FileExists, CheckCD), all names ours, and the caches' AddDir that follow.

#include "Winstuff.h"
#include "I3D_cache.h"

#include <stdio.h>

#include "IGraph/IGraph.h"
#include "res/resource.h"

// FUNCTION: GAME 0x5662a0
int LoadResString(HINSTANCE hinstance, uint id, char *buffer, int size)
{
    return LoadString(hinstance, id, buffer, size);
}

// FUNCTION: GAME 0x5662c0
void SetWindowIcon(HINSTANCE hinstance, HWND hwnd)
{
    HICON icon = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_MAFIA));
    if (icon)
        SetClassLong(hwnd, GCL_HICON, (LONG)icon);
}

// The drive's type is asked for and not used, as in CheckCD
// FUNCTION: GAME 0x5662e0
bool FindGameFile(const char *name, char *path)
{
    strcpy(path, name);
    HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);
        return true;
    }
    for (int drive = 'C'; drive < 'Z' + 1; drive++)
    {
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
            // STRING: GAME 0x636688
            sprintf(path, "X:\\MafiaGame\\%s", name);
            path[0] = drive;
            file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (file != INVALID_HANDLE_VALUE)
            {
                CloseHandle(file);
                return true;
            }
        }
    }
    return false;
}

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

// The caches' AddDir (I3D_cache.h) lie at this file's end, after the CD check, with the C_str helpers they are the first
// to need, though InitSystem is their only caller: something here called them, which the linker dropped. This stand-in,
// which the linker drops too, has VC6 compile them here (it ignores explicit instantiations of member functions).
void AddCacheDirs(const char *dir)
{
    model_cache.AddDir(dir);
    sound_cache.AddDir(dir);
    anim_cache.AddDir(dir);
}
