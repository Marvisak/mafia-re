#pragma once

#include "ls3df.h"
#include "I3D/I3d_math.h"

#include <d3d8.h>

// The graphics interface (PS2 file IGraph.cpp), video playback (PS2 IShow.cpp) and textures (PS2 ITexture.cpp). The
// virtual functions the game calls are __stdcall. IGraph's state is in globals (PS2 m_MainHWnd, m_pD3DDevice8, ...):
// the object itself is only its vtable pointer (new IGraph allocates 4 bytes), and as members the globals give other
// code in most of IGraph.cpp. Names are the PS2 build's unless noted; the PC-only parts have guessed names.

// From the DirectX 8.1 SDK: dinput8.lib's data formats, dxguid.lib's GUIDs and dxerr8.lib's error strings
// GLOBAL: LS3DF 0x100a160c
// GLOBAL: LS3DF11 0x1009e60c
// c_dfDIKeyboard
// GLOBAL: LS3DF 0x100a1814
// GLOBAL: LS3DF11 0x1009e814
// c_dfDIMouse
// GLOBAL: LS3DF 0x100a189c
// GLOBAL: LS3DF11 0x1009e89c
// GUID_SysKeyboard
// GLOBAL: LS3DF 0x100a18ac
// GLOBAL: LS3DF11 0x1009e8ac
// GUID_SysMouse
// GLOBAL: LS3DF 0x100a18fc
// GLOBAL: LS3DF11 0x1009e8fc
// IID_IDirectInput8A
// LIBRARY: LS3DF 0x1007d9d6 SYMBOL
// LIBRARY: LS3DF11 0x1007a806 SYMBOL
// _DXGetErrorString8A@4

// strmiids.lib's DirectShow GUIDs, which IShow.cpp and the base classes use: one object, laid out as in the
// original
// GLOBAL: LS3DF 0x100a1a30
// GLOBAL: LS3DF11 0x1009ea30
// IID_IGraphBuilder
// GLOBAL: LS3DF 0x100a1a40
// GLOBAL: LS3DF11 0x1009ea40
// IID_IMediaEventSink
// GLOBAL: LS3DF 0x100a1a50
// GLOBAL: LS3DF11 0x1009ea50
// IID_IQualityControl
// GLOBAL: LS3DF 0x100a1a60
// GLOBAL: LS3DF11 0x1009ea60
// IID_IFilterMapper
// GLOBAL: LS3DF 0x100a1a70
// GLOBAL: LS3DF11 0x1009ea70
// IID_IMediaSeeking
// GLOBAL: LS3DF 0x100a1a80
// GLOBAL: LS3DF11 0x1009ea80
// IID_IAMovieSetup
// GLOBAL: LS3DF 0x100a1a90
// GLOBAL: LS3DF11 0x1009ea90
// IID_IMemInputPin
// GLOBAL: LS3DF 0x100a1aa0
// GLOBAL: LS3DF11 0x1009eaa0
// IID_IMemAllocator
// GLOBAL: LS3DF 0x100a1ab0
// GLOBAL: LS3DF11 0x1009eab0
// IID_IMediaSample2
// GLOBAL: LS3DF 0x100a1ac0
// GLOBAL: LS3DF11 0x1009eac0
// IID_IBaseFilter
// GLOBAL: LS3DF 0x100a1ad0
// GLOBAL: LS3DF11 0x1009ead0
// IID_IMediaFilter
// GLOBAL: LS3DF 0x100a1ae0
// GLOBAL: LS3DF11 0x1009eae0
// IID_IEnumMediaTypes
// GLOBAL: LS3DF 0x100a1af0
// GLOBAL: LS3DF11 0x1009eaf0
// IID_IEnumPins
// GLOBAL: LS3DF 0x100a1b00
// GLOBAL: LS3DF11 0x1009eb00
// IID_IPin
// GLOBAL: LS3DF 0x100a1b10
// GLOBAL: LS3DF11 0x1009eb10
// IID_IBasicAudio
// GLOBAL: LS3DF 0x100a1b20
// GLOBAL: LS3DF11 0x1009eb20
// IID_IMediaPosition
// GLOBAL: LS3DF 0x100a1b30
// GLOBAL: LS3DF11 0x1009eb30
// IID_IMediaEvent
// GLOBAL: LS3DF 0x100a1b40
// GLOBAL: LS3DF11 0x1009eb40
// IID_IMediaControl
// GLOBAL: LS3DF 0x100a1b50
// GLOBAL: LS3DF11 0x1009eb50
// LIBID_QuartzTypeLib
// GLOBAL: LS3DF 0x100a1b60
// GLOBAL: LS3DF11 0x1009eb60
// TIME_FORMAT_MEDIA_TIME
// GLOBAL: LS3DF 0x100a1b70
// GLOBAL: LS3DF11 0x1009eb70
// IID_IQualProp
// GLOBAL: LS3DF 0x100a1b80
// GLOBAL: LS3DF11 0x1009eb80
// FORMAT_VideoInfo
// GLOBAL: LS3DF 0x100a1b90
// GLOBAL: LS3DF11 0x1009eb90
// CLSID_MemoryAllocator
// GLOBAL: LS3DF 0x100a1ba0
// GLOBAL: LS3DF11 0x1009eba0
// CLSID_DSoundRender
// GLOBAL: LS3DF 0x100a1bb0
// GLOBAL: LS3DF11 0x1009ebb0
// CLSID_FilterGraph
// GLOBAL: LS3DF 0x100a1bc0
// GLOBAL: LS3DF11 0x1009ebc0
// CLSID_FilterMapper
// GLOBAL: LS3DF 0x100a1bd0
// GLOBAL: LS3DF11 0x1009ebd0
// MEDIASUBTYPE_RGB32
// GLOBAL: LS3DF 0x100a1be0
// GLOBAL: LS3DF11 0x1009ebe0
// MEDIASUBTYPE_RGB555
// GLOBAL: LS3DF 0x100a1bf0
// GLOBAL: LS3DF11 0x1009ebf0
// MEDIASUBTYPE_RGB565
// GLOBAL: LS3DF 0x100a1c00
// GLOBAL: LS3DF11 0x1009ec00
// MEDIATYPE_Audio
// GLOBAL: LS3DF 0x100a1c10
// GLOBAL: LS3DF11 0x1009ec10
// MEDIATYPE_Video

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

// What GetMouseButtons returns (Insanity3D's names and values)
#define MBUT_LEFT 1
#define MBUT_RIGHT 2
#define MBUT_MIDDLE 4

// What GetShiftKeys returns, besides the left and right keys' bits (Insanity3D's names and values)
#define SKEY_SHIFT 1
#define SKEY_CTRL 4
#define SKEY_ALT 8

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

// A TGA file's header (the names are Insanity3D's TGA loader's)
#pragma pack(push, 1)
struct S_tga_header {
    char id_length;
    char color_map_type;
    char type;
    short cm_index;
    short cm_length;
    char cm_entry_size;
    short x_org, y_org;
    short width, height;                // signed: SaveShot sign-extends them
    char bits_per_pixel;
    char desc;                          // UL_TGA_*
};
#pragma pack(pop)

// S_tga_header::desc (Insanity3D's names)
enum {
    UL_TGA_LR = 0x10,                   // the rows run right to left
    UL_TGA_BT = 0x20,                   // the first row is the top one
};

// VTABLE: LS3DF 0x1009d500
// VTABLE: LS3DF11 0x1009a500
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
    virtual uint __stdcall GetShiftKeys(bool update);
    virtual uchar __stdcall TranslateKey(uchar key, uint shift_keys);
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
    virtual LS3D_RESULT __stdcall AddMapsDir(const char *dir, bool recursive);
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
// guessed names, but m_RefCount, which Release's message names. The members' follow it and IGraph's PS2 globals
// (m_pD3DDevice8, m_SceneBegun): m_ and the name in mixed case, p for pointers, no other type prefix.
// VTABLE: LS3DF 0x1009d670
// VTABLE: LS3DF11 0x1009a670
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
    // FUNCTION: LS3DF11 0x10072680
    virtual float __stdcall GetVolume()
    {
        return m_Volume;
    }

    virtual int __stdcall GetRegionCnt();
    virtual S_show_region *__stdcall GetRegions();
#if ENGINE_VERSION >= 395
    virtual LS3D_RESULT __stdcall ProcessBinkFrame();
#endif
    virtual LS3D_RESULT __stdcall RenderCurrentFrame();
    virtual LS3D_RESULT __stdcall RenderVideoUntilKeyPressOrEnd();

    // FUNCTION: LS3DF 0x10075830
    // FUNCTION: LS3DF11 0x10072690
    virtual int __stdcall GetDecodedFrameNum()
    {
        return m_DecodedFrameNum;
    }

    // PC only: music through the sound driver's stream functions (IShowStream.cpp)
    virtual LS3D_RESULT __stdcall OpenStream(const char *name, int unused);
    virtual void __stdcall CloseStream();
    virtual LS3D_RESULT __stdcall PlayStream(bool loop);

    // FUNCTION: LS3DF 0x10075840
    // FUNCTION: LS3DF11 0x100726a0
    virtual int __stdcall IsStreamPlaying()
    {
        return !m_StreamDone;
    }

    virtual LS3D_RESULT __stdcall StopStream();
    virtual LS3D_RESULT __stdcall PauseStream();
    // Two settings of the stream (guessed: the volume and the pan)
    virtual LS3D_RESULT __stdcall SetStreamVolume(int volume);
    virtual LS3D_RESULT __stdcall SetStreamPan(int pan);

    void m_FillBuffer(bool first);
    void m_PrepareVerts();
    HRESULT m_CreateCTextureRenderer(const WCHAR *name);

    IGraphBuilder *m_pGraphBuilder;     // 0x04, DirectShow
    IMediaControl *m_pMediaControl;     // 0x08
    IMediaSeeking *m_pMediaSeeking;     // 0x0c
    int unknown_10;                     // 0x10
    IBasicAudio *m_pBasicAudio;         // 0x14
    IMediaEvent *m_pMediaEvent;         // 0x18
#if ENGINE_VERSION >= 395
    BINK *m_pBink;                      // 0x1c
#endif
    // In 1.1 the members from here to m_DecodedFrameNum are 4 lower, m_pBink comes after them
    bool m_Init;                        // 0x20
    int m_RefCount;                     // 0x24
    uint m_Flags;                       // 0x28, ISHOW_*
    RECT m_Rect;                        // 0x2c, where on the screen
    float m_Volume;                     // 0x3c
    C_list<S_show_region> m_RegionList; // 0x40
    CTextureRenderer *m_pTextureRenderer; // 0x50, DirectShow's AVI files
    int m_DecodedFrameNum;              // 0x54, counted by the renderer
#if ENGINE_VERSION < 395
    BINK *m_pBink;                      // 0x54
#endif
    int m_Stream;                       // 0x58, the sound driver's stream, -1 if none
    bool m_StreamInit;                  // 0x5c
    bool m_StreamPaused;                // 0x5d
    bool m_StreamDone;                  // 0x5e, the stream callback's end
    DWORD m_ThreadID;                   // 0x60, Ogg: the thread that decodes into the sound buffer
    HANDLE m_Thread;                    // 0x64
    HANDLE m_Event;                     // 0x68, signalled at each of the buffer's notification positions
    IDirectSoundNotify *m_pDSNotify;    // 0x6c
    IDirectSoundBuffer8 *m_pDSBuffer8;  // 0x70
    int m_NotifyCount;                  // 0x74, notifications since the buffer was filled from its start
    int m_WritePos;                     // 0x78, how far the buffer is filled (1.1 only clears it)
    int m_BufferSize;                   // 0x7c
#if ENGINE_VERSION >= 395
    bool m_Filling;                     // 0x80, m_FillBuffer runs (not in 1.1: the next two 4 lower, 0x88 bytes)
#endif
    OggVorbis_File *m_pVorbisFile;      // 0x84
    int m_DtaHandle;                    // 0x88, the Ogg file
};

// IShow.cpp's texture renderer (CTextureRenderer, the DirectX SDK sample's, declared there): what the compiler
// generates for it and what it emits of the base classes' inline functions, at the end of IShow.cpp
// SYNTHETIC: LS3DF 0x100753e0
// SYNTHETIC: LS3DF11 0x10072240
// CTextureRenderer::`scalar deleting destructor'

// The adjustor thunks of its IAMovieSetup, IQualProp and IQualityControl vtables to DECLARE_IUNKNOWN's members
// LIBRARY: LS3DF 0x10075770
// LIBRARY: LS3DF11 0x100725d0
// ?QueryInterface@CBaseVideoRenderer@@W3AGJABU_GUID@@PAPAX@Z
// LIBRARY: LS3DF 0x10075780
// LIBRARY: LS3DF11 0x100725e0
// ?AddRef@CBaseVideoRenderer@@W3AGKXZ
// LIBRARY: LS3DF 0x10075790
// LIBRARY: LS3DF11 0x100725f0
// ?Release@CBaseVideoRenderer@@W3AGKXZ
// LIBRARY: LS3DF 0x100757a0
// LIBRARY: LS3DF11 0x10072600
// ?QueryInterface@CBaseVideoRenderer@@WNE@AGJABU_GUID@@PAPAX@Z
// LIBRARY: LS3DF 0x100757b0
// LIBRARY: LS3DF11 0x10072610
// ?AddRef@CBaseVideoRenderer@@WNE@AGKXZ
// LIBRARY: LS3DF 0x100757c0
// LIBRARY: LS3DF11 0x10072620
// ?Release@CBaseVideoRenderer@@WNE@AGKXZ
// LIBRARY: LS3DF 0x100757d0
// LIBRARY: LS3DF11 0x10072630
// ?QueryInterface@CBaseVideoRenderer@@WNI@AGJABU_GUID@@PAPAX@Z
// LIBRARY: LS3DF 0x100757e0
// LIBRARY: LS3DF11 0x10072640
// ?AddRef@CBaseVideoRenderer@@WNI@AGKXZ
// LIBRARY: LS3DF 0x100757f0
// LIBRARY: LS3DF11 0x10072650
// ?Release@CBaseVideoRenderer@@WNI@AGKXZ

// Copies of the base classes' empty in-class functions, which 1.1's linker kept from this file and 1.2's from others
// (OnReceiveFirstSample's, 0x100726d0 between them, is in the scene's group, I3D.h), and 1.2's copy of the empty
// in-class void functions (renbase.cpp's, with I3D_visual::m_AddFGroups), which our 1.2 link keeps from this file.
// By name: the names our PDB keeps for the groups' copies (tools/foldfix.py).
// LIBRARY: LS3DF11 0x100726c0
// ?OnStopStreaming@CBaseRenderer@@UAEJXZ
// LIBRARY: LS3DF11 0x100726e0
// ?OnWaitEnd@CBaseRenderer@@UAEXXZ
// LIBRARY: LS3DF 0x10088970
// ?OnWaitStart@CBaseRenderer@@UAEXXZ

// DECLARE_IUNKNOWN's members themselves (renbase.h)
// LIBRARY: LS3DF 0x10075860
// LIBRARY: LS3DF11 0x100726f0
// ?QueryInterface@CBaseVideoRenderer@@UAGJABU_GUID@@PAPAX@Z
// LIBRARY: LS3DF 0x10075870
// LIBRARY: LS3DF11 0x10072700
// ?AddRef@CBaseVideoRenderer@@UAGKXZ
// LIBRARY: LS3DF 0x10075880
// LIBRARY: LS3DF11 0x10072710
// ?Release@CBaseVideoRenderer@@UAGKXZ

// Its class id, __uuidof(CLSID_TextureRenderer)
// GLOBAL: LS3DF 0x100aacd8
// GLOBAL: LS3DF11 0x100a7df8
// _GUID_71771540_2017_11cf_ae26_0020afd79767

// m_CreateCTextureRenderer's statics. This compiler puts a function's static into the PDB as a module-level symbol
// with its plain name, where reccmp doesn't look for one, so a by-name marker matches it.
// GLOBAL: LS3DF 0x101c5be8
// GLOBAL: LS3DF11 0x101c2ce8
// pins_found
// GLOBAL: LS3DF 0x101c5bf0
// GLOBAL: LS3DF11 0x101c2cf0
// media_types_array
// GLOBAL: LS3DF 0x101c5c4c
// GLOBAL: LS3DF11 0x101c2d4c
// types_found
// GLOBAL: LS3DF 0x101c5c50
// GLOBAL: LS3DF11 0x101c2d50
// name_buf
// GLOBAL: LS3DF 0x101c5d50
// GLOBAL: LS3DF11 0x101c2e50
// pins_list
// GLOBAL: LS3DF 0x101c5da0
// GLOBAL: LS3DF11 0x101c2ea0
// splitter
// GLOBAL: LS3DF 0x101c5da4
// GLOBAL: LS3DF11 0x101c2ea4
// enum_mt
// GLOBAL: LS3DF 0x101c5da8
// GLOBAL: LS3DF11 0x101c2ea8
// pin_enum

// The destructor IShowStream.cpp's thunk registers for stream_shows: the tenth $E5 in link order, after Globals.cpp's
// SYNTHETIC: LS3DF 0x100758c0
// SYNTHETIC: LS3DF11 0x10072750
// $E5

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
// Names are the PS2 build's where it has the function and m_RefCount Release's message's, the rest guessed (the members
// in IShow's style).
// VTABLE: LS3DF 0x1009d8d4
// VTABLE: LS3DF11 0x1009a8d4
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

    D3DFORMAT GetFormat() const
    {
        return m_Format;
    }

private:
    bool m_TryPresavedDXT(const char *name, const char *alpha_name, int dxt, FILETIME *time, FILETIME *alpha_time);
    bool m_TryPresaved_555_565(const char *name, FILETIME *time);
    bool m_Convert(uchar *src, IDirect3DSurface8 *surface, uint sx, uint sy, int no_dither);

public:
    ushort m_UID;                       // 0x04, from IGraph::CreateITexture
    IDirect3DTexture8 *m_pD3DTexture8;  // 0x08
    IDirect3DSurface8 *m_pLockSurface;  // 0x0c, while locked
    bool m_Init;                        // 0x10
    int m_RefCount;                     // 0x14
    C_str m_FileName;                   // 0x18
    const char *m_pName;                // 0x1c, m_FileName without its directory, which the cache compares
    C_str m_AlphaFileName;              // 0x20
    const char *m_pAlphaName;           // 0x24
    ushort m_Width, m_Height;           // 0x28
    D3DFORMAT m_Format;                 // 0x2c
    uint m_Flags;                       // 0x30, ITC_*, as adjusted by Create
    uint m_CreateFlags;                 // 0x34, as Create got them, which the cache compares
    uint m_ColorKey;                    // 0x38, in the texture's format
    uchar *m_pPixels;                   // 0x3c, the image as 32 bits (ARGB bytes)
    ushort m_Levels;                    // 0x40
    uint m_MemSize;                     // 0x44, of all levels
    S_managed_texture *m_pManaged;      // 0x48, ITC_MANAGED
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
// TEMPLATE: LS3DF11 0x1002a020
// C_list<S_texture_slot>::C_list<S_texture_slot>

// And its destructor, which the linker folds with the other lists' identical ones: the original's copy is in I3D code
// (another list's, not known yet). The marker names the decorated symbol our PDB keeps for the copy, which changes
// with the link (tools/foldfix.py re-points it).
// TEMPLATE: LS3DF 0x10034350
// ??1?$C_list@PAUS_VB_block@@@@QAE@XZ

// TEMPLATE: LS3DF11 0x1004e760
// ??1?$C_list@UOFFSET_STRUCT@@@@QAE@XZ

// The destructors ITexture.cpp's thunks register for managed_textures and for pool_textures, through the vector
// destructor iterator: the eleventh $E5 in link order, after IShowStream.cpp's, and the fourth $E10, after
// Globals.cpp's
// SYNTHETIC: LS3DF 0x1007a5a0
// SYNTHETIC: LS3DF11 0x10077410
// $E5

// SYNTHETIC: LS3DF 0x1007a5f0
// SYNTHETIC: LS3DF11 0x10077460
// $E10

// 1.1 has IShow.cpp's CreateRegions before CopyFrameToAllTextures, out of the file's order
// FUNCTION: LS3DF11 0x100707c0
// CreateRegions

// Globals.cpp
void ErrorHandler(LS3D_RESULT result, E_SEVERITY severity, const char *message);
