// The lines drawn over each frame (PC only; the file and its names are ours, the file at 0x5d6e50-0x5d6f7c between the
// collisions' and the car encyclopedia's, docs/game-files.md): AddLine, which the explosions' debris calls, DrawLines and
// ClearLines, which the scene's callback calls, all of it. DrawLines passes the file's copy of the engine header's
// identity matrix (0x625f30, one of the original's five).

#include "g_lines.h"

#include "IGraph/IGraph.h"
#include "I3D/I3D.h"

// The lines' ends, two vertices a line, and their count, which leave .bss in this order by their buckets (791 and 851)
// GLOBAL: GAME 0x648248
static LVERTEX line_verts[MAX_LINES * 2];
// GLOBAL: GAME 0x6bd548
static uint num_lines;
// Where AddLine writes the next line
// GLOBAL: GAME 0x637f90
static LVERTEX *line_end = line_verts;

// The ends' texture coordinates stay as they are. VC6 copies the line_end++ after the if/else into each branch.
// FUNCTION: GAME 0x5d6e50
void AddLine(const S_vector &from, const S_vector &to, uint color, uint color_to)
{
    if (num_lines >= MAX_LINES)
        return;
    num_lines++;
    *(S_vector *)line_end = from;
    line_end->color = color;
    line_end++;
    *(S_vector *)line_end = to;
    if (color_to)
        line_end->color = color_to;
    else
        line_end->color = color;
    line_end++;
}

// FUNCTION: GAME 0x5d6ee0
void DrawLines()
{
    if (!num_lines)
        return;
    igraph->SetTexture(NULL);
    igraph->SetWorldMatrix(IdentityMatrix);
    igraph->DrawPrimitiveList(PT_LINELIST, num_lines, line_verts, LS3D_STREAM_LVERTEX);
}

// FUNCTION: GAME 0x5d6f30
void ClearLines()
{
    num_lines = 0;
    line_end = line_verts;
}
