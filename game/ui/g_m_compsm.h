#pragma once

// The class of g_m_compsm.cpp's component, the PC's grid ('grid'). In a header of its own, which only its file includes.

#include "ui/g_m_comps.h"

#include <string.h>

// A grid's cell (the names are ours), 0x4c bytes
#define GRID_CELL_TEXT_SIZE 0x40
struct GM_GridCell
{
    char text[GRID_CELL_TEXT_SIZE];
    uint color;                         // 0x40
    float size;                         // 0x44, the text's height
    uint flags;                         // 0x48, the grid's COMPF_FONT, COMPF_TEXT_RIGHT and COMPF_TEXT_CENTER
};

// A grid (PC only, the name is ours, after its type), 'grid': rows of cells, each a text in its color, height, font
// and alignment, 5 by 5 to start (Create), with a scroll bar along its right edge as the scrolled window's, while the
// rows are higher than the grid. In the alternate color every second row is grey, in the alternate style the cells have
// black outlines. Its value is the scroll, 0 to the rows' height less its own: the mouse's left button moves the
// thumb's middle to the mouse (captured), Up and Down by 10. It can be activated. Its copy has no cells.
// VTABLE: GAME 0x627290
class GM_Grid : public GM_Component
{
public:
    GM_Grid(GM_DesignStruct *design, GM_Component *parent) : GM_Component(design, parent)
    {
        flags |= COMPF_ACTIVATABLE | COMPF_MOUSE_CAPTURE;
        scroll = 0.0f;
        scroll_max = 100.0f;
        cells = NULL;
        col_widths = NULL;
        row_heights = NULL;
        Create(5, 5, 20.0f, 0xff7fff7f);
        bar_width = GM_Menu::menuScreenScale.x * SCROLL_BAR_WIDTH;
    }
    GM_Grid(GM_Component *component) : GM_Component(component)
    {
        bar_width = GM_Menu::menuScreenScale.x * SCROLL_BAR_WIDTH;
    }

    // The cells of that many rows and columns, each with the grid's text and its font and alignment, in the color and
    // of the size on the 800x600 screen: the columns share the grid's width, the rows are the size high
    void Create(uint rows, uint cols, float size, uint color);
    // The scroll's range: the rows' height less the grid's
    void UpdateScrollMax();

    virtual uint GetType() { return 'grid'; }
    // The original kept GM_WindowClean's copy
    virtual bool CanBeParent() { return true; }
    virtual void GetClientOrigin(float &x, float &y)
    {
        x = this->x;
        y = this->y - scroll;
    }
    // Over the bar
    virtual bool IsOverActive(float x, float y)
    {
        x -= this->x + sx - bar_width;
        y -= this->y;
        if (x < 0.0f || x > bar_width || y < 0.0f || y > sy)
            return false;
        return true;
    }
    // The scroll, as it is
    virtual void SetFloat(float value) { scroll = value; }
    // The bar where the rows are higher than the grid, the rows' stripes and the outlines, then each column's cells
    // inside the column (the viewport set to it) as far as the grid shows them
    virtual void Draw(uint draw_flags);
    // Never clicks
    virtual uint Mouse(int x, int y, int z, uint buttons, uint time)
    {
        if (buttons & MENUMOUSE_LEFT)
        {
            scroll = (y - this->y - bar_width * 1.5f) / (sy - bar_width * 3.0f) * scroll_max;
            if (scroll < 0.0f)
                scroll = 0.0f;
            if (scroll > scroll_max)
                scroll = scroll_max;
        }
        return 0;
    }
    // Never clicks
    virtual uint Key(uint shift_keys, uint key)
    {
        if (key == DIK_UP)
            scroll -= 10.0f;
        else if (key == DIK_DOWN)
            scroll += 10.0f;
        if (scroll < 0.0f)
            scroll = 0.0f;
        if (scroll > scroll_max)
            scroll = scroll_max;
        return 0;
    }

    // The names are ours
    uint rows, cols;                    // 0x40
    GM_GridCell *cells;                 // 0x48, the rows' cells, one row after the other
    float *row_heights;                 // 0x4c
    float *col_widths;                  // 0x50
    float scroll_max;                   // 0x54, the range's maximum (not max, windows.h's macro)
    float scroll;                       // 0x58
    float bar_width;                    // 0x5c, SCROLL_BAR_WIDTH on the screen
};

// The copies of the class's functions, by name, as g_m_comps.h's
// FUNCTION: GAME 0x5e0420
// ?GetType@GM_Grid@@UAEIXZ

// FUNCTION: GAME 0x5e0430
// ?GetClientOrigin@GM_Grid@@UAEXAAM0@Z

// FUNCTION: GAME 0x5e0450
// ?IsOverActive@GM_Grid@@UAE_NMM@Z

// FUNCTION: GAME 0x5e04b0
// ?SetFloat@GM_Grid@@UAEXM@Z

// FUNCTION: GAME 0x5e04c0
// ?Mouse@GM_Grid@@UAEIHHHII@Z

// FUNCTION: GAME 0x5e0520
// ?Key@GM_Grid@@UAEIII@Z

// The factory's functions
// TEMPLATE: GAME 0x5e0360
// ?Make@?$G_Factory@VGM_Grid@@@@SIPAVGM_Component@@PAUGM_DesignStruct@@PAV2@@Z

// TEMPLATE: GAME 0x5e05a0
// ?MakeDup@?$G_Factory@VGM_Grid@@@@SIPAVGM_Component@@PAV2@@Z
