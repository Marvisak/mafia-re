// The game's tools (PS2 tools.cpp, which has C_sound_table; the PC's file is most likely the one at 0x55cfa0-0x56040c,
// docs/game-files.md). Written so far: PlayLogo; the rest is what InitSystem and CloseSystem need to link.

#include "tools.h"
#include "Winstuff.h"

#include "stub.h"

#include <string.h>

#include "IGraph/IGraph.h"

// The original's has a dynamic initializer (0x55d430)
// GLOBAL: GAME 0x646f50
C_sound_table sound_table;

// The AVI file first, the Bink file if there is no AVI file or it doesn't open. The video keeps its aspect ratio at the
// screen's width (a band above and below), and on a screen more than twice or three times as wide as high it takes the
// left half or the middle third: two or three monitors side by side. The original multiplies by 6 and 9 and divides
// by 3 (lea, then the reciprocal multiply), which VC6 doesn't fold, so the source has * 6 / 3 and * 9 / 3.
// FUNCTION: GAME 0x55d460
bool PlayLogo(const char *name, float sx, float sy)
{
    char file_name[256];
    char path[256];
    strcpy(file_name, name);
    if (file_name[strlen(file_name) - 1] != '.')
        // STRING: GAME 0x636628
        strcat(file_name, ".");
    // STRING: GAME 0x636624
    strcat(file_name, "avi");
    bool avi = true;
    if (!FindGameFile(file_name, path))
    {
        avi = false;
        strcpy(file_name, name);
        if (file_name[strlen(file_name) - 1] != '.')
            strcat(file_name, ".");
        // STRING: GAME 0x636620
        strcat(file_name, "bik");
        if (!FindGameFile(file_name, path))
            return false;
    }
    IShow *show = igraph->CreateIShow();
    if (show->Open(path, 0) != I3D_OK)
    {
        if (!avi)
        {
            show->Release();
            return false;
        }
        strcpy(file_name, name);
        if (file_name[strlen(file_name) - 1] != '.')
            strcat(file_name, ".");
        strcat(file_name, "bik");
        if (!FindGameFile(file_name, path))
        {
            show->Release();
            return false;
        }
        if (show->Open(path, 0) != I3D_OK)
        {
            show->Release();
            return false;
        }
    }
    int band = (int)(igraph->Scrn_sy() * (1.0f - sy / sx * (4.0f / 3.0f)));
    RECT rc;
    rc.left = 0;
    rc.right = igraph->Scrn_sx();
    rc.top = band / 2;
    rc.bottom = igraph->Scrn_sy() - band;
    if (igraph->Scrn_sx() > igraph->Scrn_sy() * 6 / 3)
    {
        rc.left = 0;
        rc.right = igraph->Scrn_sx() / 2;
    }
    if (igraph->Scrn_sx() > igraph->Scrn_sy() * 9 / 3)
    {
        rc.left = igraph->Scrn_sx() / 3;
        rc.right = igraph->Scrn_sx() / 3;
    }
    show->SetRect(&rc);
    show->Play();
    show->RenderVideoUntilKeyPressOrEnd();
    show->Release();
    return true;
}

#pragma auto_inline(off)

// The original deletes the seven buffers of each table entry (0xd0 bytes) and clears the list, as PS2's
// STUB: GAME 0x55ee60
void C_sound_table::FreeTables()
{
    StubLog("C_sound_table::FreeTables");
}

// STUB: GAME 0x55f390
void C_sound_table::LoadTables(const char *name)
{
    StubLog("C_sound_table::LoadTables %s", name);
}

#pragma auto_inline(on)
