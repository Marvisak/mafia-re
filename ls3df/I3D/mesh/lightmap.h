#pragma once

// The lightmaps' textures (PS2 lightmap.cpp; lightmap.cpp): a lit object's lightmaps are rectangles of texels, which
// LightMap packs into shared textures. The textures with room left are in the file's list of TEXBL_STRUCTs, each with
// its free rectangles (lightmap.cpp defines the structure). PS2 types, member names guessed, LightMap's in the m_ style
// of litLod's (the lit object file's messages name those).

#include "ls3df.h"

class ITexture;

// A lightmap's rectangle of texels (PS2 RECTANGLE_INFO, 0x1c bytes). The lit object computes data; m_CreateLM packs
// the rectangle into a texture, converts its texels and sets u and v.
struct RECTANGLE_INFO {
    short sx, sy;                       // 0x00, data's size
    short tex_sx, tex_sy;               // 0x04, its size in the texture (m_CreateLM may halve it, m_Resize)
    int area;                           // 0x08, tex_sx * tex_sy
    float u, v;                         // 0x0c, its corner in the texture; u is 2 more for a rotated rectangle
    uchar *data;                        // 0x14, sx * sy RGB texels; a 3x3 rectangle's single color instead
    void *tex_data;                     // 0x18, the texels in the texture's format (m_ConvertRGB)
};

// A free rectangle of a texture (PS2 RECT_STRUCT, 0x1c bytes), in its TEXBL_STRUCT's list, sorted by area
struct RECT_STRUCT {
    int area;                           // 0x00
    RECT rc;                            // 0x04, its corner and, in right and bottom, its size
    RECT_STRUCT *prev;                  // 0x14
    RECT_STRUCT *next;                  // 0x18
};

// Where in its texture a rectangle is (PS2 OFFSET_STRUCT, 0xc bytes)
struct OFFSET_STRUCT {
    int x, y;                           // 0x00
    bool rotated;                       // 0x08, its x and y swapped
};

struct TEXBL_STRUCT;

// A lit object level's lightmaps (PS2 LightMap): their texture, where each rectangle is in it, and the rectangles,
// the largest first
class LightMap {
public:
    LightMap();
    ~LightMap();

    // PS2 names
    bool m_ConvertRGB(ushort sx, ushort sy, void *src, void **dst);
    bool m_AddRectToRectList(TEXBL_STRUCT *tb, RECT *rc);
    bool m_ReplaceRectFromRectList(TEXBL_STRUCT *tb, RECT_STRUCT *r);
    void m_RemoveRectFromRectList(TEXBL_STRUCT *tb, RECT_STRUCT *r);
    void m_EraseRectList(TEXBL_STRUCT *tb);
    bool m_DuplicateRectList(TEXBL_STRUCT *src, TEXBL_STRUCT *dst);
    int m_AddTexture(TEXBL_STRUCT *tb, RECTANGLE_INFO *ri);
    void m_FillLineWord(ushort **src, ushort **dst, int num, uint pitch);
    void m_FillLineRGB(uchar **src, uchar **dst, int num, uint pitch);
    void m_FillLineRGB(uchar **src, uchar **r, uchar **g, uchar **b, int num, uint pitch);
    bool m_Resize(RECTANGLE_INFO *ri, int sx, int sy, uchar **dst);
    bool m_CreateLM(ushort *indices, RECTANGLE_INFO *rects, int num, uint *mem);

    ITexture *m_pTexture;               // 0x00
    C_list<OFFSET_STRUCT> m_OffsetList; // 0x04, of the rectangles in order
    RECTANGLE_INFO *m_pRects;           // 0x14, the lit object's
    C_list<ushort> m_OrderList;         // 0x18, m_pRects' indices, the largest first
    int m_Area;                         // 0x28, of all of them
};

// The destructor lightmap.cpp's thunk registers for its list of textures: the seventh $E5 in link order, after the
// target file's
// SYNTHETIC: LS3DF 0x1005d040
// $E5

// TEXBL_STRUCT's constructor and destructor, out of line for the vector constructor and destructor iterators
// SYNTHETIC: LS3DF 0x1005da50
// TEXBL_STRUCT::TEXBL_STRUCT

// SYNTHETIC: LS3DF 0x1005da70
// TEXBL_STRUCT::~TEXBL_STRUCT

// The lists' out-of-line templates, at the file's end
// TEMPLATE: LS3DF 0x1005fc90
// C_list<TEXBL_STRUCT>::~C_list<TEXBL_STRUCT>

// TEMPLATE: LS3DF 0x1005fcd0
// ?push_back@?$C_list@UTEXBL_STRUCT@@@@QAEXABUTEXBL_STRUCT@@@Z

// SYNTHETIC: LS3DF 0x1005ff70
// TEXBL_STRUCT::`vector deleting destructor'

// SYNTHETIC: LS3DF 0x1005ffd0
// TEXBL_STRUCT::operator=

// TEMPLATE: LS3DF 0x100600d0
// C_list<OFFSET_STRUCT>::operator=

// TEMPLATE: LS3DF 0x10060190
// ?push_back@?$C_list@UTEXBL_STRUCT@@@@QAEAAUTEXBL_STRUCT@@XZ
