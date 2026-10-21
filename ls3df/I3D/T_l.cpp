// The lighting (PS2 T&l.cpp; the & would split the build's link command, which runs through cmd.exe), at
// 0x10065a10-0x10068880 in the original. PS2 picks up to 8 of the current sector's lights for an object and the hardware
// lights it; the PC lights the vertices itself, PrepareLights picking the lights and an assembly loop lighting each
// vertex. After the lighting come the lightmaps' VertexLightingProcess (0x10066e90), the header's zero vector's $E
// (0x10067d00) and the vertex helpers the objects and billboards render with. Names are the PS2 build's unless noted.
// Not matching yet: the copy of I3D_dummy::GetInvMatrix comes after EnvProcess, its first caller, where the original's
// is at the file's end (0x10068840), so the original's headers have more inline code before I3D_dummy's.
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

// The file's .bss isn't laid out yet: the original has TempLightList (PS2 name, bucket 286) after pTempLight (704),
// and two 12-byte objects nothing references at 0x101c4d24 and 0x101c4d64. TempLightList's $E (0x10065a10) is annotated
// by name in I3D.h.
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

// A face's texel's effect on the color a ray has gathered (color): 2 if the texel is transparent (alpha below 0.1),
// 0 if it's opaque (above 0.95), else 1 with the color blended towards itself times the texel's color by the alpha.
// The texture's bytes are A, R, G, B; a material with bit 31 set takes the texel's luminance for its alpha, one
// without pixels its diffuse color and alpha. pos, f and frm are unused. PS2's has the same blend in both branches
// (the S_vector temporary times a, through __ml__8S_vectorCFRCf); it scales the coordinates by the size - 1 where the
// PC clamps them. The x clamp is windows.h's min: as an if, VC6 keeps v on the FPU stack, not in a stack slot.
// Not matching yet:
// - The original inlines less: in the diffuse branch operator*'s S_vector constructor is a call, which keeps col in
//   esi (pushed at the start) and moves the frame's slots. Ours needs about 30 units less budget there; less source
//   moves it little, and no spelling of the color, the luminance or the blend (a helper, the diffuse branch first)
//   gave it.
// - The original loads texture->pixels again after the __ftol calls where ours keeps the test's load, so x gets a
//   register instead of context's slot. A volatile load gives the original's registers; no spelling of the test or the
//   lookup did.
// FUNCTION: LS3DF 0x10066be0
int __stdcall TexelCallback(const S_vector &pos, float f, I3D_frame *frm, void *color, void *context)
{
    S_texel_info *ti = (S_texel_info *)context;
    S_vector &col = *(S_vector *)color;
    I3D_material *mat = ti->mat;
    ITexture *texture = mat->texture;
    if (texture && texture->pixels) {
        float u = ti->u;
        float v = ti->v;
        uint x = min((uint)(int)(texture->width * u), (uint)(texture->width - 1));
        uint y = min((uint)(int)(texture->height * v), (uint)(texture->height - 1));
        const uchar *texel = &texture->pixels[(y * texture->width + x) * 4];
        S_vector c;
        c.x = texel[1] * (1.0f / 255.0f);
        c.y = texel[2] * (1.0f / 255.0f);
        c.z = texel[3] * (1.0f / 255.0f);
        float a;
        if (mat->flags & MATF_ADD_MODE)
            a = c.x * 0.3f + c.y * 0.6f + c.z * 0.1f;
        else
            a = texel[0] * (1.0f / 255.0f);
        a *= mat->alpha;
        if (a < 0.1)
            return 2;
        if (a > 0.95)
            return 0;
        col *= (float)(1.0 - a);
        col += S_vector(col.x * c.x, col.y * c.y, col.z * c.z) * a;
    } else {
        S_vector c = mat->diffuse;
        float a = mat->alpha;
        if (a < 0.1)
            return 2;
        if (a > 0.95)
            return 0;
        col *= (float)(1.0 - a);
        col += S_vector(col.x * c.x, col.y * c.y, col.z * c.z) * a;
    }
    return 1;
}

// The light at a lightmap's texel (pos, with the normal) from the current sector's lightmap lights: the ambient lights'
// sum, and the diffuse sum of the point, spot and directional lights, each by its angle to the normal and its range or
// cone. For a light with I3DLIGHTMODE_SHADOW (with a scene and flags & 1) a ray from just off the surface to the light
// (TestColGeo, its texels through TexelCallback) filters it, and the rays that hit a face from behind are counted: if
// all did, the texel is inside the geometry, out is magenta and *inside true. Otherwise out is the material's emissive
// color plus the sums times its ambient and diffuse colors (white for a texture without MATF_KEEP_DIFFUSE), scaled
// down to a maximum of 2 and clamped at 0. x above 0 adds 0x2000000 to the ray's flags and passes frm to the test.
// PS2 name and parameters, pos and normal by value on PC. PS2's (and the Xbox's, 0x2301e0) attenuates by factors,
// doubles the lights and takes the material at the end; the shadow test is the same in all three. The cases are in the
// original's order (its jump table's); the unused light types are listed, as their entries go to the loop's end where
// values above 7 go to the shadow test, in the PS2 and Xbox builds too. Case 1's square is Square() (the Xbox calls it
// in both cases), which gives the original's order of the FPU code; its range scale divides through
// S_vector::operator/, whose reciprocal takes the stack slot the original stores it in.
// Not matching yet:
// - Registers: the original keeps lp on the stack (in ecx between calls) and gives esi to *lp, edi to col_flags (saving
//   out and restoring it at the end of each pass) and ebp to x; ours gives lp a register, so the frame is 4 bytes
//   smaller and the temporaries' copies are scheduled differently. Declaration order, the loop's spelling, a named
//   light pointer and nested ifs for the first tests (as PS2 and the Xbox have them) didn't move it.
// - Inlining: the original leaves only -ldir's S_vector constructor and the final sum's outer one out of line; ours
//   leaves -ldir's too, but also both products' constructors and both sums' operator+. Probes: about 2 more units of
//   source and 5 more zero-cost inline calls between the switch and the final sum give the original's calls.
// FUNCTION: LS3DF 0x10066e90
void VertexLightingProcess(S_vector &out, S_vector pos, S_vector normal, I3D_material *mat, I3D_scene *scene,
    uint flags, bool *inside, I3D_frame *frm, int x)
{
    S_vector ambient, diffuse, emissive;
    if (mat) {
        emissive = mat->emissive;
        if (mat->flags & MATF_TEXTURE && !(mat->flags & MATF_KEEP_DIFFUSE)) {
            ambient.x = ambient.y = ambient.z = 1.0f;
            diffuse = ambient;
        } else {
            ambient = mat->ambient;
            diffuse = mat->diffuse;
        }
    } else {
        ambient.x = ambient.y = ambient.z = 1.0f;
        emissive.Zero();
        diffuse = ambient;
    }
    S_vector diff_sum(0.0f, 0.0f, 0.0f);
    S_vector amb_sum(0.0f, 0.0f, 0.0f);
    uint num_back = 0;
    uint num_rays = 0;
    S_vector color, dir, from;
    I3D_light **lp = &pCurrentSector->lights[0];
    for (int n = pCurrentSector->lights.size(); n--; lp++) {
        if (!((*lp)->mode & I3DLIGHTMODE_LIGHTMAP) || !((*lp)->frm_flags & FRMFLAGS_ON))
            continue;
        if (IsAbsMrgZero((*lp)->power))
            continue;
        uint col_flags = 0x20130000;
        if (x > 0)
            col_flags = 0x22130000;
        switch ((*lp)->ltype) {
        case I3DLIGHT_AMBIENT:
            amb_sum += (*lp)->color;
            continue;
        case I3DLIGHT_POINT: {
            S_vector lpos = (*lp)->w_pos;
            S_vector d = lpos - pos;
            float sq = d.Square();
            if (sq > (*lp)->range_f_2)
                continue;
            float dot = d.Dot(normal);
            if (dot <= 0.0f)
                continue;
            float dist = (float)sqrt(sq);
            dot /= dist;
            if (dist <= (*lp)->range_n)
                color = (*lp)->color * dot;
            else
                color = (*lp)->color * dot * ((*lp)->range_f - dist) / (*lp)->range_delta;
            from = pos + normal * 0.0075f;
            dir = lpos - from;
            break;
        }
        case I3DLIGHT_SPOT: {
            S_vector ldir = -(*lp)->w_dir;
            S_vector lpos = (*lp)->w_pos;
            dir = lpos - pos;
            float sq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
            if (sq > (*lp)->range_f_2)
                continue;
            float dot = dir.Dot(normal);
            if (dot <= 0.0f)
                continue;
            float dist = (float)sqrt(sq);
            if (IsMrgZeroLess(dist))
                continue;
            float att;
            if (dist > (*lp)->range_n)
                att = ((*lp)->range_f - dist) / (*lp)->range_delta;
            else
                att = 1.0f;
            float inv = 1.0 / dist;
            float cos = ldir.Dot(dir) * inv;
            if (cos <= (*lp)->outer_cos)
                continue;
            if (cos <= (*lp)->inner_cos)
                att *= (cos - (*lp)->outer_cos) * (*lp)->cone_scale;
            color = (*lp)->color * att * dot * inv;
            from = pos + normal * 0.0075f;
            dir = lpos - from;
            break;
        }
        case I3DLIGHT_DIRECTIONAL: {
            S_vector ldir = (*lp)->w_dir;
            float dot = ldir.Dot(normal);
            if (dot >= 0.0f)
                continue;
            color = (*lp)->color * -dot;
            from = pos + normal * 0.0075f;
            dir = -ldir;
            col_flags |= 0x40000;
            break;
        }
        case I3DLIGHT_NULL:
        case I3DLIGHT_FOG:
        case I3DLIGHT_POINTAMBIENT:
        case I3DLIGHT_POINTFOG:
            continue;
        }
        if (scene && flags & 1 && (*lp)->mode & I3DLIGHTMODE_SHADOW) {
            if (IsAbsMrgZero(color.Sum()))
                continue;
            num_rays++;
            S_vector filter(1.0f, 1.0f, 1.0f);
            float col_dist;
            S_vector col_normal;
            I3D_frame *f = scene->TestColGeo(from, dir, col_flags, &col_dist, &col_normal, NULL, &filter, TexelCallback,
                x > 0 ? frm : NULL, x);
            if (f && (f->GetType() != FRAME_VISUAL || ((I3D_visual *)f)->visual_type != I3D_VISUAL_BILLBOARD)) {
                dir.Normalize();
                if (dir.Dot(col_normal) >= 0.0f)
                    num_back++;
            }
            color *= filter;
        }
        diff_sum += color;
    }
    if (inside && num_back > 0 && num_back == num_rays) {
        out = S_vector(1.0f, 0.0f, 1.0f);
        *inside = true;
        return;
    }
    out = emissive + (amb_sum * ambient + diffuse * diff_sum);
    float m = max(out.x, max(out.y, out.z));
    if (m > 2.0) {
        float r = 1.0f / m;
        out.x = out.x * r * 2.0f;
        out.y = out.y * r * 2.0f;
        out.z = out.z * r * 2.0f;
    }
    if (out.x < 0.0f)
        out.x = 0.0f;
    if (out.y < 0.0f)
        out.y = 0.0f;
    if (out.z < 0.0f)
        out.z = 0.0f;
    if (inside)
        *inside = false;
}

// The vertices' environment map coordinates (u, v at dst) from their positions and normals at src, m being the
// object's world matrix: modes 0 and 1 sphere map the direction from each vertex to the camera at cam_pos reflected
// by its normal (around z with v flipped, around y), shifted by u_ofs and v_ofs; 2 to 4 map the world position on a
// plane (x and z, y and z, x and y), scaled by RS_ENVMAP_SCALE and scale; 5 maps it into the dummy's box, clamped.
// The sphere maps are assembly; mode 1's center case jumps back into mode 0's loop. PS2 EnvProcess has no camera
// position, offsets or dummy.
// VC6 compiles the C here as without optimization (count-- through memory, every local in memory, no inlining:
// S_vector's empty constructor and GetInvMatrix are calls), unlike FogProcess's, which has __asm too; `#pragma
// optimize("", off)` around it changes nothing. The pointer local per vertex gives the original's copy of the vertex
// into a slot of its own, a cast in the call doesn't. Not matching yet: the slot order (each sphere mode's vector
// and float are swapped, and the dummy pointer sits above the matrix where the original has it below; casts,
// references and the declaration order didn't move them).
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
