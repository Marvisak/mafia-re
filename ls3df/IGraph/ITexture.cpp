// Textures (PS2 ITexture.cpp), at 0x10075c10-0x1007b053 in the original. Create loads a TGA or BMP image (and an
// optional alpha image), converts it to the texture's format level by level, compressing it to DXT through S3's
// S3TC library where asked, and saves the result next to the image to load it faster the next time. PC only: the
// texture manager at the end of the file, which loads ITC_MANAGED textures from those saved files on a thread of
// its own into a few pools of default pool textures. The header's zero vector has its $E after Create
// (0x10078170), as in IGraph.cpp.
#include "IGraph/IGraph.h"
#include "S3TC/s3_intrf.h"
#include "dta.h"

#include <stdlib.h>

// windows.h's macro for LoadImageA/W
#undef LoadImage

// IGraph.cpp
extern CRITICAL_SECTION m_FrameSection;
extern D3DFORMAT m_TrueColorTexturePixelFormat;
extern D3DFORMAT m_DXTTexturePixelFormat;
extern C_list<ITexture *> m_UncachedTextureList;
extern C_list<ITexture *> m_CachedTextureList;
extern D3DFORMAT m_Alpha1TexturePixelFormat;
extern D3DFORMAT m_AlphaTexturePixelFormat;
extern uint m_CurrVertexFormat;
extern uint m_MaxTextureHeight;
extern D3DFORMAT m_TexturePixelFormat;
extern uint m_HardwareCaps;
extern D3DFORMAT m_DXTAlphaTexturePixelFormat;
extern uint m_MaxTextureWidth;
extern IDirect3DDevice8 *m_pD3DDevice8;
extern bool m_SceneBegun;
extern bool m_RenderThreadRunning;
extern int m_FrameCounter;

// What Preload draws: one point, which makes the driver upload the texture
// GLOBAL: LS3DF 0x100aad38
static LS3D_TLVERTEX preload_vertex = {0.0f, 0.0f, 0.0f, 1.0f, 0, 0, 0.0f, 0.0f};

// Draw(int, int)'s quad, a triangle strip; Draw fills in the positions
// GLOBAL: LS3DF 0x100aad58
static LS3D_TLVERTEX draw_quad[4] = {
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 1.0f, 1.0f},
};

// Draw(RECT, ...)'s, whose colors are the arguments
// GLOBAL: LS3DF 0x100aadd8
static LS3D_TLVERTEX draw_quad_colored[4] = {
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff, 0, 1.0f, 1.0f},
};

// The texture manager's three pools: how much video memory each may take before its thread evicts the least recently
// used texture of the pool, and the size of its textures (a managed texture's file name ends in its pool's number)
// GLOBAL: LS3DF 0x100aae58
static int pool_budgets[3] = {0x100000, 0x100000, 0x100000};

// GLOBAL: LS3DF 0x100aae64
static int pool_sizes[3] = {256, 128, 64};

// The versions of the presaved files (PS2 name for the DXT one)
// GLOBAL: LS3DF 0x1009d8d0
static const short DXTCurrentVersion = 9;

// GLOBAL: LS3DF 0x1009d8d2
static const short TEXCurrentVersion = 1;

bool LoadImage(const char *name, uint sx, uint sy, uint flags, void **bits, void **palette, BITMAPINFOHEADER *bmi,
    bool from_memory, FILETIME *time, int handle);
bool CompressTexture(const char *name, int type, IDirect3DSurface8 *src, IDirect3DSurface8 *dst);
bool AddTextureToTextureManager(ITexture *texture, const char *name, uint flags);
bool RemoveTextureFromTextureManager(ITexture *texture);

// The texture size Create makes of an image: half of it with ITC_HALF, scaled down to fit the card's limits, then
// rounded up to powers of two, at least 4 where the card needs that for DXT (m_HardwareCaps & 8)
#define TEXTURE_SIZE(sx, sy)                                                                                         \
    {                                                                                                                \
        float scale_x = 1.0f;                                                                                        \
        if (sx > m_MaxTextureWidth)                                                                                  \
            scale_x = (float)m_MaxTextureWidth / sx;                                                                 \
        float scale_y = 1.0f;                                                                                        \
        if (sy > m_MaxTextureHeight)                                                                                 \
            scale_y = (float)m_MaxTextureHeight / sy;                                                                \
        float scale = min(scale_x, scale_y);                                                                         \
        sx = (ushort)(sx * scale);                                                                                   \
        sy = (ushort)(sy * scale);                                                                                   \
        ushort x = m_HardwareCaps & 8 ? 4 : 1, y = m_HardwareCaps & 8 ? 4 : 1;                                       \
        while (sx > x)                                                                                               \
            x <<= 1;                                                                                                 \
        while (sy > y)                                                                                               \
            y <<= 1;                                                                                                 \
        width = x;                                                                                                   \
        height = y;                                                                                                  \
    }

// FUNCTION: LS3DF 0x10075c10
ITexture::ITexture()
{
    uid = 0;
    init = false;
    texture = NULL;
    lock_surface = NULL;
    ref = 1;
    pixels = NULL;
    mem_size = 0;
    flags = 0;
    create_flags = 0;
    managed = NULL;
}

// FUNCTION: LS3DF 0x10075c50
ITexture::~ITexture()
{
    Close();
}

// FUNCTION: LS3DF 0x10075cd0
int ITexture::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! m_RefCount  ITexture this:0x%x", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// Makes this a copy of another texture, with a texture of its own
// FUNCTION: LS3DF 0x10075d10
LS3D_RESULT ITexture::Duplicate(ITexture *src)
{
    IDirect3DSurface8 *src_surface, *dst_surface;
    Close();
    if (src && src->init) {
        flags = src->flags;
        create_flags = src->create_flags;
        file_name = src->file_name;
        name = src->name;
        alpha_file_name = src->alpha_file_name;
        alpha_name = src->alpha_name;
        width = src->width;
        height = src->height;
        color_key = src->color_key;
        if (src->pixels) {
            pixels = new uchar[width * height * 4];
            if (!pixels)
                return I3DERR_OUTOFMEM;
            memcpy(pixels, src->pixels, width * height * 4);
        }
        levels = src->levels;
        mem_size = src->mem_size;
        D3DSURFACE_DESC desc;
        src->texture->GetLevelDesc(0, &desc);
        m_pD3DDevice8->CreateTexture(width, height, levels, 0, desc.Format, D3DPOOL_MANAGED, &texture);
        for (int i = levels; i; ) {
            src->texture->GetSurfaceLevel(--i, &src_surface);
            texture->GetSurfaceLevel(i, &dst_surface);
            m_pD3DDevice8->CopyRects(src_surface, NULL, 0, dst_surface, NULL);
            src_surface->Release();
            dst_surface->Release();
        }
        m_UncachedTextureList.push_back(this);
        init = true;
        Preload();
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10075f30
LS3D_RESULT ITexture::Reload()
{
    dbgPrintf("ITexture::Reload() - WAS NOT IMPLEMENTED !");
    return I3DERR_UNSUPPORTED;
}

// Creates the texture from an image file (or one in memory, ITC_FROM_MEMORY), with the alpha from another image if
// given, or an empty one (ITC_EMPTY). sx and sy override the image's size, levels the number of mipmap levels.
// FUNCTION: LS3DF 0x10075f50
LS3D_RESULT ITexture::Create(const char *name, const char *alpha_name, uint flags, ushort sx, ushort sy,
    ushort levels)
{
    // PS2 names: the PS2 build has these as globals
    void *pSrcBMPPtr = NULL;
    void *pSrcBMPPal = NULL;
    int SrcBMPPitchAdder = 0;
    void *pSrcOptBMPPtr = NULL;
    void *pSrcOptBMPPal = NULL;
    int SrcOptBMPPitchAdder = 0;
    FILETIME SrcOptBMPLastWriteTime;
    SrcOptBMPLastWriteTime.dwHighDateTime = 0;
    SrcOptBMPLastWriteTime.dwLowDateTime = 0;
    uchar *pOptBMPPtr = NULL;
    uchar TempBPP = 0;
    BITMAPINFOHEADER SrcBMPInfo, SrcOptBMPInfo;
    FILETIME SrcBMPLastWriteTime;

    Close();
    this->flags = flags;
    create_flags = flags;
    if (flags & ITC_MANAGED) {
        file_name = name;
        int len = file_name.Size();
        const char *cp = &file_name[0], *slash = (const char *)-1;
        for (; len; len--, cp++) {
            if (*cp == '\\')
                slash = cp;
        }
        this->name = slash + 1 ? slash + 1 : cp;
        if (!AddTextureToTextureManager(this, name, this->flags))
            return I3DERR_GENERIC;
        init = true;
        m_CachedTextureList.push_back(this);
        return I3D_OK;
    }

    if (flags & ITC_FROM_MEMORY) {
        this->flags = flags | ITC_UNCACHED;
        if (!name)
            return I3DERR_INVALIDPARAMS;
        if (sx > 0)
            width = sx;
        else
            width = (ushort)((BITMAPINFOHEADER *)name)->biWidth;
        if (sy > 0)
            height = sy;
        else
            height = (ushort)((BITMAPINFOHEADER *)name)->biHeight;
        ushort w = width, h = height;
        if (this->flags & ITC_HALF) {
            w >>= 1;
            h >>= 1;
        }
        TEXTURE_SIZE(w, h);
        if (!LoadImage(name, 0, 0, this->flags, &pSrcBMPPtr, &pSrcBMPPal, &SrcBMPInfo, true, NULL, -1))
            return I3DERR_FILECORRUPTED;
        SrcBMPPitchAdder = SrcBMPInfo.biBitCount / 8 * SrcBMPInfo.biWidth + SrcBMPInfo.biXPelsPerMeter;
    } else if (name) {
        this->flags = flags & ~ITC_EMPTY;
        file_name = name;
        int handle = -1;
        LS3D_RESULT result = pGlobalIGraph->GetImageInfo(name, width, height, TempBPP, 0, &handle);
        if (result < 0)
            return result;
        int len = file_name.Size();
        const char *cp = &file_name[0], *slash = (const char *)-1;
        for (; len; len--, cp++) {
            if (*cp == '\\')
                slash = cp;
        }
        this->name = slash + 1 ? slash + 1 : cp;
        if (this->flags & ITC_ALPHA) {
            if (TempBPP == 8 || TempBPP == 24)
                this->flags &= ~ITC_ALPHA;
            if (TempBPP == 16)
                this->flags |= ITC_COLORKEY;
        }
        if (sx > 0)
            width = sx;
        if (sy > 0)
            height = sy;
        if (!(this->flags & ITC_ANYSIZE)) {
            ushort w = width, h = height;
            if (this->flags & ITC_HALF) {
                w >>= 1;
                h >>= 1;
            }
            TEXTURE_SIZE(w, h);
        }
        if (!LoadImage(name, 0, 0, this->flags, &pSrcBMPPtr, &pSrcBMPPal, &SrcBMPInfo, false, &SrcBMPLastWriteTime,
                handle))
            return I3DERR_FILECORRUPTED;
        SrcBMPPitchAdder = SrcBMPInfo.biBitCount / 8 * SrcBMPInfo.biWidth + SrcBMPInfo.biXPelsPerMeter;
    } else {
        if (!(flags & ITC_EMPTY)) {
            dbgPrintf("ITexture::Create() - Warning! Neither Texture nor Alpha was specified, but ITC_EMPTY wasn't "
                "used!");
            this->flags |= ITC_EMPTY;
        }
        width = sx;
        height = sy;
        ushort w = sx, h = sy;
        if (this->flags & ITC_HALF) {
            w >>= 1;
            h >>= 1;
        }
        if (!(this->flags & ITC_ANYSIZE))
            TEXTURE_SIZE(w, h);
    }

    if (alpha_name && *alpha_name && !(this->flags & ITC_USE_ALPHA)) {
        dbgPrintf("ITexture::Create() - Warning! AlphaName was specified, but ITC_USE_ALPHA wasn't! (%s, %s)", name,
            alpha_name);
        this->flags |= ITC_USE_ALPHA;
    }
    if (this->flags & ITC_USE_ALPHA) {
        if (alpha_name) {
            alpha_file_name = alpha_name;
            if (!LoadImage(alpha_name, SrcBMPInfo.biWidth, SrcBMPInfo.biHeight, this->flags, &pSrcOptBMPPtr,
                    &pSrcOptBMPPal, &SrcOptBMPInfo, false, &SrcOptBMPLastWriteTime, -1)) {
                delete[] pSrcBMPPal;
                delete[] pSrcBMPPtr;
                return I3DERR_NOFILE1;
            }
            int len = alpha_file_name.Size();
            const char *cp = &alpha_file_name[0], *slash = (const char *)-1;
            for (; len; len--, cp++) {
                if (*cp == '\\')
                    slash = cp;
            }
            this->alpha_name = slash + 1 ? slash + 1 : cp;
            pOptBMPPtr = new uchar[SrcOptBMPInfo.biHeight * SrcOptBMPInfo.biXPelsPerMeter];
            if (!pOptBMPPtr) {
                delete[] pSrcBMPPtr;
                delete[] pSrcOptBMPPtr;
                delete[] pSrcBMPPal;
                delete[] pSrcOptBMPPal;
                return I3DERR_OUTOFMEM;
            }
            // The alpha is the image's brightness
            switch (SrcOptBMPInfo.biBitCount / 8) {
            case 3: {
                uchar *src = (uchar *)pSrcOptBMPPtr, *dst = pOptBMPPtr;
                for (int i = SrcOptBMPInfo.biHeight - 1; i >= 0; i--) {
                    for (int j = 0; j < SrcOptBMPInfo.biWidth; j++) {
                        *dst++ = (src[2] * 77 + src[1] * 154 + src[0] * 25) >> 8;
                        src += 3;
                    }
                }
                break;
            }
            case 1: {
                uchar *src = (uchar *)pSrcOptBMPPtr, *dst = pOptBMPPtr;
                for (int i = SrcOptBMPInfo.biHeight - 1; i >= 0; i--) {
                    for (int j = 0; j < SrcOptBMPInfo.biWidth; j++) {
                        uchar *c = (uchar *)pSrcOptBMPPal + *src++ * 4;
                        *dst++ = (c[2] * 77 + c[1] * 154 + c[0] * 25) >> 8;
                    }
                }
                break;
            }
            default:
                dbgPrintf("IImage::Create() - Unsuppoted ALPHA source file %i bit [ %i(%i), %i ] format. Only 8,24 bit "
                    "are supported ! : %s", SrcOptBMPInfo.biBitCount, SrcOptBMPInfo.biWidth,
                    SrcOptBMPInfo.biWidth * SrcOptBMPInfo.biBitCount / 8, SrcOptBMPInfo.biHeight, alpha_name);
                delete[] pSrcBMPPtr;
                delete[] pSrcOptBMPPtr;
                delete[] pSrcBMPPal;
                delete[] pSrcOptBMPPal;
                return I3DERR_BADTEXTUREFORMAT;
            }
            SrcOptBMPInfo.biXPelsPerMeter = SrcOptBMPInfo.biWidth;
            SrcOptBMPInfo.biBitCount = 8;
            SrcOptBMPPitchAdder = SrcOptBMPInfo.biWidth * 2;
            delete[] pSrcOptBMPPtr;
            pSrcOptBMPPtr = NULL;
            delete[] pSrcOptBMPPal;
            pSrcOptBMPPal = NULL;
        } else if (!(this->flags & ITC_EMPTY)) {
            this->flags &= ~ITC_USE_ALPHA;
            dbgPrintf("ITexture::Create() - Warning! ITC_USE_ALPHA specified, but ( AlphaName == NULL ) !!  %s", name);
        }
    }

    // The format, and the uncompressed one a DXT texture is converted to first
    D3DFORMAT format;
    D3DFORMAT dxt_format = (D3DFORMAT)0;
    ushort dxt = 0;
    if (this->flags & ITC_TRUECOLOR)
        format = m_TrueColorTexturePixelFormat;
    if (!(this->flags & ITC_TRUECOLOR) || !format) {
        this->flags &= ~ITC_TRUECOLOR;
        if ((this->flags & (ITC_ALPHA | ITC_USE_ALPHA)) && !(this->flags & ITC_COLORKEY)) {
            if (this->flags & ITC_DXT)
                format = m_DXTAlphaTexturePixelFormat;
            if (!(this->flags & ITC_DXT) || !format)
                format = m_AlphaTexturePixelFormat;
            else {
                dxt = 3;
                dxt_format = m_AlphaTexturePixelFormat;
            }
        } else if (this->flags & ITC_COLORKEY) {
            if (this->flags & ITC_MIPMAP) {
                if (this->flags & ITC_DXT)
                    format = m_DXTAlphaTexturePixelFormat;
                if (!(this->flags & ITC_DXT) || !format)
                    format = m_AlphaTexturePixelFormat;
                else {
                    dxt = 3;
                    dxt_format = m_AlphaTexturePixelFormat;
                }
            } else {
                if (this->flags & ITC_DXT)
                    format = m_DXTTexturePixelFormat;
                if (!(this->flags & ITC_DXT) || !format)
                    format = m_Alpha1TexturePixelFormat;
                else {
                    dxt = 2;
                    dxt_format = m_Alpha1TexturePixelFormat;
                }
            }
        } else {
            if (this->flags & ITC_DXT)
                format = m_DXTTexturePixelFormat;
            if (!(this->flags & ITC_DXT) || !format)
                format = m_TexturePixelFormat;
            else {
                dxt = 1;
                dxt_format = m_Alpha1TexturePixelFormat;
            }
        }
    }
    this->format = format;
    uint num_levels = this->flags & ITC_MIPMAP ? levels : 1;
    m_pD3DDevice8->CreateTexture(width, height, num_levels, 0, format, D3DPOOL_MANAGED, &texture);
    this->levels = (ushort)texture->GetLevelCount();

    if (!(this->flags & (ITC_EMPTY | ITC_KEEP_PIXELS | ITC_FROM_MEMORY))) {
        bool presaved = false;
        if (dxt == 1)
            presaved = m_TryPresavedDXT(name, NULL, 1, &SrcBMPLastWriteTime, NULL);
        else if (dxt > 1)
            presaved = m_TryPresavedDXT(name, this->alpha_name, dxt, &SrcBMPLastWriteTime, &SrcOptBMPLastWriteTime);
        else if (this->format == D3DFMT_R5G6B5 || this->format == D3DFMT_X1R5G5B5 ||
                this->format == D3DFMT_A1R5G5B5)
            presaved = m_TryPresaved_555_565(name, &SrcBMPLastWriteTime);
        if (presaved) {
            init = true;
            if (!(this->flags & ITC_UNCACHED))
                m_CachedTextureList.push_back(this);
            else
                m_UncachedTextureList.push_back(this);
            delete[] pSrcBMPPtr;
            delete[] pSrcBMPPal;
            Preload();
            return I3D_OK;
        }
    }

    if (!(this->flags & ITC_EMPTY)) {
        pixels = new uchar[SrcBMPInfo.biWidth * SrcBMPInfo.biHeight * 4];
        if (!pixels) {
            delete[] pSrcBMPPtr;
            delete[] pSrcBMPPal;
            return I3DERR_OUTOFMEM;
        }
        // To ARGB bytes, the rows from the top: the image's start bottom-up
        color_key = 0;
        uchar *dst = pixels;
        switch (SrcBMPInfo.biBitCount) {
        case 8: {
            // The color key is palette entry 0's color in the texture's format
            uchar *pal = (uchar *)pSrcBMPPal;
            switch (format) {
            case D3DFMT_A8R8G8B8:
                color_key = pal[0] | pal[2] << 16 | pal[1] << 8;
                break;
            case D3DFMT_A4R4G4B4:
            case D3DFMT_DXT3:
                color_key = pal[0] >> 4 | pal[2] >> 4 << 8 | pal[1] & 0xf0;
                break;
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
            case D3DFMT_DXT1:
                color_key = pal[0] >> 3 | pal[2] >> 3 << 10 | pal[1] >> 3 << 5;
                break;
            case D3DFMT_R5G6B5:
                color_key = pal[0] >> 3 | pal[2] >> 3 << 11 | pal[1] >> 2 << 5;
                break;
            }
            uchar *opt = NULL;
            if (pOptBMPPtr)
                opt = pOptBMPPtr + (SrcOptBMPInfo.biHeight - 1) * SrcOptBMPInfo.biWidth;
            uchar *src = (uchar *)pSrcBMPPtr + (SrcBMPInfo.biHeight - 1) * SrcBMPInfo.biXPelsPerMeter;
            for (int i = SrcBMPInfo.biHeight - 1; i >= 0; i--) {
                for (int j = 0; j < SrcBMPInfo.biWidth; j++) {
                    uchar c = *src;
                    if (opt)
                        *dst = *opt++;
                    else if (this->flags & ITC_COLORKEY) {
                        uint color;
                        uchar *p = pal + c * 4;
                        switch (format) {
                        case D3DFMT_A8R8G8B8:
                            color = p[0] | p[2] << 16 | p[1] << 8;
                            break;
                        case D3DFMT_A4R4G4B4:
                        case D3DFMT_DXT3:
                            color = p[0] >> 4 | p[2] >> 4 << 8 | p[1] & 0xf0;
                            break;
                        case D3DFMT_X1R5G5B5:
                        case D3DFMT_A1R5G5B5:
                        case D3DFMT_DXT1:
                            color = p[0] >> 3 | p[2] >> 3 << 10 | p[1] >> 3 << 5;
                            break;
                        case D3DFMT_R5G6B5:
                            color = p[0] >> 3 | p[2] >> 3 << 11 | p[1] >> 2 << 5;
                            break;
                        }
                        *dst = color == color_key ? 0 : 0xff;
                    } else
                        *dst = 0xff;
                    dst[1] = pal[c * 4 + 2];
                    dst[2] = pal[c * 4 + 1];
                    dst[3] = pal[c * 4];
                    dst += 4;
                    src++;
                }
                src -= SrcBMPPitchAdder;
                opt -= SrcOptBMPPitchAdder;
            }
            break;
        }
        case 16: {
            // X1R5G5B5
            uchar *opt = NULL;
            if (pOptBMPPtr)
                opt = pOptBMPPtr + (SrcOptBMPInfo.biHeight - 1) * SrcOptBMPInfo.biXPelsPerMeter;
            SrcBMPPitchAdder = (uint)SrcBMPPitchAdder >> 1;
            ushort *src = (ushort *)((uchar *)pSrcBMPPtr + (SrcBMPInfo.biHeight - 1) * SrcBMPInfo.biXPelsPerMeter);
            for (int i = SrcBMPInfo.biHeight - 1; i >= 0; i--) {
                for (int j = 0; j < SrcBMPInfo.biWidth; j++) {
                    ushort c = *src;
                    if (this->flags & ITC_ALPHA)
                        *dst = c & 0x8000 ? 0xff : 0;
                    else if (opt)
                        *dst = *opt++;
                    else if ((this->flags & ITC_COLORKEY) && c == color_key)
                        *dst = 0;
                    else
                        *dst = 0xff;
                    dst[1] = (c & 0x7c00) >> 7;
                    dst[2] = (c & 0x3e0) >> 2;
                    dst[3] = (c & 0x1f) << 3;
                    dst += 4;
                    src++;
                }
                src -= SrcBMPPitchAdder;
                opt -= SrcOptBMPPitchAdder;
            }
            break;
        }
        case 24: {
            uchar *opt = NULL;
            if (pOptBMPPtr)
                opt = pOptBMPPtr + (SrcOptBMPInfo.biHeight - 1) * SrcOptBMPInfo.biXPelsPerMeter;
            uchar *src = (uchar *)pSrcBMPPtr + (SrcBMPInfo.biHeight - 1) * SrcBMPInfo.biXPelsPerMeter;
            for (int i = SrcBMPInfo.biHeight - 1; i >= 0; i--) {
                for (int j = 0; j < SrcBMPInfo.biWidth; j++) {
                    uchar g = src[1], b = src[0], r = src[2];
                    if (opt)
                        *dst = *opt++;
                    else if ((this->flags & ITC_COLORKEY) && b == color_key)
                        *dst = 0;
                    else
                        *dst = 0xff;
                    dst[1] = r;
                    dst[2] = g;
                    dst[3] = b;
                    dst += 4;
                    src += 3;
                }
                src -= SrcBMPPitchAdder;
                opt -= SrcOptBMPPitchAdder;
            }
            break;
        }
        case 32: {
            uchar *opt = NULL;
            if (pOptBMPPtr)
                opt = pOptBMPPtr + (SrcOptBMPInfo.biHeight - 1) * SrcOptBMPInfo.biXPelsPerMeter;
            uchar *src = (uchar *)pSrcBMPPtr + (SrcBMPInfo.biHeight - 1) * SrcBMPInfo.biXPelsPerMeter;
            for (int i = SrcBMPInfo.biHeight - 1; i >= 0; i--) {
                for (int j = 0; j < SrcBMPInfo.biWidth; j++) {
                    uchar g = src[1], b = src[0], r = src[2];
                    if (this->flags & ITC_ALPHA)
                        *dst = src[3];
                    else if (opt)
                        *dst = *opt++;
                    else if ((this->flags & ITC_COLORKEY) && b == color_key)
                        *dst = 0;
                    else
                        *dst = 0xff;
                    dst[1] = r;
                    dst[2] = g;
                    dst[3] = b;
                    dst += 4;
                    src += 4;
                }
                src -= SrcBMPPitchAdder;
                opt -= SrcOptBMPPitchAdder;
            }
            break;
        }
        default:
            dbgPrintf("IImage::Create() - Unsuppoted source file %i bit [ %i(%i), %i ] format. Only 8,16,24,32 bit are "
                "supported ! : %s", SrcBMPInfo.biBitCount, SrcBMPInfo.biWidth,
                SrcBMPInfo.biWidth * SrcBMPInfo.biBitCount / 8, SrcBMPInfo.biHeight, name);
            delete[] pSrcBMPPtr;
            delete[] pSrcBMPPal;
            delete[] pOptBMPPtr;
            return I3DERR_BADTEXTUREFORMAT;
        }
        delete[] pSrcBMPPtr;
        delete[] pSrcBMPPal;
        delete[] pOptBMPPtr;
    }

    if (!(this->flags & ITC_EMPTY)) {
        if (width != SrcBMPInfo.biWidth || height != SrcBMPInfo.biHeight) {
            uchar *resized = NULL;
            pGlobalIGraph->ResizeImage(SrcBMPInfo.biWidth, SrcBMPInfo.biHeight, SrcBMPInfo.biWidth * 4, pixels,
                width, height, width * 4, &resized, 4, 1);
            delete[] pixels;
            pixels = resized;
        }
    }

    if (!(this->flags & ITC_FROM_MEMORY) && dxt >= 1) {
        if (m_TryPresavedDXT(name, this->alpha_name, dxt, &SrcBMPLastWriteTime, &SrcOptBMPLastWriteTime)) {
            init = true;
            if (!(this->flags & ITC_UNCACHED))
                m_CachedTextureList.push_back(this);
            else
                m_UncachedTextureList.push_back(this);
            Preload();
            return I3D_OK;
        }
    }

    // The levels: the first from the image, each further one a quarter of the one before, filtered from it
    uint level_sx = width, level_sy = height;
    uint pitch = width * 4;
    uchar *level_src = NULL, *level_dst = NULL;
    if (!(this->flags & ITC_EMPTY) && this->levels > 1) {
        level_src = new uchar[level_sx * level_sy * 4];
        if (!level_src)
            return I3DERR_GENERIC;
        memcpy(level_src, pixels, width * height * 4);
        level_dst = new uchar[width * height * 2];
        if (!level_dst) {
            delete[] level_src;
            return I3DERR_OUTOFMEM;
        }
    }
    IDirect3DTexture8 *dxt_texture = NULL;
    if (!(this->flags & ITC_EMPTY) && dxt_format) {
        if (FAILED(m_pD3DDevice8->CreateTexture(width, height, num_levels, 0, dxt_format, D3DPOOL_MANAGED,
                &dxt_texture))) {
            delete[] level_src;
            delete[] level_dst;
            return I3DERR_GENERIC;
        }
    }
    for (uint level = 0; level < this->levels; level++) {
        IDirect3DSurface8 *surface;
        if (FAILED(texture->GetSurfaceLevel(level, &surface))) {
            if (dxt_texture)
                dxt_texture->Release();
            delete[] level_src;
            delete[] level_dst;
            return I3DERR_GENERIC;
        }
        D3DSURFACE_DESC desc;
        if (FAILED(surface->GetDesc(&desc))) {
            surface->Release();
            if (dxt_texture)
                dxt_texture->Release();
            delete[] level_src;
            delete[] level_dst;
            return I3DERR_GENERIC;
        }
        mem_size += desc.Size;
        if (!(this->flags & ITC_EMPTY)) {
            bool ok;
            if (!level) {
                if (dxt_texture) {
                    IDirect3DSurface8 *dxt_surface;
                    if (FAILED(dxt_texture->GetSurfaceLevel(level, &dxt_surface))) {
                        surface->Release();
                        dxt_texture->Release();
                        delete[] level_src;
                        delete[] level_dst;
                        return I3DERR_GENERIC;
                    }
                    ok = m_Convert(pixels, dxt_surface, width, height, 0);
                    CompressTexture(file_name, dxt, dxt_surface, surface);
                    dxt_surface->Release();
                } else
                    ok = m_Convert(pixels, surface, width, height, dxt);
            } else {
                // Each pixel of the smaller level from four: 3/8 of the two on one diagonal, 1/8 of the others
                uint half_pitch = pitch >> 1;
                for (uint y = 0; y < level_sy - 1; y += 2) {
                    for (uint x = 0; x < level_sx - 1; x += 2) {
                        uchar *p00 = level_src + y * pitch + x * 4, *p01 = level_src + y * pitch + (x + 1) * 4;
                        uchar *p10 = level_src + (y + 1) * pitch + x * 4;
                        uchar *p11 = level_src + (y + 1) * pitch + (x + 1) * 4;
                        uchar *d = level_dst + y / 2 * half_pitch + x / 2 * 4;
                        d[0] = (p11[0] >> 3) + (p00[0] >> 3) + (p10[0] * 3 >> 3) + (p01[0] * 3 >> 3);
                        d[1] = (p11[1] >> 3) + (p00[1] >> 3) + (p10[1] * 3 >> 3) + (p01[1] * 3 >> 3);
                        d[2] = (p11[2] >> 3) + (p00[2] >> 3) + (p10[2] * 3 >> 3) + (p01[2] * 3 >> 3);
                        d[3] = (p11[3] >> 3) + (p00[3] >> 3) + (p10[3] * 3 >> 3) + (p01[3] * 3 >> 3);
                    }
                }
                level_sx >>= 1;
                level_sy >>= 1;
                if (!level_sx)
                    level_sx = 1;
                if (!level_sy)
                    level_sy = 1;
                pitch = half_pitch;
                if (dxt_texture) {
                    IDirect3DSurface8 *dxt_surface;
                    if (FAILED(dxt_texture->GetSurfaceLevel(level, &dxt_surface))) {
                        surface->Release();
                        dxt_texture->Release();
                        delete[] level_src;
                        delete[] level_dst;
                        return I3DERR_GENERIC;
                    }
                    ok = m_Convert(level_dst, dxt_surface, level_sx, level_sy, 0);
                    CompressTexture(file_name, dxt, dxt_surface, surface);
                    dxt_surface->Release();
                } else
                    ok = m_Convert(level_dst, surface, level_sx, level_sy, dxt);
                uchar *swap = level_dst;
                level_dst = level_src;
                level_src = swap;
            }
            if (!ok) {
                if (dxt_texture)
                    dxt_texture->Release();
                delete[] level_src;
                delete[] level_dst;
                return I3DERR_GENERIC;
            }
        }
        surface->Release();
    }

    if (this->flags & ITC_EMPTY) {
        if (this->flags & ITC_FILL)
            Fill(NULL, 0, 0);
        m_UncachedTextureList.push_back(this);
        init = true;
        return I3D_OK;
    }
    if (!(this->flags & ITC_KEEP_PIXELS)) {
        delete[] pixels;
        pixels = NULL;
    }
    if (dxt_texture)
        dxt_texture->Release();
    delete[] level_src;
    delete[] level_dst;
    if (!(this->flags & ITC_UNCACHED))
        m_CachedTextureList.push_back(this);
    else
        m_UncachedTextureList.push_back(this);
    init = true;
    Preload();

    // Saves the levels for m_TryPresavedDXT and m_TryPresaved_555_565 (PS2 saves only the DXT ones)
    if (!(this->flags & ITC_FROM_MEMORY) && dxt > 0) {
        char file[256];
        memset(file, 0, 255);
        int len = strlen(name) - 4;
        strncpy(file, name, len);
        if (this->alpha_name && dxt == 3) {
            strcat(file, "_");
            int alpha_len = strlen(this->alpha_name) - 4;
            len += alpha_len + 1;
            strncat(file, this->alpha_name, alpha_len);
        }
        char num[20];
        _itoa(this->levels, num, 10);
        file[len] = '_';
        strcat(file, num);
        if (this->flags & ITC_HALF)
            strcat(file, "L");
        strcat(file, ".");
        len = strlen(file);
        file[len] = 'D';
        file[len + 1] = 'X';
        file[len + 2] = '0' + dxt;
        file[len + 3] = 0;
        int handle = dtaOpenWrite(file, 0x10000);
        if (handle == -1) {
            dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
            return I3D_OK;
        }
        dtaWrite(handle, "DXT", 4);
        dtaWrite(handle, &DXTCurrentVersion, 2);
        struct {
            int sx, sy;
        } size;
        size.sx = width;
        size.sy = height;
        dtaWrite(handle, &size, 8);
        dtaWrite(handle, &SrcBMPLastWriteTime, 8);
        if (format == D3DFMT_DXT3)
            dtaWrite(handle, &SrcOptBMPLastWriteTime, 8);
        for (uint i = 0; i < this->levels; i++) {
            IDirect3DSurface8 *surface;
            if (FAILED(texture->GetSurfaceLevel(i, &surface))) {
                dtaClose(handle);
                dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
                return I3D_OK;
            }
            D3DSURFACE_DESC desc;
            if (FAILED(surface->GetDesc(&desc))) {
                dtaClose(handle);
                dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
                return I3D_OK;
            }
            D3DLOCKED_RECT lr;
            if (FAILED(surface->LockRect(&lr, NULL, 0))) {
                dtaClose(handle);
                dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
                return I3D_OK;
            }
            dtaWrite(handle, lr.pBits, desc.Size);
            surface->UnlockRect();
            surface->Release();
        }
        dtaClose(handle);
    }
    if (!(this->flags & ITC_FROM_MEMORY) && (this->format == D3DFMT_R5G6B5 || this->format == D3DFMT_X1R5G5B5 ||
            this->format == D3DFMT_A1R5G5B5)) {
        char file[256];
        memset(file, 0, 255);
        int len = strlen(name) - 4;
        strncpy(file, name, len);
        char num[20];
        _itoa(this->levels, num, 10);
        file[len] = '_';
        strcat(file, num);
        if (this->flags & ITC_HALF)
            strcat(file, "L");
        strcat(file, ".");
        len = strlen(file);
        file[len] = '5';
        if (this->format == D3DFMT_R5G6B5)
            file[len + 1] = '6';
        else
            file[len + 1] = '5';
        file[len + 2] = '5';
        file[len + 3] = 0;
        int handle = dtaOpenWrite(file, 0x10000);
        if (handle == -1) {
            dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
            return I3D_OK;
        }
        dtaWrite(handle, "TEX", 4);
        dtaWrite(handle, &TEXCurrentVersion, 2);
        struct {
            int sx, sy;
        } size;
        size.sx = width;
        size.sy = height;
        dtaWrite(handle, &size, 8);
        dtaWrite(handle, &SrcBMPLastWriteTime, 8);
        for (uint i = 0; i < this->levels; i++) {
            IDirect3DSurface8 *surface;
            if (FAILED(texture->GetSurfaceLevel(i, &surface))) {
                dtaClose(handle);
                dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
                return I3D_OK;
            }
            D3DSURFACE_DESC desc;
            if (FAILED(surface->GetDesc(&desc))) {
                dtaClose(handle);
                dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
                return I3D_OK;
            }
            D3DLOCKED_RECT lr;
            if (FAILED(surface->LockRect(&lr, NULL, 0))) {
                dtaClose(handle);
                dbgPrintf("ITexture::Create() - Can't create %s !!!", file);
                return I3D_OK;
            }
            dtaWrite(handle, lr.pBits, desc.Size);
            surface->UnlockRect();
            surface->Release();
        }
        dtaClose(handle);
    }
    return I3D_OK;
}

// Loads the levels Create saved (<name>[_<alpha name>]_<levels>[L].DX<type>), if the file is there and fits
// FUNCTION: LS3DF 0x10078190
bool ITexture::m_TryPresavedDXT(const char *name, const char *alpha_name, int dxt, FILETIME *time,
    FILETIME *alpha_time)
{
    char file[256];
    memset(file, 0, 255);
    int len = strlen(name) - 4;
    strncpy(file, name, len);
    if (alpha_name && dxt == 3) {
        strcat(file, "_");
        int alpha_len = strlen(alpha_name) - 4;
        len += alpha_len + 1;
        strncat(file, alpha_name, alpha_len);
    }
    char num[20];
    _itoa(levels, num, 10);
    file[len] = '_';
    strcat(file, num);
    if (flags & ITC_HALF)
        strcat(file, "L");
    strcat(file, ".");
    len = strlen(file);
    file[len] = 'D';
    file[len + 1] = 'X';
    file[len + 2] = '0' + dxt;
    file[len + 3] = 0;
    int handle = dtaOpen(file, false);
    if (handle == -1)
        return false;
    char header[5];
    header[4] = 0;
    dtaRead(handle, header, 4);
    if (header[0] == 'D' && header[1] == 'X' && header[2] == 'T') {
        ushort version = 0xffff;
        dtaRead(handle, &version, 2);
        if (version == DXTCurrentVersion) {
            struct {
                int sx, sy;
            } size;
            size.sx = 0;
            size.sy = 0;
            dtaRead(handle, &size, 8);
            if (size.sx == width && size.sy == height) {
                FILETIME file_time;
                dtaRead(handle, &file_time, 8);
                if (dxt == 3) {
                    FILETIME file_alpha_time;
                    dtaRead(handle, &file_alpha_time, 8);
                }
                mem_size = 0;
                for (int i = 0; i < levels; i++) {
                    IDirect3DSurface8 *surface;
                    texture->GetSurfaceLevel(i, &surface);
                    D3DSURFACE_DESC desc;
                    surface->GetDesc(&desc);
                    mem_size += desc.Size;
                    D3DLOCKED_RECT lr;
                    surface->LockRect(&lr, NULL, 0);
                    dtaRead(handle, lr.pBits, desc.Size);
                    surface->UnlockRect();
                    surface->Release();
                }
                dtaClose(handle);
                return true;
            }
        }
    }
    dtaClose(handle);
    return false;
}

// The same for the 16-bit formats (<name>_<levels>[L].555 or .565)
// FUNCTION: LS3DF 0x10078430
bool ITexture::m_TryPresaved_555_565(const char *name, FILETIME *time)
{
    char file[256];
    memset(file, 0, 255);
    int len = strlen(name) - 4;
    strncpy(file, name, len);
    char num[20];
    _itoa(levels, num, 10);
    file[len] = '_';
    strcat(file, num);
    if (flags & ITC_HALF)
        strcat(file, "L");
    strcat(file, ".");
    len = strlen(file);
    file[len] = '5';
    if (format == D3DFMT_R5G6B5)
        file[len + 1] = '6';
    else
        file[len + 1] = '5';
    file[len + 2] = '5';
    file[len + 3] = 0;
    int handle = dtaOpen(file, false);
    if (handle == -1)
        return false;
    char header[5];
    header[4] = 0;
    dtaRead(handle, header, 4);
    if (header[0] == 'T' && header[1] == 'E' && header[2] == 'X') {
        ushort version = 0xffff;
        dtaRead(handle, &version, 2);
        if (version == TEXCurrentVersion) {
            struct {
                int sx, sy;
            } size;
            size.sx = 0;
            size.sy = 0;
            dtaRead(handle, &size, 8);
            if (size.sx == width && size.sy == height) {
                FILETIME file_time;
                dtaRead(handle, &file_time, 8);
                mem_size = 0;
                for (int i = 0; i < levels; i++) {
                    IDirect3DSurface8 *surface;
                    texture->GetSurfaceLevel(i, &surface);
                    D3DSURFACE_DESC desc;
                    surface->GetDesc(&desc);
                    mem_size += desc.Size;
                    D3DLOCKED_RECT lr;
                    surface->LockRect(&lr, NULL, 0);
                    dtaRead(handle, lr.pBits, desc.Size);
                    surface->UnlockRect();
                    surface->Release();
                }
                dtaClose(handle);
                return true;
            }
        }
    }
    dtaClose(handle);
    return false;
}

// FUNCTION: LS3DF 0x10078670
bool ITexture::IsInit()
{
    return init;
}

// FUNCTION: LS3DF 0x10078680
void ITexture::Close()
{
    Unlock();
    if (flags & ITC_MANAGED) {
        RemoveTextureFromTextureManager(this);
        managed = NULL;
        texture = NULL;
    }
    if (pixels) {
        delete[] pixels;
        pixels = NULL;
    }
    width = 0;
    height = 0;
    format = (D3DFORMAT)0;
    flags = 0;
    color_key = 0;
    mem_size = 0;
    file_name = "";
    name = NULL;
    alpha_file_name = "";
    alpha_name = NULL;
    m_CachedTextureList.remove(this);
    m_UncachedTextureList.remove(this);
    if (texture) {
        texture->Release();
        texture = NULL;
    }
    init = false;
}

// FUNCTION: LS3DF 0x100787c0
void *ITexture::Lock(RECT *rect, uint &pitch, ushort level)
{
    if (lock_surface)
        return NULL;
    texture->GetSurfaceLevel(level, &lock_surface);
    D3DLOCKED_RECT lr;
    lock_surface->LockRect(&lr, rect, D3DLOCK_NO_DIRTY_UPDATE);
    pitch = lr.Pitch;
    return lr.pBits;
}

// FUNCTION: LS3DF 0x10078820
void ITexture::Unlock()
{
    if (lock_surface) {
        lock_surface->UnlockRect();
        lock_surface->Release();
        lock_surface = NULL;
    }
}

// FUNCTION: LS3DF 0x10078850
HRESULT ITexture::AddDirtyRect(const RECT *rect)
{
    return texture->AddDirtyRect(rect);
}

// Draws a point with the texture, which makes the driver upload it
// FUNCTION: LS3DF 0x10078860
LS3D_RESULT ITexture::Preload()
{
    if (m_SceneBegun) {
        if (m_CurrVertexFormat != 4) {
            m_CurrVertexFormat = 4;
            m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
        }
        m_pD3DDevice8->SetTexture(0, texture);
        m_pD3DDevice8->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &preload_vertex, sizeof(LS3D_TLVERTEX));
        return I3D_OK;
    }
    m_pD3DDevice8->BeginScene();
    if (m_CurrVertexFormat != 4) {
        m_CurrVertexFormat = 4;
        m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    }
    m_pD3DDevice8->SetTexture(0, texture);
    m_pD3DDevice8->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &preload_vertex, sizeof(LS3D_TLVERTEX));
    m_pD3DDevice8->EndScene();
    return I3D_OK;
}

// Fills a rectangle of a level (the whole level without one) with a color, given as X8R8G8B8
// FUNCTION: LS3DF 0x10078940
bool ITexture::Fill(RECT *rect, uint color, ushort level)
{
    D3DSURFACE_DESC desc;
    if (FAILED(texture->GetLevelDesc(level, &desc)))
        return false;
    uint pitch;
    uchar *p = (uchar *)Lock(rect, pitch, level);
    if (!p)
        return false;
    uint sx, sy;
    if (!rect) {
        sx = desc.Width;
        sy = desc.Height;
    } else {
        sx = rect->right - rect->left;
        sy = rect->bottom - rect->top;
    }
    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8: {
        uint add = pitch - sx * 4;
        for (uint y = sy; y; y--) {
            for (uint x = sx; x; x--) {
                *(uint *)p = color;
                p += 4;
            }
            p += add;
        }
        break;
    }
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4: {
        uint add = pitch - sx * 2;
        for (uint y = sy; y; y--) {
            for (uint x = sx; x; x--) {
                *(ushort *)p = (ushort)color;
                p += 2;
            }
            p += add;
        }
        break;
    }
    case D3DFMT_L8: {
        // Three planes, as m_Convert writes them
        uchar *p0 = p, *p1 = p + desc.Width * desc.Height, *p2 = p + desc.Width * desc.Height * 2;
        uchar r = (uchar)(color >> 16), g = (uchar)(color >> 8);
        for (uint y = sy; y; y--) {
            for (uint x = sx; x; x--) {
                *p0++ = r;
                *p1++ = g;
                *p2++ = (uchar)color;
            }
        }
        break;
    }
    case D3DFMT_R8G8B8: {
        uint add = pitch - sx * 3;
        uchar r = (uchar)(color >> 16), g = (uchar)(color >> 8);
        uchar *d = p;
        for (uint y = sy; y; y--) {
            for (uint x = sx; x; x--) {
                d[0] = r;
                d[1] = g;
                d[2] = (uchar)color;
                d += 3;
            }
            d += add;
        }
        break;
    }
    default:
        Unlock();
        return false;
    }
    Unlock();
    texture->AddDirtyRect(rect);
    return true;
}

// Draws the texture on the screen at its size
// FUNCTION: LS3DF 0x10078b70
bool ITexture::Draw(int x, int y)
{
    RECT rect, screen, visible;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
    screen.left = 0;
    screen.top = 0;
    screen.right = pGlobalIGraph->Scrn_sx();
    screen.bottom = pGlobalIGraph->Scrn_sy();
    if (!IntersectRect(&visible, &rect, &screen))
        return true;
    draw_quad[0].x = (float)x;
    draw_quad[0].y = (float)y;
    draw_quad[1].x = (float)x + width;
    draw_quad[1].y = (float)y;
    draw_quad[2].x = (float)x;
    draw_quad[2].y = (float)y + height;
    draw_quad[3].x = (float)x + width;
    draw_quad[3].y = (float)y + height;
    pGlobalIGraph->SetTexture(this);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    if (pGlobalIGraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, draw_quad, LS3D_STREAM_TLVERTEX) < 0)
        return false;
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    return true;
}

// Draws the texture stretched over a rectangle, with a color at each corner
// FUNCTION: LS3DF 0x10078cf0
bool ITexture::Draw(RECT rect, uint color0, uint color1, uint color2, uint color3)
{
    RECT screen, visible;
    screen.left = 0;
    screen.top = 0;
    screen.right = pGlobalIGraph->Scrn_sx();
    screen.bottom = pGlobalIGraph->Scrn_sy();
    if (!IntersectRect(&visible, &rect, &screen))
        return true;
    draw_quad_colored[0].diffuse = color0;
    draw_quad_colored[0].x = (float)rect.left;
    draw_quad_colored[0].y = (float)rect.top;
    draw_quad_colored[1].diffuse = color1;
    draw_quad_colored[1].x = (float)rect.right;
    draw_quad_colored[1].y = (float)rect.top;
    draw_quad_colored[2].diffuse = color2;
    draw_quad_colored[2].x = (float)rect.left;
    draw_quad_colored[2].y = (float)rect.bottom;
    draw_quad_colored[3].diffuse = color3;
    draw_quad_colored[3].x = (float)rect.right;
    draw_quad_colored[3].y = (float)rect.bottom;
    pGlobalIGraph->SetTexture(this);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    if (pGlobalIGraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, draw_quad_colored, LS3D_STREAM_TLVERTEX) < 0)
        return false;
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    return true;
}

// Saves level 0 as an uncompressed TGA (the 16-bit formats as 1555, A4R4G4B4 as 32 bits)
// FUNCTION: LS3DF 0x10078e40
bool ITexture::SaveToTGA(const char *name)
{
    if (format == D3DFMT_DXT1 || format == D3DFMT_DXT2 || format == D3DFMT_DXT3 || format == D3DFMT_DXT4 ||
            format == D3DFMT_DXT5) {
        dbgPrintf("!! ITexture::SaveToTGA() - Cannot create file %s  Compresed pixel format isn't supported", name);
        return false;
    }
    S_tga_header header;
    memset(&header, 0, sizeof(header));
    header.image_type = 2;
    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_A4R4G4B4:
        header.bpp = 32;
        break;
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        header.bpp = 16;
        break;
    case D3DFMT_R8G8B8:
    case D3DFMT_L8:
        header.bpp = 24;
        break;
    }
    header.width = width;
    header.height = height;
    int handle = dtaOpenWrite(name, 0x10000);
    if (handle < 0) {
        dbgPrintf("!! ITexture::SaveToTGA() - Cannot create file %s", name);
        return false;
    }
    dtaWrite(handle, &header, sizeof(header));
    uint pitch;
    uchar *p = (uchar *)Lock(NULL, pitch, 0);
    if (!p) {
        dbgPrintf("!! ITexture::SaveToTGA() - Cannot lock texture");
        return false;
    }
    // TGA's rows go up
    p += (header.height - 1) * pitch;
    switch (format) {
    case D3DFMT_R5G6B5: {
        ushort *row = new ushort[header.width];
        for (int y = header.height; y; y--) {
            for (int x = header.width; x; ) {
                ushort c = ((ushort *)p)[--x];
                row[x] = c >> 1 & 0x7fe0 | c & 0x1f;
            }
            dtaWrite(handle, row, header.bpp / 8 * header.width);
            p -= pitch;
        }
        delete[] row;
        break;
    }
    case D3DFMT_A4R4G4B4: {
        uint *row = new uint[header.width];
        for (int y = header.height; y; y--) {
            for (int x = header.width; x; ) {
                ushort c = ((ushort *)p)[--x];
                row[x] = (c & 0xf000) << 16 | (c & 0xf00) << 12 | (c & 0xf0) << 8 | (c & 0xf) << 4;
            }
            dtaWrite(handle, row, header.bpp / 8 * header.width);
            p -= pitch;
        }
        delete[] row;
        break;
    }
    default:
        for (int y = header.height; y; y--) {
            dtaWrite(handle, p, header.bpp / 8 * header.width);
            p -= pitch;
        }
    }
    dtaClose(handle);
    Unlock();
    return true;
}

// Reads a BMP or a TGA, or a bitmap in memory, into a buffer of rows bottom-up (TGAs as stored), the palette as
// 4 bytes per entry. The BITMAPINFOHEADER's biXPelsPerMeter holds the row pitch. sx and sy scale the image (no
// filtering). The PS2 build also reads its own .imc format.
// FUNCTION: LS3DF 0x10079150
bool LoadImage(const char *name, uint sx, uint sy, uint flags, void **bits, void **palette, BITMAPINFOHEADER *bmi,
    bool from_memory, FILETIME *time, int handle)
{
    const uchar *mem = (const uchar *)name;
    uchar *pal = NULL;
    bool flip = false;
    BITMAPINFOHEADER info;
    int type;
    BITMAPFILEHEADER file_header;
    S_tga_header header;
    if (from_memory) {
        type = 0;
        memcpy(&info, mem, sizeof(info));
        mem += sizeof(info);
    } else {
        const char *ext = strchr(name, '.'), *cp;
        while ((cp = strchr(ext + 1, '.')) != NULL)
            ext = cp;
        if (ext[1] == 't' || ext[1] == 'T')
            type = 1;
        else if (ext[1] == 'b' || ext[1] == 'B')
            type = 0;
        else
            return false;
        if (handle == -1) {
            handle = dtaOpen(name, false);
            if (handle == -1) {
                dbgPrintf("LoadImage() - (%s) File not found !!", name);
                return false;
            }
        }
        if (time)
            dtaGetTime(handle, time, false);
        switch (type) {
        case 0: {
            if ((int)dtaRead(handle, &file_header, sizeof(file_header)) <= 0) {
                dtaClose(handle);
                return false;
            }
            if ((int)dtaRead(handle, &info, sizeof(info)) <= 0) {
                dtaClose(handle);
                return false;
            }
            break;
        }
        case 1: {
            if ((int)dtaRead(handle, &header, sizeof(header)) <= 0) {
                dtaClose(handle);
                return false;
            }
            if (header.id_length > 0)
                dtaSeek(handle, header.id_length, 1);
            if (header.descriptor & 0x10)
                dbgPrintf("!! Error. Right-to-left TGA detected, but not reversed !");
            if (header.descriptor & 0x20)
                flip = true;
            info.biSize = sizeof(info);
            info.biClrImportant = 0;
            info.biClrUsed = 0;
            info.biCompression = 0;
            info.biHeight = header.height;
            info.biBitCount = header.bpp;
            info.biWidth = header.width;
            info.biYPelsPerMeter = 0;
            info.biPlanes = 1;
            info.biXPelsPerMeter = header.bpp / 8 * header.width;
            info.biSizeImage = header.bpp / 8 * header.width * header.height;
            break;
        }
        }
    }
    if (info.biCompression) {
        dbgPrintf("LoadImage() - (%s) Unsupported compresion :%lu!!", name, info.biCompression);
        dtaClose(handle);
        return false;
    }
    uchar bpp = (uchar)(info.biBitCount / 8);
    if (!type) {
        // BMP rows are padded to 4 bytes
        int row = bpp * info.biWidth;
        info.biXPelsPerMeter = flags & ITC_NOALIGN || row / 4 * 4 == row ? row : (row / 4 + 1) * 4;
    }
    if (!info.biClrUsed && info.biBitCount <= 8)
        info.biClrUsed = 1 << info.biBitCount;
    if (!info.biClrUsed)
        *palette = NULL;
    else {
        pal = new uchar[0x400];
        if (!pal) {
            dtaClose(handle);
            return false;
        }
        memset(pal, 0, 0x400);
        if (from_memory) {
            memcpy(pal, mem, info.biClrUsed * 4);
            mem += info.biClrUsed * 4;
            *palette = pal;
        } else {
            switch (type) {
            case 0:
                if (dtaRead(handle, pal, info.biClrUsed * 4) != info.biClrUsed * 4) {
                    dbgPrintf("LoadImage() - (%s) Error reading palette !!", name);
                    delete[] pal;
                    dtaClose(handle);
                    return false;
                }
                break;
            case 1:
                for (int i = 0; i < (int)info.biClrUsed; i++) {
                    if (dtaRead(handle, pal + i * 4, 3) != 3) {
                        dbgPrintf("LoadImage() - (%s) Error reading palette !!", name);
                        delete[] pal;
                        dtaClose(handle);
                        return false;
                    }
                }
                break;
            }
            *palette = pal;
        }
    }
    uint size = info.biXPelsPerMeter * info.biHeight;
    uchar *data = new uchar[size];
    if (!data) {
        delete[] pal;
        dtaClose(handle);
        return false;
    }
    if (from_memory)
        memcpy(data, mem, size);
    else {
        if (dtaRead(handle, data, size) != size) {
            dbgPrintf("LoadImage() - (%s) Error reading bitmap !!", name);
            delete[] data;
            delete[] pal;
            dtaClose(handle);
            return false;
        }
        dtaClose(handle);
    }
    if (flip) {
        uchar *row = new uchar[info.biXPelsPerMeter];
        if (!row) {
            delete[] data;
            delete[] pal;
            return false;
        }
        uchar *top = data, *bottom = data + (info.biHeight - 1) * info.biXPelsPerMeter;
        for (int i = info.biHeight / 2; i; i--) {
            memcpy(row, top, info.biXPelsPerMeter);
            memcpy(top, bottom, info.biXPelsPerMeter);
            memcpy(bottom, row, info.biXPelsPerMeter);
            bottom -= info.biXPelsPerMeter;
            top += info.biXPelsPerMeter;
        }
        delete[] row;
    }
    if (!sx || !sy || (sx == (uint)info.biWidth && sy == (uint)info.biHeight)) {
        if (bmi)
            memcpy(bmi, &info, sizeof(info));
        *bits = data;
        return true;
    }
    int row = bpp * sx;
    int new_size = row * sy;
    uchar *scaled = new uchar[new_size];
    if (!scaled) {
        delete[] data;
        delete[] pal;
        return false;
    }
    float scale_x = (float)info.biWidth / sx, scale_y = (float)info.biHeight / sy;
    uchar *dst = scaled;
    for (int y = 0; y < (int)sy; y++) {
        for (int x = 0; x < (int)sx; x++) {
            memcpy(dst, data + (int)(x * scale_x) * bpp + (int)(y * scale_y) * info.biXPelsPerMeter, bpp);
            dst += bpp;
        }
    }
    delete[] data;
    *bits = scaled;
    if (bmi) {
        info.biWidth = sx;
        info.biXPelsPerMeter = row;
        info.biSizeImage = new_size;
        info.biHeight = sy;
        memcpy(bmi, &info, sizeof(info));
    }
    return true;
}

// The dithering's error buffer (PS2 name)
struct S_vector4 {
    float x, y, z, w;
};

// Converts a level from ARGB bytes to the surface's format. The plain conversions are assembly, the dithered ones
// (ITC_DITHER, which no_dither turns off) Floyd-Steinberg. L8 holds three planes, of R, G and B.
// FUNCTION: LS3DF 0x100798c0
bool ITexture::m_Convert(uchar *src, IDirect3DSurface8 *surface, uint sx, uint sy, int no_dither)
{
    S_vector4 *err = NULL;
    if (!no_dither && (flags & ITC_DITHER)) {
        int n = (sx + 2) * (sy + 1);
        err = new S_vector4[n];
        if (sx <= 0x400 && sy <= 0x400)
            memset(err, 0, n * sizeof(S_vector4));
        else {
            while (n--) {
                err[n].x = 0.0f;
                err[n].y = 0.0f;
                err[n].z = 0.0f;
                err[n].w = 0.0f;
            }
        }
    }
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);
    D3DLOCKED_RECT lr;
    surface->LockRect(&lr, NULL, 0);
    switch (desc.Format) {
    case D3DFMT_A4R4G4B4: {
        uchar *dst = (uchar *)lr.pBits;
        int skip = lr.Pitch - sx * 2;
        if (!(flags & ITC_DITHER)) {
            __asm {
                mov esi, src
                mov edi, dst
                mov ebx, sx
                mov ecx, sy
            row_4444:
                push ecx
                mov ecx, ebx
            pixel_4444:
                movzx dx, byte ptr [esi]
                and dl, 0xf0
                shl dx, 8
                inc esi
                movzx ax, byte ptr [esi]
                and al, 0xf0
                shl ax, 4
                add dx, ax
                inc esi
                movzx ax, byte ptr [esi]
                and al, 0xf0
                add dx, ax
                inc esi
                movzx eax, byte ptr [esi]
                shr al, 4
                add dl, al
                mov word ptr [edi], dx
                inc esi
                add edi, 2
                dec ecx
                jnz pixel_4444
                add edi, skip
                pop ecx
                dec ecx
                jnz row_4444
            }
        } else {
            for (int y = 0; y < (int)sy; y++) {
                ushort *d = (ushort *)lr.pBits + y * (lr.Pitch >> 1);
                for (int x = 0; x < (int)sx; x++) {
                    S_vector4 &e = err[y * (sx + 2) + x + 1];
                    float a = src[0] + e.w, r = src[1] + e.x, g = src[2] + e.y, b = src[3] + e.z;
                    src += 4;
                    float ea = (float)fmod(a, 8.0) * 0.0625f, er = (float)fmod(r, 8.0) * 0.0625f,
                        eg = (float)fmod(g, 8.0) * 0.0625f, eb = (float)fmod(b, 8.0) * 0.0625f;
                    S_vector4 &e1 = err[y * (sx + 2) + x + 2], &e2 = err[(y + 1) * (sx + 2) + x];
                    S_vector4 &e3 = err[(y + 1) * (sx + 2) + x + 1], &e4 = err[(y + 1) * (sx + 2) + x + 2];
                    e1.w += ea * 7.0f;
                    e1.x += er * 7.0f;
                    e1.y += eg * 7.0f;
                    e1.z += eb * 7.0f;
                    e2.w += ea * 3.0f;
                    e2.x += er * 3.0f;
                    e2.y += eg * 3.0f;
                    e2.z += eb * 3.0f;
                    e3.w += ea * 5.0f;
                    e3.x += er * 5.0f;
                    e3.y += eg * 5.0f;
                    e3.z += eb * 5.0f;
                    e4.w += ea;
                    e4.x += er;
                    e4.y += eg;
                    e4.z += eb;
                    if (a > 255.0f)
                        a = 255.0f;
                    if (r > 255.0f)
                        r = 255.0f;
                    if (g > 255.0f)
                        g = 255.0f;
                    if (b > 255.0f)
                        b = 255.0f;
                    int ia = FloatToInt(a), ir = FloatToInt(r), ig = FloatToInt(g), ib = FloatToInt(b);
                    *d++ = (ushort)((ia >> 4) << 12 | ((uchar)ir >> 4) << 8 | ((uchar)ig >> 4) << 4 | (ib >> 4) & 0xf);
                }
            }
        }
        break;
    }
    case D3DFMT_R5G6B5: {
        uchar *dst = (uchar *)lr.pBits;
        int skip = lr.Pitch - sx * 2;
        if (!(flags & ITC_DITHER)) {
            __asm {
                mov esi, src
                mov edi, dst
                mov ebx, sx
                mov ecx, sy
            row_565:
                push ecx
                mov ecx, ebx
            pixel_565:
                inc esi
                movzx dx, byte ptr [esi]
                and dl, 0xf8
                shl dx, 8
                inc esi
                movzx ax, byte ptr [esi]
                and al, 0xfc
                shl ax, 3
                add dx, ax
                inc esi
                movzx eax, byte ptr [esi]
                shr al, 3
                add dl, al
                mov word ptr [edi], dx
                inc esi
                add edi, 2
                dec ecx
                jnz pixel_565
                add edi, skip
                pop ecx
                dec ecx
                jnz row_565
            }
        } else {
            for (int y = 0; y < (int)sy; y++) {
                ushort *d = (ushort *)lr.pBits + y * (lr.Pitch >> 1);
                for (int x = 0; x < (int)sx; x++) {
                    S_vector4 &e = err[y * (sx + 2) + x + 1];
                    float r = src[1] + e.x, g = src[2] + e.y, b = src[3] + e.z;
                    src += 4;
                    float er = (float)fmod(r, 8.0) * 0.0625f, eg = (float)fmod(g, 4.0) * 0.0625f,
                        eb = (float)fmod(b, 8.0) * 0.0625f;
                    S_vector4 &e1 = err[y * (sx + 2) + x + 2], &e2 = err[(y + 1) * (sx + 2) + x];
                    S_vector4 &e3 = err[(y + 1) * (sx + 2) + x + 1], &e4 = err[(y + 1) * (sx + 2) + x + 2];
                    e1.x += er * 7.0f;
                    e1.y += eg * 7.0f;
                    e1.z += eb * 7.0f;
                    e2.x += er * 3.0f;
                    e2.y += eg * 3.0f;
                    e2.z += eb * 3.0f;
                    e3.x += er * 5.0f;
                    e3.y += eg * 5.0f;
                    e3.z += eb * 5.0f;
                    e4.x += er;
                    e4.y += eg;
                    e4.z += eb;
                    if (r > 255.0f)
                        r = 255.0f;
                    if (g > 255.0f)
                        g = 255.0f;
                    if (b > 255.0f)
                        b = 255.0f;
                    int ir = FloatToInt(r), ig = FloatToInt(g), ib = FloatToInt(b);
                    *d++ = (ushort)((ir >> 3) << 11 | ((uchar)ig >> 2) << 5 | (ib >> 3) & 0x1f);
                }
            }
        }
        break;
    }
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5: {
        uchar *dst = (uchar *)lr.pBits;
        int skip = lr.Pitch - sx * 2;
        if (!(flags & ITC_DITHER)) {
            __asm {
                mov esi, src
                mov edi, dst
                mov ebx, sx
                mov ecx, sy
            row_1555:
                push ecx
                mov ecx, ebx
            pixel_1555:
                movzx dx, byte ptr [esi]
                and dl, 0x80
                shl dx, 8
                inc esi
                movzx ax, byte ptr [esi]
                and al, 0xf8
                shl ax, 7
                add dx, ax
                inc esi
                movzx ax, byte ptr [esi]
                and al, 0xf8
                shl ax, 2
                add dx, ax
                inc esi
                movzx eax, byte ptr [esi]
                shr al, 3
                add dl, al
                mov word ptr [edi], dx
                inc esi
                add edi, 2
                dec ecx
                jnz pixel_1555
                add edi, skip
                pop ecx
                dec ecx
                jnz row_1555
            }
        } else {
            for (int y = 0; y < (int)sy; y++) {
                ushort *d = (ushort *)lr.pBits + y * (lr.Pitch >> 1);
                for (int x = 0; x < (int)sx; x++) {
                    S_vector4 &e = err[y * (sx + 2) + x + 1];
                    float a = src[0] + e.w, r = src[1] + e.x, g = src[2] + e.y, b = src[3] + e.z;
                    src += 4;
                    float ea = (float)fmod(a, 128.0) * 0.0625f, er = (float)fmod(r, 8.0) * 0.0625f,
                        eg = (float)fmod(g, 8.0) * 0.0625f, eb = (float)fmod(b, 8.0) * 0.0625f;
                    S_vector4 &e1 = err[y * (sx + 2) + x + 2], &e2 = err[(y + 1) * (sx + 2) + x];
                    S_vector4 &e3 = err[(y + 1) * (sx + 2) + x + 1], &e4 = err[(y + 1) * (sx + 2) + x + 2];
                    e1.w += ea * 7.0f;
                    e1.x += er * 7.0f;
                    e1.y += eg * 7.0f;
                    e1.z += eb * 7.0f;
                    e2.w += ea * 3.0f;
                    e2.x += er * 3.0f;
                    e2.y += eg * 3.0f;
                    e2.z += eb * 3.0f;
                    e3.w += ea * 5.0f;
                    e3.x += er * 5.0f;
                    e3.y += eg * 5.0f;
                    e3.z += eb * 5.0f;
                    e4.w += ea;
                    e4.x += er;
                    e4.y += eg;
                    e4.z += eb;
                    if (a > 255.0f)
                        a = 255.0f;
                    if (r > 255.0f)
                        r = 255.0f;
                    if (g > 255.0f)
                        g = 255.0f;
                    if (b > 255.0f)
                        b = 255.0f;
                    int ia = FloatToInt(a), ir = FloatToInt(r), ig = FloatToInt(g), ib = FloatToInt(b);
                    *d++ = (ushort)((ia >> 7) << 15 | ((uchar)ir >> 3) << 10 | ((uchar)ig >> 3) << 5 | (ib >> 3) & 0x1f);
                }
            }
        }
        break;
    }
    case D3DFMT_A8R8G8B8: {
        uchar *dst = (uchar *)lr.pBits;
        int skip = lr.Pitch - sx * 4;
        __asm {
            mov esi, src
            mov edi, dst
            mov ebx, sx
            mov ecx, sy
        row_8888:
            push ecx
            mov ecx, ebx
        pixel_8888:
            movzx edx, byte ptr [esi]
            shl edx, 24
            inc esi
            movzx eax, byte ptr [esi]
            shl eax, 16
            add edx, eax
            inc esi
            movzx eax, byte ptr [esi]
            shl eax, 8
            add edx, eax
            inc esi
            movzx eax, byte ptr [esi]
            add edx, eax
            mov dword ptr [edi], edx
            inc esi
            add edi, 4
            dec ecx
            jnz pixel_8888
            add edi, skip
            pop ecx
            dec ecx
            jnz row_8888
        }
        break;
    }
    case D3DFMT_L8: {
        uchar *p0 = (uchar *)lr.pBits, *p1 = p0 + sx * sy, *p2 = p0 + sx * sy * 2;
        while (sy--) {
            for (uint x = sx; x; x--) {
                *p0++ = src[1];
                *p1++ = src[2];
                *p2++ = src[3];
                src += 4;
            }
        }
        break;
    }
    }
    surface->UnlockRect();
    delete[] err;
    return true;
}

// Compresses a level, converted to dxt_format, to DXT1 (type 1, or 2 with 1-bit alpha) or DXT3 (type 3)
// FUNCTION: LS3DF 0x1007a380
bool CompressTexture(const char *name, int type, IDirect3DSurface8 *src, IDirect3DSurface8 *dst)
{
    float weights[3] = {0.3086f, 0.6094f, 0.0820f};
    D3DSURFACE_DESC desc;
    if (FAILED(src->GetDesc(&desc)))
        return false;
    D3DLOCKED_RECT src_lr;
    if (FAILED(src->LockRect(&src_lr, NULL, D3DLOCK_READONLY)))
        return false;
    D3DLOCKED_RECT dst_lr;
    if (FAILED(dst->LockRect(&dst_lr, NULL, 0)))
        return false;
    DDSURFACEDESC src_desc, dst_desc;
    src_desc.dwSize = sizeof(src_desc);
    src_desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE | DDSD_PIXELFORMAT;
    src_desc.lPitch = src_lr.Pitch;
    src_desc.dwWidth = desc.Width;
    src_desc.dwHeight = desc.Height;
    src_desc.lpSurface = src_lr.pBits;
    src_desc.ddpfPixelFormat.dwSize = sizeof(src_desc.ddpfPixelFormat);
    DWORD encode = S3TC_ENCODE_RGB_FULL;
    switch (type) {
    case 2:
        encode = S3TC_ENCODE_RGB_ALPHA_COMPARE;
    case 1:
        src_desc.ddpfPixelFormat.dwRBitMask = 0x7c00;
        src_desc.ddpfPixelFormat.dwGBitMask = 0x3e0;
        src_desc.ddpfPixelFormat.dwBBitMask = 0x1f;
        src_desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
        src_desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        src_desc.ddpfPixelFormat.dwRGBBitCount = 16;
        break;
    case 3:
        encode = S3TC_ENCODE_ALPHA_EXPLICIT;
        src_desc.ddpfPixelFormat.dwRBitMask = 0xf00;
        src_desc.ddpfPixelFormat.dwGBitMask = 0xf0;
        src_desc.ddpfPixelFormat.dwBBitMask = 0xf;
        src_desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
        src_desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        src_desc.ddpfPixelFormat.dwRGBBitCount = 16;
        break;
    }
    dst_desc.dwSize = sizeof(dst_desc);
    if (m_InitDesc.flags & IGRAPH_LOG)
        pGlobalIGraph->SetAppNameDebug("Compressing texture %s [%ix%i]", name, desc.Width, desc.Height);
    S3TCencode(&src_desc, NULL, &dst_desc, dst_lr.pBits, encode, weights);
    if (m_InitDesc.flags & IGRAPH_LOG)
        pGlobalIGraph->SetAppNameDebug("");
    src->UnlockRect();
    dst->UnlockRect();
    return true;
}

// The texture manager (PC only, names guessed). A managed texture's levels are in the file Create would have saved,
// <name>_1.DX1 or <name>_1.565 for a <name> ending in its pool's number (0 to 2). Request queues it for the thread,
// which reads the file into the pool's system memory texture and copies it to a default pool texture of the pool,
// adding one or evicting the least recently used one.

// The record of a managed texture
struct S_managed_texture {
    ITexture *texture;
    char *file_name;
    D3DFORMAT format;
    int pool;
    uint last_frame;                    // m_FrameCounter when it was last requested
};

// A texture of a pool, and its owner while it holds one
struct S_texture_slot {
    ITexture *texture;
    IDirect3DTexture8 *d3d_texture;
    IDirect3DSurface8 *surface;
    D3DSURFACE_DESC desc;
};

// GLOBAL: LS3DF 0x101c5dc8
static ITexture *load_queue[256];

// GLOBAL: LS3DF 0x101c628c
static C_list<S_managed_texture *> managed_textures(0x400);

// GLOBAL: LS3DF 0x101c61c8
static C_list<S_texture_slot> pool_textures[3];

// GLOBAL: LS3DF 0x101c6208
static S_texture_slot sysmem_textures[3];

// GLOBAL: LS3DF 0x101c629c
static int load_queue_out;

// GLOBAL: LS3DF 0x101c62a0
static int load_queue_put;

// GLOBAL: LS3DF 0x101c62a4
static int load_queue_len;

// GLOBAL: LS3DF 0x101c62a8
static int pools_memory[3];

void UnloadManagedTextures();

// FUNCTION: LS3DF 0x1007a610
void ReleaseManagedTextures()
{
    UnloadManagedTextures();
    for (int i = 0; i < 3; i++) {
        if (sysmem_textures[i].surface) {
            sysmem_textures[i].surface->Release();
            sysmem_textures[i].surface = NULL;
        }
        if (sysmem_textures[i].d3d_texture) {
            sysmem_textures[i].d3d_texture->Release();
            sysmem_textures[i].d3d_texture = NULL;
        }
        for (int j = pool_textures[i].size(); j--; ) {
            pool_textures[i][j].surface->Release();
            pool_textures[i][j].d3d_texture->Release();
        }
        pool_textures[i].clear();
        pools_memory[i] = 0;
    }
}

// Creates the pools' textures up to their budgets
// FUNCTION: LS3DF 0x1007a710
void RestoreManagedTextures(uint flags)
{
    ReleaseManagedTextures();
    D3DFORMAT format;
    if (flags & ITC_MANAGED_DXT && m_DXTTexturePixelFormat)
        format = m_DXTTexturePixelFormat;
    else
        format = m_TexturePixelFormat;
    for (int i = 0; i < 3; i++) {
        pool_textures[i].SetGrowSize(0x400);
        m_pD3DDevice8->CreateTexture(pool_sizes[i], pool_sizes[i], 1, 0, format, D3DPOOL_SYSTEMMEM,
            &sysmem_textures[i].d3d_texture);
        sysmem_textures[i].d3d_texture->GetSurfaceLevel(0, &sysmem_textures[i].surface);
        sysmem_textures[i].surface->GetDesc(&sysmem_textures[i].desc);
        sysmem_textures[i].texture = NULL;
        while (pool_budgets[i] > pools_memory[i]) {
            S_texture_slot &slot = pool_textures[i].push_back();
            m_pD3DDevice8->CreateTexture(pool_sizes[i], pool_sizes[i], 1, 0, format, D3DPOOL_DEFAULT,
                &slot.d3d_texture);
            slot.d3d_texture->GetSurfaceLevel(0, &slot.surface);
            slot.surface->GetDesc(&slot.desc);
            slot.texture = NULL;
            pools_memory[i] += slot.desc.Size;
        }
    }
    managed_textures.reserve(0x1000);
}

// Whether a managed texture is loaded; queues it for the thread if it isn't
// FUNCTION: LS3DF 0x1007a910
bool ITexture::Request()
{
    if (!(flags & ITC_MANAGED))
        return true;
    EnterCriticalSection(&m_FrameSection);
    managed->last_frame = m_FrameCounter;
    if (flags & ITC_LOADED) {
        LeaveCriticalSection(&m_FrameSection);
        return true;
    }
    if (!(flags & ITC_QUEUED) && load_queue_len < 256) {
        load_queue[load_queue_put] = this;
        if (++load_queue_put >= 256)
            load_queue_put = 0;
        load_queue_len++;
        flags |= ITC_QUEUED;
    }
    LeaveCriticalSection(&m_FrameSection);
    return false;
}

// The loading thread (IGraph::Init starts it)
// FUNCTION: LS3DF 0x1007a9b0
DWORD WINAPI RenderThreadProc(void *param)
{
    while (m_RenderThreadRunning) {
        if (load_queue_len) {
            EnterCriticalSection(&m_FrameSection);
            ITexture *texture;
            // Skip the textures nobody asked for since the last frame
            while (load_queue_len > 0) {
                texture = load_queue[load_queue_out];
                if (texture) {
                    if (texture->managed->last_frame + 1 >= (uint)m_FrameCounter)
                        break;
                    texture->flags &= ~ITC_QUEUED;
                }
                if (++load_queue_out >= 256)
                    load_queue_out = 0;
                load_queue_len--;
            }
            if (!load_queue_len) {
                LeaveCriticalSection(&m_FrameSection);
                continue;
            }
            texture->ref++;
            LeaveCriticalSection(&m_FrameSection);
            S_managed_texture *managed = texture->managed;
            int pool = managed->pool;
            S_texture_slot *slot;
            int oldest = 0;
            ITexture *first = pool_textures[pool][0].texture;
            if (first) {
                EnterCriticalSection(&m_FrameSection);
                uint oldest_frame = first->managed->last_frame;
                for (int i = 1; i < pool_textures[pool].size(); i++) {
                    if (!pool_textures[pool][i].texture) {
                        oldest_frame = 0;
                        oldest = i;
                        break;
                    }
                    if (pool_textures[pool][i].texture->managed->last_frame < oldest_frame) {
                        oldest_frame = pool_textures[pool][i].texture->managed->last_frame;
                        oldest = i;
                    }
                }
                if (oldest_frame >= (uint)m_FrameCounter - 1) {
                    // All in use: the pool grows
                    LeaveCriticalSection(&m_FrameSection);
                    D3DFORMAT format = managed->format;
                    slot = &pool_textures[pool].push_back();
                    m_pD3DDevice8->CreateTexture(pool_sizes[pool], pool_sizes[pool], 1, 0, format, D3DPOOL_DEFAULT,
                        &slot->d3d_texture);
                    slot->d3d_texture->GetSurfaceLevel(0, &slot->surface);
                    slot->surface->GetDesc(&slot->desc);
                    slot->texture = NULL;
                    pools_memory[pool] += slot->desc.Size;
                    goto load;
                }
                ITexture *old = pool_textures[pool][oldest].texture;
                if (old) {
                    old->texture = NULL;
                    old->flags &= ~(ITC_QUEUED | ITC_LOADED);
                }
                LeaveCriticalSection(&m_FrameSection);
            }
            slot = &pool_textures[pool][oldest];
        load:
            D3DLOCKED_RECT lr;
            sysmem_textures[pool].surface->LockRect(&lr, NULL, 0);
            texture->width = (ushort)slot->desc.Width;
            texture->height = (ushort)slot->desc.Height;
            int handle = dtaOpen(managed->file_name, false);
            // After the header: "DXT", the version, the size and the image's time
            dtaSeek(handle, 0x16, 1);
            dtaRead(handle, lr.pBits, slot->desc.Size);
            dtaClose(handle);
            sysmem_textures[pool].surface->UnlockRect();
            m_pD3DDevice8->UpdateTexture(sysmem_textures[pool].d3d_texture, slot->d3d_texture);
            EnterCriticalSection(&m_FrameSection);
            slot->texture = texture;
            texture->texture = slot->d3d_texture;
            texture->flags = texture->flags & ~(ITC_QUEUED | ITC_LOADED) | ITC_LOADED;
            texture->Release();
            if (++load_queue_out >= 256)
                load_queue_out = 0;
            load_queue_len--;
            LeaveCriticalSection(&m_FrameSection);
        }
        Sleep(0);
    }
    return 0;
}

// Creates the record of a managed texture
// FUNCTION: LS3DF 0x1007ad20
bool AddTextureToTextureManager(ITexture *texture, const char *name, uint flags)
{
    char file[256];
    S_managed_texture *managed = new S_managed_texture;
    managed->texture = texture;
    managed->last_frame = 0;
    int len, size;
    if ((flags & ITC_DXT) && m_DXTTexturePixelFormat) {
        managed->format = m_DXTTexturePixelFormat;
        len = strlen(name) - 4;
        strncpy(file, name, len);
        managed->pool = name[len - 1] - '0';
        file[len] = 0;
        strncat(file, "_1.DX1", 7);
        size = len + 7;
    } else {
        len = strlen(name) - 4;
        strncpy(file, name, len);
        managed->pool = name[len - 1] - '0';
        file[len] = 0;
        strncat(file, "_1.565", 7);
        managed->format = m_TexturePixelFormat;
        size = len + 7;
        if (m_TexturePixelFormat != D3DFMT_R5G6B5)
            file[len + 1] = '5';
    }
    managed->file_name = new char[size];
    if (!managed->file_name) {
        delete managed;
        dbgPrintf("AddTextureToTextureManager() - ERROR! Memory low !!!");
        return false;
    }
    strncpy(managed->file_name, file, size);
    texture->texture = NULL;
    texture->managed = managed;
    managed_textures.push_back(managed);
    return true;
}

// Deletes the record of a managed texture and takes it out of the queue and the pools
// FUNCTION: LS3DF 0x1007aee0
bool RemoveTextureFromTextureManager(ITexture *texture)
{
    EnterCriticalSection(&m_FrameSection);
    for (int i = managed_textures.size(); i--; ) {
        if (managed_textures[i]->texture == texture) {
            if (load_queue_len > 0 && (texture->flags & ITC_QUEUED)) {
                for (ITexture **p = load_queue + 256; p != load_queue; ) {
                    if (*--p == texture)
                        *p = NULL;
                }
            }
            delete[] managed_textures[i]->file_name;
            delete managed_textures[i];
            managed_textures.erase(i);
            break;
        }
    }
    if (texture->flags & ITC_LOADED) {
        for (int j = 0; j < 3; j++) {
            for (int k = pool_textures[j].size(); k--; ) {
                if (pool_textures[j][k].texture == texture)
                    pool_textures[j][k].texture = NULL;
            }
        }
    }
    texture->flags &= ~(ITC_QUEUED | ITC_LOADED);
    LeaveCriticalSection(&m_FrameSection);
    return true;
}

// Takes the pools' textures from their owners (I3D code calls it)
// FUNCTION: LS3DF 0x1007afe0
void UnloadManagedTextures()
{
    EnterCriticalSection(&m_FrameSection);
    for (int i = 0; i < 3; i++) {
        for (int j = pool_textures[i].size(); j--; ) {
            if (pool_textures[i][j].texture) {
                pool_textures[i][j].texture->flags &= ~ITC_LOADED;
                pool_textures[i][j].texture = NULL;
            }
        }
    }
    LeaveCriticalSection(&m_FrameSection);
}
