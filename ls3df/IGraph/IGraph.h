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

// A movie playing into a texture or onto the screen (PS2 IShow.cpp, 0x8c bytes). Not written yet: only what IGraph
// needs.
// VTABLE: LS3DF 0x1009d670
class IShow {
public:
    IShow();

    virtual void __stdcall vfunc_00();
    virtual void __stdcall vfunc_01();
    virtual void __stdcall vfunc_02();
    virtual void __stdcall vfunc_03(int a, int b);
    virtual void __stdcall vfunc_04();
    virtual void __stdcall vfunc_05();
    virtual void __stdcall vfunc_06();
    virtual void __stdcall vfunc_07();
    virtual void __stdcall vfunc_08(int a);
    virtual void __stdcall vfunc_09(int a);
    virtual void __stdcall vfunc_10(int a);
    virtual void __stdcall vfunc_11();
    virtual void __stdcall vfunc_12(int a);
    virtual void __stdcall vfunc_13(int a);
    virtual void __stdcall vfunc_14(int a);
    virtual void __stdcall vfunc_15();
    virtual void __stdcall vfunc_16();
    virtual void __stdcall vfunc_17();
    virtual void __stdcall vfunc_18();
    virtual void __stdcall vfunc_19();
    virtual void __stdcall vfunc_20();
    virtual void __stdcall vfunc_21();
    virtual void __stdcall vfunc_22(int a, int b);
    virtual void __stdcall vfunc_23();
    virtual void __stdcall vfunc_24(int a);
    virtual void __stdcall vfunc_25();
    virtual void __stdcall vfunc_26();
    virtual void __stdcall vfunc_27();
    virtual void __stdcall vfunc_28(int a);
    virtual void __stdcall vfunc_29(int a);

private:
    uchar unknown_04[0x88];
};

// A texture (PS2 ITexture.cpp, 0x4c bytes). Not written yet: only what IGraph needs.
// VTABLE: LS3DF 0x1009d8d4
class ITexture {
public:
    ITexture();

    virtual int __stdcall Release();
    virtual void __stdcall vfunc_01(int a);
    virtual LS3D_RESULT __stdcall Create(const char *name, const char *alpha_name, uint flags, ushort sx, ushort sy,
        ushort unknown);
    virtual void __stdcall vfunc_03();
    virtual void __stdcall vfunc_04();
    virtual void __stdcall vfunc_05();
    virtual void __stdcall vfunc_06(int a, int b, int c);
    virtual void __stdcall vfunc_07();
    virtual void __stdcall vfunc_08();
    virtual void __stdcall vfunc_09();
    virtual void __stdcall vfunc_10(int a, int b, int c);
    virtual void __stdcall vfunc_11(int a, int b, int c, int d, int e, int f, int g, int h);
    virtual void __stdcall vfunc_12(int a, int b);
    virtual void __stdcall vfunc_13(int a);
    virtual void __stdcall vfunc_14();

    ushort uid;                         // 0x04, from IGraph::CreateITexture
    IDirect3DTexture8 *texture;         // 0x08
    uchar unknown_0c[8];
    int ref;                            // 0x14
    C_str file_name;                    // 0x18
    const char *name;                   // 0x1c, as created, which the cache compares
    C_str alpha_file_name;              // 0x20
    const char *alpha_name;             // 0x24
    uchar unknown_28[0xc];
    uint flags;                         // 0x34
    uchar unknown_38[0x14];
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

// The image files': the game's render thread
DWORD WINAPI RenderThreadProc(void *param);

// Globals.cpp
void ErrorHandler(LS3D_RESULT result, E_SEVERITY severity, const char *message);
