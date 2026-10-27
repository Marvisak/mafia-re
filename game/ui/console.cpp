// The console's text (PS2 console.cpp; the PC's file is most likely the one at 0x4f5dc0-0x4f6a2c, which loads
// editfont.bmp and fnt_matrix.bin, docs/game-files.md). Written so far: the consoles' vector (PS2 _consoles, whose
// initializer starts the file), C_console's constructor, destructor and Clear, ConsoleReg, ConsoleUnReg, the font,
// CreateFontTexture, ConsolesInit and ConsolesDone. Not written: C_console's tick (0x4f5f50) and text drawing
// (0x4f6040), ConsoleRender (most likely 0x4f6850).

#include "ui/console.h"

#include <string.h>

#include "IGraph/IGraph.h"
#include "rw_data.h"

// The registered consoles, which ConsoleRender (0x4f6850) draws (PS2's name, static as PS2's; a global vector, which
// VC6 assumes a call may change, leaves ConsoleReg's push_back a test it folds here). Defined first: its initializer
// starts the file.
// GLOBAL: GAME 0x646b28
static std::vector<S_con_register> _consoles;

// The font's characters by code, as fnt_matrix.bin has them (PS2 name; nothing else reads the file)
// GLOBAL: GAME 0x645728
S_font_char fnt_matrix[256];

// The font's texture (PS2 name). Zero-initialized, so not hashed: it comes after the hashed globals in .bss, after the
// consoles' vector, whose bucket (845) is higher than its own (729)
// GLOBAL: GAME 0x646b38
ITexture *fonty = NULL;

// The vector of no lines is the one of 0 elements, which allocates (PS2's constructs it with 0 too)
// FUNCTION: GAME 0x4f5e20
C_console::C_console() : text_lines(0)
{
    r = 0xff;
    g = 0xff;
    b = 0xff;
    x = .1f;
    y = .1f;
    sx = .1f;
    sy = .11f;
    lines = 100;
    time = 1000;
    unknown_38 = false;
    unknown_39 = false;
    unknown_3a = false;
}

// FUNCTION: GAME 0x4f5e80
C_console::~C_console()
{
    Clear();
}

// FUNCTION: GAME 0x4f5ed0
void C_console::Clear()
{
    for (int i = text_lines.size(); i--; )
        delete[] text_lines[i].text;
    text_lines.clear();
    unknown_34 = 0;
}

// A console registered again counts one more (PS2's flag, which VC6 threads into a return)
// FUNCTION: GAME 0x4f6590
void ConsoleReg(C_console *console, uint slot)
{
    bool add = true;
    for (int i = _consoles.size(); i--; )
    {
        if (_consoles[i].console == console)
        {
            _consoles[i].count++;
            add = false;
            break;
        }
    }
    if (add)
    {
        S_con_register reg;
        reg.console = console;
        reg.slot = slot;
        reg.count = 1;
        _consoles.push_back(reg);
    }
}

// FUNCTION: GAME 0x4f67a0
void ConsoleUnReg(C_console *console)
{
    for (int i = _consoles.size(); i--; )
    {
        if (_consoles[i].console == console)
        {
            _consoles[i].count--;
            if (!_consoles[i].count)
            {
                _consoles[i].console->Clear();
                _consoles.erase(_consoles.begin() + i);
            }
            break;
        }
    }
}

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
