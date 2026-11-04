// The last file of the menus' components (PS2 g_m_compsm.cpp, which has the big static texts, the carousel and the
// color selection; the PC's file is most likely the one at 0x5dfdb0-0x5e063c, docs/game-files.md): the factory of the
// PC's grid ('grid'), then its class's functions written here, out of line, and the copies of its in-class functions
// (g_m_compsm.h) after the factory's Make. Written whole.

// No precompiled header (#pragma hdrstop before the includes): the copies of each class's in-class functions follow
// its factory's Make, as the original's do, where under /YX they came after all the factories' functions.
#pragma hdrstop

#include "ui/g_m_compsm.h"

// The factory (the name and its class's are ours, after the type)
// GLOBAL: GAME 0x6bd79c
G_Factory<GM_Grid> GM_GridFactory;

// FUNCTION: GAME 0x5dfe00
void GM_Grid::Create(uint rows, uint cols, float size, uint color)
{
    size *= GM_Menu::menuScreenScale.y;
    if (cells)
        delete[] cells;
    if (row_heights)
        delete[] row_heights;
    if (col_widths)
        delete[] col_widths;
    this->cols = cols;
    this->rows = rows;
    col_widths = new float[cols];
    uint i;
    for (i = 0; i < cols; i++)
        col_widths[i] = sx / cols;
    row_heights = new float[rows];
    for (i = 0; i < rows; i++)
        row_heights[i] = size;
    cells = new GM_GridCell[rows * cols];
    for (i = 0; i < rows; i++)
        for (uint j = 0; j < cols; j++)
        {
            GM_GridCell *cell = &cells[i * cols + j];
            strncpy(cell->text, (char *)text, GRID_CELL_TEXT_SIZE);
            cell->size = size;
            cell->color = color;
            cell->flags = flags & (COMPF_FONT | COMPF_TEXT_RIGHT | COMPF_TEXT_CENTER);
        }
    UpdateScrollMax();
}

// FUNCTION: GAME 0x5dff60
void GM_Grid::Draw(uint draw_flags)
{
    float width;
    if (scroll_max <= 0.0f)
    {
        width = sx;
        SetViewport((int)x, (int)y, (int)(sx + 1.0f), (int)(sy + 1.0f));
    }
    else
    {
        width = sx - bar_width;
        float thumb_y = scroll / scroll_max * (sy - bar_width * 3.0f);
        if (flags & COMPF_ACTIVE)
            RectangleFilled(x + sx - bar_width, y + bar_width + thumb_y, bar_width, bar_width, 0xffff0000, NULL);
        else
            RectangleFilled(x + sx - bar_width, y + bar_width + thumb_y, bar_width, bar_width, 0xff7f7f7f, NULL);
        Rectangle(x + sx - bar_width, y, bar_width, sy, 0xff000000);
        Rectangle(x + sx - bar_width, y + bar_width, bar_width, sy - bar_width * 2, 0xff000000);
        SetViewport((int)x, (int)y, (int)(sx - bar_width), (int)(sy + 1.0f));
    }
    uint i;
    float cy;
    if (flags & COMPF_ALTERNATE_COLOR)
    {
        cy = y - scroll;
        for (i = 1; i < rows; i += 2)
        {
            cy += row_heights[i - 1];
            RectangleFilled(x, cy, width, row_heights[i], 0x7f7f7f7f, NULL);
            cy += row_heights[i];
        }
    }
    float cx;
    if (flags & COMPF_ALTERNATE_STYLE)
    {
        Rectangle(x, y, sx, sy, 0xff000000);
        cy = y - scroll;
        for (i = 1; i < rows; i++)
        {
            cy += row_heights[i - 1];
            Line(x, cy, width, 0.0f, 0xff000000);
        }
        cx = x;
        for (i = 1; i < cols; i++)
        {
            cx += col_widths[i - 1];
            Line(cx, y, 0.0f, sy, 0xff000000);
        }
    }
    cx = x;
    for (uint col = 0; col < cols; col++)
    {
        cy = y - scroll;
        if (cx > x + width)
            break;
        if (cx + col_widths[col] < x + width)
            SetViewport((int)cx, (int)y, (int)col_widths[col], (int)sy);
        else
        {
            float rest = x + width - cx;
            if (rest < 2.0f)
                break;
            SetViewport((int)cx, (int)y, (int)rest, (int)sy);
        }
        for (uint row = 0; row < rows; row++)
        {
            if (cy + row_heights[row] >= y)
            {
                GM_GridCell *cell = &cells[row * cols + col];
                if (cell->flags & COMPF_TEXT_CENTER)
                    mafiaIndicators.OutText(cell->text, cx, cy, col_widths[col], cell->size, cell->color,
                        OUTTEXT_CENTER, cell->flags & COMPF_FONT, NULL);
                else if (cell->flags & COMPF_TEXT_RIGHT)
                    mafiaIndicators.OutText(cell->text, cx, cy, col_widths[col], cell->size, cell->color, OUTTEXT_RIGHT,
                        cell->flags & COMPF_FONT, NULL);
                else
                    mafiaIndicators.OutText(cell->text, cx, cy, 0.0f, cell->size, cell->color, 0,
                        cell->flags & COMPF_FONT, NULL);
            }
            cy += row_heights[row];
            if (cy > y + sy)
                break;
        }
        cx += col_widths[col];
    }
    SetViewport(0, 0, igraph->Scrn_sx(), igraph->Scrn_sy());
}

// FUNCTION: GAME 0x5e0340
void GM_Grid::UpdateScrollMax()
{
    float height = 0.0f;
    for (uint i = 0; i < rows; i++)
        height += row_heights[i];
    scroll_max = height - sy;
}
