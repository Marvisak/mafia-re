#pragma once

#include "ls3df.h"
#include "I3D/I3d_math.h"

#include <d3d8.h>

// The graphics interface (PS2 file IGraph.cpp), video playback (PS2 IShow.cpp) and textures (PS2 ITexture.cpp). The
// virtual functions the game calls are __stdcall. IGraph's state is in globals (PS2 m_MainHWnd, m_pD3DDevice8, ...):
// the object itself is only its vtable pointer. Names are the PS2 build's unless noted; the PC-only parts have
// guessed names.

// From the DirectX 8.1 SDK: dinput8.lib's data formats, dxguid.lib's GUIDs and dxerr8.lib's error strings
// GLOBAL: LS3DF 0x100a160c
// c_dfDIKeyboard
// GLOBAL: LS3DF 0x100a1814
// c_dfDIMouse
// GLOBAL: LS3DF 0x100a189c
// GUID_SysKeyboard
// GLOBAL: LS3DF 0x100a18ac
// GUID_SysMouse
// GLOBAL: LS3DF 0x100a18fc
// IID_IDirectInput8A
// LIBRARY: LS3DF 0x1007d9d6 SYMBOL
// _DXGetErrorString8A@4

// strmiids.lib's DirectShow GUIDs, which IShow.cpp uses
// GLOBAL: LS3DF 0x100a1a30
// IID_IGraphBuilder
// GLOBAL: LS3DF 0x100a1a70
// IID_IMediaSeeking
// GLOBAL: LS3DF 0x100a1b10
// IID_IBasicAudio
// GLOBAL: LS3DF 0x100a1b30
// IID_IMediaEvent
// GLOBAL: LS3DF 0x100a1b40
// IID_IMediaControl
// GLOBAL: LS3DF 0x100a1bb0
// CLSID_FilterGraph

class IShow;
class ITexture;

// How bad an error is (PS2 type; the value names are guesses). ErrorHandler exits from 3 on.
enum E_SEVERITY {
    ERR_INFO,
    ERR_WARNING,
    ERR_ERROR,
    ERR_FATAL,
};

// The game's error callback. The engine's own functions are __fastcall (/Gr), the game's callback __cdecl.
typedef void __cdecl t_error_handler(LS3D_RESULT result, E_SEVERITY severity, const char *message);

// IGRAPH_INIT_DESC::flags (names guessed)
enum {
    IGRAPH_FULLSCREEN = 0x2,            // the main window is a topmost popup the size of the mode
    IGRAPH_TRIPLE_BUFFER = 0x4,         // full screen: two back buffers
    IGRAPH_MENU = 0x8,                  // windowed: the window has a menu (LS3DF's menu 123 if none is given)
    IGRAPH_HWTL = 0x10,                 // use hardware transformation and lighting if the card has it
    IGRAPH_SLEEP = 0x20,                // the engine sleeps while its window is inactive
    IGRAPH_LOG = 0x40,                  // Init logs the modes and the antialiasing it tries
    IGRAPH_CHILD_WINDOW = 0x80,         // windowed: render into a child window at child_rect
    IGRAPH_VSYNC = 0x100,               // full screen: wait for the vertical retrace
    IGRAPH_LOCKABLE_ZBUFFER = 0x400,
    IGRAPH_REFERENCE = 0x800,           // Direct3D's reference rasterizer
    IGRAPH_FSAA2 = 0x1000,              // full scene antialiasing with 2 samples
    IGRAPH_FSAA4 = 0x2000,              // with 4 samples
    IGRAPH_MULTIPASS = 0x4000,          // one texture per pass (Init sets it for cards that need it)
    IGRAPH_RENDER_THREAD = 0x100000,    // Present counts frames for a thread the engine suspends while sleeping
};

// What IGraph::Init takes (PS2 name, 0x38 bytes). The member names are guesses.
struct IGRAPH_INIT_DESC {
    HINSTANCE hinstance;                // 0x00
    uint adapter;                       // 0x04
    int x, y;                           // 0x08, the main window's position
    int width, height;                  // 0x10, of the screen mode or the window
    RECT child_rect;                    // 0x18, left, top, then width and height
    uchar bpp;                          // 0x28
    uchar refresh_rate;                 // 0x29, full screen
    uint flags;                         // 0x2c, IGRAPH_*
    HMENU menu;                         // 0x30
    t_error_handler *error_handler;     // 0x34
};

// The render states IGraph::SetState and GetState keep (PS2 type DX_STATE; the value names are guesses)
enum DX_STATE {
    DXS_ALPHA,                          // 0 none, 1 blend and test, 2 blend and test (ref 0), 3 test (the alpha ref)
    DXS_BLEND,                          // 0 alpha, 1 additive alpha, 2 additive, 3 multiply, 4 multiply by source
    DXS_CLIPPING,
    DXS_CULL,                           // a D3DCULL, or 3 for IGraph's own cull mode
    DXS_COLOROP,                        // D3DTOP of stages 0, 1, 2
    DXS_COLOROP1,
    DXS_COLOROP2,
    DXS_ALPHAOP,                        // D3DTOP of stage 0
    DXS_ZBIAS,
    DXS_ADDRESSU,
    DXS_ADDRESSV,
    DXS_ALPHAREF,
    DXS_ZFUNC,                          // 0 always, else less or equal
    DXS_ZWRITE,
    DXS_ZENABLE,
    DXS_CLAMP,                          // stage 0 wraps (0) or clamps
    DXS_FOG,
};

// IGraph::DrawPrimitiveList's primitives (PS2 type; the value names are guesses)
enum PRIMITIVE_TYPE {
    PT_POINTLIST,
    PT_LINELIST,
    PT_LINESTRIP,
    PT_TRIANGLELIST,
    PT_TRIANGLEFAN,
    PT_TRIANGLESTRIP,
    PT_POINTSPRITE,                     // drawn as points too
};

// The vertices IGraph::DrawPrimitiveList takes (PS2 type; the value names are guesses)
enum LS3D_STREAM_TYPE {
    LS3D_STREAM_LVERTEX,                // 24 bytes, through the default vertex shader
    LS3D_STREAM_TLVERTEX,               // 32 bytes, transformed and lit (D3DFVF_XYZRHW | DIFFUSE | SPECULAR | TEX1)
};

// The viewport (PS2 name), laid out as D3DVIEWPORT8
struct LS3D_VIEWPORT {
    uint x, y;
    uint width, height;
    float min_z, max_z;
};

// A gamma ramp (PS2 name), laid out as D3DGAMMARAMP
struct LS3DGAMMARAMP {
    ushort red[256];
    ushort green[256];
    ushort blue[256];
};

// A TGA file's header (names guessed)
#pragma pack(push, 1)
struct S_tga_header {
    char id_length;
    char color_map_type;
    char image_type;
    short color_map_first;
    short color_map_length;
    char color_map_bpp;
    short x, y;
    short width, height;                // signed: SaveShot sign-extends them
    char bpp;
    char descriptor;
};
#pragma pack(pop)

// VTABLE: LS3DF 0x1009d500
class IGraph {
public:
    IGraph();
    ~IGraph();

    virtual IShow *__stdcall CreateIShow();
    // Overloads share the slot of the first one, in reverse order: this one is slot 2
    virtual ITexture *__stdcall CreateITexture();
    virtual LS3D_RESULT __stdcall CreateITexture(const char *name, const char *alpha_name, uint flags,
        ITexture **texture);
    virtual bool __stdcall SetTexture(ITexture *texture);
    virtual HWND __stdcall GetMainHWND();
    virtual HWND __stdcall GetChildHWND();
    virtual void __stdcall SetAppName(const char *name);
    virtual void __cdecl SetAppNameDebug(const char *format, ...);
    virtual const char *__stdcall GetAppName();
    virtual bool __stdcall AddDlgHWND(void *hwnd);
    virtual bool __stdcall DeleteDlgHWND(void *hwnd);
    virtual void __stdcall ProcessWinMessages();
    virtual void __stdcall Clear(uint color, float z, uint flags);
    virtual void __stdcall BeginScene();
    virtual void __stdcall EndScene();
    virtual void __stdcall Present();
    virtual void __stdcall ResetRenderProps();
    virtual void __stdcall SetState(DX_STATE state, uint value);
    virtual uint __stdcall GetState(DX_STATE state);
    virtual LS3D_RESULT __stdcall SetViewPort(LS3D_VIEWPORT *viewport);
    virtual LS3D_VIEWPORT *__stdcall GetViewPort();
    virtual void __stdcall SetWorldMatrix(const S_matrix &m);
    virtual LS3D_RESULT __stdcall DrawPrimitiveList(PRIMITIVE_TYPE type, uint num_prims, const void *verts,
        LS3D_STREAM_TYPE stream);
    virtual LS3D_RESULT __stdcall DrawIndexedPrimitiveList(PRIMITIVE_TYPE type, uint num_verts, const void *indices,
        uint num_prims, const void *verts, LS3D_STREAM_TYPE stream);
    virtual LS3D_RESULT __stdcall Init(IGRAPH_INIT_DESC *desc);
    virtual IGRAPH_INIT_DESC *__stdcall GetResInitDesc();
    virtual bool __stdcall IsInit();
    virtual void __stdcall Close();
    virtual int __stdcall Scrn_sx();
    virtual int __stdcall Scrn_sy();
    virtual int __stdcall BPP();
    virtual D3DFORMAT __stdcall GetD3DFormat();
    virtual const char *__stdcall GetFormatName(D3DFORMAT format) const;
    virtual LS3D_RESULT __stdcall GetImageInfo(const char *name, ushort &sx, ushort &sy, uchar &bpp, uint flags,
        int *handle);
    virtual bool __stdcall SaveShot(const char *name, RECT *rect, uint flags, void *z_buffer);
    virtual LS3D_RESULT __stdcall KeyboardInit(uint flags);
    virtual bool __stdcall IsKeyboardInit();
    virtual void __stdcall KeyboardClose();
    virtual LS3D_RESULT __stdcall UpdateKeyboardData();
    virtual uint __stdcall ReadKey();
    virtual uint __stdcall ReadChar();
    virtual uint __stdcall ReadCharBuffer();
    virtual void __stdcall ClearCharBuffer();
    virtual bool __stdcall TestKey(uchar key);
    virtual LS3D_RESULT __stdcall DefineKeys(const uchar *keys, uint count);
    virtual ulong __stdcall ReadKeys();
    virtual bool __stdcall KeyShift();
    virtual bool __stdcall KeyCtrl();
    virtual bool __stdcall KeyAlt();
    virtual uint __stdcall GetShiftKeys(bool unknown);
    virtual uchar __stdcall TranslateKey(uchar key, uint flags);
    virtual const char *__stdcall GetKeyName(int key);
    virtual LS3D_RESULT __stdcall MouseInit(int mode);
    virtual bool __stdcall IsMouseInit();
    virtual void __stdcall MouseClose();
    virtual LS3D_RESULT __stdcall UpdateMouseData();
    virtual int __stdcall Mouse_rx();
    virtual int __stdcall Mouse_ry();
    virtual int __stdcall Mouse_rz();
    virtual void __stdcall SetMousePos(int x, int y);
    virtual int __stdcall Mouse_x();
    virtual int __stdcall Mouse_y();
    virtual uint __stdcall GetMouseButtons();
    virtual void __stdcall NullTimer();
    virtual uint __stdcall GetTimer(uint min_time, uint max_time);
    virtual LS3D_RESULT __stdcall AddMapsDir(const char *dir, bool unknown);
    virtual LS3D_RESULT __stdcall DeleteMapsDir(const char *dir);
    virtual void __stdcall ClearMapDirs();
    virtual ITexture **__stdcall GetCachedTextures();
    virtual int __stdcall GetCachedTextureCnt();
    virtual ITexture **__stdcall GetUncachedTextures();
    virtual int __stdcall GetUncachedTextureCnt();
    virtual bool __stdcall SetGammaRamp(LS3DGAMMARAMP *ramp, uint flags);
    virtual bool __stdcall GetGammaRamp(LS3DGAMMARAMP *ramp);
    virtual bool __stdcall ResizeImage(int sx, int sy, uint pitch, uchar *src, int dst_sx, int dst_sy,
        uint dst_pitch, uchar **dst, uint bpp, int filter);
};

// A transformed and lit vertex, as IGraph::DrawPrimitiveList takes them (LS3D_STREAM_TLVERTEX; the name is a guess)
struct LS3D_TLVERTEX {
    float x, y, z, rhw;
    uint diffuse, specular;
    float u, v;
};

// IShow::Open's flags and the state IShow keeps in the same word (names guessed)
enum {
    ISHOW_TRUECOLOR = 0x400,            // the movie's textures are 32-bit (ITC_TRUECOLOR)
    ISHOW_FILLED = 0x8000000,           // the Ogg sound buffer holds decoded sound
    ISHOW_PAUSED = 0x10000000,
    ISHOW_PLAYING = 0x20000000,
    ISHOW_EOF = 0x40000000,             // the Ogg file is decoded to its end
    ISHOW_OGG = 0x80000000,             // an Ogg Vorbis file, played through DirectSound
};

// A part of the movie in a texture of its own: the movie is split into textures no larger than the card allows. The
// vertices are a triangle strip in screen coordinates (name guessed).
struct S_show_region {
    ITexture *texture;                  // 0x00
    LS3D_TLVERTEX verts[4];             // 0x04
    int x, y;                           // 0x84, in the movie
    int sx, sy;                         // 0x8c
};

struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;
struct IBasicAudio;
struct IMediaEvent;
struct IDirectSoundNotify;
struct IDirectSoundBuffer8;
struct BINK;
struct OggVorbis_File;
class CTextureRenderer;

// A movie playing into textures (PS2 IShow.cpp, 0x8c bytes): a Bink video, an AVI file through DirectShow (rendered
// by CTextureRenderer, the DirectX SDK's texture renderer sample) or any other file DirectShow plays, or an Ogg
// Vorbis file, which plays through DirectSound (PC only). PS2 names; the PC-only functions and the members have
// guessed names.
// VTABLE: LS3DF 0x1009d670
class IShow {
public:
    IShow();
    ~IShow();

    virtual int __stdcall AddRef();
    virtual int __stdcall Release();
    virtual bool __stdcall IsInit();
    virtual LS3D_RESULT __stdcall Open(const char *name, uint flags);
    virtual void __stdcall Close();
    virtual LS3D_RESULT __stdcall Play();
    virtual LS3D_RESULT __stdcall Stop();
    virtual LS3D_RESULT __stdcall Pause();
    virtual LS3D_RESULT __stdcall SetRect(const RECT *rc);
    virtual LS3D_RESULT __stdcall GetRect(RECT *rc);
    virtual LS3D_RESULT __stdcall GetVideoRect(RECT *rc);
    virtual int __stdcall GetEvent();
    virtual LS3D_RESULT __stdcall SetCurrentPos(uint pos);
    virtual LS3D_RESULT __stdcall GetCurrentPos(uint *pos);
    virtual LS3D_RESULT __stdcall SetVolume(float vol);

    // FUNCTION: LS3DF 0x10075820
    virtual float __stdcall GetVolume()
    {
        return volume;
    }

    virtual int __stdcall GetRegionCnt();
    virtual S_show_region *__stdcall GetRegions();
    virtual LS3D_RESULT __stdcall ProcessBinkFrame();
    virtual LS3D_RESULT __stdcall RenderCurrentFrame();
    virtual LS3D_RESULT __stdcall RenderVideoUntilKeyPressOrEnd();

    // FUNCTION: LS3DF 0x10075830
    virtual int __stdcall GetDecodedFrameNum()
    {
        return decoded_frame;
    }

    // PC only: music through the sound driver's stream functions (IShowStream.cpp)
    virtual LS3D_RESULT __stdcall OpenStream(const char *name, int unused);
    virtual void __stdcall CloseStream();
    virtual LS3D_RESULT __stdcall PlayStream(bool loop);

    // FUNCTION: LS3DF 0x10075840
    virtual int __stdcall IsStreamPlaying()
    {
        return !stream_done;
    }

    virtual LS3D_RESULT __stdcall StopStream();
    virtual LS3D_RESULT __stdcall PauseStream();
    // Two settings of the stream (guessed: the volume and the pan)
    virtual LS3D_RESULT __stdcall SetStreamVolume(int volume);
    virtual LS3D_RESULT __stdcall SetStreamPan(int pan);

    void m_FillBuffer(bool first);
    void m_PrepareVerts();
    HRESULT m_CreateCTextureRenderer(const WCHAR *name);

    IGraphBuilder *graph;               // 0x04, DirectShow
    IMediaControl *control;             // 0x08
    IMediaSeeking *seeking;             // 0x0c
    int unknown_10;                     // 0x10
    IBasicAudio *audio;                 // 0x14
    IMediaEvent *media_event;           // 0x18
    BINK *bink;                         // 0x1c
    bool init;                          // 0x20
    int ref;                            // 0x24
    uint flags;                         // 0x28, ISHOW_*
    RECT rect;                          // 0x2c, where on the screen
    float volume;                       // 0x3c
    C_list<S_show_region> regions;      // 0x40
    CTextureRenderer *renderer;         // 0x50, DirectShow's AVI files
    int decoded_frame;                  // 0x54, counted by the renderer
    int stream;                         // 0x58, the sound driver's stream, -1 if none
    bool stream_init;                   // 0x5c
    bool stream_paused;                 // 0x5d
    bool stream_done;                   // 0x5e, the stream callback's end
    DWORD thread_id;                    // 0x60, Ogg: the thread that decodes into the sound buffer
    HANDLE thread;                      // 0x64
    HANDLE event;                       // 0x68, signalled at each of the buffer's notification positions
    IDirectSoundNotify *notify;         // 0x6c
    IDirectSoundBuffer8 *sound_buffer;  // 0x70
    int notify_count;                   // 0x74, notifications since the buffer was filled from its start
    int write_pos;                      // 0x78, how far the buffer is filled
    int buffer_size;                    // 0x7c
    bool filling;                       // 0x80, m_FillBuffer runs
    OggVorbis_File *vf;                 // 0x84
    int dta_handle;                     // 0x88, the Ogg file
};

// ITexture::Create's flags. The Xbox build's messages name ITC_USE_ALPHA and ITC_EMPTY; the other names are guesses.
enum {
    ITC_FROM_MEMORY = 0x1,              // the name points to a bitmap in memory (a BITMAPINFOHEADER, the bits after it)
    ITC_ALPHA = 0x2,                    // the image's own alpha: a 32-bit image's, a 16-bit image's top bit
    ITC_USE_ALPHA = 0x4,                // the alpha comes from a second image, alpha_name
    ITC_COLORKEY = 0x8,                 // the color of palette entry 0 (or blue 0 of a true color image) is transparent
    ITC_MANAGED = 0x40,                 // loaded from the presaved file by the texture manager's thread when used
    ITC_DITHER = 0x80,                  // dither when converting to a 16-bit format
    ITC_MIPMAP = 0x100,
    ITC_DXT = 0x200,                    // compress, if the card has a DXT format
    ITC_TRUECOLOR = 0x400,
    ITC_KEEP_PIXELS = 0x1000,           // keep the 32-bit copy of the image after creating the texture
    ITC_HALF = 0x2000,                  // half the image's size (the presaved files' names get an "L")
    ITC_FILL = 0x4000,                  // ITC_EMPTY: clear the texture
    ITC_EMPTY = 0x8000,                 // no image, only the texture
    ITC_NOALIGN = 0x40000,              // LoadImage: a BMP's rows aren't padded to 4 bytes
    ITC_MANAGED_DXT = 0x80000,          // the texture manager's textures are DXT1
    ITC_UNCACHED = 0x200000,            // not in the cache that IGraph::CreateITexture searches
    ITC_ANYSIZE = 0x400000,             // keep the image's size, not a power of two within the card's limits
    ITC_QUEUED = 0x40000000,            // the texture manager: waiting for the loading thread
    ITC_LOADED = 0x80000000,            // the texture manager: in one of its texture slots
};

struct S_managed_texture;

// A texture (PS2 ITexture.cpp, 0x4c bytes). PS2's ITexture isn't virtual; the PC's virtual functions are __stdcall.
// Names are the PS2 build's where it has the function, the rest guessed.
// VTABLE: LS3DF 0x1009d8d4
class ITexture {
public:
    ITexture();
    ~ITexture();

    virtual int __stdcall Release();
    virtual LS3D_RESULT __stdcall Duplicate(ITexture *texture);
    virtual LS3D_RESULT __stdcall Create(const char *name, const char *alpha_name, uint flags, ushort sx, ushort sy,
        ushort levels);
    virtual bool __stdcall IsInit();
    virtual void __stdcall Close();
    virtual LS3D_RESULT __stdcall Reload();
    virtual void *__stdcall Lock(RECT *rect, uint &pitch, ushort level);
    virtual void __stdcall Unlock();
    virtual HRESULT __stdcall AddDirtyRect(const RECT *rect);
    virtual LS3D_RESULT __stdcall Preload();
    virtual bool __stdcall Fill(RECT *rect, uint color, ushort level);
    // Overloads share the slot of the first one, in reverse order: this one is slot 12
    virtual bool __stdcall Draw(int x, int y);
    virtual bool __stdcall Draw(RECT rect, uint color0, uint color1, uint color2, uint color3);
    virtual bool __stdcall SaveToTGA(const char *name);
    virtual bool __stdcall Request();

private:
    bool m_TryPresavedDXT(const char *name, const char *alpha_name, int dxt, FILETIME *time, FILETIME *alpha_time);
    bool m_TryPresaved_555_565(const char *name, FILETIME *time);
    bool m_Convert(uchar *src, IDirect3DSurface8 *surface, uint sx, uint sy, int no_dither);

public:
    ushort uid;                         // 0x04, from IGraph::CreateITexture
    IDirect3DTexture8 *texture;         // 0x08
    IDirect3DSurface8 *lock_surface;    // 0x0c, while locked
    bool init;                          // 0x10
    int ref;                            // 0x14
    C_str file_name;                    // 0x18
    const char *name;                   // 0x1c, file_name without its directory, which the cache compares
    C_str alpha_file_name;              // 0x20
    const char *alpha_name;             // 0x24
    ushort width, height;               // 0x28
    D3DFORMAT format;                   // 0x2c
    uint flags;                         // 0x30, ITC_*, as adjusted by Create
    uint create_flags;                  // 0x34, as Create got them, which the cache compares
    uint color_key;                     // 0x38, in the texture's format
    uchar *pixels;                      // 0x3c, the image as 32 bits (ARGB bytes)
    ushort levels;                      // 0x40
    uint mem_size;                      // 0x44, of all levels
    S_managed_texture *managed;         // 0x48, ITC_MANAGED
};

// PS2 name
extern IGRAPH_INIT_DESC m_InitDesc;

// The processor, from cpuid.c (PC only, a C file): the CPUID sample's _p_info, with the sample's member names
struct _p_info {
    char v_name[13];                    // 0x00, "GenuineIntel", "AuthenticAMD"
    char model_name[30];                // 0x0d
    int family;                         // 0x2c
    int model;                          // 0x30
    int stepping;                       // 0x34
    int feature;                        // 0x38
    int checks;                         // 0x3c
};

extern "C" {
int _cpuid(_p_info *pinfo);
int GetCPUSpeed();
}

// ITexture.cpp: the texture manager (PC only, names guessed). Its thread loads the managed textures (ITC_MANAGED)
// into default pool textures, which a device reset loses: IGraph releases them before and creates them after.
void ReleaseManagedTextures();
void RestoreManagedTextures(uint flags);
DWORD WINAPI RenderThreadProc(void *param);

// The pool array's constructor, out of line for the vector constructor iterator (its default grow size is two of the
// 0x2c-byte texture slots)
// TEMPLATE: LS3DF 0x1007b040
// C_list<S_texture_slot>::C_list<S_texture_slot>

// And its destructor, which the linker folds with the other lists' identical ones: the original's copy is in I3D code
// (another list's, not known yet), ours keeps the name of C_IBuffer's list of blocks (IBManager.h)
// TEMPLATE: LS3DF 0x10034350
// C_list<S_IB_block *>::~C_list<S_IB_block *>

// Globals.cpp
void ErrorHandler(LS3D_RESULT result, E_SEVERITY severity, const char *message);
