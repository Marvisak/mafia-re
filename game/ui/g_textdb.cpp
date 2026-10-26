// The game's texts (PS2 g_textdb.cpp; the PC's file is the last one, at 0x60fa00-0x60fc2c, docs/game-files.md): the
// global's initializer, the constructor, the destructor, LoadDatabase, Unload, GetText, then GetTextOrNULL (0x60fba0),
// which nothing written calls yet. PS2's Transpose isn't on PC.

#include "ui/g_textdb.h"

#include "rw_data.h"

// GLOBAL: GAME 0x6d8714
G_TextDatabase textDatabase;

// FUNCTION: GAME 0x60fa40
G_TextDatabase::G_TextDatabase()
{
    data = NULL;
}

// FUNCTION: GAME 0x60fa50
G_TextDatabase::~G_TextDatabase()
{
    if (data)
        delete[] data;
}

// FUNCTION: GAME 0x60fa60
bool G_TextDatabase::LoadDatabase(char *name)
{
    if (!name)
        return false;
    int file = dtaOpen(name, false);
    if (file < 0)
        return false;
    if (data)
        delete[] data;
    data = NULL;
    num_texts = 0;
    int size = dtaSeek(file, 0, SEEK_END);
    dtaSeek(file, 0, SEEK_SET);
    data = new char[size + 1];
    dtaRead(file, data, size);
    dtaClose(file);
    num_texts = *(uint *)data;
    texts = (S_text *)(data + 8);
    for (uint i = 0; i < num_texts; i++)
        texts[i].text += (uint)data;
    return true;
}

// FUNCTION: GAME 0x60fb10
void G_TextDatabase::Unload()
{
    if (data)
        delete[] data;
    data = NULL;
    num_texts = 0;
}

// A binary search of the ids, which the file has in ascending order, in steps of powers of two from the largest under
// the count (4096 at most), the last ones' from the end. PS2's is the same.
// FUNCTION: GAME 0x60fb40
char *G_TextDatabase::GetText(uint id)
{
    uint step = 0x1000;
    while (step && step > num_texts)
        step >>= 1;
    if (step)
    {
        uint pos = texts[step].id < id ? num_texts - step - 1 : 0;
        for (step >>= 1; step; )
        {
            uint i = pos + step;
            step >>= 1;
            if (texts[i].id <= id)
                pos = i;
        }
        if (texts[pos + step].id == id)
            return texts[pos + step].text;
    }
    // STRING: GAME 0x638390
    return " ";
}
