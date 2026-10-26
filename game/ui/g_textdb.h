#pragma once

#include "Main.h"

// The game's texts (PS2 G_TextDatabase, g_textdb.cpp): tables\textdb_<language>.def, which G_LoadSaveClass::
// LanguageSelect loads. The file holds the count of texts, a dword, then an id and an offset into the file for each
// text, the texts (zero-terminated) after them; LoadDatabase reads the whole file and turns the offsets into pointers.
// Only what the start uses so far.
class G_TextDatabase
{
public:
    // A text and its id (the struct's name is ours)
    struct S_text
    {
        uint id;
        char *text;
    };

    char *data;                         // 0x0, the whole file
    uint num_texts;                     // 0x4
    S_text *texts;                      // 0x8, in the file, after the count and the dword

    G_TextDatabase();
    ~G_TextDatabase();
    // Loads the file, false if there is none. PS2's has a second parameter, a bool it doesn't use.
    bool LoadDatabase(char *name);
    // Frees the file, at CloseSystem
    void Unload();
};

extern G_TextDatabase textDatabase;
