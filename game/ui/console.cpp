// The console's text (PS2 console.cpp; the PC's file is most likely the one at 0x4f5dc0-0x4f6a2c, which loads
// editfont.bmp and fnt_matrix.bin, docs/game-files.md). Written so far: the consoles' vector (PS2 _consoles, whose
// initializer starts the file), C_console's constructor, destructor, Clear and Render, ConsoleReg, ConsoleUnReg,
// ConsoleRender, the font, CreateFontTexture, ConsolesInit and ConsolesDone. Not written: C_console's tick (0x4f5f50).

#include "ui/console.h"

#include <string.h>

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"
#include "rw_data.h"

// The registered consoles, which ConsoleRender draws (PS2's name, static as PS2's; a global vector, which VC6 assumes a
// call may change, leaves ConsoleReg's push_back a test it folds here). Defined first: its initializer starts the file.
// GLOBAL: GAME 0x646b28
static std::vector<S_con_register> _consoles;

// Whether ConsoleRender draws the consoles registered without CONREG_RENDER (PS2 name; on PS2 too nothing turns it off)
// GLOBAL: GAME 0x634d00
int _is_on_ = 1;

// The size of the consoles' vertex buffer: 166 characters' quads (the name is ours)
#define CONSOLE_MAX_VERTS 1000

// The consoles' characters waiting to be drawn, two triangles each, and their count (PS2 names; PS2's are LVERTEX-like
// vertices of 28 bytes). The buffer is drawn when a line would overflow it and at the end of each console's Render.
// GLOBAL: GAME 0x63da20
int __vert_count;
// GLOBAL: GAME 0x63da28
TLVERTEX __vert[CONSOLE_MAX_VERTS];

// The font's characters by code, as fnt_matrix.bin has them (PS2's fnt_matrix). Not PS2's name on the PC: with it the
// hash would put the table first in .bss (bucket 105), ahead of the vertex buffer (594 and 607), where the original
// has it after (fnt_table, the name is ours, 663).
// GLOBAL: GAME 0x645728
S_font_char fnt_table[256];

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
    upwards = false;
    right_aligned = false;
    reversed = false;
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

// One vertex of a character's quad (the name is ours). Through an inline function's parameters, as the original's
// texture coordinates go through the FPU, which VC6 copies with integer moves where they are assigned directly.
static inline void SetCharVertex(int i, float x, float y, uint color, float u, float v)
{
    __vert[i].x = x;
    __vert[i].y = y;
    __vert[i].z = 0.0f;
    __vert[i].rhw = 1.0f;
    __vert[i].color = color;
    __vert[i].u = u;
    __vert[i].v = v;
}

// The lines from y down (or from a line above it up), each until one would leave the screen; a line with a place of its
// own takes none. Each character is a quad in the font's texture, its width the character's advance; one without
// width is a gap of 0.4 of the height. The constants of the color's alpha and the gap's sign fold into the code: 0xff
// into r (add edi, 0xffffff00), the subtraction into a product by -0.4. The characters' loop is a while (true) with
// the test first, which VC6 doesn't rotate (the original jumps back to the test from both steps).
// Open: the registers (ours spills this to use ebx in the characters' loop, keeps the line's offset in esi where the
// original keeps it on the stack and the length in esi), the constants' registers in the vertices (the original's z
// from a zero register, rhw from ecx after the first), and the character's width, which VC6 reuses from the zero test
// where the original multiplies again.
// FUNCTION: GAME 0x4f6040
void C_console::Render()
{
    int base_x = (int)(igraph->Scrn_sx() * x);
    float scrn_sy = (float)igraph->Scrn_sy();
    float line_y = y;
    if (upwards)
        line_y -= sy;
    int first = reversed ? text_lines.size() - 1 : 0;
    int last = reversed ? -1 : text_lines.size();
    int step = reversed ? -1 : 1;
    for (int i = first; i != last; i += step)
    {
        if (line_y >= 1.0f || line_y <= 0.0f)
            continue;
        int len = strlen(text_lines[i].text);
        float size = scrn_sy * sx;
        int r = text_lines[i].r == -1 ? this->r : text_lines[i].r;
        int g = text_lines[i].g == -1 ? this->g : text_lines[i].g;
        int b = text_lines[i].b == -1 ? this->b : text_lines[i].b;
        uint color = 0xff000000 + (r << 16) + (g << 8) + b;
        if (__vert_count + len * 6 > CONSOLE_MAX_VERTS)
        {
            igraph->DrawPrimitiveList(PT_TRIANGLELIST, __vert_count / 3, __vert, LS3D_STREAM_TLVERTEX);
            __vert_count = 0;
        }
        int cx = base_x;
        float cy = line_y * scrn_sy;
        int j = right_aligned ? len - 1 : 0;
        bool advance = true;
        if (text_lines[i].x > 0.0f)
        {
            advance = false;
            cx = (int)(igraph->Scrn_sx() * text_lines[i].x);
            cy = scrn_sy * text_lines[i].y;
        }
        while (true)
        {
            if (j >= len || j < 0)
                break;
            char c = text_lines[i].text[j];
            if (size * fnt_table[c].width == 0.0f)
            {
                if (right_aligned)
                    cx -= (int)(size * .4);
                else
                    cx += (int)(size * .4);
            }
            else
            {
                int w = (int)(size * fnt_table[c].width);
                if (right_aligned)
                    cx -= w;
                SetCharVertex(__vert_count, (float)cx, cy, color, fnt_table[c].left, fnt_table[c].top);
                __vert_count++;
                SetCharVertex(__vert_count, (float)(cx + w), cy + size, color, fnt_table[c].right, fnt_table[c].bottom);
                __vert_count++;
                SetCharVertex(__vert_count, (float)cx, cy + size, color, fnt_table[c].left, fnt_table[c].bottom);
                __vert_count++;
                SetCharVertex(__vert_count, (float)cx, cy, color, fnt_table[c].left, fnt_table[c].top);
                __vert_count++;
                SetCharVertex(__vert_count, (float)(cx + w), cy, color, fnt_table[c].right, fnt_table[c].top);
                __vert_count++;
                SetCharVertex(__vert_count, (float)(cx + w), cy + size, color, fnt_table[c].right, fnt_table[c].bottom);
                __vert_count++;
                if (!right_aligned)
                    cx += w;
            }
            if (right_aligned)
                j--;
            else
                j++;
        }
        if (advance)
        {
            if (upwards)
                line_y -= sy;
            else
                line_y += sy;
        }
    }
    if (__vert_count)
    {
        igraph->DrawPrimitiveList(PT_TRIANGLELIST, __vert_count / 3, __vert, LS3D_STREAM_TLVERTEX);
        __vert_count = 0;
    }
}

// A console registered again counts one more (PS2's flag, which VC6 threads into a return)
// FUNCTION: GAME 0x4f6590
void ConsoleReg(C_console *console, uint flags)
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
        reg.flags = flags;
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

// The font's texture is set once for all of them. The texture is tested again where the consoles are drawn, as PS2's.
// FUNCTION: GAME 0x4f6850
void ConsoleRender()
{
    if (!fonty)
        return;
    int use_zb = driver->GetState(RS_USEZB);
    driver->SetState(RS_USEZB, false);
    igraph->SetState(DXS_ALPHA, ALPHASTATE_TEST);
    igraph->SetTexture(fonty);
    if (fonty)
    {
        for (int i = _consoles.size(); i--; )
        {
            if (_is_on_ || _consoles[i].flags & CONREG_RENDER)
                _consoles[i].console->Render();
            if (_consoles[i].flags & CONREG_CLEAR)
                _consoles[i].console->Clear();
        }
    }
    driver->SetState(RS_USEZB, use_zb);
}

// FUNCTION: GAME 0x4f6920
void CreateFontTexture(ITexture **texture)
{
    *texture = NULL;
    // STRING: GAME 0x634d14
    if (igraph->CreateITexture("editfont.bmp", "editfont.bmp", ITC_USE_ALPHA, texture) < I3D_OK)
        return;
    memset(fnt_table, 0, sizeof(fnt_table));
    // STRING: GAME 0x634d04
    int file = dtaOpen("fnt_matrix.bin", false);
    if (file == DTA_INVALID_HANDLE)
        return;
    dtaRead(file, fnt_table, sizeof(fnt_table));
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
