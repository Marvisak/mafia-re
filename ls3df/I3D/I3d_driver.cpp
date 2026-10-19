// The 3D driver (PS2 I3d_driver.cpp), at 0x10015530-0x1001a1e0 in the original: I3D_driver in PS2's order, with the
// PC's resource icon and shadow texture rasterizer after GetState, and the C_list templates' out-of-line functions at
// the end. The header's zero vector has its $E at 0x10016fd0, after DrawLines. The order of the file's .bss
// (0x100af8e8-0x100c0010, VC6's hash) isn't worked out. The functions of other files the driver calls are stubs at the
// end. Names are the PS2 build's unless noted.
//
// The debugging draws set D3D's render states through IGraph's caches. Their color operation and alpha state code is
// the inline helpers' (I3D.h): the same code written out takes other registers. The blend and vertex format code is a
// call in the short draws (render.h's inline helpers, which this file doesn't see) and written out in the long ones,
// which may be other helpers of the same kind. The long ones also
// leave calls out of line that our build inlines (C_list::reserve in DrawText, S_vector's constructor in
// DrawCircle2D), as if all of it were inline helpers and VC6's inline budget ran out earlier than in our build; with
// everything as helpers, our build inlines the short draws' blend code too. Open.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "I3D/VBManager.h"
#include "ISound/ISound.h"

#include <stdio.h>

// VBManager.cpp
extern C_VBManager cVBMan;

// Set when the geometry has to be recomputed (another I3D file's)
extern bool RecomputeAllGeometry;

// clipper.cpp's
void FreeClipArrays();

// .data

// GLOBAL: LS3DF 0x100a69d8
uint ZBufferEnable = D3DZB_TRUE;

// The shadow texture rasterizer's buffer (PC only, names guessed)
static uchar shadow_mem[0x10000];

// GLOBAL: LS3DF 0x100a69dc
uchar *shadow_buffer = shadow_mem;

// Nothing references it (PC only)
// GLOBAL: LS3DF 0x100a69e0
int unknown_100a69e0[8] = {1, 2, 3, 4, 5, 6, 7, 8};

// DrawSphere's lines: the three circles of VolumeBall
// GLOBAL: LS3DF 0x100a6a00
ushort VolumeBallIndx[96] = {
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
    8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 0,
    16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24,
    24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 16,
    32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40,
    40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 32,
};

// .bss

// GLOBAL: LS3DF 0x100bfca0
int renderState[RS_LAST];

// GLOBAL: LS3DF 0x100c0009
bool I3DIsInit;

// GLOBAL: LS3DF 0x100afb84
uint volumeStats[8];

// GLOBAL: LS3DF 0x100bffc4
int driverTickTime;

// GLOBAL: LS3DF 0x100af928
float RsNonOrthoTessellation;

// GLOBAL: LS3DF 0x100afba8
int RsZBiasShadow;

// GLOBAL: LS3DF 0x100bfc70
int RsZBiasProjector;

// RS_LOD_SCALE / 100 (PC only, the name is a guess; Insanity3D's driver keeps it as lod_scale)
// GLOBAL: LS3DF 0x100afb70
float LODScale;

// The fast find tree's root and the parent FindFrameFast_ came from
// GLOBAL: LS3DF 0x100bfc68
I3D_frame *pFFRoot;

// GLOBAL: LS3DF 0x100afba4
I3D_frame *pFFLastUp_;

// OptimalizeFastFindData's list of the frames in name order
// GLOBAL: LS3DF 0x100bfc6c
I3D_frame **OFFD_pFrameList;

// GLOBAL: LS3DF 0x100bfc9c
int OFFD_ListSize;

// GLOBAL: LS3DF 0x100bfd9c
int OFFD_ListUsed;

// The frame counter, which render.cpp counts and the frames compare
// GLOBAL: LS3DF 0x100afc24
uint RenderNumber;

// GLOBAL: LS3DF 0x100bfda0
uint RenderNumberMinus1;

// The texts' font, LS3DF's bitmap 125
// GLOBAL: LS3DF 0x100c000c
static I3D_material *pFontMat;

// The shadow rasterizer's spans: the leftmost and rightmost pixel of each row (PC only, the name is a guess)
// GLOBAL: LS3DF 0x100bfda8
static uchar shadow_spans[256][2];

// The globals with dynamic initializers, in the order the original constructs them (.CRT$XCU)

// GetIcon's materials (the name is a guess; Insanity3D's driver keeps them as icon_materials)
// GLOBAL: LS3DF 0x100bffb0
C_list<I3D_material *> IconMaterialList;

// GLOBAL: LS3DF 0x100bfc78
C_list<TLVERTEX> TLVertList;

// GLOBAL: LS3DF 0x100afb74
C_list<LVERTEX> LVertList;

// FUNCTION: LS3DF 0x10015620
I3D_driver::I3D_driver()
{
    unknown_58 = 0;
}

// FUNCTION: LS3DF 0x10015670
I3D_driver::~I3D_driver()
{
    if (I3DIsInit)
        dbgPrintf("!! I3D_driver was initialised, but not closed");
    FreeClipArrays();
}

// FUNCTION: LS3DF 0x10015730
void I3D_driver::Tick(int time)
{
    driverTickTime += time;
}

// FUNCTION: LS3DF 0x10015750
int I3D_driver::GetTickTime() const
{
    return driverTickTime;
}

// FUNCTION: LS3DF 0x10015760
LS3D_RESULT I3D_driver::Init(uint flags)
{
    init_flags = flags;
    driverTickTime = 0;
    memset(renderState, 0, sizeof(renderState));
    renderState[5] = 1;
    renderState[6] = 1;
    renderState[RS_USEZB] = 1;
    renderState[13] = 1;
    renderState[18] = 1;
    renderState[32] = 1;
    renderState[26] = 1;
    renderState[RS_MIPMAP] = 2;
    renderState[RS_LINEARFILTER] = 1;
    renderState[1] = 1;
    renderState[RS_DITHER] = 1;
    renderState[34] = 1;
    renderState[35] = 1;
    renderState[40] = 1;
    renderState[16] = 1;
    renderState[38] = -1;
    renderState[RS_LOD_QUALITY] = I3DFloatAsInt(1.0f);
    renderState[39] = 1;
    if (!(m_ProblemFlags & 0x80))
        renderState[RS_FOG] = 1;
    renderState[RS_ENVMAP_SCALE] = I3DFloatAsInt(1.0f);
    RsZBiasProjector = m_ProblemFlags & 0x100 ? 2 : 8;
    renderState[60] = I3DFloatAsInt(0.001f);
    renderState[25] = 1;
    renderState[43] = 1;
    renderState[45] = 1;
    renderState[RS_SHADOW_SW] = 1;
    renderState[49] = 1;
    renderState[RS_LOD_SCALE] = 100;
    LODScale = 1.0f;
    renderState[51] = 1;
    renderState[RS_NONORTHO_TESSELLATION] = 50;
    RsNonOrthoTessellation = 0.02f;
    renderState[RS_ZBIAS_PROJECTOR] = 8;
    renderState[59] = 1;
    renderState[RS_ZBIAS_SHADOW] = 3;
    renderState[61] = I3DFloatAsInt(0.001f);
    if (m_ProblemFlags & 0x100)
        RsZBiasShadow = 1;
    else
        RsZBiasShadow = 3;
    Close();
    I3DIsInit = true;
    initRender();
    pFFRoot = NULL;
    pFFLastUp_ = NULL;
    pFontMat = GetIcon(125);
    if (!(m_InitDesc.flags & 0x40))
        disableLog = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10015930
LS3D_RESULT I3D_driver::Close()
{
    closeRender();
    I3D_material **p = &IconMaterialList[0];
    for (int i = IconMaterialList.size(); i--; p++) {
        if (*p) {
            (*p)->Release();
            *p = NULL;
        }
    }
    IconMaterialList.clear();
    if (pFontMat) {
        pFontMat->Release();
        pFontMat = NULL;
    }
    I3DIsInit = false;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100159b0
LS3D_RESULT I3D_driver::GetStats(I3DSTATSINDEX index, void *data)
{
    if (!data)
        return I3DERR_INVALIDPARAMS;
    switch (index) {
    case I3DSTATS_VOLUME:
        memcpy(data, volumeStats, sizeof(volumeStats));
        return I3D_OK;
    case I3DSTATS_RENDER:
        return I3DERR_UNSUPPORTED;
    case I3DSTATS_SCENE:
        return I3DERR_UNSUPPORTED;
    case I3DSTATS_VIDEO:
        return I3DERR_UNSUPPORTED;
    case I3DSTATS_MEMORY:
        return I3DERR_UNSUPPORTED;
    case I3DSTATS_SOUND:
        return I3DERR_UNSUPPORTED;
    case I3DSTATS_BSP:
        return I3DERR_UNSUPPORTED;
    }
    dbgPrintf("!! Invalid index:%d  I3D_driver::GetStats()", index);
    return I3DERR_INVALIDPARAMS;
}

// FUNCTION: LS3DF 0x10015a20
LS3D_RESULT I3D_driver::SetState(I3D_RENDERSTATE state, int value)
{
    if (state >= RS_LAST) {
        dbgPrintf("!! Invalid parameter  state:%u  I3D_driver::SetState()", state);
        return I3DERR_INVALIDPARAMS;
    }
    switch (state) {
    case RS_LINEARFILTER:
        if (!value) {
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_POINT);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
        } else {
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        }
        break;
    case RS_MIPMAP:
        switch (value) {
        case 2:
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
            break;
        case 1:
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
            break;
        case 0:
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
            m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_NONE);
            break;
        }
        break;
    case RS_USEZB:
        m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, value ? ZBufferEnable : D3DZB_FALSE);
        break;
    case RS_WBUFFER:
        ZBufferEnable = value ? D3DZB_USEW : D3DZB_TRUE;
        if (renderState[RS_USEZB])
            m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, ZBufferEnable);
        break;
    case RS_LOD_QUALITY:
        // A case of its own in the jump table, so its body did something VC6 then optimized away
        value = value;
        break;
    case RS_DITHER:
        m_pD3DDevice8->SetRenderState(D3DRS_DITHERENABLE, value);
        break;
    case RS_MIPMAP_BIAS:
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, value);
        m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MIPMAPLODBIAS, value);
        break;
    case RS_WIREFRAME:
        if (!value)
            m_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        else
            m_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        break;
    case RS_RECOMPUTE_GEOMETRY:
        if (value)
            RecomputeAllGeometry = true;
        break;
    case RS_FOG:
        if (value && m_ProblemFlags & 0x80)
            return I3D_OK;
        break;
    case RS_LOD_SCALE:
        LODScale = value * .01f;
        break;
    case RS_NONORTHO_TESSELLATION:
        RsNonOrthoTessellation = 1.0f / value;
        break;
    case RS_ZBIAS_SHADOW:
        if (m_ProblemFlags & 0x100)
            RsZBiasShadow = (value + 3) / 4;
        else
            RsZBiasShadow = value;
        break;
    case RS_ZBIAS_PROJECTOR:
        if (m_ProblemFlags & 0x100)
            RsZBiasProjector = (value + 3) / 4;
        else
            RsZBiasProjector = value;
        break;
    case RS_ENVMAP_SCALE:
        if (value == 666) {
            cVBMan.m_FreeAllVRAMVBuffers();
            return I3D_OK;
        }
        break;
    }
    renderState[state] = value;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10015e50
int I3D_driver::GetState(I3D_RENDERSTATE state) const
{
    if (state >= RS_LAST) {
        dbgPrintf("!! Invalid parameter state:%u  I3D_driver::GetState()", state);
        return 0;
    }
    return renderState[state];
}

// Walks a triangle's edge from top to bottom, widening each row's span to the edge's pixel. An inline function: the
// original stores the copies of its parameters (ye, xe, ys) before the ye == ys test, which a macro or the code written
// out doesn't give. The loop takes its own x and y (not the parameters walked in place) and x >> 7 at each use.
static inline void ScanEdge(int xs, int ys, int xe, int ye)
{
    if (ye == ys)
        return;
    if (ye < ys) {
        int t = ye;
        ye = ys;
        ys = t;
        t = xe;
        xe = xs;
        xs = t;
    }
    int dx = ((xe - xs) << 7) / (ye - ys);
    int x = xs << 7;
    for (int y = ys; y <= ye; y++) {
        if (x >> 7 < shadow_spans[y][0])
            shadow_spans[y][0] = x >> 7;
        if (x >> 7 > shadow_spans[y][1])
            shadow_spans[y][1] = x >> 7;
        x += dx;
    }
}

// The shadow texture rasterizer (PC only, names guessed). A shadow's silhouette comes in texture coordinates, pairs of
// ints; each triangle's rows go into shadow_spans (its leftmost and rightmost pixel per row, in 7-bit fixed point
// while walking an edge), which an __asm block then fills: alpha 15 in a 16-bit texture, or 255 in SW's 8-bit buffer
// of 256 bytes a row.
// FUNCTION: LS3DF 0x10015e80
void I3D_driver::DrawShadowTriangles(const int *verts, const ushort *indx, uint num_faces, void *dst, int pitch)
{
    for (uint i = num_faces; i; i--, indx += 3) {
        const int *v0 = &verts[indx[0] * 2];
        int x0 = v0[0], y0 = v0[1];
        const int *v1 = &verts[indx[1] * 2];
        int x1 = v1[0], y1 = v1[1];
        const int *v2 = &verts[indx[2] * 2];
        int x2 = v2[0], y2 = v2[1];
        __asm {
            lea edi, shadow_spans
            mov ecx, 0x80
            mov eax, 0x00ff00ff
            rep stosd
        }
        int y_min = y0, y_max = y0;
        if (y1 < y_min)
            y_min = y1;
        if (y2 < y_min)
            y_min = y2;
        if (y1 > y_max)
            y_max = y1;
        if (y2 > y_max)
            y_max = y2;
        ScanEdge(x0, y0, x1, y1);
        ScanEdge(x1, y1, x2, y2);
        ScanEdge(x2, y2, x0, y0);
        __asm {
            lea esi, shadow_spans
            mov eax, y_min
            mov ebx, y_max
            sub ebx, eax
            jz done
            mov edi, eax
            imul edi, pitch
            add edi, dst
            shl eax, 1
            add esi, eax
            align 16
        row:
            push edi
            xor ecx, ecx
            xor edx, edx
            mov dl, [esi]
            mov cl, [esi + 1]
            sub cl, dl
            jz next
            shl edx, 1
            add edi, edx
            mov eax, 0xf000f000
            shr ecx, 1
            jc odd
            rep stosd
            jmp next
        odd:
            stosw
            jz next
            rep stosd
        next:
            pop edi
            add edi, pitch
            add esi, 2
            dec ebx
            jnz row
        done:
        }
    }
}

// FUNCTION: LS3DF 0x100160d0
void I3D_driver::DrawShadowTrianglesSW(const int *verts, const ushort *indx, uint num_faces)
{
    for (uint i = num_faces; i; i--, indx += 3) {
        const int *v0 = &verts[indx[0] * 2];
        int x0 = v0[0], y0 = v0[1];
        const int *v1 = &verts[indx[1] * 2];
        int x1 = v1[0], y1 = v1[1];
        const int *v2 = &verts[indx[2] * 2];
        int x2 = v2[0], y2 = v2[1];
        __asm {
            lea edi, shadow_spans
            mov ecx, 0x80
            mov eax, 0x00ff00ff
            rep stosd
        }
        int y_min = y0, y_max = y0;
        if (y1 < y_min)
            y_min = y1;
        if (y2 < y_min)
            y_min = y2;
        if (y1 > y_max)
            y_max = y1;
        if (y2 > y_max)
            y_max = y2;
        ScanEdge(x0, y0, x1, y1);
        ScanEdge(x1, y1, x2, y2);
        ScanEdge(x2, y2, x0, y0);
        __asm {
            lea esi, shadow_spans
            mov eax, y_min
            mov ebx, y_max
            sub ebx, eax
            jz done
            mov edi, eax
            imul edi, 0x100
            add edi, shadow_buffer
            shl eax, 1
            add esi, eax
            xor ecx, ecx
            xor edx, edx
            mov al, 0xff
            align 16
        row:
            push edi
            mov dl, [esi]
            mov cl, [esi + 1]
            sub cl, dl
            jz next
            add edi, edx
            rep stosb
            align 16
        next:
            pop edi
            add edi, 0x100
            add esi, 2
            dec ebx
            jnz row
        done:
        }
    }
}

// Renders a shadow's triangles (verts: x, y pairs, inside box) into its texture's alpha: through the 16-bit
// rasterizer, or with RS_SHADOW_SW at twice the size into shadow_buffer, then averaged down 2x2 into 4-bit alpha
// FUNCTION: LS3DF 0x10016310
LS3D_RESULT I3D_driver::RenderShadowTexture(const float *verts, uint num_verts, const ushort *indx, uint num_faces,
    ITexture *texture, const float *box)
{
    static C_list<int> points;
    points.reserve(num_verts * 2);
    uint pitch;
    if (renderState[RS_SHADOW_SW]) {
        // Twice the texture's size, less one: the last pixel's coordinate
        ushort sx = texture->width, sy = texture->height;
        sy = sy * 2 - 1;
        __asm {
            mov edi, shadow_buffer
            xor ecx, ecx
            mov cx, sy
            inc ecx
            imul ecx, 0x40
            xor eax, eax
            rep stosd
        }
        sx = sx * 2 - 1;
        float scale_x = sx / (box[3] - box[0]);
        float scale_y = sy / (box[4] - box[1]);
        __asm {
            mov ecx, num_verts
            mov edi, dword ptr points
            mov ebx, box
            mov esi, verts
            fld dword ptr [ebx]
            fld dword ptr [ebx + 4]
            fld scale_x
            fld scale_y
            align 16
        convert_sw:
            fld dword ptr [esi]
            fsub st, st(4)
            fmul st, st(2)
            fistp dword ptr [edi]
            fld dword ptr [esi + 4]
            fsub st, st(3)
            fmul st, st(1)
            fistp dword ptr [edi + 4]
            add esi, 8
            add edi, 8
            dec ecx
            jnz convert_sw
            fcompp
            fcompp
        }
        DrawShadowTrianglesSW(&points[0], indx, num_faces);
        uchar *dst = (uchar *)texture->Lock(NULL, pitch, 0);
        int side = texture->width;
        int skip = 0x200 - (side << 1);
        __asm {
            mov edi, dst
            mov esi, shadow_buffer
            mov ecx, side
            align 16
        row:
            mov edx, edi
            mov ebx, side
            align 16
        column:
            xor eax, eax
            mov al, [esi]
            add al, [esi + 1]
            adc ah, 0
            add al, [esi + 0x100]
            adc ah, 0
            add al, [esi + 0x101]
            adc ah, 0
            shr eax, 6
            shl eax, 12
            mov [edx], ax
            add edx, 2
            add esi, 2
            dec ebx
            jnz column
            add edi, pitch
            add esi, skip
            dec ecx
            jnz row
        }
    } else {
        texture->Fill(NULL, 0, 0);
        float scale_x = (texture->width - 1) / (box[3] - box[0]);
        float scale_y = (texture->height - 1) / (box[4] - box[1]);
        __asm {
            mov ecx, num_verts
            mov edi, dword ptr points
            mov ebx, box
            mov esi, verts
            fld dword ptr [ebx]
            fld dword ptr [ebx + 4]
            fld scale_x
            fld scale_y
            align 16
        convert:
            fld dword ptr [esi]
            fsub st, st(4)
            fmul st, st(2)
            fistp dword ptr [edi]
            fld dword ptr [esi + 4]
            fsub st, st(3)
            fmul st, st(1)
            fistp dword ptr [edi + 4]
            add esi, 8
            add edi, 8
            dec ecx
            jnz convert
            fcompp
            fcompp
        }
        void *dst = texture->Lock(NULL, pitch, 0);
        DrawShadowTriangles(&points[0], indx, num_faces, dst, pitch);
    }
    texture->Unlock();
    texture->AddDirtyRect(NULL);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10016600
I3D_material *I3D_driver::GetIcon(uint res_id)
{
    HMODULE module = GetModuleHandleA("LS3DF.dll");
    HGLOBAL res = LoadResource(module, FindResourceA(module, (LPCSTR)res_id, RT_BITMAP));
    if (!res) {
        dbgPrintf("ERROR LOADING RESOURCES !!!!");
        return NULL;
    }
    void *data = LockResource(res);
    I3D_material *mat = CreateMaterial();
    if (!mat) {
        FreeResource(res);
        return NULL;
    }
    ITexture *texture = pGlobalIGraph->CreateITexture();
    texture->Create((const char *)data, NULL, 0x209, 0, 0, 0);
    mat->SetTexture(texture);
    texture->Release();
    mat->flags |= MATF_TXT_ALPHA;
    FreeResource(res);
    return mat;
}

// The draws' color: the alpha byte is a transparency
static inline uint MakeColor(float a, const S_vector &color)
{
    return FloatToInt(a * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 | FloatToInt(color.y * 255.0f) << 8 |
        FloatToInt(color.z * 255.0f);
}

// FUNCTION: LS3DF 0x100166c0
void I3D_driver::DrawPoint(const S_vector &pos, const S_vector &color, uint alpha)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    float a = (255 - (alpha & 0xff)) * (1.0f / 255.0f);
    uint c = MakeColor(a, color);
    LVERTEX v;
    v.x = pos.x;
    v.y = pos.y;
    v.z = pos.z;
    v.color = c;
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    SetBlendState(0);
    m_pD3DDevice8->SetTexture(0, NULL);
    SetVertexFormat(0);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &v, sizeof(v));
}

// FUNCTION: LS3DF 0x100168f0
void I3D_driver::DrawPoint2D(int x, int y, const S_vector &color, uint alpha)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    float a = (255 - (alpha & 0xff)) * (1.0f / 255.0f);
    uint c = MakeColor(a, color);
    TLVERTEX v;
    v.x = (float)x;
    v.y = (float)y;
    v.z = 0.0f;
    v.rhw = 1.0f;
    v.color = c;
    v.specular = 0;
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    SetBlendState(0);
    m_pD3DDevice8->SetTexture(0, NULL);
    SetVertexFormat(4);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &v, sizeof(v));
}

// FUNCTION: LS3DF 0x10016b20
void I3D_driver::DrawLine(const S_vector &from, const S_vector &to, const S_vector &color, uint alpha)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    float a = (255 - (alpha & 0xff)) * (1.0f / 255.0f);
    uint c = MakeColor(a, color);
    LVERTEX v[2];
    v[0].x = from.x;
    v[0].y = from.y;
    v[0].z = from.z;
    v[0].color = c;
    v[1].x = to.x;
    v[1].y = to.y;
    v[1].z = to.z;
    v[1].color = c;
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    SetBlendState(0);
    m_pD3DDevice8->SetTexture(0, NULL);
    SetVertexFormat(0);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_LINELIST, 1, v, sizeof(LVERTEX));
}

// FUNCTION: LS3DF 0x10016d70
void I3D_driver::DrawLines(const S_vector *verts, uint num_verts, const ushort *indx, uint num_indx,
    const S_vector &color, uint vstride, uint alpha)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    float a = (255 - (alpha & 0xff)) * (1.0f / 255.0f);
    uint c = MakeColor(a, color);
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    SetBlendState(0);
    m_pD3DDevice8->SetTexture(0, NULL);
    SetVertexFormat(0);
    LVertList.resize(num_verts);
    LVERTEX *lv = &LVertList[0];
    for (uint i = 0; i < num_verts; i++) {
        const S_vector *v = (const S_vector *)((const uchar *)verts + i * vstride);
        lv->x = v->x;
        lv->y = v->y;
        lv->z = v->z;
        lv->color = c;
        lv++;
    }
    m_pD3DDevice8->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, num_verts, num_indx / 2, indx, D3DFMT_INDEX16,
        &LVertList[0], sizeof(LVERTEX));
}

// DrawTriangles' flags: the transparency is the low byte (Insanity3D's name for its mask, spelling included; the
// other names are guesses)
#define I3DDRAW_TRANPARENCY_MASK 0xff
#define I3DDRAW_TRANSFORMED 0x100       // the vertices are transformed (TLVERTEX)
#define I3DDRAW_NOCULL 0x200
#define I3DDRAW_CULL_CW 0x400

// FUNCTION: LS3DF 0x10016ff0
LS3D_RESULT I3D_driver::DrawTriangles(const void *verts, uint num_verts, const ushort *indx, uint num_indx,
    const S_vector &color, uint vstride, uint flags, const I3D_text_coor *tx)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    float a = (255 - (flags & I3DDRAW_TRANPARENCY_MASK)) * (1.0f / 255.0f);
    uint c = MakeColor(a, color);
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    if (m_CurrBlendState) {
        m_CurrBlendState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    uint no_cull = flags & I3DDRAW_NOCULL;
    if (no_cull) {
        if (m_CurrCullState != D3DCULL_NONE) {
            m_CurrCullState = D3DCULL_NONE;
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        }
    } else if (flags & I3DDRAW_CULL_CW) {
        if (m_CurrCullState != D3DCULL_CW) {
            m_CurrCullState = D3DCULL_CW;
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        }
    } else {
        if (m_CurrCullState != D3DCULL_CCW) {
            m_CurrCullState = D3DCULL_CCW;
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, m_CullMode);
        }
    }
    if (!(flags & I3DDRAW_TRANSFORMED)) {
        LVertList.resize(num_verts);
        LVERTEX *lv = &LVertList[0];
        if (tx) {
            for (uint i = 0; i < num_verts; i++) {
                const S_vector *v = (const S_vector *)((const uchar *)verts + i * vstride);
                const I3D_text_coor *t = (const I3D_text_coor *)((const uchar *)tx + i * vstride);
                lv->x = v->x;
                lv->y = v->y;
                lv->z = v->z;
                lv->color = c;
                lv->u = t->u;
                lv->v = t->v;
                lv++;
            }
        } else {
            for (uint i = 0; i < num_verts; i++) {
                const S_vector *v = (const S_vector *)((const uchar *)verts + i * vstride);
                lv->x = v->x;
                lv->y = v->y;
                lv->z = v->z;
                lv->color = c;
                lv++;
            }
            m_pD3DDevice8->SetTexture(0, NULL);
        }
        if (m_CurrVertexFormat) {
            m_CurrVertexFormat = 0;
            m_pD3DDevice8->SetVertexShader(m_LVertexVS);
        }
        m_pD3DDevice8->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, num_verts, num_indx / 3, indx, D3DFMT_INDEX16,
            &LVertList[0], sizeof(LVERTEX));
    } else {
        TLVertList.resize(num_verts);
        TLVERTEX *tlv = &TLVertList[0];
        if (tx) {
            for (uint i = 0; i < num_verts; i++) {
                const float *v = (const float *)((const uchar *)verts + i * vstride);
                const I3D_text_coor *t = (const I3D_text_coor *)((const uchar *)tx + i * vstride);
                tlv->x = v[0];
                tlv->y = v[1];
                tlv->z = v[2];
                tlv->rhw = v[3];
                tlv->color = c;
                tlv->specular = 0;
                tlv->u = t->u;
                tlv->v = t->v;
                tlv++;
            }
        } else {
            for (uint i = 0; i < num_verts; i++) {
                const float *v = (const float *)((const uchar *)verts + i * vstride);
                tlv->x = v[0];
                tlv->y = v[1];
                tlv->z = v[2];
                tlv->rhw = v[3];
                tlv->color = c;
                tlv->specular = 0;
                tlv++;
            }
            m_pD3DDevice8->SetTexture(0, NULL);
        }
        SetVertexFormat(4);
        m_pD3DDevice8->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, num_verts, num_indx / 3, indx, D3DFMT_INDEX16,
            &TLVertList[0], sizeof(TLVERTEX));
    }
    if (no_cull || flags & I3DDRAW_CULL_CW) {
        if (m_CurrCullState != D3DCULL_CCW) {
            m_CurrCullState = D3DCULL_CCW;
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, m_CullMode);
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10017580
void I3D_driver::DrawCircle2D(const S_vector2 &pos, float radius, const S_vector &color, int num_segments,
    uint alpha)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    float a = (255 - (alpha & 0xff)) * (1.0f / 255.0f);
    uint c = MakeColor(a, color);
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    SetBlendState(0);
    S_matrix m = RotateZMatrix(PI * 2.0f / num_segments);
    num_segments++;
    TLVertList.resize(num_segments);
    TLVERTEX *v = &TLVertList[0];
    S_vector dir(0.0f, -radius, 0.0f);
    for (int i = num_segments; i--; v++) {
        v->x = dir.x + pos.x;
        v->y = dir.y + pos.y;
        v->z = 0.0f;
        v->rhw = 1.0f;
        v->color = c;
        v->specular = 0;
        dir = dir.RotateByMatrix(m);
    }
    TLVertList[--num_segments] = TLVertList[0];
    m_pD3DDevice8->SetTexture(0, NULL);
    SetVertexFormat(4);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_LINESTRIP, num_segments, &TLVertList[0], sizeof(TLVERTEX));
}

// FUNCTION: LS3DF 0x10017890
void I3D_driver::DrawText(const S_vector &pos, const char *text, uchar alpha, float size)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    if (m_CurrCullState != D3DCULL_NONE) {
        m_CurrCullState = D3DCULL_NONE;
        m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }
    I3D_frame *cam = pCurrentCamera;
    if (!cam)
        return;
    S_matrix m = cam->GetMatrix();
    m(3) = pos;
    pGlobalIGraph->SetWorldMatrix(m);
    I3D_material *mat = pFontMat;
    float a = (255 - alpha) * (1.0f / 255.0f);
    uint c = MakeColor(a, mat->diffuse);
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    if (m_CurrBlendState) {
        m_CurrBlendState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    ITexture *texture = mat->texture;
    m_pD3DDevice8->SetTexture(0, texture ? texture->texture : NULL);
    if (m_CurrVertexFormat) {
        m_CurrVertexFormat = 0;
        m_pD3DDevice8->SetVertexShader(m_LVertexVS);
    }
    size *= 16.0f;
    float du = 16.0f / pFontMat->GetTexture()->width;
    float dv = 16.0f / pFontMat->GetTexture()->height;
    int len = strlen(text);
    LVertList.resize(len * 6);
    LVERTEX *v = &LVertList[0];
    for (int i = 0; i < len; i++) {
        uchar ch = text[i];
        float u = ch % 16 * du;
        float t = (ch >> 4) * dv;
        v[0].x = i * size;
        v[0].y = 0.0f;
        v[0].z = 0.0f;
        v[0].color = c;
        v[0].u = u;
        v[0].v = t + dv;
        v[1].x = (i + 1.0f) * size;
        v[1].y = size;
        v[1].z = 0.0f;
        v[1].color = c;
        v[1].u = u + du;
        v[1].v = t;
        v[2].x = i * size;
        v[2].y = size;
        v[2].z = 0.0f;
        v[2].color = c;
        v[2].u = u;
        v[2].v = t;
        v[3].x = (i + 1.0f) * size;
        v[3].y = 0.0f;
        v[3].z = 0.0f;
        v[3].color = c;
        v[3].u = u + du;
        v[3].v = t + dv;
        v[4] = v[1];
        v[5] = v[0];
        v += 6;
    }
    m_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_TRIANGLELIST, len * 2, &LVertList[0], sizeof(LVERTEX));
    if (renderState[RS_WIREFRAME])
        m_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
}

// FUNCTION: LS3DF 0x10017d40
void I3D_driver::DrawText2D(int x, int y, const char *text, uchar alpha, float size)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    if (m_CurrCullState != D3DCULL_NONE) {
        m_CurrCullState = D3DCULL_NONE;
        m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }
    I3D_material *mat = pFontMat;
    if (!mat)
        return;
    float a = (255 - alpha) * (1.0f / 255.0f);
    uint c = MakeColor(a, mat->diffuse);
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else
        SetAlphaState(1);
    if (m_CurrBlendState) {
        m_CurrBlendState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    if (mat->texture)
        m_pD3DDevice8->SetTexture(0, mat->texture->texture);
    else
        m_pD3DDevice8->SetTexture(0, NULL);
    if (m_CurrVertexFormat != 4) {
        m_CurrVertexFormat = 4;
        m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    }
    size *= 16.0f;
    float du = 16.0f / pFontMat->GetTexture()->width;
    float dv = 16.0f / pFontMat->GetTexture()->height;
    int len = strlen(text);
    TLVertList.resize(len * 6);
    TLVERTEX *v = &TLVertList[0];
    for (int i = 0; i < len; i++) {
        uchar ch = text[i];
        float u = ch % 16 * du;
        float t = (ch >> 4) * dv;
        v[0].x = i * size + x;
        v[0].y = (float)y;
        v[0].z = 1.0f;
        v[0].rhw = 1.0f;
        v[0].color = c;
        v[0].specular = 0;
        v[0].u = u;
        v[0].v = t;
        v[1].x = (i + 1.0f) * size + x;
        v[1].y = y + size;
        v[1].z = 1.0f;
        v[1].rhw = 1.0f;
        v[1].color = c;
        v[1].specular = 0;
        v[1].u = u + du;
        v[1].v = t + dv;
        v[2].x = i * size + x;
        v[2].y = y + size;
        v[2].z = 1.0f;
        v[2].rhw = 1.0f;
        v[2].color = c;
        v[2].specular = 0;
        v[2].u = u;
        v[2].v = t + dv;
        v[3].x = (i + 1.0f) * size + x;
        v[3].y = (float)y;
        v[3].z = 1.0f;
        v[3].rhw = 1.0f;
        v[3].color = c;
        v[3].specular = 0;
        v[3].u = u + du;
        v[3].v = t;
        v[4] = v[2];
        v[5] = v[0];
        v += 6;
    }
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_TRIANGLELIST, len * 2, &TLVertList[0], sizeof(TLVERTEX));
    if (!renderState[RS_LINEARFILTER]) {
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    }
    if (renderState[RS_WIREFRAME])
        m_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, ZBufferEnable);
}

// FUNCTION: LS3DF 0x100182f0
void I3D_driver::DrawSprite(const S_vector &pos, I3D_material *mat, uchar alpha, float size)
{
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != TRUE) {
        m_CurrClipState = TRUE;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    I3D_frame *cam = pCurrentCamera;
    if (!cam)
        return;
    S_matrix m = cam->GetMatrix();
    m(3) = pos;
    pGlobalIGraph->SetWorldMatrix(m);
    uint c = MakeColor((255 - alpha) * (1.0f / 255.0f), mat->diffuse);
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    float lo = size * -.5f, hi = size * .5f;
    LVERTEX v[4];
    v[0].x = lo;
    v[0].y = lo;
    v[0].z = 0.0f;
    v[0].color = c;
    v[0].u = .001f;
    v[0].v = .999f;
    v[1].x = lo;
    v[1].y = hi;
    v[1].z = 0.0f;
    v[1].color = c;
    v[1].u = .001f;
    v[1].v = .001f;
    v[2].x = hi;
    v[2].y = lo;
    v[2].z = 0.0f;
    v[2].color = c;
    v[2].u = .999f;
    v[2].v = .999f;
    v[3].x = hi;
    v[3].y = hi;
    v[3].z = 0.0f;
    v[3].color = c;
    v[3].u = .999f;
    v[3].v = .001f;
    if (mat->flags & MATF_ADD_MODE) {
        SetAlphaState(1);
        if (m_CurrBlendState != 1) {
            m_CurrBlendState = 1;
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        }
    } else if (mat->flags & MATF_TXT_ALPHA) {
        SetAlphaState(1);
        if (*(int *)&mat->emissive.x || *(int *)&mat->emissive.y || *(int *)&mat->emissive.z)
            SetBlendState(1);
        else
            SetBlendState(0);
    } else if (mat->flags & MATF_CKEY_ALPHA) {
        SetAlphaState(3);
        SetBlendState(0);
    } else
        SetAlphaState(0);
    ITexture *texture = mat->texture;
    m_pD3DDevice8->SetTexture(0, texture ? texture->texture : NULL);
    SetVertexFormat(0);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(LVERTEX));
}

// FUNCTION: LS3DF 0x100186a0
void I3D_driver::DrawBox(const I3D_bbox &bbox, const S_vector *line_color, const S_vector *fill_color, ushort alpha)
{
    ushort line_indx[24] = {0, 1, 0, 2, 0, 4, 1, 3, 1, 5, 2, 3, 2, 6, 3, 7, 4, 5, 4, 6, 5, 7, 6, 7};
    ushort fill_indx[36] = {
        0, 1, 2, 2, 1, 3, 4, 6, 5, 5, 6, 7, 0, 2, 4, 2, 6, 4,
        1, 5, 3, 5, 7, 3, 0, 4, 1, 1, 4, 5, 2, 3, 6, 3, 7, 6,
    };
    if (bbox.min.x <= bbox.max.x) {
        m_pD3DDevice8->SetTexture(0, NULL);
        const S_vector *verts = BBoxVertices(bbox);
        if (fill_color)
            DrawTriangles(verts, 8, fill_indx, 36, *fill_color, sizeof(S_vector), alpha, NULL);
        if (line_color)
            DrawLines(verts, 8, line_indx, 24, *line_color, sizeof(S_vector), alpha >> 8);
    }
}

// A unit sphere's three circles, DrawSphere's lines
// GLOBAL: LS3DF 0x100af930
static S_vector VolumeBall[48] = {
    S_vector(1.0f, 0.0f, 0.0f), S_vector(.92f, .39f, 0.0f), S_vector(.71f, .71f, 0.0f), S_vector(.39f, .92f, 0.0f),
    S_vector(0.0f, 1.0f, 0.0f), S_vector(-.39f, .92f, 0.0f), S_vector(-.71f, .71f, 0.0f), S_vector(-.92f, .39f, 0.0f),
    S_vector(-1.0f, 0.0f, 0.0f), S_vector(-.92f, -.39f, 0.0f), S_vector(-.71f, -.71f, 0.0f),
    S_vector(-.39f, -.92f, 0.0f), S_vector(0.0f, -1.0f, 0.0f), S_vector(.39f, -.92f, 0.0f), S_vector(.71f, -.71f, 0.0f),
    S_vector(.92f, -.39f, 0.0f),
    S_vector(1.0f, 0.0f, 0.0f), S_vector(.92f, 0.0f, .39f), S_vector(.71f, 0.0f, .71f), S_vector(.39f, 0.0f, .92f),
    S_vector(0.0f, 0.0f, 1.0f), S_vector(-.39f, 0.0f, .92f), S_vector(-.71f, 0.0f, .71f), S_vector(-.92f, 0.0f, .39f),
    S_vector(-1.0f, 0.0f, 0.0f), S_vector(-.92f, 0.0f, -.39f), S_vector(-.71f, 0.0f, -.71f),
    S_vector(-.39f, 0.0f, -.92f), S_vector(0.0f, 0.0f, -1.0f), S_vector(.39f, 0.0f, -.92f), S_vector(.71f, 0.0f, -.71f),
    S_vector(.92f, 0.0f, -.39f),
    S_vector(0.0f, 0.0f, 1.0f), S_vector(0.0f, .39f, .92f), S_vector(0.0f, .71f, .71f), S_vector(0.0f, .92f, .39f),
    S_vector(0.0f, 1.0f, 0.0f), S_vector(0.0f, .92f, -.39f), S_vector(0.0f, .71f, -.71f), S_vector(0.0f, .39f, -.92f),
    S_vector(0.0f, 0.0f, -1.0f), S_vector(0.0f, -.39f, -.92f), S_vector(0.0f, -.71f, -.71f),
    S_vector(0.0f, -.92f, -.39f), S_vector(0.0f, -1.0f, 0.0f), S_vector(0.0f, -.92f, .39f), S_vector(0.0f, -.71f, .71f),
    S_vector(0.0f, -.39f, .92f),
};

// FUNCTION: LS3DF 0x10018cc0
void I3D_driver::DrawSphere(const S_matrix &matrix, const I3D_bsphere &bsphere, const S_vector &color, ushort alpha)
{
    if (bsphere.radius <= 0.0f)
        return;
    static S_matrix m;
    m.Identity();
    m(3) = bsphere.pos;
    m(0, 0) = m(1, 1) = m(2, 2) = bsphere.radius;
    m = m * matrix;
    pGlobalIGraph->SetWorldMatrix(m);
    DrawLines(VolumeBall, 48, VolumeBallIndx, 96, color, sizeof(S_vector), alpha >> 8);
}

// FUNCTION: LS3DF 0x10018d80
I3D_visual *I3D_driver::CreateVisual(I3D_VISUAL_TYPE type)
{
    I3D_visual *visual;
    switch (type) {
    case I3D_VISUAL_OBJECT:
        visual = new I3D_object;
        break;
    case I3D_VISUAL_LIT_OBJECT:
        visual = new I3D_lit_object;
        break;
    case I3D_VISUAL_BILLBOARD:
        visual = new I3D_billboard;
        break;
    case I3D_VISUAL_SINGLEMESH:
        visual = new I3D_singlemesh;
        break;
    case I3D_VISUAL_LENSF:
        visual = new I3D_lensflare;
        break;
    case I3D_VISUAL_MORPH:
        visual = new I3D_morph;
        break;
    case I3D_VISUAL_SINGLEMORPH:
        visual = new I3D_singlemorph;
        break;
    case I3D_VISUAL_PROJECTOR:
        visual = new I3D_projector;
        break;
    case I3D_VISUAL_PART_BASE:
        visual = new I3D_part_base;
        break;
    case I3D_VISUAL_PART_ELEMENT:
        visual = new I3D_part_element;
        break;
    case I3D_VISUAL_LANDPATCH:
        visual = new I3D_landpatch;
        break;
    case I3D_VISUAL_MIRROR:
        visual = new I3D_mirror;
        break;
    default:
        dbgPrintf("!! invalid visual type: %d  %s  I3D_driver::CreateVisual()", type, VisualType2String(type));
        return NULL;
    }
    if (!visual)
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
    return visual;
}

// FUNCTION: LS3DF 0x10019090
I3D_material *I3D_driver::CreateMaterial()
{
    I3D_material *mat = new I3D_material;
    if (!mat)
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
    return mat;
}

// FUNCTION: LS3DF 0x10019100
I3D_anim_material *I3D_driver::CreateAnimMaterial()
{
    I3D_anim_material *mat = new I3D_anim_material;
    if (!mat)
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
    return mat;
}

// FUNCTION: LS3DF 0x10019180
I3D_triface *I3D_driver::CreateTrifaceArray(int num)
{
    return new I3D_triface[num];
}

// FUNCTION: LS3DF 0x100191a0
I3D_face_group *I3D_driver::CreateFaceGroup()
{
    return new I3D_face_group;
}

// FUNCTION: LS3DF 0x10019200
I3D_mesh_level *I3D_driver::CreateMeshLevel()
{
    return new I3D_mesh_level;
}

// FUNCTION: LS3DF 0x10019260
I3D_mesh_object *I3D_driver::CreateMesh()
{
    return new I3D_mesh_object;
}

// FUNCTION: LS3DF 0x100192c0
I3D_animation_set *I3D_driver::CreateAnimationSet()
{
    return new I3D_animation_set;
}

// FUNCTION: LS3DF 0x10019320
I3D_pose *I3D_driver::CreatePose()
{
    return new I3D_pose;
}

// FUNCTION: LS3DF 0x10019380
I3D_pose_set *I3D_driver::CreatePoseSet()
{
    return new I3D_pose_set;
}

// FUNCTION: LS3DF 0x100193e0
I3D_frame *I3D_driver::CreateFrame(I3D_FRAME_TYPE type)
{
    I3D_frame *frame;
    switch (type) {
    case FRAME_LIGHT:
        frame = new I3D_light;
        break;
    case FRAME_CAMERA:
        frame = new I3D_camera;
        break;
    case FRAME_SOUND:
        frame = new I3D_sound;
        if (frame)
            sounds.push_back((I3D_sound *)frame);
        break;
    case FRAME_SECTOR:
        frame = new I3D_sector;
        break;
    case FRAME_DUMMY:
        frame = new I3D_dummy;
        break;
    case FRAME_USER:
        frame = new I3D_user;
        break;
    case FRAME_MODEL:
        frame = new I3D_model;
        break;
    case FRAME_JOINT:
        frame = new I3D_joint;
        break;
    case FRAME_VOLUME:
        frame = new I3D_volume;
        break;
    case FRAME_OCCLUDER:
        frame = new I3D_occluder;
        break;
    case FRAME_SCENE: {
        I3D_scene *scene = new I3D_scene;
        LS3D_VIEWPORT *vp = pGlobalIGraph->GetViewPort();
        scene->SetViewport(vp->x, vp->y, vp->width - vp->x, vp->height - vp->y);
        if (pISndDriver && pISndDriver->IsInit())
            pISndDriver->CreateListener(&scene->listener);
        scenes.push_back(scene);
        frame = scene;
    } break;
    case FRAME_AREA:
        frame = new I3D_area;
        break;
    case FRAME_TARGET:
        frame = new I3D_target;
        break;
    case FRAME_SHADOW:
        frame = new I3D_shadow;
        break;
    case FRAME_LANDSCAPE:
        frame = new I3D_landscape;
        break;
    case FRAME_EMITOR:
        frame = new I3D_emitor;
        break;
    default:
        dbgPrintf("!! invalid frame type %d   I3D_driver::CreateFrame()", type);
        return NULL;
    }
    if (!frame)
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
    return frame;
}

// FUNCTION: LS3DF 0x10019880
LS3D_RESULT I3D_driver::EnumVisualTypes(I3D_ENUMVISUALPROC *proc, bool all) const
{
    if (!proc)
        return I3DERR_INVALIDPARAMS;
    proc(I3D_VISUAL_BILLBOARD, "Billboard");
    proc(I3D_VISUAL_LIT_OBJECT, "Lit-object");
    proc(I3D_VISUAL_MORPH, "Morph");
    proc(I3D_VISUAL_OBJECT, "Object");
    proc(I3D_VISUAL_SINGLEMESH, "Single-mesh");
    proc(I3D_VISUAL_SINGLEMORPH, "Single-morph");
    proc(I3D_VISUAL_LENSF, "Lensflare/Glow");
    proc(I3D_VISUAL_PROJECTOR, "Projector");
    proc(I3D_VISUAL_MIRROR, "Mirror");
    if (all) {
        proc(I3D_VISUAL_PART_ELEMENT, "Particle element");
        proc(I3D_VISUAL_SHADOW, "Shadow");
        proc(I3D_VISUAL_PART_BASE, "Part. element base");
    }
    return I3D_OK;
}

#define CASE_NAME(x) \
    case x: \
        return #x;

// FUNCTION: LS3DF 0x10019930
const char *FrameType2StringFunc(I3D_FRAME_TYPE type)
{
    switch (type) {
    CASE_NAME(FRAME_VISUAL)
    CASE_NAME(FRAME_LIGHT)
    CASE_NAME(FRAME_CAMERA)
    CASE_NAME(FRAME_SOUND)
    CASE_NAME(FRAME_SECTOR)
    CASE_NAME(FRAME_DUMMY)
    CASE_NAME(FRAME_TARGET)
    CASE_NAME(FRAME_USER)
    CASE_NAME(FRAME_MODEL)
    CASE_NAME(FRAME_JOINT)
    CASE_NAME(FRAME_VOLUME)
    CASE_NAME(FRAME_OCCLUDER)
    CASE_NAME(FRAME_SCENE)
    CASE_NAME(FRAME_AREA)
    CASE_NAME(FRAME_SHADOW)
    CASE_NAME(FRAME_EMITOR)
    CASE_NAME(FRAME_LANDSCAPE)
    }
    static char str[64];
    sprintf(str, "Unknown type:0x%x", type);
    return str;
}

// FUNCTION: LS3DF 0x10019a10
const char *I3D_driver::FrameType2String(I3D_FRAME_TYPE type) const
{
    return FrameType2StringFunc(type);
}

// FUNCTION: LS3DF 0x10019a20
const char *I3D_driver::VisualType2String(I3D_VISUAL_TYPE type) const
{
    switch (type) {
    CASE_NAME(I3D_VISUAL_OBJECT)
    CASE_NAME(I3D_VISUAL_LIT_OBJECT)
    CASE_NAME(I3D_VISUAL_BILLBOARD)
    CASE_NAME(I3D_VISUAL_SINGLEMESH)
    CASE_NAME(I3D_VISUAL_LENSF)
    CASE_NAME(I3D_VISUAL_MORPH)
    CASE_NAME(I3D_VISUAL_SINGLEMORPH)
    CASE_NAME(I3D_VISUAL_PROJECTOR)
    CASE_NAME(I3D_VISUAL_PART_BASE)
    CASE_NAME(I3D_VISUAL_PART_ELEMENT)
    CASE_NAME(I3D_VISUAL_SHADOW)
    CASE_NAME(I3D_VISUAL_MIRROR)
    }
    static char si[5];
    static char str[64];
    *(uint *)si = type;
    si[4] = 0;
    sprintf(str, "Unknown type:0x%x  %s", type, si);
    return str;
}

// The fast find tree: the frames by name, those of one name a list to the right. Both searches test a link before
// they follow it (VC6 loads it once, into eax), and test the compare's result a third time (the original's dead test
// that loops back to the compare). Unlike PS2's, this one goes on past a sector of another scene.
// FUNCTION: LS3DF 0x10019b10
I3D_frame *I3D_driver::FindFrameFast(I3D_scene *scene, const char *name) const
{
    if (!pFFRoot)
        return NULL;
    I3D_frame *frm = pFFRoot;
    for (;;) {
        int cmp = strcmp(name, frm->GetName());
        if (cmp < 0) {
            if (!frm->ff_left)
                return NULL;
            frm = frm->ff_left;
        } else if (cmp > 0) {
            if (!frm->ff_right)
                return NULL;
            frm = frm->ff_right;
        } else if (!cmp) {
            do {
                for (I3D_frame *f = frm; f; f = f->parent) {
                    if (f->GetType() == FRAME_SECTOR && ((I3D_sector *)f)->scene == scene)
                        return frm;
                }
                if (!frm->ff_right)
                    return NULL;
                frm = frm->ff_right;
            } while (!strcmp(name, frm->GetName()));
            return NULL;
        }
    }
}

// FUNCTION: LS3DF 0x10019c10
I3D_frame *I3D_driver::FindFrameFast_(const char *name, I3D_frame *frame)
{
    pFFLastUp_ = NULL;
    if (!pFFRoot)
        return NULL;
    I3D_frame *frm = pFFRoot;
    for (;;) {
        int cmp = strcmp(name, frm->GetName());
        if (cmp < 0) {
            if (!frm->ff_left)
                return NULL;
            pFFLastUp_ = frm;
            frm = frm->ff_left;
        } else if (cmp > 0) {
            if (!frm->ff_right)
                return NULL;
            pFFLastUp_ = frm;
            frm = frm->ff_right;
        } else if (!cmp) {
            // A while (true): its first test returns, so VC6 keeps it at the top
            while (true) {
                if (frm == frame)
                    return frm;
                if (!frm->ff_right) {
                    pFFLastUp_ = NULL;
                    return NULL;
                }
                pFFLastUp_ = frm;
                frm = frm->ff_right;
                if (strcmp(name, frm->GetName()))
                    return NULL;
            }
        }
    }
}

// FUNCTION: LS3DF 0x10019d30
void I3D_driver::OptimalizeFastFindData()
{
    OFFD_ListSize = pFFRoot->OFFD_CountFrames();
    OFFD_ListUsed = 0;
    OFFD_pFrameList = new I3D_frame *[OFFD_ListSize];
    if (!OFFD_pFrameList) {
        dbgPrintf("!! Out of memory !!  OptimalizeFastFindData() [1]");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return;
    }
    pFFRoot->AlphabeticalListOfFrames_();
    OFFD_RebuildBranchOfFFTree_(&pFFRoot, 0, OFFD_ListUsed);
    delete[] OFFD_pFrameList;
}

// FUNCTION: LS3DF 0x10019dc0
void I3D_driver::OFFD_RebuildBranchOfFFTree_(I3D_frame **branch, int first, int num)
{
    if (!num)
        return;
    // The middle, moved back to the first of the frames of its name
    int half = num / 2;
    for (; half > 0; half--) {
        if (strcmp(OFFD_pFrameList[first + half - 1]->GetName(), OFFD_pFrameList[first + half]->GetName()))
            break;
    }
    // The frames of the same name are a list to the right
    I3D_frame **last = branch;
    *branch = OFFD_pFrameList[first + half];
    int n = 1;
    for (; half + n < num; n++) {
        if (strcmp(OFFD_pFrameList[first + half]->GetName(), OFFD_pFrameList[first + half + n]->GetName()))
            break;
        (*last)->ff_left = NULL;
        last = &(*last)->ff_right;
        *last = OFFD_pFrameList[first + half + n];
    }
    if (half > 0)
        OFFD_RebuildBranchOfFFTree_(&(*branch)->ff_left, first, half);
    else
        (*branch)->ff_left = NULL;
    if (half + n < num)
        OFFD_RebuildBranchOfFFTree_(&(*last)->ff_right, first + half + n, num - half - n);
    else
        (*last)->ff_right = NULL;
}

// FUNCTION: LS3DF 0x10019fd0
LS3D_RESULT I3D_driver::EnumFFTree(I3D_ENUMPROC *proc, uint context) const
{
    struct _hlp {
        // FUNCTION: LS3DF 0x1001a050
        I3DENUMRET enumSubtree(I3D_ENUMPROC *proc, uint context, I3D_frame *frm)
        {
            I3DENUMRET er;
            if (frm->ff_left) {
                er = enumSubtree(proc, context, frm->ff_left);
                if (er)
                    return er;
            }
            er = proc(frm, context);
            if (er)
                return er;
            if (frm->ff_right)
                return enumSubtree(proc, context, frm->ff_right);
            return er;
        }
    } hlp;
    if (!pFFRoot)
        return I3D_OK;
    I3DENUMRET er = hlp.enumSubtree(proc, context, pFFRoot);
    return er ? I3DERR_CANCELED : I3D_OK;
}

// FUNCTION: LS3DF 0x1001a0a0
void I3D_driver::DeleteFromSceneTickList(I3D_frame *frame)
{
    for (int i = scenes.size(); i--; ) {
        if (!scenes[i]->DeleteTickFrame(frame))
            break;
    }
}

// FUNCTION: LS3DF 0x1001a0e0
void I3D_driver::DeleteFromAllShadows(I3D_frame *frame)
{
    I3D_shadow::DeleteFromAnyShadow(frame);
}

// FUNCTION: LS3DF 0x1001a0f0
void I3D_driver::DeleteFromAllTargets(I3D_frame *frame)
{
    I3D_target::DeleteFromAnyTarget(frame);
}

// Empty, as on PS2. The linker folds it with the DirectShow base classes' empty CBaseRenderer::OnRenderEnd, whose
// copy the original kept (0x10087ec0, annotated in library_strmbase.h).
void I3D_driver::m_RecomputeUsedVRAM()
{
}

// The other files' globals

// GLOBAL: LS3DF 0x100a93f0
bool RecomputeAllGeometry;


// The functions of other files the driver calls, until those are written. They come after the driver's functions:
// VC6 knows that a function compiled earlier in the file can't throw, and would leave the EH states out of
// ~I3D_driver and the Create functions. Their bodies differ, or the linker would fold them; auto_inline(off) keeps
// VC6 from inlining them here. The markers of the ones below the driver's addresses are in I3D.h, by name, as the
// linter checks the order of a file's markers.
#pragma auto_inline(off)

// STUB: LS3DF 0x1001ef70
I3D_landpatch::I3D_landpatch()
{
    renderState[36]++;
}

// STUB: LS3DF 0x1001fbb0
I3D_landscape::I3D_landscape()
{
    renderState[24]++;
}

// STUB: LS3DF 0x10021570
I3D_lit_object::I3D_lit_object()
{
    renderState[27]++;
}

// STUB: LS3DF 0x1002ade0
I3D_material::I3D_material()
{
    renderState[38]++;
}

// STUB: LS3DF 0x1002b190
I3D_anim_material::I3D_anim_material()
{
    renderState[39]++;
}

// STUB: LS3DF 0x10031820
I3D_mirror::I3D_mirror()
{
    renderState[37]++;
}

// STUB: LS3DF 0x10034e30
I3D_morph::I3D_morph()
{
    renderState[31]++;
}

// STUB: LS3DF 0x1003a430
I3D_occluder::I3D_occluder()
{
    renderState[19]++;
}

// STUB: LS3DF 0x1003de50
I3D_emitor::I3D_emitor()
{
    renderState[25]++;
}

// STUB: LS3DF 0x1003fa20
I3D_part_element::I3D_part_element()
{
    renderState[35]++;
}

// STUB: LS3DF 0x100406c0
I3D_part_base::I3D_part_base()
{
    renderState[34]++;
}

// STUB: LS3DF 0x10045e30
I3D_projector::I3D_projector()
{
    renderState[33]++;
}

// STUB: LS3DF 0x10050950
I3D_shadow::I3D_shadow()
{
    renderState[23]++;
}

// STUB: LS3DF 0x10051480
void I3D_shadow::DeleteFromAnyShadow(I3D_frame *frame)
{
    renderState[5]++;
}

// STUB: LS3DF 0x10054240
I3D_singlemesh::I3D_singlemesh()
{
    renderState[29]++;
}

// STUB: LS3DF 0x10056e80
I3D_singlemorph::I3D_singlemorph()
{
    renderState[32]++;
}

// STUB: LS3DF 0x10057650
I3D_sound::I3D_sound()
{
    renderState[12]++;
}

// STUB: LS3DF 0x10062010
void I3D_driver::initRender()
{
    renderState[0]++;
}

// STUB: LS3DF 0x10062020
void I3D_driver::closeRender()
{
    renderState[1]++;
}

// STUB: LS3DF 0x10062030
LS3D_RESULT I3D_driver::Render(I3D_scene *scene)
{
    renderState[2]++;
    return I3D_OK;
}

// STUB: LS3DF 0x100658f0
void I3D_driver::BuildAllVBuffers(I3D_scene *scene)
{
    renderState[3]++;
}

#pragma auto_inline(on)
