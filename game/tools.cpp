// The game's tools (PS2 tools.cpp, which has C_sound_table; the PC's file is most likely the one at 0x55cfa0-0x56040c,
// docs/game-files.md). Written so far: PlayLogo, and the sound table's constructor, destructor, FreeTables and
// LoadTables, which InitSystem and CloseSystem call; its virtual functions, which the vtable needs, are stubs.

#include "tools.h"
#include "Winstuff.h"

#include "stub.h"

#include <string.h>

#include "IGraph/IGraph.h"
#include "rw_data.h"

// Open: the original has it right after sound_table, where its bucket (169; sound_table's is 788) would put it before
// (tools/bsshash.py)
// GLOBAL: GAME 0x646f6c
std::vector<S_sound_group> sound_group;

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

// FUNCTION: GAME 0x55edd0
C_sound_table::C_sound_table()
{
    next_group_id = 0;
    next_sound_id = 0;
}

// FUNCTION: GAME 0x55ee00
C_sound_table::~C_sound_table()
{
    FreeTables();
}

// The sounds from the last one, as PS2's
// FUNCTION: GAME 0x55ee60
void C_sound_table::FreeTables()
{
    for (int i = tables.size(); i--;)
    {
        delete[] tables[i].name;
        delete[] tables[i].file_names[0];
        delete[] tables[i].file_names[1];
        delete[] tables[i].file_names[2];
        delete[] tables[i].file_names[3];
        delete[] tables[i].file_names[4];
        delete[] tables[i].file_names[5];
    }
    tables.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x55ef30
void C_sound_table::AddGroup(const char *name)
{
    StubLog("C_sound_table::AddGroup %s", name);
}

// STUB: GAME 0x55ef80
void C_sound_table::DelGroup(int index)
{
    StubLog("C_sound_table::DelGroup %d", index);
}

// STUB: GAME 0x55f080
void C_sound_table::AddSound(const char *name, uint group)
{
    StubLog("C_sound_table::AddSound %s", name);
}

// STUB: GAME 0x55f120
void C_sound_table::DelSound(uint id)
{
    StubLog("C_sound_table::DelSound %u", id);
}

// STUB: GAME 0x55f210
C_sound_table::S_sound_table *C_sound_table::GetTableFromIDX(uint id)
{
    StubLog("C_sound_table::GetTableFromIDX %u", id);
    return NULL;
}

// STUB: GAME 0x55f270
void C_sound_table::SetFileName(S_sound_table *table, int index, const char *file_name)
{
    StubLog("C_sound_table::SetFileName");
}

// STUB: GAME 0x55f2d0
void C_sound_table::OptimalizeTable(S_sound_table *table)
{
    StubLog("C_sound_table::OptimalizeTable");
}

#pragma auto_inline(on)

// Version 2 only. The file has the two counters, the groups and the sounds, each with its name, which the game skips
// (only the editor needs them), its files and its parameters.
// FUNCTION: GAME 0x55f390
void C_sound_table::LoadTables(const char *name)
{
    int h = dtaOpen(name, false);
    if (h == -1)
        return;
    int version;
    dtaRead(h, &version, 4);
    if (version != 2)
    {
        dtaClose(h);
        return;
    }
    dtaRead(h, &next_group_id, 4);
    dtaRead(h, &next_sound_id, 4);
    int num;
    dtaRead(h, &num, 4);
    sound_group.clear();
    if (num)
        dtaSeek(h, num * sizeof(S_sound_group), 1);
    tables.clear();
    dtaRead(h, &num, 4);
    for (int i = 0; i < num; i++)
    {
        S_sound_table t;
        memset(&t, 0, sizeof(t));
        tables.push_back(t);
        memset(&tables.back(), 0, sizeof(S_sound_table));
        tables.back().visible = true;
        int len;
        dtaRead(h, &len, 4);
        if (len)
            dtaSeek(h, len, 1);
        dtaRead(h, &tables.back().id, 4);
        dtaRead(h, &tables.back().group, 4);
        dtaRead(h, &tables.back().num_files, 1);
        dtaRead(h, &tables.back().num_params, 1);
        for (int j = 0; j < tables.back().num_files; j++)
        {
            dtaRead(h, &len, 4);
            if (len)
            {
                tables.back().file_names[j] = new char[len + 1];
                dtaRead(h, tables.back().file_names[j], len);
                tables.back().file_names[j][len] = 0;
            }
            dtaRead(h, &tables.back().file_probability[j], 4);
        }
        if (tables.back().num_params > 0)
            dtaRead(h, tables.back().params, tables.back().num_params * sizeof(S_sound_param));
    }
    dtaClose(h);
}

#pragma auto_inline(off)

// STUB: GAME 0x55f5e0
void C_sound_table::SaveTables(const char *name)
{
    StubLog("C_sound_table::SaveTables %s", name);
}

// The original's jumps to ConstructSound (0x55f820), which sets the sound up from its table
// STUB: GAME 0x55fa00
bool C_sound_table::TestConstructSound(I3D_sound *sound, uint id)
{
    StubLog("C_sound_table::TestConstructSound %u", id);
    return false;
}

// STUB: GAME 0x55fa10
void C_sound_table::ClearInvisibleFlag()
{
    StubLog("C_sound_table::ClearInvisibleFlag");
}

#pragma auto_inline(on)
