// The lighting (PS2 T&l.cpp; the & would split the build's link command, which runs through cmd.exe), at
// 0x10065a10-0x10068880 in the original. PS2 picks up to 8 of the current sector's lights for an object and the hardware
// lights it; the PC lights the vertices itself, PrepareLights picking the lights and an assembly loop lighting each
// vertex. After the lighting come the lightmaps' VertexLightingProcess (0x10066e90, not written yet), the header's zero
// vector's $E (0x10067d00) and the vertex helpers the objects and billboards render with. Names are the PS2 build's
// unless noted.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

#include <math.h>

// A vector with a fourth float, which the constructors clear (the assembly steps through vectors by 16 bytes; PS2 has
// the name, __ct__9S_vector4Fv)
struct S_vector4 : public S_vector {
    float w;

    S_vector4() : w(0.0f) {}

    S_vector4(float x1, float y1, float z1) : w(0.0f)
    {
        x = x1;
        y = y1;
        z = z1;
    }

    S_vector4 &operator=(const S_vector &v)
    {
        S_vector::operator=(v);
        return *this;
    }
};

// One of the lights PrepareLights picked, in the object's space (PS2 TempLightList's entries have the light and the
// energy only)
struct S_temp_light {
    S_vector4 pos;                      // 0x00
    S_vector4 dir;                      // 0x10, towards the light
    S_vector4 color;                    // 0x20, scaled by the object's color
    float range_n;                      // 0x30, scaled
    float range_f;                      // 0x34
    float range_f_2;                    // 0x38
    float range_delta;                  // 0x3c, its inverse if it's positive
    I3D_light *light;                   // 0x40
    float energy;                       // 0x44, how much the light matters: its color over its distance
    uint unknown_48[2];
};

// GLOBAL: LS3DF 0x101c4d20
int TempLightIndex;

// The lights' scale of the vertex colors: 128, or 255 without MODULATE2X (IGraph::Init). Insanity3D's driver has the
// same choice in a member of this name (0.5 or 1.0).
// GLOBAL: LS3DF 0x100a9470
float vertex_light_mult = 128.0f;

// The assembly's constants (names guessed)
// GLOBAL: LS3DF 0x100a9474
float TwoFloat = 2.0f;

// GLOBAL: LS3DF 0x100a9478
float HalfFloat = 0.5f;

// GLOBAL: LS3DF 0x100a947c
float OneFloat = 1.0f;

// GLOBAL: LS3DF 0x101c4d80
S_temp_light TempLightList[8];

// The ambient lights' sum (the name is a guess)
// GLOBAL: LS3DF 0x101c4d30
S_vector4 TempAmbient(0.0f, 0.0f, 0.0f);

// The type of PrepareLights' static color: it has a destructor, empty, which the static's first use registers with
// atexit (the name is a guess)
struct S_color : public S_vector {
    ~S_color() {}

    void Set(float f)
    {
        x = f;
        y = f;
        z = f;
    }
};

// The vertex the assembly lights (the name is a guess)
// GLOBAL: LS3DF 0x101c4d40
const uchar *pVertices;

// GLOBAL: LS3DF 0x101c4d5c
float TempLightEnergy;

// GLOBAL: LS3DF 0x101c4d60
I3D_light *pTempLight;

// The vertex colors' alpha, in the top byte (the name is a guess)
// GLOBAL: LS3DF 0x101c4d70
uint TempAlpha;

// GLOBAL: LS3DF 0x101c5000
uint GlobalLightCount = 0;

// GLOBAL: LS3DF 0x101c5004
I3D_sector *pCurrentSector = NULL;

// FUNCTION: LS3DF 0x10065a60
void LightingSetSector(I3D_sector *sector)
{
    pCurrentSector = sector;
}

// A slot of TempLightList for pTempLight, by TempLightEnergy: the next free one, or the first weaker light's, or -1
static inline int GetTempLightIndex()
{
    if (GlobalLightCount < 8)
        return GlobalLightCount++;
    for (uint i = 0; i < GlobalLightCount; i++) {
        if (TempLightList[i].energy < TempLightEnergy)
            return i;
    }
    return -1;
}

// The color of the vertex at pVertices (its position, then its normal) from TempAmbient and TempLightList, with
// TempAlpha's alpha, scaled by vertex_light_mult. Assembly; the color is left in eax.
#pragma warning(push)
#pragma warning(disable: 4035)
static inline uint LightVertex()
{
    float t;
    __asm {
        mov ecx, GlobalLightCount
        lea esi, TempLightList
        mov edi, pVertices
        fld dword ptr TempAmbient
        fld dword ptr TempAmbient + 4
        fld dword ptr TempAmbient + 8
        cmp ecx, 0
        jne light_loop
        jmp light_end
        align 16
light_loop:
        mov edx, [esi + 0x40]
        mov eax, [edx + 0x154]
        cmp al, 3
        je directional
        cmp al, 1
        je point
        cmp al, 2
        je spot
        jmp next_light
        align 16
directional:
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edi + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 0x18]
        fmul dword ptr [edi + 0x14]
        faddp st(1), st
        fst t
        test byte ptr t + 3, 0x80
        je add_light
        fstp t
        jmp next_light
        align 16
point:
        sub esp, 0xc
        fld dword ptr [esi]
        fsub dword ptr [edi]
        fstp dword ptr [esp]
        fld dword ptr [esi + 4]
        fsub dword ptr [edi + 4]
        fstp dword ptr [esp + 4]
        fld dword ptr [esi + 8]
        fsub dword ptr [edi + 8]
        fstp dword ptr [esp + 8]
        fld dword ptr [esp]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esp + 4]
        fmul dword ptr [edi + 0x10]
        faddp st(1), st
        fld dword ptr [esp + 8]
        fmul dword ptr [edi + 0x14]
        faddp st(1), st
        fst t
        test byte ptr t + 3, 0x80
        jne point_back
        fld dword ptr [esp]
        fmul st, st
        fld dword ptr [esp + 4]
        fmul st, st
        faddp st(1), st
        fld dword ptr [esp + 8]
        fmul st, st
        faddp st(1), st
        fst t
        mov eax, [esi + 0x38]
        cmp eax, t
        jl point_out
        fsqrt
        fst t
        mov eax, [esi + 0x30]
        cmp eax, t
        jge point_near
        fld dword ptr [esi + 0x34]
        fsub st, st(1)
        fmul dword ptr [esi + 0x3c]
        fmulp st(2), st
point_near:
        fdivp st(1), st
        add esp, 0xc
        jmp add_light
point_out:
        fstp st
point_back:
        fstp t
        add esp, 0xc
        jmp next_light
        align 16
spot:
        sub esp, 0x10
        fld dword ptr [esi]
        fsub dword ptr [edi]
        fstp dword ptr [esp]
        fld dword ptr [esi + 4]
        fsub dword ptr [edi + 4]
        fstp dword ptr [esp + 4]
        fld dword ptr [esi + 8]
        fsub dword ptr [edi + 8]
        fstp dword ptr [esp + 8]
        fld dword ptr [esp]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esp + 4]
        fmul dword ptr [edi + 0x10]
        faddp st(1), st
        fld dword ptr [esp + 8]
        fmul dword ptr [edi + 0x14]
        faddp st(1), st
        fst t
        test byte ptr t + 3, 0x80
        jne spot_back
        fld dword ptr [esp]
        fmul st, st
        fld dword ptr [esp + 4]
        fmul st, st
        faddp st(1), st
        fld dword ptr [esp + 8]
        fmul st, st
        faddp st(1), st
        fst t
        mov eax, [esi + 0x38]
        cmp eax, t
        jl spot_out
        fsqrt
        fst t
        mov eax, [esi + 0x30]
        cmp eax, t
        jge spot_near
        fld dword ptr [esi + 0x34]
        fsub st, st(1)
        fmul dword ptr [esi + 0x3c]
        fmulp st(2), st
spot_near:
        fld OneFloat
        fdiv st, st(1)
        fstp dword ptr [esp + 0xc]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [esp]
        fld dword ptr [esi + 0x14]
        fmul dword ptr [esp + 4]
        faddp st(1), st
        fld dword ptr [esi + 0x18]
        fmul dword ptr [esp + 8]
        faddp st(1), st
        fmul dword ptr [esp + 0xc]
        fst t
        test byte ptr t + 3, 0x80
        jne spot_cone_out
        mov eax, [edx + 0x1a0]
        cmp eax, t
        jae spot_cone_out
        mov eax, [edx + 0x1b0]
        cmp eax, t
        jle spot_cone_in
        fld st
        fsub dword ptr [edx + 0x1a0]
        fmul dword ptr [edx + 0x1c0]
        fmulp st(3), st
spot_cone_in:
        fstp st
        fstp t
        fmul dword ptr [esp + 0xc]
        add esp, 0x10
        jmp add_light
spot_cone_out:
        fstp st
spot_out:
        fstp st
spot_back:
        fstp t
        add esp, 0x10
        jmp next_light
        align 16
add_light:
        fst t
        cmp t, MRG_ZERO_BITMASK
        jl add_skip
        fld dword ptr [esi + 0x20]
        fmul st, st(1)
        faddp st(4), st
        fld dword ptr [esi + 0x24]
        fmul st, st(1)
        faddp st(3), st
        fld dword ptr [esi + 0x28]
        fmul st, st(1)
        faddp st(2), st
add_skip:
        fstp t
next_light:
        dec ecx
        cmp ecx, 0
        je light_end
        add esi, 0x50
        jmp light_loop
light_end:
        mov eax, TempAlpha
        fmul vertex_light_mult
        xor edx, edx
        fistp t
        mov ecx, t
        test ecx, 0xffffff00
        je blue_ok
        mov ecx, 0xff
blue_ok:
        or eax, ecx
        fmul vertex_light_mult
        xor edx, edx
        fistp t
        mov ecx, t
        test ecx, 0xffffff00
        je green_ok
        mov ecx, 0xff
green_ok:
        shl ecx, 8
        or eax, ecx
        fmul vertex_light_mult
        xor edx, edx
        fistp t
        mov ecx, t
        test ecx, 0xffffff00
        je red_ok
        mov ecx, 0xff
red_ok:
        shl ecx, 0x10
        or eax, ecx
    }
}
#pragma warning(pop)

// Lights a mesh level's vertices for an object: picks up to 8 of the current sector's lights of the mode (with their
// position and direction in the object's space, m being the object's inverse world matrix), and writes each vertex's
// color, or the average of them all for VISF_UNKNOWN_80, unless the lights are the ones rec was lit by. Returns whether
// it wrote the colors. Its first half is PS2's PrepareLights.
// FUNCTION: LS3DF 0x10065a70
bool PrepareLights(uchar *dst, uint dst_stride, I3D_mesh_level *level, const uchar *src, uint src_stride,
    I3D_object *obj, const S_matrix &m, uint mode, bool force, lod *rec)
{
    if (!level || !level->fgroups)
        return false;
    TempAmbient.x = 0.0f;
    TempAmbient.y = 0.0f;
    TempAmbient.z = 0.0f;
    // The object's color (the name is a guess)
    static S_color TempColor;
    TempColor.Set(obj->lightness);
    I3D_material *mat = level->fgroups->material;
    if (mat) {
        TempAlpha = FloatToInt(obj->fade * obj->transparency * mat->alpha * 255.0f) << 24;
        if (!(level->mesh_flags & MESHF_TEXTURE) || level->mesh_flags & MESHF_KEEP_DIFFUSE) {
            TempColor.x *= mat->diffuse.x;
            TempColor.y *= mat->diffuse.y;
            TempColor.z *= mat->diffuse.z;
        }
    } else
        TempAlpha = FloatToInt(obj->fade * obj->transparency * 255.0f) << 24;
    const I3D_bvolume &bvol = obj->GetWBoundVolume();
    S_vector bpos = bvol.bsphere.pos;
    float bradius = bvol.bsphere.radius;
    float scale = m.GetUScale();

    uint key = 0;
    GlobalLightCount = 0;
    I3D_light **lp = &pCurrentSector->lights[0];
    for (int i = pCurrentSector->lights.size(); i--; lp++) {
        pTempLight = *lp;
        if (!(pTempLight->mode & mode) || !(pTempLight->frm_flags & FRMFLAGS_ON))
            continue;
        if (IsAbsMrgZero(pTempLight->power))
            continue;
        switch (pTempLight->ltype) {
        case I3DLIGHT_AMBIENT:
            TempAmbient += pTempLight->color * obj->lightness;
            key += pTempLight->update_count;
            break;
        case I3DLIGHT_SPOT: {
            float dist_2 = (bpos - pTempLight->bsphere.pos).Square();
            float r = bradius + pTempLight->bsphere.radius;
            if (!(r * r <= dist_2)) {
                TempLightEnergy = (pTempLight->color.x + pTempLight->color.y + pTempLight->color.z) /
                    (float)sqrt(dist_2);
                TempLightIndex = GetTempLightIndex();
                if (TempLightIndex >= 0) {
                    TempLightList[TempLightIndex].light = pTempLight;
                    TempLightList[TempLightIndex].energy = TempLightEnergy;
                    TempLightList[TempLightIndex].dir = -pTempLight->w_dir.RotateByMatrix(m);
                    TempLightList[TempLightIndex].dir.Normalize();
                    TempLightList[TempLightIndex].pos = pTempLight->w_pos * m;
                    TempLightList[TempLightIndex].range_n = scale * pTempLight->range_n;
                    TempLightList[TempLightIndex].range_f = scale * pTempLight->range_f;
                    TempLightList[TempLightIndex].range_delta = TempLightList[TempLightIndex].range_f -
                        TempLightList[TempLightIndex].range_n;
                    if (TempLightList[TempLightIndex].range_delta > 0.0f)
                        TempLightList[TempLightIndex].range_delta = 1.0f / TempLightList[TempLightIndex].range_delta;
                    TempLightList[TempLightIndex].color = S_vector(TempColor.x * pTempLight->color.x,
                        TempColor.y * pTempLight->color.y, TempColor.z * pTempLight->color.z);
                    TempLightList[TempLightIndex].range_f_2 = TempLightList[TempLightIndex].range_f *
                        TempLightList[TempLightIndex].range_f;
                }
            }
            break;
        }
        case I3DLIGHT_POINT: {
            const S_vector &lpos = pTempLight->GetMatrix()(3);
            float dist_2 = (bpos - lpos).Square();
            float r = bradius + pTempLight->range_f;
            if (!(r * r <= dist_2)) {
                TempLightEnergy = (pTempLight->color.x + pTempLight->color.y + pTempLight->color.z) /
                    (float)sqrt(dist_2);
                TempLightIndex = GetTempLightIndex();
                if (TempLightIndex >= 0) {
                    TempLightList[TempLightIndex].light = pTempLight;
                    TempLightList[TempLightIndex].energy = TempLightEnergy;
                    TempLightList[TempLightIndex].pos = pTempLight->w_pos * m;
                    TempLightList[TempLightIndex].range_n = scale * pTempLight->range_n;
                    TempLightList[TempLightIndex].range_f = scale * pTempLight->range_f;
                    TempLightList[TempLightIndex].range_delta = TempLightList[TempLightIndex].range_f -
                        TempLightList[TempLightIndex].range_n;
                    if (TempLightList[TempLightIndex].range_delta > 0.0f)
                        TempLightList[TempLightIndex].range_delta = 1.0f / TempLightList[TempLightIndex].range_delta;
                    TempLightList[TempLightIndex].color = S_vector(TempColor.x * pTempLight->color.x,
                        TempColor.y * pTempLight->color.y, TempColor.z * pTempLight->color.z);
                    TempLightList[TempLightIndex].range_f_2 = TempLightList[TempLightIndex].range_f *
                        TempLightList[TempLightIndex].range_f;
                }
            }
            break;
        }
        case I3DLIGHT_DIRECTIONAL:
            TempLightEnergy = pTempLight->color.x + pTempLight->color.y + pTempLight->color.z;
            TempLightIndex = GetTempLightIndex();
            if (TempLightIndex >= 0) {
                TempLightList[TempLightIndex].light = pTempLight;
                TempLightList[TempLightIndex].energy = TempLightEnergy;
                TempLightList[TempLightIndex].dir = -pTempLight->w_dir.RotateByMatrix(m);
                TempLightList[TempLightIndex].dir.Normalize();
                TempLightList[TempLightIndex].color = S_vector(TempColor.x * pTempLight->color.x,
                    TempColor.y * pTempLight->color.y, TempColor.z * pTempLight->color.z);
            }
            break;
        }
    }

    if (GlobalLightCount) {
        S_temp_light *tl = &TempLightList[GlobalLightCount];
        for (uint j = GlobalLightCount; j--; ) {
            tl--;
            key += tl->light->update_count;
        }
        key *= GlobalLightCount;
    }
    uint *cached = obj->vis_flags & VISF_UNKNOWN_10 ? &obj->lods[0]->light_key : &obj->lods[obj->cur_lod]->light_key;
    if (!force || !GlobalLightCount && key > 0) {
        if (key == *cached)
            return false;
    }
    *cached = key;
    if (mat) {
        if (!(level->mesh_flags & MESHF_TEXTURE) || level->mesh_flags & MESHF_KEEP_DIFFUSE) {
            TempAmbient.x *= mat->ambient.x;
            TempAmbient.y *= mat->ambient.y;
            TempAmbient.z *= mat->ambient.z;
        }
        TempAmbient += mat->emissive;
    }

    pVertices = src;
    if (obj->vis_flags & VISF_UNKNOWN_80) {
        uint r = 0, g = 0, b = 0;
        for (uint n = level->num_vertices; n; n--) {
            uint color = LightVertex();
            r += (color >> 16) & 0xff;
            g += (color >> 8) & 0xff;
            b += color & 0xff;
            pVertices += src_stride;
        }
        uint num = level->num_vertices;
        uint rr = r / num;
        if (rr > 0xff)
            rr = 0xff;
        uint gg = g / num;
        if (gg > 0xff)
            gg = 0xff;
        uint bb = b / num;
        if (bb > 0xff)
            bb = 0xff;
        uint color = TempAlpha | rr << 16 | gg << 8 | bb;
        for (; num; num--) {
            *(uint *)dst = color;
            dst += dst_stride;
        }
    } else {
        for (uint n = level->num_vertices; n; n--) {
            *(uint *)dst = LightVertex();
            dst += dst_stride;
            pVertices += src_stride;
        }
    }
    if (rec) {
        rec->alpha = (uchar)(TempAlpha >> 24);
        rec->alpha_dirty = false;
    }
    return true;
}

// Writes a mesh level's alpha into its vertex colors (at dst, the colors' bytes) if it changed since rec's last (the
// name is a guess)
// FUNCTION: LS3DF 0x10066b50
void SetVertexAlpha(uchar *dst, uint dst_stride, I3D_mesh_level *level, I3D_object *obj, lod *rec)
{
    if (!level || !level->fgroups)
        return;
    I3D_material *mat = level->fgroups->material;
    uchar alpha;
    if (mat)
        alpha = FloatToInt(obj->fade * obj->transparency * mat->alpha * 255.0f);
    else
        alpha = FloatToInt(obj->fade * obj->transparency * 255.0f);
    rec->alpha_dirty = false;
    if (alpha != rec->alpha) {
        rec->alpha = alpha;
        uchar *p = dst + 3;
        for (int n = level->num_vertices; n; n--) {
            *p = alpha;
            p += dst_stride;
        }
    }
}

// What the lightmaps' texel callback is given about a face (the name is a guess)
struct S_texel_info {
    float u, v;                         // the texel's coordinates
    I3D_material *mat;                  // sampled
};

// A face's texel's effect on a lightmap color: 0 if it's opaque (0.95 and more), 2 if transparent (below 0.1), else 1
// with the color blended by its transparency and color. The material's texture's bytes are B, G, R, A; a material
// with bit 31 set takes the texel's luminance for its alpha.
// FUNCTION: LS3DF 0x10066be0
int __stdcall TexelCallback(const S_vector &pos, float f, I3D_frame *frm, void *color, void *context)
{
    I3D_material *mat = ((S_texel_info *)context)->mat;
    ITexture *texture = mat->texture;
    S_vector c;
    float a;
    if (texture && texture->pixels) {
        float u = ((S_texel_info *)context)->u;
        float v = ((S_texel_info *)context)->v;
        int sx = texture->width;
        uint x = (int)(sx * u);
        if (x >= (uint)(sx - 1))
            x = sx - 1;
        int sy = texture->height;
        uint y = (int)(sy * v);
        if (y >= (uint)(sy - 1))
            y = sy - 1;
        const uchar *texel = &texture->pixels[(y * sx + x) * 4];
        c.x = texel[1] * (1.0f / 255.0f);
        c.y = texel[2] * (1.0f / 255.0f);
        c.z = texel[3] * (1.0f / 255.0f);
        if (mat->flags & MATF_ADD_MODE)
            a = c.x * 0.3f + c.y * 0.6f + c.z * 0.1f;
        else
            a = texel[0] * (1.0f / 255.0f);
        a *= mat->alpha;
        if (a < 0.1)
            return 2;
        if (a > 0.95)
            return 0;
        S_vector *col = (S_vector *)color;
        S_vector t = *col * (float)(1.0 - a);
        *col = t + S_vector(t.x * c.x, t.y * c.y, t.z * c.z) * f;
        return 1;
    }
    c = mat->diffuse;
    a = mat->alpha;
    if (a < 0.1)
        return 2;
    if (a > 0.95)
        return 0;
    S_vector *col = (S_vector *)color;
    *col *= (float)(1.0 - a);
    *col += S_vector(col->x * c.x * f, col->y * c.y * f, col->z * c.z * f);
    return 1;
}

// The vertices' environment map coordinates (u, v at dst) from their positions and normals at src, m being the
// object's world matrix: modes 0 and 1 sphere map the direction from each vertex to the camera at cam_pos reflected
// by its normal (around z with v flipped, around y), shifted by u_ofs and v_ofs; 2 to 4 map the world position on a
// plane (x and z, y and z, x and y), scaled by RS_ENVMAP_SCALE and scale; 5 maps it into the dummy's box, clamped.
// The sphere maps are assembly; mode 1's center case jumps back into mode 0's loop. PS2 EnvProcess has no camera
// position, offsets or dummy.
// FUNCTION: LS3DF 0x10067d20
void EnvProcess(void *dst, uint dst_stride, const void *src, uint src_stride, uint count, const S_vector &cam_pos,
    const S_matrix &m, float scale, float u_ofs, float v_ofs, int mode, I3D_frame *frm)
{
    switch (mode) {
    case 0: {
        S_vector n;
        float t;
        __asm {
            mov ecx, count
            mov esi, src
            mov edi, dst
            align 16
sphere_z:
            mov edx, cam_pos
            fld dword ptr [edx + 8]
            fld dword ptr [edx + 4]
            fld dword ptr [edx]
            mov edx, m
            fld dword ptr [esi]
            fmul dword ptr [edx]
            fadd dword ptr [edx + 0x30]
            fld dword ptr [esi + 4]
            fmul dword ptr [edx + 0x10]
            faddp st(1), st
            fld dword ptr [esi + 8]
            fmul dword ptr [edx + 0x20]
            faddp st(1), st
            fsubp st(1), st
            fld dword ptr [esi]
            fmul dword ptr [edx + 4]
            fadd dword ptr [edx + 0x34]
            fld dword ptr [esi + 4]
            fmul dword ptr [edx + 0x14]
            faddp st(1), st
            fld dword ptr [esi + 8]
            fmul dword ptr [edx + 0x24]
            faddp st(1), st
            fsubp st(2), st
            fld dword ptr [esi]
            fmul dword ptr [edx + 8]
            fadd dword ptr [edx + 0x38]
            fld dword ptr [esi + 4]
            fmul dword ptr [edx + 0x18]
            faddp st(1), st
            fld dword ptr [esi + 8]
            fmul dword ptr [edx + 0x28]
            faddp st(1), st
            fsubp st(3), st
            fld dword ptr [esi + 0xc]
            fmul dword ptr [edx]
            fld dword ptr [esi + 0x10]
            fmul dword ptr [edx + 0x10]
            faddp st(1), st
            fld dword ptr [esi + 0x14]
            fmul dword ptr [edx + 0x20]
            faddp st(1), st
            fst n.x
            fmul st, st(1)
            fld dword ptr [esi + 0xc]
            fmul dword ptr [edx + 4]
            fld dword ptr [esi + 0x10]
            fmul dword ptr [edx + 0x14]
            faddp st(1), st
            fld dword ptr [esi + 0x14]
            fmul dword ptr [edx + 0x24]
            faddp st(1), st
            fst n.y
            fmul st, st(3)
            faddp st(1), st
            fld dword ptr [esi + 0xc]
            fmul dword ptr [edx + 8]
            fld dword ptr [esi + 0x10]
            fmul dword ptr [edx + 0x18]
            faddp st(1), st
            fld dword ptr [esi + 0x14]
            fmul dword ptr [edx + 0x28]
            faddp st(1), st
            fst n.z
            fmul st, st(4)
            faddp st(1), st
            fmul TwoFloat
            fld n.x
            fmul st, st(1)
            fsub st, st(2)
            fxch st(2)
            fstp t
            fld n.y
            fmul st, st(1)
            fsub st, st(3)
            fxch st(3)
            fstp t
            fld n.z
            fmul st, st(1)
            fsub st, st(4)
            fxch st(4)
            fcompp
            fld st
            fmul st, st
            fld st(2)
            fmul st, st
            faddp st(1), st
            fld st(3)
            fmul st, st
            faddp st(1), st
            fsqrt
            fld OneFloat
            fdivr st(1), st
            fxch st(1)
            fmul st(2), st
            fmul st(3), st
            fmulp st(4), st
            fadd st(3), st
            fxch st(3)
            fst t
            cmp t, MRG_ZERO_BITMASK
            jl sphere_z_center
            fsqrt
            fmul scale
            fdivr st, st(3)
            fmul st(1), st
            fchs
            fmulp st(2), st
            fadd HalfFloat
            fadd u_ofs
            fstp dword ptr [edi]
            fadd HalfFloat
            fadd v_ofs
            fstp dword ptr [edi + 4]
            fstp t
            add edi, dst_stride
            add esi, src_stride
            dec ecx
            jne sphere_z
            jmp sphere_z_end
            align 16
sphere_z_center:
            fcompp
            fld HalfFloat
            fadd u_ofs
            fstp dword ptr [edi]
            fstp t
            fld HalfFloat
            fadd v_ofs
            fstp dword ptr [edi + 4]
            fstp t
            add edi, dst_stride
            add esi, src_stride
            dec ecx
            jne sphere_z
            align 16
sphere_z_end:
        }
        break;
    }
    case 1: {
        S_vector n;
        float t;
        __asm {
            mov ecx, count
            mov esi, src
            mov edi, dst
            align 16
sphere_y:
            mov edx, cam_pos
            fld dword ptr [edx + 8]
            fld dword ptr [edx + 4]
            fld dword ptr [edx]
            mov edx, m
            fld dword ptr [esi]
            fmul dword ptr [edx]
            fadd dword ptr [edx + 0x30]
            fld dword ptr [esi + 4]
            fmul dword ptr [edx + 0x10]
            faddp st(1), st
            fld dword ptr [esi + 8]
            fmul dword ptr [edx + 0x20]
            faddp st(1), st
            fsubp st(1), st
            fld dword ptr [esi]
            fmul dword ptr [edx + 4]
            fadd dword ptr [edx + 0x34]
            fld dword ptr [esi + 4]
            fmul dword ptr [edx + 0x14]
            faddp st(1), st
            fld dword ptr [esi + 8]
            fmul dword ptr [edx + 0x24]
            faddp st(1), st
            fsubp st(2), st
            fld dword ptr [esi]
            fmul dword ptr [edx + 8]
            fadd dword ptr [edx + 0x38]
            fld dword ptr [esi + 4]
            fmul dword ptr [edx + 0x18]
            faddp st(1), st
            fld dword ptr [esi + 8]
            fmul dword ptr [edx + 0x28]
            faddp st(1), st
            fsubp st(3), st
            fld dword ptr [esi + 0xc]
            fmul dword ptr [edx]
            fld dword ptr [esi + 0x10]
            fmul dword ptr [edx + 0x10]
            faddp st(1), st
            fld dword ptr [esi + 0x14]
            fmul dword ptr [edx + 0x20]
            faddp st(1), st
            fst n.x
            fmul st, st(1)
            fld dword ptr [esi + 0xc]
            fmul dword ptr [edx + 4]
            fld dword ptr [esi + 0x10]
            fmul dword ptr [edx + 0x14]
            faddp st(1), st
            fld dword ptr [esi + 0x14]
            fmul dword ptr [edx + 0x24]
            faddp st(1), st
            fst n.y
            fmul st, st(3)
            faddp st(1), st
            fld dword ptr [esi + 0xc]
            fmul dword ptr [edx + 8]
            fld dword ptr [esi + 0x10]
            fmul dword ptr [edx + 0x18]
            faddp st(1), st
            fld dword ptr [esi + 0x14]
            fmul dword ptr [edx + 0x28]
            faddp st(1), st
            fst n.z
            fmul st, st(4)
            faddp st(1), st
            fmul TwoFloat
            fld n.x
            fmul st, st(1)
            fsub st, st(2)
            fxch st(2)
            fstp t
            fld n.y
            fmul st, st(1)
            fsub st, st(3)
            fxch st(3)
            fstp t
            fld n.z
            fmul st, st(1)
            fsub st, st(4)
            fxch st(4)
            fcompp
            fld st
            fmul st, st
            fld st(2)
            fmul st, st
            faddp st(1), st
            fld st(3)
            fmul st, st
            faddp st(1), st
            fsqrt
            fld OneFloat
            fdivr st(1), st
            fxch st(1)
            fmul st(2), st
            fmul st(3), st
            fmulp st(4), st
            fadd st(2), st
            fxch st(2)
            fst t
            cmp t, MRG_ZERO_BITMASK
            jl sphere_y_center
            fsqrt
            fmul scale
            fdivr st, st(2)
            fmul st(1), st
            fmulp st(3), st
            fadd HalfFloat
            fadd u_ofs
            fstp dword ptr [edi]
            fstp t
            fadd HalfFloat
            fadd v_ofs
            fstp dword ptr [edi + 4]
            add edi, dst_stride
            add esi, src_stride
            dec ecx
            jne sphere_y
            jmp sphere_y_end
            align 16
sphere_y_center:
            fcompp
            fld HalfFloat
            fadd u_ofs
            fstp dword ptr [edi]
            fcompp
            fld HalfFloat
            fadd v_ofs
            fstp dword ptr [edi + 4]
            add edi, dst_stride
            add esi, src_stride
            dec ecx
            jne sphere_z
sphere_y_end:
        }
        break;
    }
    case 2: {
        float f = *(float *)&renderState[RS_ENVMAP_SCALE];
        float k = f * scale * 0.3535534f;
        while (count--) {
            const S_vector *v = (const S_vector *)src;
            S_vector p = *v * m;
            ((float *)dst)[0] = p.x * k;
            ((float *)dst)[1] = p.z * k;
            dst = (uchar *)dst + dst_stride;
            src = (const uchar *)src + src_stride;
        }
        break;
    }
    case 3: {
        float f = *(float *)&renderState[RS_ENVMAP_SCALE];
        float k = f * scale * 0.3535534f;
        while (count--) {
            const S_vector *v = (const S_vector *)src;
            S_vector p = *v * m;
            ((float *)dst)[0] = p.y * k;
            ((float *)dst)[1] = p.z * k;
            dst = (uchar *)dst + dst_stride;
            src = (const uchar *)src + src_stride;
        }
        break;
    }
    case 4: {
        float f = *(float *)&renderState[RS_ENVMAP_SCALE];
        float k = f * scale * 0.3535534f;
        while (count--) {
            const S_vector *v = (const S_vector *)src;
            S_vector p = *v * m;
            ((float *)dst)[0] = p.x * k;
            ((float *)dst)[1] = p.y * k;
            dst = (uchar *)dst + dst_stride;
            src = (const uchar *)src + src_stride;
        }
        break;
    }
    case 5: {
        if (!frm)
            break;
        I3D_dummy *dummy = (I3D_dummy *)frm;
        S_matrix mm = m * dummy->GetInvMatrix();
        while (count--) {
            const S_vector *v = (const S_vector *)src;
            S_vector p = *v * mm;
            if (p.z < 0.0f)
                ((float *)dst)[0] = 0.0f;
            else if (p.z > 1.0f)
                ((float *)dst)[0] = 1.0f;
            else
                ((float *)dst)[0] = p.z;
            if (p.y < 0.0f)
                ((float *)dst)[1] = 1.0f;
            else if (p.y > 1.0f)
                ((float *)dst)[1] = 0.0f;
            else
                ((float *)dst)[1] = 1.0f - p.y;
            dst = (uchar *)dst + dst_stride;
            src = (const uchar *)src + src_stride;
        }
        break;
    }
    }
}

// The vertices' positions and normals at src (I3D_vertex) through m and mn into dst (a position and a normal each;
// the name is a guess)
// FUNCTION: LS3DF 0x10068360
void TransformVertices(void *dst, const void *src, uint count, const S_matrix &m, const S_matrix &mn)
{
    __asm {
        mov ecx, count
        mov esi, src
        mov edi, dst
        mov edx, m
        mov eax, mn
        align 16
transform_loop:
        fld dword ptr [esi]
        fmul dword ptr [edx]
        fadd dword ptr [edx + 0x30]
        fld dword ptr [esi + 4]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi]
        fld dword ptr [esi]
        fmul dword ptr [edx + 4]
        fadd dword ptr [edx + 0x34]
        fld dword ptr [esi + 4]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 4]
        fld dword ptr [esi]
        fmul dword ptr [edx + 8]
        fadd dword ptr [edx + 0x38]
        fld dword ptr [esi + 4]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 8]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [eax]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [eax + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [eax + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0xc]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [eax + 4]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [eax + 0x14]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [eax + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x10]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [eax + 8]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [eax + 0x18]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [eax + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x14]
        add edi, 0x18
        add esi, 0x20
        dec ecx
        jne transform_loop
    }
}

// The same blended: weights[i] of the first matrices' result, the rest of the second's (the name is a guess)
// FUNCTION: LS3DF 0x10068430
void TransformVerticesBlend(void *dst, const void *src, uint count, const S_matrix &m1, const S_matrix &mn1,
    const float *weights, const S_matrix &m2, const S_matrix &mn2)
{
    __asm {
        mov ecx, count
        mov esi, src
        mov edi, dst
        push ebx
        mov ebx, m1
        mov eax, weights
        align 16
blend_loop:
        fld dword ptr [esi]
        fmul dword ptr [ebx]
        fadd dword ptr [ebx + 0x30]
        fld dword ptr [esi + 4]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fld dword ptr [esi]
        fmul dword ptr [ebx + 4]
        fadd dword ptr [ebx + 0x34]
        fld dword ptr [esi + 4]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fld dword ptr [esi]
        fmul dword ptr [ebx + 8]
        fadd dword ptr [ebx + 0x38]
        fld dword ptr [esi + 4]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        mov edx, m2
        fld dword ptr [esi]
        fmul dword ptr [edx]
        fadd dword ptr [edx + 0x30]
        fld dword ptr [esi + 4]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fld dword ptr [esi]
        fmul dword ptr [edx + 4]
        fadd dword ptr [edx + 0x34]
        fld dword ptr [esi + 4]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fld dword ptr [esi]
        fmul dword ptr [edx + 8]
        fadd dword ptr [edx + 0x38]
        fld dword ptr [esi + 4]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [esi + 8]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        fld dword ptr [eax]
        fmul st(4), st
        fmul st(5), st
        fmul st(6), st
        fld OneFloat
        fsubrp st(1), st
        fmul st(3), st
        fmul st(2), st
        fmulp st(1), st
        faddp st(3), st
        faddp st(3), st
        faddp st(3), st
        fstp dword ptr [edi + 8]
        fstp dword ptr [edi + 4]
        fstp dword ptr [edi]
        mov edx, mn1
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edx]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edx + 4]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edx + 8]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        mov edx, mn2
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edx]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edx + 4]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edx + 8]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        fld dword ptr [eax]
        fmul st(4), st
        fmul st(5), st
        fmul st(6), st
        fld OneFloat
        fsubrp st(1), st
        fmul st(3), st
        fmul st(2), st
        fmulp st(1), st
        faddp st(3), st
        faddp st(3), st
        faddp st(3), st
        fstp dword ptr [edi + 0x14]
        fstp dword ptr [edi + 0x10]
        fstp dword ptr [edi + 0xc]
        add eax, 4
        add edi, 0x18
        add esi, 0x20
        dec ecx
        jne blend_loop
        pop ebx
    }
}

// The vertices' fog (the byte at dst) from their depth at src (z, or z / w for z below 1) and the fog light's ranges,
// both scaled by the current camera's far clipping plane: 255 up to the near range and 0 from the far one on (mode 0),
// or the other way round (mode 1). One stride steps both. (The name is a guess.)
// FUNCTION: LS3DF 0x100685d0
void FogProcess(uchar *dst, const float *src, uint stride, uint count, I3D_light *light, int mode)
{
    if (!light)
        return;
    switch (mode) {
    case 0: {
        float range_f = pCurrentCamera->fcp * light->range_f;
        float range_n = range_f * light->range_n;
        float scale = 255.0f / (range_f - range_n);
        float t;
        __asm {
            mov ecx, count
            mov esi, src
            mov edx, esi
            add edx, 4
            mov edi, dst
            mov ebx, stride
            fld1
            align 16
fog_loop:
            fld dword ptr [esi]
            test byte ptr [esi + 3], 0x80
            jne fog_depth
            fsub st, st(1)
            fstp t
            test byte ptr t + 3, 0x80
            fld dword ptr [esi]
            je fog_depth
            fdiv dword ptr [edx]
fog_depth:
            fld st
            fsub range_f
            fstp t
            test byte ptr t + 3, 0x80
            fld st
            jne fog_in_range
            mov byte ptr [edi], 0
            fcompp
            jmp fog_next
fog_in_range:
            fsub range_n
            fstp t
            test byte ptr t + 3, 0x80
            je fog_between
            mov byte ptr [edi], 0xff
            fstp t
            jmp fog_next
fog_between:
            fsubr range_f
            fmul scale
            fistp t
            mov eax, t
            mov byte ptr [edi], al
fog_next:
            add esi, ebx
            add edi, ebx
            add edx, ebx
            dec ecx
            jne fog_loop
            fstp t
        }
        break;
    }
    case 1: {
        float range_f = pCurrentCamera->fcp * light->range_f;
        float range_n = range_f * light->range_n;
        float scale = 255.0f / (range_f - range_n);
        float t;
        __asm {
            mov ecx, count
            mov esi, src
            mov edx, esi
            add edx, 4
            mov edi, dst
            mov ebx, stride
            fld1
            align 16
fog_inv_loop:
            fld dword ptr [esi]
            test byte ptr [esi + 3], 0x80
            jne fog_inv_depth
            fsub st, st(1)
            fstp t
            test byte ptr t + 3, 0x80
            fld dword ptr [esi]
            je fog_inv_depth
            fdiv dword ptr [edx]
fog_inv_depth:
            fld st
            fsub range_f
            fstp t
            test byte ptr t + 3, 0x80
            fld st
            jne fog_inv_in_range
            mov byte ptr [edi], 0xff
            fcompp
            jmp fog_inv_next
fog_inv_in_range:
            fsub range_n
            fstp t
            test byte ptr t + 3, 0x80
            je fog_inv_between
            mov byte ptr [edi], 0
            fstp t
            jmp fog_inv_next
fog_inv_between:
            fsubr range_f
            fmul scale
            fistp t
            mov eax, t
            mov byte ptr [edi], 0xff
            sub byte ptr [edi], al
fog_inv_next:
            add esi, ebx
            add edi, ebx
            add edx, ebx
            dec ecx
            jne fog_inv_loop
            fstp t
        }
        break;
    }
    }
}

// Swaps the byte at a with the one at b in each vertex of the faces, a vertex once: the byte before b says whether
// it is swapped, and restore swaps those back (the name is a guess)
// FUNCTION: LS3DF 0x10068760
void SwapVertexBytes(uchar *a, uchar *b, uint stride, const I3D_triface *faces, uint num_faces, int restore)
{
    uchar from, to;
    if (!restore) {
        from = 0;
        to = 1;
    } else {
        from = 1;
        to = 0;
    }
    for (uint i = num_faces; i; i--, faces++) {
        uint ofs = faces->vi[0] * stride;
        if (b[ofs - 1] == from) {
            uchar c = a[ofs];
            a[ofs] = b[ofs];
            b[ofs] = c;
            b[ofs - 1] = to;
        }
        ofs = faces->vi[1] * stride;
        if (b[ofs - 1] == from) {
            uchar c = a[ofs];
            a[ofs] = b[ofs];
            b[ofs] = c;
            b[ofs - 1] = to;
        }
        ofs = faces->vi[2] * stride;
        if (b[ofs - 1] == from) {
            uchar c = a[ofs];
            a[ofs] = b[ofs];
            b[ofs] = c;
            b[ofs - 1] = to;
        }
    }
}
