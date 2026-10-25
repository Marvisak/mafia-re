// The console's text (PS2 console.cpp; the PC's file is most likely the one at 0x4f5dc0-0x4f6a2c, which loads
// editfont.bmp and fnt_matrix.bin, docs/game-files.md). Written so far: the font, CreateFontTexture, ConsolesInit and
// ConsolesDone. Not written: the consoles' vector (PS2 _consoles, 0x646b28, whose initializer starts the file),
// ConsoleReg, ConsoleUnReg and ConsoleRender (most likely 0x4f6590, 0x4f67a0, 0x4f6850) and the text drawing
// (0x4f6040).

#include "ui/console.h"

#include <string.h>

#include "IGraph/IGraph.h"
#include "rw_data.h"

// The font's characters by code, as fnt_matrix.bin has them (PS2 name; nothing else reads the file)
// GLOBAL: GAME 0x645728
S_font_char fnt_matrix[256];

// The font's texture (PS2 name). Zero-initialized, so not hashed: it comes after the hashed globals in .bss, after the
// consoles' vector, whose bucket (845) is higher than its own (729)
// GLOBAL: GAME 0x646b38
ITexture *fonty = NULL;

// FUNCTION: GAME 0x4f6920
void CreateFontTexture(ITexture **texture)
{
    *texture = NULL;
    // STRING: GAME 0x634d14
    if (igraph->CreateITexture("editfont.bmp", "editfont.bmp", ITC_USE_ALPHA, texture) < I3D_OK)
        return;
    memset(fnt_matrix, 0, sizeof(fnt_matrix));
    // STRING: GAME 0x634d04
    int file = dtaOpen("fnt_matrix.bin", false);
    if (file == DTA_INVALID_HANDLE)
        return;
    dtaRead(file, fnt_matrix, sizeof(fnt_matrix));
    dtaClose(file);
}

// FUNCTION: GAME 0x4f6980
void ConsolesInit()
{
    CreateFontTexture(&fonty);
}

// The texture isn't cleared, as PS2's isn't
// FUNCTION: GAME 0x4f6990
void ConsolesDone()
{
    if (fonty)
        fonty->Release();
}
