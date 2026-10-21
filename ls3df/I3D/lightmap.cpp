// The lightmaps' textures (PS2 lightmap.cpp), at 0x1005d010-0x10060390 in the original: the list of the textures with
// room left, then LightMap in PS2's order (m_ConvertRGB, the constructor and destructor, the free rectangles' lists,
// m_AddTexture, m_FillLineRGB, m_Resize and m_CreateLM), with the header's zero vector's $E (0x1005e380) in the middle,
// as in I3d_math.cpp, and the lists' out-of-line templates at the end. Names are the PS2 build's unless noted.
// LightMap packs a lit object level's lightmaps into a texture of 128 to 256 texels a side, shared with other levels'
// while it has room: each texture's free room is a list of free rectangles, sorted by area, which m_AddTexture takes
// the first fitting one of and splits.
//
// Not matching yet: the zero vector's $E comes after m_CreateLM in ours. An unused function of 25 to 55 statements
// before m_FillLineRGB puts it at the original's place (tools/autopch.py), so the original's source up to there was
// that much longer; PS2's FreeTexBlList is in it already.
#include "I3D/lightmap.h"
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

#include <math.h>
#include <string.h>

// IGraph.cpp's texture formats
extern D3DFORMAT m_TrueColorTexturePixelFormat;
extern D3DFORMAT m_Alpha1TexturePixelFormat;
extern uint m_MaxTextureWidth;
extern uint m_MaxTextureHeight;

// A texture's free room (PS2 TEXBL_STRUCT, 0x28 bytes): its free rectangles, and the rectangles packed into it.
// Defined here: the copy of its constructor follows the destructor, its first user, where a precompiled header's
// inline function would come later (tools/autopch.py).
struct TEXBL_STRUCT {
    short sx, sy;                       // 0x00, the texture's size
    int free_area;                      // 0x04, the free rectangles' area
    int lost_area;                      // 0x08, of the rectangles too thin to use (less than 3 texels)
    RECT_STRUCT *first;                 // 0x0c, the free rectangles, the smallest first
    RECT_STRUCT *last;                  // 0x10
    C_list<OFFSET_STRUCT> offsets;      // 0x14, of the rectangles packed while m_CreateLM tries it
    ITexture *texture;                  // 0x24, NULL until created

    TEXBL_STRUCT() : offsets(10) {}
};

// The textures with room left, sorted by their free area, the smallest first (PS2's static)
// GLOBAL: LS3DF 0x101b5128
static C_list<TEXBL_STRUCT> GlobalRemainTexBlList(3);

// Frees the list, adding the memory it held to mem. PS2's, which calls it; on PC nothing does, and the linker drops it.
void FreeTexBlList(int *mem)
{
    for (int i = 0; i < GlobalRemainTexBlList.size(); i++) {
        *mem += GlobalRemainTexBlList[i].offsets.size() * sizeof(OFFSET_STRUCT) + sizeof(TEXBL_STRUCT);
        GlobalRemainTexBlList[i].offsets.clear();
        RECT_STRUCT *r = GlobalRemainTexBlList[i].first;
        while (r) {
            *mem += sizeof(RECT_STRUCT);
            RECT_STRUCT *next = r->next;
            delete r;
            r = next;
        }
    }
    GlobalRemainTexBlList.clear();
}

// A pixel's error, which the dithering passes on to its neighbours (PC only, the name is a guess)
struct S_dither_error {
    float r, g, b;
};

// Converts a rectangle's texels to the lightmaps' texture format: copied for the 24 and 32-bit formats, to 16 bits
// for the 1-5-5-5 ones, dithered with RS_LMDITHER. PS2's only copies them. A 3x3 rectangle keeps its single color.
// Not matching yet: the original keeps sx in bx and sy in bp, and the dithering's 16-bit color is computed otherwise.
// FUNCTION: LS3DF 0x1005d090
bool LightMap::m_ConvertRGB(ushort sx, ushort sy, void *src, void **dst)
{
    if (sx == 3 && sy == 3)
        return true;
    D3DFORMAT fmt;
    if (renderState[RS_LMTRUECOLOR] && m_TrueColorTexturePixelFormat)
        fmt = m_TrueColorTexturePixelFormat;
    else
        fmt = m_Alpha1TexturePixelFormat;
    S_dither_error *err = NULL;
    if ((fmt == D3DFMT_X1R5G5B5 || fmt == D3DFMT_A1R5G5B5) && renderState[RS_LMDITHER]) {
        int n = (sx + 2) * (sy + 1);
        err = new S_dither_error[n];
        memset(err, 0, n * sizeof(S_dither_error));
    }
    uchar *s = (uchar *)src;
    switch (fmt) {
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        *dst = new ushort[sy * sx];
        if (renderState[RS_LMDITHER]) {
            for (int y = 0; y < sy; y++) {
                for (int x = 0; x < sx; x++) {
                    S_dither_error &e = err[y * (sx + 2) + x + 1];
                    float r = *s++ + e.r;
                    float g = *s++ + e.g;
                    float b = *s++ + e.b;
                    float er = (float)(fmod(r, 8.0) / 16);
                    float eg = (float)(fmod(g, 8.0) / 16);
                    float eb = (float)(fmod(b, 8.0) / 16);
                    S_dither_error &e1 = err[y * (sx + 2) + x + 2];
                    e1.r += er * 7.0f;
                    e1.g += eg * 7.0f;
                    e1.b += eb * 7.0f;
                    S_dither_error &e2 = err[(y + 1) * (sx + 2) + x];
                    e2.r += er * 3.0f;
                    e2.g += eg * 3.0f;
                    e2.b += eb * 3.0f;
                    S_dither_error &e3 = err[(y + 1) * (sx + 2) + x + 1];
                    e3.r += er * 5.0f;
                    e3.g += eg * 5.0f;
                    e3.b += eb * 5.0f;
                    S_dither_error &e4 = err[(y + 1) * (sx + 2) + x + 2];
                    e4.r += er;
                    e4.g += eg;
                    e4.b += eb;
                    if (r > 255.0f)
                        r = 255.0f;
                    if (g > 255.0f)
                        g = 255.0f;
                    if (b > 255.0f)
                        b = 255.0f;
                    int ir = FloatToInt(r);
                    int ig = FloatToInt(g);
                    int ib = FloatToInt(b);
                    ((ushort *)*dst)[y * sx + x] =
                        0x8000 | ((ir >> 3) & 0x1f) << 10 | ((ig >> 3) & 0x1f) << 5 | ((ib >> 3) & 0x1f);
                }
            }
        } else {
            for (int y = 0; y < sy; y++) {
                for (int x = 0; x < sx; x++) {
                    uchar r = *s++;
                    uchar g = *s++;
                    uchar b = *s++;
                    ((ushort *)*dst)[y * sx + x] = (b >> 3) | (r >> 3) << 10 | 0x8000 | (g >> 3) << 5;
                }
            }
        }
        break;
    case D3DFMT_R8G8B8:
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_L8:
        *dst = new uchar[sx * sy * 3];
        memcpy(*dst, src, sx * sy * 3);
        break;
    default:
        delete[] err;
        return false;
    }
    delete[] err;
    return true;
}

// FUNCTION: LS3DF 0x1005d520
LightMap::LightMap() : offsets(10), order(64)
{
    texture = NULL;
    rects = NULL;
}

// Releases the texture. Where other levels still use it, its rectangles become free room again, filled white; where
// not, the texture leaves the driver's list and its free room the file's list. The offsets are read without a
// reference: the calls are pending inline calls, which keep m_EraseRectList's clear() out of line and give the
// insertion's copies the budget to inline TEXBL_STRUCT's assignment.
// Not matching yet: push_back's store of the size (ls3df.h) and the inline budget. With the original's push_back, the
// original also inlines the vector destructor of push_back's grow, which two dead statements more (if (0) area = 0;)
// give exactly, anywhere before it; no source for them is known.
// FUNCTION: LS3DF 0x1005d550
LightMap::~LightMap()
{
    if (texture) {
        if (texture->Release() <= 0) {
            pI3DDriver->textures.remove(texture);
            pI3DDriver->m_RecomputeUsedVRAM();
            for (int i = 0; i < GlobalRemainTexBlList.size(); i++) {
                if (GlobalRemainTexBlList[i].texture == texture) {
                    m_EraseRectList(&GlobalRemainTexBlList[i]);
                    GlobalRemainTexBlList.erase_ordered(i);
                    break;
                }
            }
        } else {
            int i = GlobalRemainTexBlList.size();
            while (i--) {
                if (GlobalRemainTexBlList[i].texture == texture)
                    break;
            }
            if (i > -1) {
                TEXBL_STRUCT tb;
                memcpy(&tb, &GlobalRemainTexBlList[i], sizeof(tb));
                GlobalRemainTexBlList.erase_ordered(i);
                for (int j = 0; j < order.size(); j++) {
                    RECT rc;
                    if (offsets[j].rotated)
                        SetRect(&rc, offsets[j].x, offsets[j].y, rects[order[j]].tex_sy, rects[order[j]].tex_sx);
                    else
                        SetRect(&rc, offsets[j].x, offsets[j].y, rects[order[j]].tex_sx, rects[order[j]].tex_sy);
                    m_AddRectToRectList(&tb, &rc);
                    rc.right += rc.left;
                    rc.bottom += rc.top;
                    texture->Fill(&rc, 0xffffffff, 0);
                }
                bool inserted = false;
                for (i = 0; i < GlobalRemainTexBlList.size(); i++) {
                    if (GlobalRemainTexBlList[i].free_area >= tb.free_area) {
                        GlobalRemainTexBlList.insert(i, tb);
                        inserted = true;
                        break;
                    }
                }
                if (!inserted)
                    GlobalRemainTexBlList.push_back(tb);
            }
        }
    }
}

// Adds a free rectangle (its size in right and bottom), sorted by its area; one less than 3 texels wide or high is lost
// FUNCTION: LS3DF 0x1005da90
bool LightMap::m_AddRectToRectList(TEXBL_STRUCT *tb, RECT *rc)
{
    if (rc->right < 3 || rc->bottom < 3) {
        tb->lost_area += rc->right * rc->bottom;
        return true;
    }
    int area = rc->right * rc->bottom;
    tb->free_area += area;
    if (!tb->first) {
        tb->first = new RECT_STRUCT;
        if (!tb->first)
            return false;
        tb->last = tb->first;
        tb->first->prev = NULL;
        tb->last->next = NULL;
        CopyRect(&tb->last->rc, rc);
        tb->last->area = area;
        return true;
    }
    RECT_STRUCT *r = new RECT_STRUCT;
    if (!r)
        return false;
    CopyRect(&r->rc, rc);
    r->area = area;
    for (RECT_STRUCT *p = tb->first; p; p = p->next) {
        if (p->area >= area) {
            r->prev = p->prev;
            r->next = p;
            if (!p->prev) {
                tb->first = r;
                p->prev = r;
                return true;
            }
            p->prev->next = r;
            p->prev = r;
            return true;
        }
    }
    r->prev = tb->last;
    r->next = NULL;
    tb->last->next = r;
    tb->last = r;
    return true;
}

// Sorts a free rectangle again after it shrank, or drops it
// FUNCTION: LS3DF 0x1005dbb0
bool LightMap::m_ReplaceRectFromRectList(TEXBL_STRUCT *tb, RECT_STRUCT *r)
{
    if (r->prev)
        r->prev->next = r->next;
    else
        tb->first = r->next;
    if (r->next)
        r->next->prev = r->prev;
    else
        tb->last = r->prev;
    if (r->rc.right < 3 || r->rc.bottom < 3) {
        tb->lost_area += r->area;
        delete r;
        return true;
    }
    int area = r->area;
    tb->free_area += area;
    for (RECT_STRUCT *p = tb->first; p; p = p->next) {
        if (p->area >= area) {
            r->prev = p->prev;
            r->next = p;
            if (!p->prev) {
                tb->first = r;
                p->prev = r;
                return true;
            }
            p->prev->next = r;
            p->prev = r;
            return true;
        }
    }
    if (!tb->first) {
        tb->first = r;
        tb->last = r;
        r->prev = NULL;
        tb->last->next = NULL;
    } else {
        r->prev = tb->last;
        r->next = NULL;
        tb->last->next = r;
        tb->last = r;
    }
    return true;
}

// Takes a free rectangle out, all of it used (inlined everywhere on PC)
void LightMap::m_RemoveRectFromRectList(TEXBL_STRUCT *tb, RECT_STRUCT *r)
{
    tb->free_area -= r->area;
    if (r->prev)
        r->prev->next = r->next;
    else
        tb->first = r->next;
    if (r->next)
        r->next->prev = r->prev;
    else
        tb->last = r->prev;
    delete r;
}

// Frees a texture's free rectangles and the offsets of what was packed (inlined everywhere on PC)
void LightMap::m_EraseRectList(TEXBL_STRUCT *tb)
{
    tb->offsets.clear();
    RECT_STRUCT *r = tb->first;
    while (r) {
        RECT_STRUCT *next = r->next;
        tb->free_area -= r->area;
        delete r;
        r = next;
    }
}

// Appends copies of src's free rectangles to dst's
// FUNCTION: LS3DF 0x1005dca0
bool LightMap::m_DuplicateRectList(TEXBL_STRUCT *src, TEXBL_STRUCT *dst)
{
    for (RECT_STRUCT *r = src->first; r; r = r->next) {
        if (!dst->first) {
            dst->first = new RECT_STRUCT;
            if (!dst->first)
                return false;
            dst->last = dst->first;
            dst->first->prev = NULL;
            dst->last->next = NULL;
        } else {
            dst->last->next = new RECT_STRUCT;
            if (!dst->last->next)
                return false;
            dst->last->next->prev = dst->last;
            dst->last->next->next = NULL;
            dst->last = dst->last->next;
        }
        CopyRect(&dst->last->rc, &r->rc);
        dst->last->area = r->area;
    }
    return true;
}

// Packs a rectangle into the first free rectangle it fits, rotated where that fits better, records where it went in the
// texture's offsets and splits the rest of the free rectangle in two, keeping the larger piece whole. 1 where it
// doesn't fit, 0 where it does. A texture without free rectangles yet gets all of itself as one.
// Not matching yet: the original adds to the new rectangle's top and to the old one's left in memory (add dword ptr
// [nr.top], sx), ours through a register; and two registers in the offset's push_back.
// FUNCTION: LS3DF 0x1005dd40
int LightMap::m_AddTexture(TEXBL_STRUCT *tb, RECTANGLE_INFO *ri)
{
    int sx = ri->tex_sx;
    int sy = ri->tex_sy;
    if (!tb->first) {
        RECT rc = {0, 0, tb->sx, tb->sy};
        m_AddRectToRectList(tb, &rc);
    }
    int mode = 0;
    RECT_STRUCT *r;
    for (r = tb->first; r; r = r->next) {
        if (r->rc.right >= sx && r->rc.bottom >= sy) {
            if (r->rc.bottom >= sx && r->rc.right == sy)
                mode = 3;
            else if (r->rc.bottom == sx && r->rc.right >= sy)
                mode = 3;
            else if (r->rc.bottom >= sx && r->rc.right >= sy)
                mode = 2;
            else
                mode = 1;
            break;
        }
        if (r->rc.bottom >= sx && r->rc.right >= sy) {
            mode = 3;
            break;
        }
    }
    if (!mode)
        return 1;
    OFFSET_STRUCT o;
    if (mode != 1 && mode != 2) {
        o.x = r->rc.left;
        o.y = r->rc.top;
        o.rotated = true;
        tb->offsets.push_back(o);
        if (r->rc.right == sy && r->rc.bottom == sx) {
            m_RemoveRectFromRectList(tb, r);
            return 0;
        }
        tb->free_area -= r->area;
        if (r->rc.right == sy) {
            r->rc.top += sx;
            r->rc.bottom -= sx;
            r->area = r->rc.right * r->rc.bottom;
            m_ReplaceRectFromRectList(tb, r);
            return 0;
        }
        if (r->rc.bottom == sx) {
            r->rc.left += sy;
            r->rc.right -= sy;
            r->area = r->rc.right * r->rc.bottom;
            m_ReplaceRectFromRectList(tb, r);
            return 0;
        }
        int b1 = (r->rc.bottom - sx) * r->rc.right;
        int b2 = (r->rc.right - sy) * r->rc.bottom;
        int m = max(b1, b2);
        RECT nr = r->rc;
        nr.top += sx;
        r->rc.left += sy;
        nr.bottom -= sx;
        if (m == b1) {
            nr.right = sy;
            r->rc.right -= sy;
        } else {
            r->rc.bottom = sx;
            r->rc.right -= sy;
        }
        r->area = r->rc.bottom * r->rc.right;
        m_ReplaceRectFromRectList(tb, r);
        m_AddRectToRectList(tb, &nr);
        return 0;
    } else {
        o.x = r->rc.left;
        o.y = r->rc.top;
        o.rotated = false;
        tb->offsets.push_back(o);
        if (r->rc.right == sx && r->rc.bottom == sy) {
            m_RemoveRectFromRectList(tb, r);
            return 0;
        }
        tb->free_area -= r->area;
        if (r->rc.right == sx) {
            r->rc.top += sy;
            r->rc.bottom -= sy;
            r->area = r->rc.right * r->rc.bottom;
            m_ReplaceRectFromRectList(tb, r);
            return 0;
        }
        if (r->rc.bottom == sy) {
            r->rc.left += sx;
            r->rc.right -= sx;
            r->area = r->rc.right * r->rc.bottom;
            m_ReplaceRectFromRectList(tb, r);
            return 0;
        }
        int a1 = (r->rc.right - sx) * r->rc.bottom;
        int a2 = (r->rc.bottom - sy) * r->rc.right;
        int a3 = (r->rc.bottom - sx) * r->rc.right;
        int a4 = (r->rc.right - sy) * r->rc.bottom;
        int m = max(a1, max(a2, max(a3, a4)));
        RECT nr;
        if (mode == 1 || m == a1 || m == a2) {
            nr = r->rc;
            nr.top += sy;
            r->rc.left += sx;
            nr.bottom -= sy;
            if (m == a1) {
                nr.right = sx;
                r->rc.right -= sx;
            } else {
                r->rc.bottom = sy;
                r->rc.right -= sx;
            }
        } else {
            nr = r->rc;
            nr.top += sx;
            r->rc.left += sy;
            nr.bottom -= sx;
            if (m == a3) {
                nr.right = sy;
                r->rc.right -= sy;
            } else {
                r->rc.bottom = sx;
                r->rc.right -= sy;
            }
            tb->offsets.pop_back();
            o.rotated = true;
            tb->offsets.push_back(o);
        }
        r->area = r->rc.bottom * r->rc.right;
        m_ReplaceRectFromRectList(tb, r);
        m_AddRectToRectList(tb, &nr);
        return 0;
    }
}

// Copies 16-bit texels, pitch texels apart (inlined everywhere on PC)
void LightMap::m_FillLineWord(ushort **src, ushort **dst, int num, uint pitch)
{
    for (int i = 0; i < num; i++) {
        **dst = **src;
        *dst += pitch;
        *src += 1;
    }
}

// Copies RGB texels to BGR ones, pitch bytes apart
// FUNCTION: LS3DF 0x1005e330
void LightMap::m_FillLineRGB(uchar **src, uchar **dst, int num, uint pitch)
{
    for (int i = 0; i < num; i++) {
        uchar r = (*src)[0];
        uchar g = (*src)[1];
        uchar b = (*src)[2];
        (*dst)[0] = b;
        (*dst)[1] = g;
        (*dst)[2] = r;
        *dst += pitch;
        *src += 3;
    }
}

// Copies RGB texels into three planes, pitch bytes apart (inlined everywhere on PC)
void LightMap::m_FillLineRGB(uchar **src, uchar **r, uchar **g, uchar **b, int num, uint pitch)
{
    for (int i = 0; i < num; i++) {
        uchar cr = (*src)[0];
        uchar cg = (*src)[1];
        uchar cb = (*src)[2];
        **r = cr;
        **g = cg;
        **b = cb;
        *r += pitch;
        *g += pitch;
        *b += pitch;
        *src += 3;
    }
}

// Scales a rectangle's texels to sx by sy into dst
// FUNCTION: LS3DF 0x1005e3a0
bool LightMap::m_Resize(RECTANGLE_INFO *ri, int sx, int sy, uchar **dst)
{
    pGlobalIGraph->ResizeImage(ri->sx, ri->sy, ri->sx * 3, ri->data, sx, sy, sx * 3, dst, 3, 1);
    return true;
}

// Packs a lit object level's rectangles into a texture: sorts them by area, the largest first (each marked with u =
// 666 once taken), converts their texels, then takes the first texture with room left that takes all of them, else the
// tightest new texture of 128 to 512 texels a side. Where none does, every rectangle of 4 texels or more is halved and
// it starts again. Then it creates the texture where it is new, counts its memory in mem, copies the texels into it by
// its format, sets each rectangle's u and v, keeps the offsets and puts the texture's free room back into the list,
// sorted. The rectangles' texels are freed but a 3x3 one's color. PS2's textures are 256 texels a side at most.
// Not matching yet: the original's frame is 8 bytes larger, and it calls the candidates' vector deleting destructor
// out of line at every failing return, ours inlines most of them (more inline calls pending after them in the
// original, as in the destructor: the offsets read without a local copy are some of them). Registers differ all
// through.
// FUNCTION: LS3DF 0x1005e3f0
bool LightMap::m_CreateLM(ushort *indices, RECTANGLE_INFO *ri, int num, uint *mem, I3D_lit_object *obj)
{
    order.reset();
    area = 0;
    rects = ri;
    for (;;) {
        int max_area = 0;
        ushort best_index = 0;
        for (ushort i = 0; i < num; i++) {
            ushort index = indices[i];
            int a = ri[index].area;
            if (ri[index].u == 0.0f && a >= max_area) {
                max_area = a;
                best_index = index;
            }
        }
        if (max_area <= 0)
            break;
        ri[best_index].u = 666.0f;
        order.push_back(best_index);
    }
    ushort i;
    for (i = 0; i < order.size(); i++)
        area += ri[order[i]].area;
    for (i = 0; i < order.size(); i++) {
        RECTANGLE_INFO &r = ri[order[i]];
        m_ConvertRGB(r.tex_sx, r.tex_sy, r.data, &r.tex_data);
    }

    // A texture with room left
    TEXBL_STRUCT tb;
    TEXBL_STRUCT *best = NULL;
    for (int j = 0; j < GlobalRemainTexBlList.size(); j++) {
        TEXBL_STRUCT &g = GlobalRemainTexBlList[j];
        if (area > g.free_area)
            continue;
        memcpy(&tb, &g, sizeof(tb));
        tb.first = NULL;
        tb.last = NULL;
        if (!m_DuplicateRectList(&g, &tb))
            return false;
        bool failed = false;
        for (ushort k = 0; k < order.size(); k++) {
            RECTANGLE_INFO &r = ri[order[k]];
            if (tb.free_area < r.area) {
                m_EraseRectList(&tb);
                failed = true;
                break;
            }
            int res = m_AddTexture(&tb, &r);
            if (res == 0x80000001) {
                m_EraseRectList(&tb);
                return false;
            }
            if (res == 1) {
                m_EraseRectList(&tb);
                k = order.size();
                failed = true;
            }
        }
        if (failed)
            continue;
        m_EraseRectList(&GlobalRemainTexBlList[j]);
        GlobalRemainTexBlList.erase_ordered(j);
        best = &tb;
        break;
    }

    // Else a new one, the tightest that takes them all
    C_list<TEXBL_STRUCT> cands(3);
    int max_sy = min(m_MaxTextureHeight, 512);
    int max_sx = min(m_MaxTextureWidth, 512);
    while (!best) {
        for (int sy = 128; sy <= max_sy; sy <<= 1) {
            for (int sx = 128; sx <= max_sx; sx <<= 1) {
                if (area > sx * sy)
                    continue;
                TEXBL_STRUCT t;
                t.sx = sx;
                t.sy = sy;
                t.free_area = 0;
                t.lost_area = 0;
                t.first = NULL;
                t.last = NULL;
                t.texture = NULL;
                bool failed = false;
                for (ushort k = 0; k < order.size(); k++) {
                    int res = m_AddTexture(&t, &ri[order[k]]);
                    if (res == 0x80000001)
                        return false;
                    if (res == 1) {
                        m_EraseRectList(&t);
                        k = order.size();
                        failed = true;
                    }
                }
                if (!failed)
                    cands.push_back(t);
            }
        }
        for (ushort k = 0; k < cands.size(); k++) {
            if (best && cands[k].free_area >= best->free_area) {
                m_EraseRectList(&cands[k]);
            } else {
                if (best)
                    m_EraseRectList(best);
                best = &cands[k];
            }
        }
        if (!best) {
            dbgPrintf("m_CreateLM() - !! NEVESLO !! => zmensuji %i LM na ctvrtinu.", order.size());
            int old_area = area;
            for (int k = 0; k < order.size(); k++) {
                RECTANGLE_INFO &r = ri[order[k]];
                if (r.tex_sx >= 4 && r.tex_sy >= 4) {
                    area -= r.area;
                    ushort sx = max(4, r.tex_sx >> 1);
                    ushort sy = max(4, r.tex_sy >> 1);
                    uchar *data = NULL;
                    m_Resize(&r, sx, sy, &data);
                    r.tex_sx = sx;
                    r.tex_sy = sy;
                    r.area = sx * sy;
                    area += r.area;
                    delete[] (uchar *)r.tex_data;
                    r.tex_data = NULL;
                    m_ConvertRGB(r.tex_sx, r.tex_sy, data, &r.tex_data);
                    delete[] data;
                }
            }
            if (area == old_area) {
                dbgPrintf("LightMap::m_CreateLM() - ERROR!! There are no LMs for RESIZE !!");
                return false;
            }
        }
    }

    // The texture
    if (best->texture) {
        texture = best->texture;
        texture->ref++;
    } else {
        texture = pGlobalIGraph->CreateITexture();
        if (!texture) {
            m_EraseRectList(best);
            return false;
        }
        if (renderState[RS_LMTRUECOLOR] && m_TrueColorTexturePixelFormat) {
            if (texture->Create(NULL, NULL, 0x88400, best->sx, best->sy, 0) < 0) {
                m_EraseRectList(best);
                return false;
            }
        } else {
            if (texture->Create(NULL, NULL, 0x88008, best->sx, best->sy, 0) < 0) {
                m_EraseRectList(best);
                return false;
            }
        }
    }
    switch (texture->format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        *mem += area * 4;
        break;
    case D3DFMT_R8G8B8:
    case D3DFMT_L8:
        *mem += area * 3;
        break;
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        *mem += area * 2;
        break;
    }
    if (!best->texture)
        texture->Fill(NULL, 0xffffffff, 0);
    uint pitch;
    uchar *bits = (uchar *)texture->Lock(NULL, pitch, 0);
    if (!bits) {
        m_EraseRectList(best);
        return false;
    }
    for (i = 0; i < order.size(); i++) {
        RECTANGLE_INFO &r = ri[order[i]];
        switch (texture->format) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5: {
            ushort *src = (ushort *)r.tex_data;
            uint pitch2 = pitch >> 1;
            ushort *d = (ushort *)bits + best->offsets[i].y * pitch2 + best->offsets[i].x;
            if (r.tex_sx == 3 && r.tex_sy == 3) {
                r.u = (float)best->offsets[i].x / best->sx;
                r.v = (float)best->offsets[i].y / best->sy;
                uint c = (uint)r.data;
                uchar cr = c >> 16;
                uchar cg = c >> 8;
                uchar cb = c;
                ushort col;
                if (texture->format == D3DFMT_R5G6B5)
                    col = (cb >> 3) | (cr >> 3) << 11 | (cg >> 2) << 5;
                else
                    col = (cb >> 3) | (cr >> 3) << 10 | 0x8000 | (cg >> 3) << 5;
                for (int y = 0; y < 3; y++) {
                    for (int x = 0; x < 3; x++)
                        *d++ = col;
                    d += pitch2 - 3;
                }
            } else if (best->offsets[i].rotated) {
                r.u = (float)best->offsets[i].x / best->sx + 2.0f;
                r.v = (float)best->offsets[i].y / best->sy;
                for (int y = 0; y < r.tex_sy; y++) {
                    ushort *dd = d;
                    m_FillLineWord(&src, &dd, r.tex_sx, pitch >> 1);
                    d++;
                }
            } else {
                int stride = pitch2 - r.tex_sx;
                r.u = (float)best->offsets[i].x / best->sx;
                r.v = (float)best->offsets[i].y / best->sy;
                for (int y = 0; y < r.tex_sy; y++) {
                    m_FillLineWord(&src, &d, r.tex_sx, 1);
                    d += stride;
                }
            }
            break;
        }
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8: {
            uchar *src = (uchar *)r.tex_data;
            uchar *d = bits + best->offsets[i].y * pitch + best->offsets[i].x * 4;
            if (r.tex_sx == 3 && r.tex_sy == 3) {
                r.u = (float)best->offsets[i].x / best->sx;
                r.v = (float)best->offsets[i].y / best->sy;
                uint c = (uint)r.data;
                uchar cr = c >> 16;
                uchar cg = c >> 8;
                for (int y = 0; y < 3; y++) {
                    for (int x = 0; x < 3; x++) {
                        *d++ = (uchar)c;
                        *d++ = cg;
                        *d = cr;
                        d += 2;
                    }
                    d += pitch - 12;
                }
            } else if (best->offsets[i].rotated) {
                r.u = (float)best->offsets[i].x / best->sx + 2.0f;
                r.v = (float)best->offsets[i].y / best->sy;
                for (int y = 0; y < r.tex_sy; y++) {
                    uchar *dd = d;
                    m_FillLineRGB(&src, &dd, r.tex_sx, pitch);
                    d += 4;
                }
            } else {
                int stride = pitch - r.tex_sx * 4;
                r.u = (float)best->offsets[i].x / best->sx;
                r.v = (float)best->offsets[i].y / best->sy;
                for (int y = 0; y < r.tex_sy; y++) {
                    m_FillLineRGB(&src, &d, r.tex_sx, 4);
                    d += stride;
                }
            }
            break;
        }
        case D3DFMT_L8: {
            uchar *src = (uchar *)r.tex_data;
            uchar *d0 = bits + best->offsets[i].y * texture->width + best->offsets[i].x;
            uchar *d1 = d0 + texture->width * texture->height;
            uchar *d2 = d0 + texture->width * texture->height * 2;
            if (r.tex_sx == 3 && r.tex_sy == 3) {
                r.u = (float)best->offsets[i].x / best->sx;
                r.v = (float)best->offsets[i].y / best->sy;
                uint c = (uint)r.data;
                uchar cr = c >> 16;
                uchar cg = c >> 8;
                for (int y = 0; y < 3; y++) {
                    for (int x = 0; x < 3; x++) {
                        *d0++ = cr;
                        *d1++ = cg;
                        *d2++ = (uchar)c;
                    }
                    d0 += best->sx - 3;
                    d1 += best->sx - 3;
                    d2 += best->sx - 3;
                }
            } else if (best->offsets[i].rotated) {
                r.u = (float)best->offsets[i].x / best->sx + 2.0f;
                r.v = (float)best->offsets[i].y / best->sy;
                for (int y = 0; y < r.tex_sy; y++) {
                    uchar *dd0 = d0;
                    uchar *dd1 = d1;
                    uchar *dd2 = d2;
                    m_FillLineRGB(&src, &dd0, &dd1, &dd2, r.tex_sx, texture->width);
                    d0++;
                    d1++;
                    d2++;
                }
            } else {
                r.u = (float)best->offsets[i].x / best->sx;
                r.v = (float)best->offsets[i].y / best->sy;
                int stride = best->sx - r.tex_sx;
                for (int y = 0; y < r.tex_sy; y++) {
                    m_FillLineRGB(&src, &d0, &d1, &d2, r.tex_sx, 1);
                    d0 += stride;
                    d1 += stride;
                    d2 += stride;
                }
            }
            break;
        }
        }
    }
    texture->Unlock();
    if (!best->texture) {
        pI3DDriver->textures.push_back(texture);
        pI3DDriver->m_RecomputeUsedVRAM();
        best->texture = texture;
    }
    for (i = 0; i < best->offsets.size(); i++)
        offsets.push_back(best->offsets[i]);
    best->offsets.clear();

    // The texture's room left goes back into the list
    bool inserted = false;
    for (i = 0; i < GlobalRemainTexBlList.size(); i++) {
        if (GlobalRemainTexBlList[i].free_area >= best->free_area) {
            GlobalRemainTexBlList.insert(i, *best);
            inserted = true;
            break;
        }
    }
    if (!inserted)
        GlobalRemainTexBlList.push_back(*best);
    for (i = 0; i < order.size(); i++) {
        if (ri[order[i]].area > 9) {
            delete[] ri[order[i]].data;
            ri[order[i]].data = NULL;
            delete[] (uchar *)ri[order[i]].tex_data;
            ri[order[i]].tex_data = NULL;
        }
    }
    texture->texture->AddDirtyRect(NULL);
    return true;
}
