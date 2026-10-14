// The graphics interface (PS2 IGraph.cpp): the window, Direct3D 8, DirectInput 8 and the timer, at
// 0x1006bd40-0x10073398 in the original. The file defines all of IGraph's globals; the header's zero vector has its
// $E in the middle (0x1006deb0, after WindowProc), as in I3d_math.cpp, and isn't here yet.
//
// The .bss: VC6's hash (tools/bsshash.py) puts the PS2 names at their PC addresses, so the PC's names are the PS2
// ones where a PS2 name fits; the PC-only globals' names are guesses. The globals initialized to zero follow, in
// declaration order (from m_Sleeping at 0x101c59ac).
#include "IGraph/IGraph.h"
#include "I3D/I3D.h"
#include "ISound/ISound.h"
#include "dta.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dxerr8.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// <new> (or any STL header, which includes it) declares operator delete throw(), so VC6 doesn't set the EH state
// before a C_str's destructor (CreateITexture, WindowProc, C_str::operator+). The other files' destructors set it:
// they didn't include it.
#include <new>

// Guards the frame counter of the game's render thread (PC only)
// GLOBAL: LS3DF 0x101c52a0
CRITICAL_SECTION m_FrameSection;

// GLOBAL: LS3DF 0x101c52b8
uint m_InternalTimer;

// GLOBAL: LS3DF 0x101c52bc
static uint m_KBInitFlags;

// GLOBAL: LS3DF 0x101c52c0
D3DPRESENT_PARAMETERS m_D3DPresentParams;

// GLOBAL: LS3DF 0x101c52f4
D3DFORMAT m_TrueColorTexturePixelFormat;

// GLOBAL: LS3DF 0x101c52f8
uint m_CurrFogColor;

// GLOBAL: LS3DF 0x101c52fc
uint m_CurrZBias;

// GLOBAL: LS3DF 0x101c5300
uchar m_KeyboardData[256];

// GLOBAL: LS3DF 0x101c5400
uint m_Modulate2X;

// GLOBAL: LS3DF 0x101c5404
bool m_LeftHandedMouse;

// GLOBAL: LS3DF 0x101c5408
HWND m_ChildHWnd;

// GLOBAL: LS3DF 0x101c540c
D3DFORMAT m_DXTTexturePixelFormat;

// The index buffer's next free index (PC only)
// GLOBAL: LS3DF 0x101c5410
int m_IBPos;

// GLOBAL: LS3DF 0x101c5434
IDirectInputDevice8A *m_pDIKeyboard8;

// GLOBAL: LS3DF 0x101c5458
HWND m_MainHWnd;

// GLOBAL: LS3DF 0x101c545c
uint m_CurrColorOp;

// GLOBAL: LS3DF 0x101c5460
D3DFORMAT m_Alpha1TexturePixelFormat;

// GLOBAL: LS3DF 0x101c5464
D3DFORMAT m_AlphaTexturePixelFormat;

// GLOBAL: LS3DF 0x101c5468
int m_MouseMode;

// GLOBAL: LS3DF 0x101c546c
static uint m_KBLastChar;

// GLOBAL: LS3DF 0x101c5474
uint m_CurrBlendState;

// GLOBAL: LS3DF 0x101c5478
static uint m_KBLastKey;

// GLOBAL: LS3DF 0x101c547c
IDirect3D8 *m_pD3D8;

// What DrawPrimitiveList set last: 0 IGraph's vertex shader, 4 transformed vertices
// GLOBAL: LS3DF 0x101c5480
uint m_CurrVertexFormat;

// GLOBAL: LS3DF 0x101c5488
uchar m_OldKeyboardData[256];

// GLOBAL: LS3DF 0x101c5588
uint m_MaxTextureHeight;

// GLOBAL: LS3DF 0x101c558c
uint m_CurrCullState;

// GLOBAL: LS3DF 0x101c5590
D3DVIEWPORT8 m_D3DViewPort8;

// GLOBAL: LS3DF 0x101c55a8
float m_CurrFogEnd;

// GLOBAL: LS3DF 0x101c55ac
IDirectInput8A *m_pDI8;

// IGraph's part of a dynamic vertex buffer, for DrawIndexedPrimitiveList (PC only)
// GLOBAL: LS3DF 0x101c55b0
COMPLETE_VB_STRUCT m_VB;

// GLOBAL: LS3DF 0x101c55bc
IGRAPH_INIT_DESC m_InitDesc;

// GLOBAL: LS3DF 0x101c55f4
uint m_CurrFogType;

// GLOBAL: LS3DF 0x101c55f8
uchar m_ReadKeysBuffer[64];

// GLOBAL: LS3DF 0x101c5638
D3DFORMAT m_TexturePixelFormat;

// GLOBAL: LS3DF 0x101c563c
uint m_HardwareCaps;

// The characters of WM_CHAR, from index 1
// The mouse's capabilities (PC only)
// GLOBAL: LS3DF 0x101c5640
DIDEVCAPS m_MouseCaps;

// GLOBAL: LS3DF 0x101c5670
static uint m_KeyBuffer[32];

// GLOBAL: LS3DF 0x101c56f0
DIMOUSESTATE m_MouseData;

// GLOBAL: LS3DF 0x101c5700
D3DFORMAT m_DXTAlphaTexturePixelFormat;

// GLOBAL: LS3DF 0x101c5704
uint m_CurrAlphaState;

// The name of a key, from GetKeyName (PC only)
// GLOBAL: LS3DF 0x101c5718
DIPROPSTRING m_KeyName;

// GLOBAL: LS3DF 0x101c5930
uint m_CurrFogState;

// What DXS_CLIPPING was set to (PC only)
// GLOBAL: LS3DF 0x101c5934
uint m_CurrClipState;

// GLOBAL: LS3DF 0x101c593c
uint m_MaxTextureWidth;

// The render thread's id (PC only)
// GLOBAL: LS3DF 0x101c5938
DWORD m_RenderThreadID;

// GLOBAL: LS3DF 0x101c5940
static uint m_KBLastChar1;

// GLOBAL: LS3DF 0x101c5944
uint m_CullMode;

// GLOBAL: LS3DF 0x101c595c
IDirectInputDevice8A *m_pDIMouse8;

// GLOBAL: LS3DF 0x101c5960
uint m_CurrColorOp1;

// GLOBAL: LS3DF 0x101c5964
uint m_CurrColorOp2;

// GLOBAL: LS3DF 0x101c5968
float m_CurrFogStart;

// What Init found the hardware lacks (the bits aren't known yet)
// GLOBAL: LS3DF 0x101c5970
uint m_ProblemFlags;

// GLOBAL: LS3DF 0x101c5974
uint m_CurrAlphaOp;

// The vertex buffer's next free vertex (PC only)
// GLOBAL: LS3DF 0x101c5978
int m_VBPos;

// GLOBAL: LS3DF 0x101c597c
IDirect3DDevice8 *m_pD3DDevice8;

// GLOBAL: LS3DF 0x101c5980
uint m_CurrAddressU;

// GLOBAL: LS3DF 0x101c5984
uint m_CurrAddressV;

// GLOBAL: LS3DF 0x101c5988
POINT m_MousePos;

// And of a dynamic index buffer (PC only)
// GLOBAL: LS3DF 0x101c5990
COMPLETE_IB_STRUCT m_IB;

// The globals with dynamic initializers, in the order the original constructs them (.CRT$XCU)

// GLOBAL: LS3DF 0x101c5424
C_list<void *> m_DlgHWndList;

// The window's title, and the title from before the engine fell asleep (PC only), constructed by one $E
// GLOBAL: LS3DF 0x101c5420
// m_AppName
// GLOBAL: LS3DF 0x101c5484
// m_SleepAppName
C_str m_AppName, m_SleepAppName;

// GLOBAL: LS3DF 0x101c599c
C_list<D3DDISPLAYMODE> m_AdapterModeList(8);

// GLOBAL: LS3DF 0x101c594c
C_list<D3DFORMAT> m_AdapterFormatList;

// GLOBAL: LS3DF 0x101c5448
C_list<ITexture *> m_CachedTextureList(0x400);

// GLOBAL: LS3DF 0x101c5438
C_list<ITexture *> m_UncachedTextureList(0x400);

// GLOBAL: LS3DF 0x101c5708
C_list<char *> m_MapsList;

// Declared after m_ProblemFlags, in the same .bss bucket
// GLOBAL: LS3DF 0x101c596c
static uint m_FPSTimer = timeGetTime();

// Initialized to zero, so in declaration order

// GLOBAL: LS3DF 0x101c59ac
bool m_Sleeping = false;

// GLOBAL: LS3DF 0x101c59ad
bool m_DisableSleeping = false;

// GLOBAL: LS3DF 0x101c59ae
ushort m_TextureUID = 0;

// GLOBAL: LS3DF 0x101c59b0
bool m_InitIsOK = false;

// GLOBAL: LS3DF 0x101c59b1
bool m_KeyboardInitIsOK = false;

// GLOBAL: LS3DF 0x101c59b2
bool m_MouseInitIsOK = false;

// GLOBAL: LS3DF 0x101c59b3
bool m_SceneBegun = false;

// The vertex shaders ResetDevice creates: positions; positions, colors and texture coordinates (the
// default for LS3D_STREAM_LVERTEX); the same with two sets of texture coordinates
// GLOBAL: LS3DF 0x101c59b4
DWORD m_VertexVS = 0;

// GLOBAL: LS3DF 0x101c59b8
DWORD m_LVertexVS = 0;

// GLOBAL: LS3DF 0x101c59bc
DWORD m_LVertex2VS = 0;

// GLOBAL: LS3DF 0x101c59c0
DWORD m_StateBlock = 0;

// GLOBAL: LS3DF 0x101c59c4
static int m_FPSCounter = 0;

// GLOBAL: LS3DF 0x101c59c8
static int m_FPSResult = 0;

// Whether Present shows the frame rate (PC only)
// GLOBAL: LS3DF 0x101c59cc
bool m_ShowFPS = false;

// GLOBAL: LS3DF 0x101c59cd
bool unknown_59cd = false;

// Whether the game's render thread runs (PC only)
// GLOBAL: LS3DF 0x101c59ce
bool m_RenderThreadRunning = false;

// The game's render thread, suspended while the engine sleeps (PC only)
// GLOBAL: LS3DF 0x101c59d0
HANDLE m_RenderThread = NULL;

// The alpha reference of DXS_ALPHA mode 3 (PC only)
// GLOBAL: LS3DF 0x100a9948
uint m_AlphaRef = 0x20;

// The device type, D3DDEVTYPE_REF with IGRAPH_REFERENCE (PC only)
// GLOBAL: LS3DF 0x100a994c
D3DDEVTYPE m_DeviceType = D3DDEVTYPE_HAL;

// GLOBAL: LS3DF 0x100a9950
uint m_CurrZFunc = 1;

// The frames presented, counted for the game's render thread (PC only)
// GLOBAL: LS3DF 0x100a9954
int m_FrameCounter = 1;

// The characters of the keys, without and with shift, for TranslateKey (PC only)
struct S_key_char {
    uchar key;                          // DIK_*
    char c;
    char shift_c;
};

// GLOBAL: LS3DF 0x100a9958
S_key_char key_table[] = {
    {DIK_SPACE, ' ', ' '},
    {DIK_0, '0', ')'},
    {DIK_1, '1', '!'},
    {DIK_2, '2', '@'},
    {DIK_3, '3', '#'},
    {DIK_4, '4', '$'},
    {DIK_5, '5', '%'},
    {DIK_6, '6', '^'},
    {DIK_7, '7', '&'},
    {DIK_8, '8', '*'},
    {DIK_9, '9', '('},
    {DIK_A, 'a', 'A'},
    {DIK_B, 'b', 'B'},
    {DIK_C, 'c', 'C'},
    {DIK_D, 'd', 'D'},
    {DIK_E, 'e', 'E'},
    {DIK_F, 'f', 'F'},
    {DIK_G, 'g', 'G'},
    {DIK_H, 'h', 'H'},
    {DIK_I, 'i', 'I'},
    {DIK_J, 'j', 'J'},
    {DIK_K, 'k', 'K'},
    {DIK_L, 'l', 'L'},
    {DIK_M, 'm', 'M'},
    {DIK_N, 'n', 'N'},
    {DIK_O, 'o', 'O'},
    {DIK_P, 'p', 'P'},
    {DIK_Q, 'q', 'Q'},
    {DIK_R, 'r', 'R'},
    {DIK_S, 's', 'S'},
    {DIK_T, 't', 'T'},
    {DIK_U, 'u', 'U'},
    {DIK_V, 'v', 'V'},
    {DIK_W, 'w', 'W'},
    {DIK_X, 'x', 'X'},
    {DIK_Y, 'y', 'Y'},
    {DIK_Z, 'z', 'Z'},
    {DIK_NUMPAD0, '0', '0'},
    {DIK_NUMPAD1, '1', '1'},
    {DIK_NUMPAD2, '2', '2'},
    {DIK_NUMPAD3, '3', '3'},
    {DIK_NUMPAD4, '4', '4'},
    {DIK_NUMPAD5, '5', '5'},
    {DIK_NUMPAD6, '6', '6'},
    {DIK_NUMPAD7, '7', '7'},
    {DIK_NUMPAD8, '8', '8'},
    {DIK_NUMPAD9, '9', '9'},
    {DIK_MULTIPLY, '*', '*'},
    {DIK_ADD, '+', '+'},
    {DIK_SUBTRACT, '-', '-'},
    {DIK_DECIMAL, '.', '.'},
    {DIK_DIVIDE, '/', '/'},
    {DIK_SEMICOLON, ';', ':'},
    {DIK_EQUALS, '=', '+'},
    {DIK_COMMA, ',', '<'},
    {DIK_MINUS, '-', '_'},
    {DIK_PERIOD, '.', '>'},
    {DIK_SLASH, '/', '?'},
    {DIK_GRAVE, '`', '~'},
    {DIK_LBRACKET, '[', '{'},
    {DIK_BACKSLASH, '\\', '|'},
    {DIK_RBRACKET, ']', '}'},
    {DIK_APOSTROPHE, '\'', '"'},
    {0, 0, 0},
};

// The I3D driver's z-buffer type for DXS_ZENABLE (I3D code)
extern uint i3d_zbuffer_type;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static int __cdecl SortModesCallback(const void *a, const void *b);
LS3D_RESULT ResetDevice(bool reset);
C_str FindMapFile(C_list<char *> *list, const char *name);

// FUNCTION: LS3DF 0x1006bf80
IGraph::IGraph()
{
    m_InitIsOK = false;
    memset(&m_InitDesc, 0, sizeof(m_InitDesc));
    memset(&m_D3DViewPort8, 0, sizeof(m_D3DViewPort8));
    m_MainHWnd = NULL;
    m_ChildHWnd = NULL;
    m_pD3D8 = NULL;
    m_pD3DDevice8 = NULL;
    m_MaxTextureWidth = 0;
    m_MaxTextureHeight = 0;
    m_VertexVS = 0;
    m_AppName = "LS3D";
    memset(&m_D3DPresentParams, 0, sizeof(m_D3DPresentParams));
    memset(m_KeyboardData, 0, sizeof(m_KeyboardData));
    memset(m_OldKeyboardData, 0, sizeof(m_OldKeyboardData));
    memset(m_ReadKeysBuffer, 0, sizeof(m_ReadKeysBuffer));
    memset(&m_MouseData, 0, sizeof(m_MouseData));
    memset(&m_VB, 0, sizeof(m_VB));
    m_StateBlock = 0;
    m_pDI8 = NULL;
    m_pDIKeyboard8 = NULL;
    m_KeyboardInitIsOK = false;
    m_pDIMouse8 = NULL;
    m_MouseInitIsOK = false;
    m_MousePos.y = 0;
    m_MousePos.x = 0;
    m_MouseMode = -1;
    m_LeftHandedMouse = false;
    m_VBPos = 0;
    m_IBPos = 0;
    memset(&m_IB, 0, sizeof(m_IB));
    m_AlphaRef = 0x20;
    m_CachedTextureList.reserve(0x1000);
    m_UncachedTextureList.reserve(0x1000);
    CoInitialize(NULL);
    InitializeCriticalSection(&m_FrameSection);
}

// FUNCTION: LS3DF 0x1006c1a0
IGraph::~IGraph()
{
    if (m_InitIsOK)
        dbgPrintf("!! WARNING  IGraph was initialised, but not closed");
    ClipCursor(NULL);
    DeleteCriticalSection(&m_FrameSection);
}

// FUNCTION: LS3DF 0x1006c1d0
IShow *IGraph::CreateIShow()
{
    IShow *show = new IShow;
    if (!show)
        return NULL;
    return show;
}

// FUNCTION: LS3DF 0x1006c230
ITexture *IGraph::CreateITexture()
{
    ITexture *texture = new ITexture;
    if (!texture)
        return NULL;
    texture->uid = ++m_TextureUID;
    if (m_TextureUID >= 0x7fff)
        m_TextureUID = 0;
    return texture;
}

// Looks for the texture among the cached ones unless flags say otherwise, then creates it from the file found in
// the maps directories, or in Maps\.
// FUNCTION: LS3DF 0x1006c2c0
LS3D_RESULT IGraph::CreateITexture(const char *name, const char *alpha_name, uint flags, ITexture **texture)
{
    *texture = NULL;
    if (alpha_name && *alpha_name)
        flags |= 4;
    if (!(flags & 0x200001)) {
        if (!name || !*name)
            return I3DERR_INVALIDPARAMS;
        for (int i = m_CachedTextureList.size(); i; ) {
            ITexture *t = m_CachedTextureList[--i];
            if (t->create_flags == flags && !_strcmpi(t->name, name) && (!(flags & 4) || !_strcmpi(t->alpha_name,
                    alpha_name))) {
                t->ref++;
                *texture = t;
                return I3D_OK;
            }
        }
    }
    ITexture *tex = CreateITexture();
    if (!tex)
        return I3DERR_OUTOFMEM;
    C_str full_name;
    if (name && !(flags & 1)) {
        full_name = FindMapFile(&m_MapsList, name);
        if (!full_name.Size()) {
            full_name = "Maps\\";
            full_name += name;
        }
    }
    C_str full_alpha_name;
    if (alpha_name && strlen(alpha_name)) {
        full_alpha_name = FindMapFile(&m_MapsList, alpha_name);
        if (!full_alpha_name.Size()) {
            full_alpha_name = "Maps\\";
            full_alpha_name += alpha_name;
        }
    }
    const char *alpha;
    if ((flags & 4) && full_alpha_name.Size() > 0)
        alpha = &full_alpha_name[0];
    else
        alpha = NULL;
    LS3D_RESULT result = tex->Create(!(flags & 1) ? &full_name[0] : name, alpha, flags, 0, 0, 0);
    if (result < 0) {
        tex->Release();
        *texture = NULL;
        return result;
    }
    *texture = tex;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1006c7c0
bool IGraph::SetTexture(ITexture *texture)
{
    if (!texture)
        return m_pD3DDevice8->SetTexture(0, NULL) >= 0 ? true : false;
    IDirect3DTexture8 *d3d_texture = texture->texture;
    return m_pD3DDevice8->SetTexture(0, d3d_texture) >= 0 ? true : false;
}

// FUNCTION: LS3DF 0x1006c800
HWND IGraph::GetMainHWND()
{
    return m_MainHWnd;
}

// FUNCTION: LS3DF 0x1006c810
HWND IGraph::GetChildHWND()
{
    return m_ChildHWnd;
}

// FUNCTION: LS3DF 0x1006c820
void IGraph::SetAppName(const char *name)
{
    if (strcmp(m_AppName, name)) {
        m_AppName = name;
        SetWindowTextA(m_MainHWnd, m_AppName);
    }
}

// FUNCTION: LS3DF 0x1006c8b0
void IGraph::SetAppNameDebug(const char *format, ...)
{
    char text[252];
    vsprintf(text, format, (va_list)(&format + 1));
    SetAppName(text);
}

// FUNCTION: LS3DF 0x1006c8f0
const char *IGraph::GetAppName()
{
    return m_AppName;
}

// FUNCTION: LS3DF 0x1006c910
bool IGraph::AddDlgHWND(void *hwnd)
{
    if (!hwnd)
        return false;
    m_DlgHWndList.push_back(hwnd);
    return true;
}

// FUNCTION: LS3DF 0x1006c9a0
bool IGraph::DeleteDlgHWND(void *hwnd)
{
    return m_DlgHWndList.remove(hwnd);
}

// Hands the window messages on, unless a dialog takes them, and keeps the characters typed. While the engine
// sleeps, it waits for messages here.
// FUNCTION: LS3DF 0x1006ca40
void IGraph::ProcessWinMessages()
{
    MSG msg;
    for (;;) {
        if (!PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (!m_Sleeping || m_DisableSleeping)
                return;
            Sleep(1);
            continue;
        }
        int i;
        for (i = 0; i < m_DlgHWndList.size(); i++) {
            if (IsDialogMessageA((HWND)m_DlgHWndList[i], &msg))
                break;
        }
        if (msg.message == WM_CHAR && !(msg.lParam & 0x80000000)) {
            for (int j = 1; j < 32; j++) {
                if (!m_KeyBuffer[j]) {
                    m_KeyBuffer[j] = msg.wParam;
                    break;
                }
            }
        }
        if (m_KBInitFlags & 2) {
            if (msg.message == WM_KEYDOWN) {
                switch (msg.wParam) {
                case VK_LEFT:
                    m_KBLastKey = 0xcb;
                    m_KBLastChar1 = 0;
                    break;
                case VK_RIGHT:
                    m_KBLastKey = 0xcd;
                    m_KBLastChar1 = 0;
                    break;
                case VK_UP:
                    m_KBLastKey = 0xc8;
                    m_KBLastChar1 = 0;
                    break;
                case VK_DOWN:
                    m_KBLastKey = 0xd0;
                    m_KBLastChar1 = 0;
                    break;
                }
            } else if (msg.message == WM_CHAR && !(msg.lParam & 0x80000000)) {
                switch (msg.wParam) {
                case 8:
                    m_KBLastKey = 0xe;
                    m_KBLastChar1 = 8;
                    break;
                case 9:
                    m_KBLastKey = 0xf;
                    m_KBLastChar1 = 9;
                    break;
                case 0x1b:
                    m_KBLastKey = 1;
                    m_KBLastChar1 = 0x1b;
                    break;
                default:
                    int vk = VkKeyScanA((char)msg.wParam);
                    if ((vk & 0xffff) == 0xffff) {
                        m_KBLastKey = 0;
                        m_KBLastChar1 = 0;
                    } else {
                        m_KBLastKey = MapVirtualKeyA(vk & 0xff, 0);
                        m_KBLastChar1 = msg.wParam;
                    }
                }
            }
        }
        if (i == m_DlgHWndList.size()) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

// FUNCTION: LS3DF 0x1006cc50
void IGraph::Clear(uint color, float z, uint flags)
{
    uint d3d_flags = 0;
    if (flags & 1)
        d3d_flags = D3DCLEAR_TARGET;
    if (flags & 2)
        d3d_flags |= D3DCLEAR_ZBUFFER;
    if (m_HardwareCaps & 0x10)
        d3d_flags |= D3DCLEAR_STENCIL;
    m_pD3DDevice8->Clear(0, NULL, d3d_flags, color, z, 0);
}

// FUNCTION: LS3DF 0x1006cca0
void IGraph::ResetRenderProps()
{
    m_pD3DDevice8->ApplyStateBlock(m_StateBlock);
    m_CurrCullState = 3;
    m_CullMode = 3;
    m_CurrFogState = 0;
    m_CurrFogType = 0;
    m_CurrFogEnd = 300.0f;
    m_CurrFogStart = 0;
    m_CurrFogColor = 0xffffffff;
    m_CurrClipState = 1;
    m_CurrAlphaState = 0;
    m_CurrBlendState = 0;
    m_CurrColorOp = 4;
    m_CurrAlphaOp = 4;
    m_CurrColorOp1 = 1;
    m_CurrColorOp2 = 1;
    m_CurrZBias = 0;
    m_CurrZFunc = 1;
    m_CurrAddressU = 1;
    m_CurrAddressV = 1;
    m_CurrVertexFormat = 0;
}

// FUNCTION: LS3DF 0x1006cd40
void IGraph::SetState(DX_STATE state, uint value)
{
    switch (state) {
    case DXS_ALPHA:
        if (m_CurrAlphaState == value)
            break;
        m_CurrAlphaState = value;
        switch (value) {
        case 0:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            break;
        case 1:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 1);
            break;
        case 3:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, m_AlphaRef);
            break;
        case 2:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 0);
            break;
        }
        break;
    case DXS_BLEND:
        if (m_CurrBlendState == value)
            break;
        m_CurrBlendState = value;
        switch (value) {
        case 0:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            break;
        case 1:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            break;
        case 2:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            break;
        case 3:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
            break;
        case 4:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
            break;
        }
        break;
    case DXS_CLIPPING:
        if (m_CurrClipState == value)
            break;
        m_CurrClipState = value;
        if (value == 1)
            m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
        else
            m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, FALSE);
        break;
    case DXS_CULL:
        if (m_CurrCullState == value)
            break;
        m_CurrCullState = value;
        if (value == 3)
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, m_CullMode);
        else
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, value);
        break;
    case DXS_COLOROP:
        if (m_CurrColorOp == value)
            break;
        m_CurrColorOp = value;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLOROP, value);
        break;
    case DXS_COLOROP1:
        if (m_CurrColorOp1 == value)
            break;
        m_CurrColorOp1 = value;
        m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLOROP, value);
        break;
    case DXS_COLOROP2:
        if (m_CurrColorOp2 == value)
            break;
        m_CurrColorOp2 = value;
        m_pD3DDevice8->SetTextureStageState(2, D3DTSS_COLOROP, value);
        break;
    case DXS_ALPHAOP:
        if (m_CurrAlphaOp == value)
            break;
        m_CurrAlphaOp = value;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAOP, value);
        break;
    case DXS_ZBIAS:
        if (m_CurrZBias == value)
            break;
        m_CurrZBias = value;
        m_pD3DDevice8->SetRenderState(D3DRS_ZBIAS, value);
        break;
    case DXS_ADDRESSU:
        if (m_CurrAddressU == value)
            break;
        m_CurrAddressU = value;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, value);
        break;
    // The original sets the u address here too
    case DXS_ADDRESSV:
        if (m_CurrAddressV == value)
            break;
        m_CurrAddressV = value;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, value);
        break;
    case DXS_ALPHAREF:
        m_AlphaRef = value;
        m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, value);
        break;
    case DXS_ZFUNC:
        if (m_CurrZFunc == value)
            break;
        m_CurrZFunc = value;
        m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, value ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS);
        break;
    case DXS_ZWRITE:
        m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, value != 0);
        break;
    case DXS_ZENABLE:
        m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, value ? i3d_zbuffer_type : D3DZB_FALSE);
        break;
    case DXS_CLAMP:
        if (!value) {
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        } else {
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
            m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        }
        break;
    case DXS_FOG:
        if (!value) {
            m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
            m_CurrFogState = 0;
        } else {
            m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, TRUE);
            m_CurrFogState = 1;
        }
        break;
    }
}

// FUNCTION: LS3DF 0x1006d350
uint IGraph::GetState(DX_STATE state)
{
    DWORD value;
    switch (state) {
    case DXS_ALPHA:
        return m_CurrAlphaState;
    case DXS_BLEND:
        return m_CurrBlendState;
    case DXS_CLIPPING:
        return m_CurrClipState;
    case DXS_CULL:
        return m_CurrCullState;
    case DXS_COLOROP:
        return m_CurrColorOp;
    case DXS_COLOROP1:
        return m_CurrColorOp1;
    case DXS_COLOROP2:
        return m_CurrColorOp2;
    case DXS_ALPHAOP:
        return m_CurrAlphaOp;
    case DXS_ZBIAS:
        return m_CurrZBias;
    case DXS_ADDRESSU:
        return m_CurrAddressU;
    case DXS_ADDRESSV:
        return m_CurrAddressU;
    case DXS_ALPHAREF:
        return m_AlphaRef;
    case DXS_FOG:
        return m_CurrFogState;
    case DXS_ZFUNC:
        return m_CurrZFunc;
    case DXS_ZWRITE:
        m_pD3DDevice8->GetRenderState(D3DRS_ZWRITEENABLE, &value);
        return value;
    case DXS_ZENABLE:
        m_pD3DDevice8->GetRenderState(D3DRS_ZENABLE, &value);
        return value;
    }
    return (uint)-1;
}

// FUNCTION: LS3DF 0x1006d450
void IGraph::SetWorldMatrix(const S_matrix &m)
{
    m_pD3DDevice8->SetTransform(D3DTS_WORLD, (const D3DMATRIX *)&m);
}

// FUNCTION: LS3DF 0x1006d470
void IGraph::BeginScene()
{
    if (m_SceneBegun)
        return;
    m_SceneBegun = true;
    m_pD3DDevice8->BeginScene();
}

// FUNCTION: LS3DF 0x1006d4a0
void IGraph::EndScene()
{
    m_SceneBegun = false;
    m_pD3DDevice8->EndScene();
}

// Shows the frame, after the frame rate if it is on. A lost device is waited for and reset.
// FUNCTION: LS3DF 0x1006d4c0
void IGraph::Present()
{
    UpdateAllTerminatedBuffs();
    if (m_ShowFPS) {
        m_FPSCounter++;
        if ((int)(timeGetTime() - m_FPSTimer) >= 1000) {
            m_FPSTimer = timeGetTime();
            m_FPSResult = m_FPSCounter;
            m_FPSCounter = 0;
        }
        m_pD3DDevice8->BeginScene();
        char text[50];
        sprintf(text, "%i", m_FPSResult);
        pI3DDriver->DrawText2D(0, 0, text, 0, 1.0f);
        m_pD3DDevice8->EndScene();
    }
    if (m_InitDesc.flags & IGRAPH_RENDER_THREAD) {
        EnterCriticalSection(&m_FrameSection);
        m_FrameCounter++;
        LeaveCriticalSection(&m_FrameSection);
    }
    Sleep(0);
    HRESULT hr = m_pD3DDevice8->Present(NULL, NULL, NULL, NULL);
    while (hr == D3DERR_DEVICELOST) {
        dbgPrintf("DEVICE LOST - 0");
        do {
            Sleep(250);
            ProcessWinMessages();
            hr = m_pD3DDevice8->TestCooperativeLevel();
        } while (hr != D3DERR_DEVICENOTRESET);
        dbgPrintf("DEVICE LOST - 1");
        if (ResetDevice(true) < 0)
            hr = D3DERR_DEVICELOST;
        else if (m_InitDesc.flags & IGRAPH_RENDER_THREAD)
            RestoreManagedTextures(m_InitDesc.flags);
        dbgPrintf("DEVICE LOST - 2");
    }
}

// FUNCTION: LS3DF 0x1006d640
LS3D_RESULT IGraph::DrawPrimitiveList(PRIMITIVE_TYPE type, uint num_prims, const void *verts, LS3D_STREAM_TYPE stream)
{
    if (!num_prims)
        return I3D_OK;
    D3DPRIMITIVETYPE d3d_type;
    switch (type) {
    case PT_POINTLIST:
        d3d_type = D3DPT_POINTLIST;
        break;
    case PT_LINELIST:
        d3d_type = D3DPT_LINELIST;
        break;
    case PT_LINESTRIP:
        d3d_type = D3DPT_LINESTRIP;
        break;
    case PT_TRIANGLELIST:
        d3d_type = D3DPT_TRIANGLELIST;
        break;
    case PT_TRIANGLESTRIP:
        d3d_type = D3DPT_TRIANGLESTRIP;
        break;
    case PT_TRIANGLEFAN:
        d3d_type = D3DPT_TRIANGLEFAN;
        break;
    case PT_POINTSPRITE:
        d3d_type = D3DPT_POINTLIST;
        break;
    default:
        return I3DERR_INVALIDPARAMS;
    }
    uint stride;
    switch (stream) {
    case LS3D_STREAM_LVERTEX:
        if (m_CurrVertexFormat) {
            m_CurrVertexFormat = 0;
            m_pD3DDevice8->SetVertexShader(m_LVertexVS);
        }
        stride = 0x18;
        break;
    case LS3D_STREAM_TLVERTEX:
        if (m_CurrVertexFormat != 4) {
            m_CurrVertexFormat = 4;
            m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
        }
        stride = 0x20;
        break;
    default:
        return I3DERR_INVALIDPARAMS;
    }
    return m_pD3DDevice8->DrawPrimitiveUP(d3d_type, num_prims, verts, stride) >= 0 ? I3D_OK : I3DERR_GENERIC;
}

// Transformed vertices are drawn from memory, the others through IGraph's dynamic buffers if the hardware has them
// (m_HardwareCaps & 1)
// FUNCTION: LS3DF 0x1006d750
LS3D_RESULT IGraph::DrawIndexedPrimitiveList(PRIMITIVE_TYPE type, uint num_verts, const void *indices, uint num_prims,
    const void *verts, LS3D_STREAM_TYPE stream)
{
    if (!num_prims)
        return I3D_OK;
    D3DPRIMITIVETYPE d3d_type;
    switch (type) {
    case PT_POINTLIST:
        d3d_type = D3DPT_POINTLIST;
        break;
    case PT_LINELIST:
        d3d_type = D3DPT_LINELIST;
        break;
    case PT_LINESTRIP:
        d3d_type = D3DPT_LINESTRIP;
        break;
    case PT_TRIANGLELIST:
        d3d_type = D3DPT_TRIANGLELIST;
        break;
    case PT_TRIANGLESTRIP:
        d3d_type = D3DPT_TRIANGLESTRIP;
        break;
    case PT_TRIANGLEFAN:
        d3d_type = D3DPT_TRIANGLEFAN;
        break;
    case PT_POINTSPRITE:
        d3d_type = D3DPT_POINTLIST;
        break;
    default:
        return I3DERR_INVALIDPARAMS;
    }
    uint stride;
    switch (stream) {
    case LS3D_STREAM_LVERTEX:
        if (m_HardwareCaps & 1) {
            BYTE *vb_data, *ib_data;
            if (m_VBPos + num_verts < 15000)
                m_VB.vb->Lock((m_VBPos + m_VB.base) * 0x18, num_verts * 0x18, &vb_data, D3DLOCK_NOOVERWRITE);
            else {
                m_VB.vb->Lock(m_VB.base * 0x18, num_verts * 0x18, &vb_data, D3DLOCK_DISCARD);
                m_VBPos = 0;
            }
            memcpy(vb_data, verts, num_verts * 0x18);
            m_VB.vb->Unlock();
            if (m_IBPos + num_prims * 3 < 45000)
                m_IB.ib->Lock((m_IB.base + m_IBPos) * 2, num_prims * 3 * 2, &ib_data, D3DLOCK_NOOVERWRITE);
            else {
                m_IB.ib->Lock(m_IB.base * 2, num_prims * 3 * 2, &ib_data, D3DLOCK_DISCARD);
                m_IBPos = 0;
            }
            memcpy(ib_data, indices, num_prims * 3 * 2);
            m_IB.ib->Unlock();
            if (m_CurrVertexFormat) {
                m_CurrVertexFormat = 0;
                m_pD3DDevice8->SetVertexShader(m_LVertexVS);
            }
            m_pD3DDevice8->SetStreamSource(0, m_VB.vb, 0x18);
            m_pD3DDevice8->SetIndices(m_IB.ib, m_VB.base + m_VBPos);
            m_pD3DDevice8->DrawIndexedPrimitive(d3d_type, 0, num_verts, m_IB.base + m_IBPos, num_prims);
            if (m_VBPos + num_verts < 15000)
                m_VBPos += num_verts;
            if (m_IBPos + num_prims * 3 < 45000)
                m_IBPos += num_prims * 3;
            return I3D_OK;
        }
        if (m_CurrVertexFormat) {
            m_CurrVertexFormat = 0;
            m_pD3DDevice8->SetVertexShader(m_LVertexVS);
        }
        stride = 0x18;
        break;
    case LS3D_STREAM_TLVERTEX:
        if (m_CurrVertexFormat != 4) {
            m_CurrVertexFormat = 4;
            m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
        }
        stride = 0x20;
        break;
    default:
        return I3DERR_INVALIDPARAMS;
    }
    return m_pD3DDevice8->DrawIndexedPrimitiveUP(d3d_type, 0, num_verts, num_prims, indices, D3DFMT_INDEX16, verts,
        stride) >= 0 ? I3D_OK : I3DERR_GENERIC;
}

// Gives the I3D driver its render states again, after the device lost them
static inline void RestoreRenderStates()
{
    for (int i = 0; i < 63; i++) {
        int value = renderState[i];
        renderState[i] = -1;
        pI3DDriver->SetState((I3D_RENDERSTATE)i, value);
    }
}

// The engine's window. While it is inactive or minimized, the engine sleeps (IGRAPH_SLEEP): it adds " ..zzZZ" to the
// title and suspends the game's render thread.
// FUNCTION: LS3DF 0x1006da60
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        pGlobalIGraph->Close();
        exit(0);
    case WM_ACTIVATE: {
        if (m_DisableSleeping) {
            if (m_Sleeping) {
                pGlobalIGraph->SetAppName(m_SleepAppName);
                m_Sleeping = false;
                if (m_pD3DDevice8)
                    RestoreRenderStates();
                if (m_RenderThread)
                    ResumeThread(m_RenderThread);
            }
            break;
        }
        // A local: the original shifts wparam before it tests the flag. LOWORD is tested as a word where it is used.
        BOOL minimized = HIWORD(wparam);
        if (m_InitDesc.flags & IGRAPH_SLEEP) {
            if (minimized) {
                if (!m_Sleeping) {
                    m_SleepAppName = m_AppName;
                    C_str name = m_AppName + " ..zzZZ";
                    pGlobalIGraph->SetAppName(name);
                    m_Sleeping = true;
                    if (m_RenderThread)
                        SuspendThread(m_RenderThread);
                }
            } else if (m_Sleeping) {
                if (LOWORD(wparam)) {
                    pGlobalIGraph->SetAppName(m_SleepAppName);
                    m_Sleeping = false;
                    if (m_pD3DDevice8)
                        RestoreRenderStates();
                    if (m_RenderThread)
                        ResumeThread(m_RenderThread);
                }
            } else if (!LOWORD(wparam)) {
                HWND foreground = GetForegroundWindow();
                if (!IsChild(m_MainHWnd, foreground) && m_MainHWnd != foreground) {
                    m_SleepAppName = m_AppName;
                    C_str name = m_AppName + " ..zzZZ";
                    pGlobalIGraph->SetAppName(name);
                    m_Sleeping = true;
                    if (m_RenderThread)
                        SuspendThread(m_RenderThread);
                }
            }
        } else if (m_Sleeping) {
            pGlobalIGraph->SetAppName(m_SleepAppName);
            m_Sleeping = false;
            if (m_pD3DDevice8 && i3d_driver_init)
                RestoreRenderStates();
            if (m_RenderThread)
                ResumeThread(m_RenderThread);
        } else if (m_pD3DDevice8 && i3d_driver_init)
            RestoreRenderStates();
        break;
    }
    case WM_ERASEBKGND:
        return 0;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        if (wparam == VK_F10 || (lparam & 0x20000000))
            return 1;
        break;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_SCREENSAVE || (wparam & 0xfff0) == SC_MONITORPOWER)
            return 1;
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

// The cases are in the original's order, which isn't the enum's
// FUNCTION: LS3DF 0x1006ded0
const char *IGraph::GetFormatName(D3DFORMAT format) const
{
    switch (format) {
    case D3DFMT_UNKNOWN:
        return "D3DFMT_UNKNOWN";
    case D3DFMT_R8G8B8:
        return "D3DFMT_R8G8B8";
    case D3DFMT_R3G3B2:
        return "D3DFMT_R3G3B2";
    case D3DFMT_A8R3G3B2:
        return "D3DFMT_A8R3G3B2";
    case D3DFMT_X4R4G4B4:
        return "D3DFMT_X4R4G4B4";
    case D3DFMT_A8P8:
        return "D3DFMT_A8P8";
    case D3DFMT_A4L4:
        return "D3DFMT_A4L4";
    case D3DFMT_X8L8V8U8:
        return "D3DFMT_X8L8V8U8";
    case D3DFMT_A8R8G8B8:
        return "D3DFMT_A8R8G8B8";
    case D3DFMT_X8R8G8B8:
        return "D3DFMT_X8R8G8B8";
    case D3DFMT_R5G6B5:
        return "D3DFMT_R5G6B5";
    case D3DFMT_X1R5G5B5:
        return "D3DFMT_X1R5G5B5";
    case D3DFMT_A1R5G5B5:
        return "D3DFMT_A1R5G5B5";
    case D3DFMT_A4R4G4B4:
        return "D3DFMT_A4R4G4B4";
    case D3DFMT_A8:
        return "D3DFMT_A8";
    case D3DFMT_P8:
        return "D3DFMT_P8";
    case D3DFMT_L8:
        return "D3DFMT_L8";
    case D3DFMT_A8L8:
        return "D3DFMT_A8L8";
    case D3DFMT_V8U8:
        return "D3DFMT_V8U8";
    case D3DFMT_L6V5U5:
        return "D3DFMT_L6V5U5";
    case D3DFMT_Q8W8V8U8:
        return "D3DFMT_Q8W8V8U8";
    case D3DFMT_W11V11U10:
        return "D3DFMT_W11V11U10";
    case D3DFMT_D32:
        return "D3DFMT_D32";
    case D3DFMT_D15S1:
        return "D3DFMT_D15S1";
    case D3DFMT_D16_LOCKABLE:
        return "D3DFMT_D16_LOCKABLE";
    case D3DFMT_D24X8:
        return "D3DFMT_D24X8";
    case D3DFMT_D24X4S4:
        return "D3DFMT_D24X4S4";
    case D3DFMT_V16U16:
        return "D3DFMT_V16U16";
    case D3DFMT_D24S8:
        return "D3DFMT_D24S8";
    case D3DFMT_D16:
        return "D3DFMT_D16";
    case D3DFMT_VERTEXDATA:
        return "D3DFMT_VERTEXDATA";
    case D3DFMT_INDEX16:
        return "D3DFMT_INDEX16";
    case D3DFMT_DXT1:
        return "D3DFMT_DXT1";
    case D3DFMT_INDEX32:
        return "D3DFMT_INDEX32";
    case D3DFMT_DXT2:
        return "D3DFMT_DXT2";
    case D3DFMT_YUY2:
        return "D3DFMT_YUY2";
    case D3DFMT_DXT3:
        return "D3DFMT_DXT3";
    case D3DFMT_DXT5:
        return "D3DFMT_DXT5";
    case D3DFMT_DXT4:
        return "D3DFMT_DXT4";
    case D3DFMT_UYVY:
        return "D3DFMT_UYVY";
    case D3DFMT_FORCE_DWORD:
        return "D3DFMT_FORCE_DWORD";
    }
    return "UNSUPPORTED_FORMAT";
}

// A float's bits, for SetRenderState
inline DWORD F2DW(float f)
{
    return *(DWORD *)&f;
}

// Logs a failed DirectX call. The original gives __FILE__ and __LINE__, which our build can't reproduce, so the file
// is the original's path and the line is passed in.
#define DX_ERROR(hr, line) dbgPrintf("!! Error detected: %s, File: %s, Line: %i !!", DXGetErrorString8(hr), \
    "C:\\MyProjects\\engine8\\IGraph\\IGraph.cpp", line)

// Frees what a lost device has to give up, resets the device (unless only the shaders and states are wanted again)
// and creates IGraph's vertex shaders and the default render states again
// FUNCTION: LS3DF 0x1006e1a0
LS3D_RESULT ResetDevice(bool reset)
{
    if (m_StateBlock)
        m_pD3DDevice8->DeleteStateBlock(m_StateBlock);
    cVBMan.m_FreeAllVRAMVBuffers();
    cIBMan.m_FreeAllVRAMIBuffers();
    if (m_InitDesc.flags & IGRAPH_RENDER_THREAD)
        ReleaseManagedTextures();
    HRESULT hr;
    if (reset) {
        hr = m_pD3DDevice8->Reset(&m_D3DPresentParams);
        if (FAILED(hr)) {
            DX_ERROR(hr, 1644);
            return I3DERR_GENERIC;
        }
        if ((m_HardwareCaps & 1) && !m_VB.vb) {
            if (!cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0x18,
                    15000, m_VB, 0, 0))
                dbgPrintf("Error creating Global Vertex Buffer !");
            if (!cIBMan.m_Alloc(D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 45000, m_IB))
                dbgPrintf("Error creating Global Index Buffer !");
        }
        if (m_pD3DDevice8)
            RestoreRenderStates();
    }
    DWORD decl[] = {D3DVSD_STREAM(0), D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3), D3DVSD_END()};
    hr = m_pD3DDevice8->CreateVertexShader(decl, NULL, &m_VertexVS, D3DUSAGE_SOFTWAREPROCESSING);
    if (FAILED(hr)) {
        DX_ERROR(hr, 1688);
        return I3DERR_GENERIC;
    }
    DWORD ldecl[] = {D3DVSD_STREAM(0), D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),
        D3DVSD_REG(D3DVSDE_DIFFUSE, D3DVSDT_D3DCOLOR), D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2), D3DVSD_END()};
    hr = m_pD3DDevice8->CreateVertexShader(ldecl, NULL, &m_LVertexVS,
        m_HardwareCaps & 1 ? 0 : D3DUSAGE_SOFTWAREPROCESSING);
    if (FAILED(hr)) {
        DX_ERROR(hr, 1697);
        return I3DERR_GENERIC;
    }
    DWORD l2decl[] = {D3DVSD_STREAM(0), D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),
        D3DVSD_REG(D3DVSDE_DIFFUSE, D3DVSDT_D3DCOLOR), D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_REG(D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT2), D3DVSD_END()};
    hr = m_pD3DDevice8->CreateVertexShader(l2decl, NULL, &m_LVertex2VS,
        m_HardwareCaps & 1 ? 0 : D3DUSAGE_SOFTWAREPROCESSING);
    if (FAILED(hr)) {
        DX_ERROR(hr, 1707);
        return I3DERR_GENERIC;
    }
    m_pD3DDevice8->BeginStateBlock();
    m_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, 0xff00ffff);
    m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    m_pD3DDevice8->SetRenderState(D3DRS_FOGEND, F2DW(300.0f));
    m_pD3DDevice8->SetRenderState(D3DRS_FOGSTART, F2DW(0.0f));
    m_pD3DDevice8->SetRenderState(D3DRS_FOGCOLOR, 0xffffffff);
    m_pD3DDevice8->SetRenderState(m_HardwareCaps & 4 ? D3DRS_FOGTABLEMODE : D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_pD3DDevice8->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_pD3DDevice8->SetRenderState(D3DRS_ZBIAS, 0);
    m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_pD3DDevice8->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
    m_pD3DDevice8->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    m_pD3DDevice8->SetRenderState(D3DRS_COLORVERTEX, TRUE);
    m_pD3DDevice8->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    m_pD3DDevice8->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    m_pD3DDevice8->SetVertexShader(m_LVertexVS);
    m_pD3DDevice8->EndStateBlock(&m_StateBlock);
    m_SceneBegun = false;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1006e740
LS3D_RESULT IGraph::SetViewPort(LS3D_VIEWPORT *viewport)
{
    m_D3DViewPort8 = *(D3DVIEWPORT8 *)viewport;
    return m_pD3DDevice8->SetViewport(&m_D3DViewPort8) >= 0 ? I3D_OK : I3DERR_GENERIC;
}

// FUNCTION: LS3DF 0x1006e780
LS3D_VIEWPORT *IGraph::GetViewPort()
{
    return (LS3D_VIEWPORT *)&m_D3DViewPort8;
}

// The DirectX 8 SDK sample's checks (PS2 names). PS2 has them out of line; the PC inlines them into Init.
static inline BOOL IsDepthFormatOk(D3DFORMAT depth_format, D3DFORMAT adapter_format, D3DFORMAT back_buffer_format)
{
    if (FAILED(m_pD3D8->CheckDeviceFormat(m_InitDesc.adapter, m_DeviceType, adapter_format,
            D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depth_format)))
        return FALSE;
    return SUCCEEDED(m_pD3D8->CheckDepthStencilMatch(m_InitDesc.adapter, m_DeviceType, adapter_format,
        back_buffer_format, depth_format));
}

static inline BOOL IsTextureFormatOk(D3DFORMAT texture_format, D3DFORMAT adapter_format)
{
    return SUCCEEDED(m_pD3D8->CheckDeviceFormat(m_InitDesc.adapter, m_DeviceType, adapter_format, 0,
        D3DRTYPE_TEXTURE, texture_format));
}

// Creates the window and the device after Close, logs the system, and finds the formats to use
// FUNCTION: LS3DF 0x1006e790
LS3D_RESULT IGraph::Init(IGRAPH_INIT_DESC *desc)
{
    // DirectX 8.1 brought this DLL
    HMODULE dll = LoadLibraryA("dpnhpast.dll");
    if (!dll) {
        dbgPrintf("ERROR! DirectX version is lower 8.1 !");
        return I3DERR_UNSUPPORTED;
    }
    FreeLibrary(dll);
    if (!desc)
        return I3DERR_INVALIDPARAMS;
    m_InitDesc = *desc;
    Close();
    memset(&i3d_stats, 0, sizeof(i3d_stats));
    WNDCLASSEXA wc;
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_InitDesc.hinstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "LS3D";
    wc.hIconSm = NULL;
    if (!RegisterClassExA(&wc)) {
        dbgPrintf("IGraph::Init() - Error register window class !!");
        return I3DERR_GENERIC;
    }
    if (m_InitDesc.flags & IGRAPH_FULLSCREEN) {
        m_InitDesc.flags &= ~IGRAPH_CHILD_WINDOW;
        SetRect(&m_InitDesc.child_rect, 0, 0, 0, 0);
        m_InitDesc.x = 0;
        m_InitDesc.y = 0;
        m_InitDesc.flags &= ~IGRAPH_MENU;
        m_InitDesc.menu = NULL;
        m_MainHWnd = CreateWindowExA(WS_EX_TOPMOST, "LS3D", "LS3D engine", WS_VISIBLE, m_InitDesc.x, m_InitDesc.y,
            m_InitDesc.width, m_InitDesc.height, GetDesktopWindow(), NULL, wc.hInstance, NULL);
        if (!m_MainHWnd)
            return I3DERR_GENERIC;
        UpdateWindow(m_MainHWnd);
    } else {
        int frame_x = GetSystemMetrics(SM_CXFIXEDFRAME);
        int frame_y = GetSystemMetrics(SM_CYFIXEDFRAME);
        int caption = GetSystemMetrics(SM_CYCAPTION);
        int menu = 0;
        if (m_InitDesc.flags & IGRAPH_MENU) {
            if (!m_InitDesc.menu) {
                HMODULE module = GetModuleHandleA("LS3D.DLL");
                m_InitDesc.menu = LoadMenuA(module, MAKEINTRESOURCE(123));
            }
            menu = GetSystemMetrics(SM_CYMENU);
        }
        // The window around the client area
        int x = m_InitDesc.x - frame_x;
        int y = m_InitDesc.y - menu - caption - frame_y;
        int sx = m_InitDesc.width + frame_x * 2;
        int sy = m_InitDesc.height + frame_y * 2 + menu + caption;
        m_MainHWnd = CreateWindowExA(0, "LS3D", "LS3D engine", WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU |
            WS_MINIMIZEBOX, x, y, sx, sy, GetDesktopWindow(), m_InitDesc.menu, wc.hInstance, NULL);
        if (!m_MainHWnd) {
            dbgPrintf("IGraph::Init() - Error create m_MainHWnd !!");
            return I3DERR_GENERIC;
        }
        UpdateWindow(m_MainHWnd);
        if (m_InitDesc.flags & IGRAPH_CHILD_WINDOW) {
            m_ChildHWnd = CreateWindowExA(0, "LS3D", "LS3D engine", WS_CHILD | WS_VISIBLE, m_InitDesc.child_rect.left,
                m_InitDesc.child_rect.top, m_InitDesc.child_rect.right, m_InitDesc.child_rect.bottom, m_MainHWnd,
                NULL, wc.hInstance, NULL);
            if (!m_ChildHWnd) {
                dbgPrintf("IGraph::Init() - Error create m_ChildHWnd !!");
                return I3DERR_GENERIC;
            }
            UpdateWindow(m_ChildHWnd);
        }
    }
    SetFocus(m_MainHWnd);
    m_pD3D8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!m_pD3D8) {
        dbgPrintf("IGraph::Init() - Error create D3D8 object !!");
        return I3DERR_GENERIC;
    }

    // The modes, once for each size and format, and the formats (the variable takes the current mode later)
    D3DDISPLAYMODE mode;
    int i;
    int num_modes = m_pD3D8->GetAdapterModeCount(m_InitDesc.adapter);
    while (num_modes) {
        m_pD3D8->EnumAdapterModes(m_InitDesc.adapter, --num_modes, &mode);
        bool found = false;
        for (i = m_AdapterModeList.size(); i--; ) {
            if (mode.Width == m_AdapterModeList[i].Width && mode.Height == m_AdapterModeList[i].Height &&
                    mode.Format == m_AdapterModeList[i].Format) {
                found = true;
                break;
            }
        }
        if (!found) {
            mode.RefreshRate = 0;
            m_AdapterModeList.push_back(mode);
        }
        found = false;
        for (i = m_AdapterFormatList.size(); i--; ) {
            if (mode.Format == m_AdapterFormatList[i]) {
                found = true;
                break;
            }
        }
        if (!found)
            m_AdapterFormatList.push_back(mode.Format);
    }
    qsort(&m_AdapterModeList[0], m_AdapterModeList.size(), sizeof(D3DDISPLAYMODE), SortModesCallback);

    _p_info cpu;
    memset(&cpu, 0, sizeof(cpu));
    _cpuid(&cpu);
    OSVERSIONINFOEXA os;
    memset(&os, 0, sizeof(os));
    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionExA((OSVERSIONINFOA *)&os);
    char os_name[50];
    strcpy(os_name, "Windows ??");
    switch (os.dwMajorVersion) {
    case 5:
        switch (os.dwMinorVersion) {
        case 0:
            strcpy(os_name, "Windows 2000");
            break;
        case 1:
            strcpy(os_name, "Windows XP");
            break;
        }
        break;
    case 4:
        if (os.dwMinorVersion)
            strcpy(os_name, "Windows 9x/ME");
        else
            strcpy(os_name, "Windows NT 4.0");
        break;
    case 3:
        switch (os.dwMinorVersion) {
        case 0:
            strcpy(os_name, "Windows NT 3.0");
            break;
        case 1:
            strcpy(os_name, "Windows 3.1");
            break;
        }
        break;
    }
    dbgPrintf("\nDetected OS: %s ver. %i.%i.%i %s", os_name, os.dwMajorVersion, os.dwMinorVersion,
        LOWORD(os.dwBuildNumber), os.szCSDVersion);
    dbgPrintf("Detected CPU: %s, %s %iMHz, family:%i, model:%i, stepping:%i", cpu.v_name, cpu.model_name, GetCPUSpeed(),
        cpu.family, cpu.model, cpu.stepping);
    dbgPrintf("This DLL uses standard x87 math");
    if (m_InitDesc.flags & IGRAPH_LOG) {
        dbgPrintf("All usable display modes:");
        for (i = m_AdapterModeList.size(); i--; ) {
            D3DDISPLAYMODE m = m_AdapterModeList[i];
            dbgPrintf("   Width: %4i, Height: %4i, Format: %s", m.Width, m.Height, GetFormatName(m.Format));
        }
        dbgPrintf("");
    }

    HRESULT hr = m_pD3D8->GetAdapterDisplayMode(m_InitDesc.adapter, &mode);
    if (FAILED(hr)) {
        DX_ERROR(hr, 2137);
        return I3DERR_GENERIC;
    }
    D3DADAPTER_IDENTIFIER8 id;
    hr = m_pD3D8->GetAdapterIdentifier(m_InitDesc.adapter, D3DENUM_NO_WHQL_LEVEL, &id);
    if (FAILED(hr)) {
        DX_ERROR(hr, 2141);
        return I3DERR_GENERIC;
    }
    dbgPrintf("Current display mode: %ix%i, %iHz, format: %s \n", mode.Width, mode.Height,
        mode.RefreshRate, GetFormatName(mode.Format));
    dbgPrintf("Using adapter number: %i", m_InitDesc.adapter);
    dbgPrintf("              Driver: %s", id.Driver);
    dbgPrintf("         Description: %s", id.Description);
    dbgPrintf("      Driver version: %d.%02d.%02d.%04d", HIWORD(id.DriverVersion.HighPart),
        LOWORD(id.DriverVersion.HighPart), HIWORD(id.DriverVersion.LowPart), LOWORD(id.DriverVersion.LowPart));
    dbgPrintf("           Vendor ID: %lX", id.VendorId);
    dbgPrintf("           Device ID: %lX", id.DeviceId);
    dbgPrintf("           SubSys ID: %lX", id.SubSysId);
    dbgPrintf("            Revision: %lX", id.Revision);
    dbgPrintGuid(&id.DeviceIdentifier);
    dbgPrintf("");
    D3DCAPS8 caps;
    hr = m_pD3D8->GetDeviceCaps(m_InitDesc.adapter, m_DeviceType, &caps);
    if (FAILED(hr)) {
        DX_ERROR(hr, 2167);
        return I3DERR_GENERIC;
    }

    // What the card can't do, by its caps and by its make
    if (m_InitDesc.flags & 0x8000)
        m_ProblemFlags |= 2;
    if (caps.MaxSimultaneousTextures < 2) {
        m_ProblemFlags |= 0xe;
        m_InitDesc.flags |= IGRAPH_MULTIPASS;
    }
    if (caps.MaxTextureWidth == 256) {
        m_ProblemFlags |= 0xc;
        m_InitDesc.flags |= IGRAPH_MULTIPASS;
    }
    if (id.VendorId == 0x121a) {
        // 3dfx
        if (id.DeviceId == 9)
            m_ProblemFlags |= 8;
    } else if (id.VendorId == 0x5333) {
        // S3
        if (id.DeviceId == 0x8a22 || id.DeviceId == 0x8c10) {
            m_ProblemFlags |= 0x3c;
            m_InitDesc.flags |= IGRAPH_MULTIPASS;
        }
    } else if (id.VendorId == 0x102b) {
        // Matrox
        if (id.DeviceId == 0x521 || id.DeviceId == 0x520 || id.DeviceId == 0x1000 || id.DeviceId == 0x1001 ||
                id.DeviceId == 0x51f || id.DeviceId == 0x51b || id.DeviceId == 0x51a || id.DeviceId == 0x519) {
            m_ProblemFlags |= 0x8e;
            m_InitDesc.flags |= IGRAPH_MULTIPASS;
        }
        if (id.DeviceId == 0x2527)
            m_ProblemFlags |= 2;
    } else if (id.VendorId == 0x1002) {
        // ATI
        m_ProblemFlags |= 0x102;
    }
    if ((m_InitDesc.flags & IGRAPH_HWTL) && (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
        m_HardwareCaps |= 1;
        dbgPrintf("Using HARDWARE T&L.");
    }
    if (m_InitDesc.flags & IGRAPH_MULTIPASS)
        dbgPrintf("Using MULTIPASS rendering.");
    else {
        m_HardwareCaps |= 0x80;
        dbgPrintf("Using SINGLEPASS rendering.");
    }
    if (!(m_HardwareCaps & 0x80) && (m_HardwareCaps & 1)) {
        dbgPrintf("\nWARNING ! HARDWARE TRANSFORMATIONS and MULTIPASS cannot be set simultaneously !!! "
            "Forcing software transformations !\n");
        m_HardwareCaps &= ~1;
    }
    if ((caps.TextureOpCaps & D3DTEXOPCAPS_MODULATE2X) && !(m_ProblemFlags & 4))
        m_Modulate2X = D3DTOP_MODULATE2X;
    else {
        m_Modulate2X = D3DTOP_MODULATE;
        i3d_light_scale = 255.0f;
    }
    if (!(caps.TextureOpCaps & D3DTEXOPCAPS_BLENDCURRENTALPHA))
        m_ProblemFlags |= 0x20;
    if (!(caps.TextureOpCaps & D3DTEXOPCAPS_BLENDFACTORALPHA))
        m_ProblemFlags |= 0x10;
    if (!(caps.TextureOpCaps & D3DTEXOPCAPS_ADD))
        m_ProblemFlags |= 0x40;
    m_MaxTextureWidth = caps.MaxTextureWidth;
    m_MaxTextureHeight = caps.MaxTextureHeight;

    // The back buffer: the desktop's format in a window, a mode of the wanted depth on the full screen
    memset(&m_D3DPresentParams, 0, sizeof(m_D3DPresentParams));
    if (!(m_InitDesc.flags & IGRAPH_FULLSCREEN)) {
        m_D3DPresentParams.Windowed = TRUE;
        m_D3DPresentParams.BackBufferFormat = mode.Format;
        if (mode.Format == D3DFMT_R5G6B5 || mode.Format == D3DFMT_X1R5G5B5 ||
                mode.Format == D3DFMT_A1R5G5B5)
            m_InitDesc.bpp = 16;
        if (mode.Format == D3DFMT_R8G8B8 || mode.Format == D3DFMT_X8R8G8B8 ||
                mode.Format == D3DFMT_A8R8G8B8)
            m_InitDesc.bpp = 32;
        m_D3DPresentParams.hDeviceWindow = m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_ChildHWnd : m_MainHWnd;
    } else {
        m_D3DPresentParams.FullScreen_RefreshRateInHz = m_InitDesc.refresh_rate;
        m_D3DPresentParams.FullScreen_PresentationInterval = m_InitDesc.flags & IGRAPH_VSYNC ?
            D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        m_D3DPresentParams.BackBufferWidth = m_InitDesc.width;
        m_D3DPresentParams.BackBufferHeight = m_InitDesc.height;
        m_D3DPresentParams.BackBufferCount = m_InitDesc.flags & IGRAPH_TRIPLE_BUFFER ? 2 : 1;
        bool found = false;
        for (i = m_AdapterModeList.size(); i--; ) {
            if (m_AdapterModeList[i].Width == m_InitDesc.width &&
                    m_AdapterModeList[i].Height == m_InitDesc.height) {
                if (m_InitDesc.bpp == 16) {
                    if (m_AdapterModeList[i].Format == D3DFMT_R5G6B5 ||
                            m_AdapterModeList[i].Format == D3DFMT_X1R5G5B5 ||
                            m_AdapterModeList[i].Format == D3DFMT_A1R5G5B5) {
                        found = true;
                        break;
                    }
                } else if (m_InitDesc.bpp == 32) {
                    if (m_AdapterModeList[i].Format == D3DFMT_R8G8B8 ||
                            m_AdapterModeList[i].Format == D3DFMT_X8R8G8B8 ||
                            m_AdapterModeList[i].Format == D3DFMT_A8R8G8B8) {
                        found = true;
                        break;
                    }
                }
            }
        }
        if (found)
            m_D3DPresentParams.BackBufferFormat = m_AdapterModeList[i].Format;
        if (!m_D3DPresentParams.BackBufferFormat) {
            dbgPrintf("IGraph::Init() - Error. Useful BackBuffer format not found !!");
            return I3DERR_GENERIC;
        }
    }

    // The depth buffer: the formats with stencil first, from the end
    m_D3DPresentParams.EnableAutoDepthStencil = TRUE;
    D3DFORMAT depth16[] = {D3DFMT_D15S1, D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D32};
    D3DFORMAT depth32[] = {D3DFMT_D24X4S4, D3DFMT_D24S8, D3DFMT_D24X8, D3DFMT_D32};
    D3DFORMAT *depth;
    if (m_InitDesc.bpp == 16)
        depth = depth16;
    else
        depth = depth32;
    for (int num_depth = 4; num_depth--; ) {
        if (IsDepthFormatOk(depth[num_depth], m_D3DPresentParams.BackBufferFormat,
                m_D3DPresentParams.BackBufferFormat)) {
            m_D3DPresentParams.AutoDepthStencilFormat = depth[num_depth];
            break;
        }
    }
    if (m_InitDesc.flags & IGRAPH_LOCKABLE_ZBUFFER)
        m_D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D16_LOCKABLE;
    else {
        if (!m_D3DPresentParams.AutoDepthStencilFormat) {
            dbgPrintf("IGraph::Init() - Error. Useful Z-Buffer format not found !!");
            return I3DERR_GENERIC;
        }
        if (m_D3DPresentParams.AutoDepthStencilFormat == D3DFMT_D15S1 ||
                m_D3DPresentParams.AutoDepthStencilFormat == D3DFMT_D24X4S4 ||
                m_D3DPresentParams.AutoDepthStencilFormat == D3DFMT_D24S8)
            m_HardwareCaps |= 0x10;
    }

    // Full scene antialiasing
    D3DMULTISAMPLE_TYPE multi_sample = D3DMULTISAMPLE_NONE;
    D3DSWAPEFFECT swap_effect = D3DSWAPEFFECT_FLIP;
    if (m_InitDesc.flags & (IGRAPH_FSAA2 | IGRAPH_FSAA4)) {
        if (m_InitDesc.flags & IGRAPH_FSAA2) {
            if (m_InitDesc.flags & IGRAPH_LOG)
                dbgPrintf("Trying to use 2 samples Full Scene AntiAliasing ...");
            multi_sample = D3DMULTISAMPLE_2_SAMPLES;
        } else {
            if (m_InitDesc.flags & IGRAPH_LOG)
                dbgPrintf("Trying to use 4 samples Full Scene AntiAliasing ...");
            multi_sample = D3DMULTISAMPLE_4_SAMPLES;
        }
        if (SUCCEEDED(m_pD3D8->CheckDeviceMultiSampleType(m_InitDesc.adapter, D3DDEVTYPE_HAL,
                m_D3DPresentParams.BackBufferFormat, m_D3DPresentParams.Windowed, multi_sample))) {
            if (m_InitDesc.flags & IGRAPH_LOG)
                dbgPrintf("OK... FSAA enabled.\n");
            swap_effect = D3DSWAPEFFECT_DISCARD;
        } else {
            if (m_InitDesc.flags & IGRAPH_LOG)
                dbgPrintf("ERROR... cannot use FSAA with current backbuffer format !\n");
            m_InitDesc.flags &= ~(IGRAPH_FSAA2 | IGRAPH_FSAA4);
            multi_sample = D3DMULTISAMPLE_NONE;
        }
    }
    m_D3DPresentParams.SwapEffect = swap_effect;
    m_D3DPresentParams.MultiSampleType = multi_sample;
    if (m_D3DPresentParams.Windowed) {
        int sy = m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_InitDesc.child_rect.bottom : m_InitDesc.height;
        int sx = m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_InitDesc.child_rect.right : m_InitDesc.width;
        dbgPrintf("Trying to create device with WINDOWED mode: %ix%i, %iHz, BackFormat: %s, DepthFormat: %s", sx, sy,
            mode.RefreshRate, GetFormatName(m_D3DPresentParams.BackBufferFormat),
            GetFormatName(m_D3DPresentParams.AutoDepthStencilFormat));
    } else {
        dbgPrintf("Trying to create device with FULLSCREEN mode: %ix%i, %iHz, BackFormat: %s, DepthFormat: %s",
            m_D3DPresentParams.BackBufferWidth, m_D3DPresentParams.BackBufferHeight,
            m_D3DPresentParams.FullScreen_RefreshRateInHz, GetFormatName(m_D3DPresentParams.BackBufferFormat),
            GetFormatName(m_D3DPresentParams.AutoDepthStencilFormat));
    }
    if (m_InitDesc.flags & IGRAPH_REFERENCE) {
        dbgPrintf("\nIGraph::Init() - Warning. SLOW REFERENCE RASTERIZER used !!\n");
        m_DeviceType = D3DDEVTYPE_REF;
    }
    hr = m_pD3D8->CreateDevice(m_InitDesc.adapter, m_DeviceType, m_MainHWnd, (m_HardwareCaps & 1 ?
        D3DCREATE_MIXED_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING) | D3DCREATE_MULTITHREADED,
        &m_D3DPresentParams, &m_pD3DDevice8);
    if (FAILED(hr)) {
        DX_ERROR(hr, 2547);
        return I3DERR_GENERIC;
    }
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice8->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    m_pD3DDevice8->SetRenderState(D3DRS_DITHERENABLE, TRUE);
    m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    if (m_InitDesc.flags & (IGRAPH_FSAA2 | IGRAPH_FSAA4))
        m_pD3DDevice8->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    if (!(m_HardwareCaps & 1))
        m_pD3DDevice8->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, TRUE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, F2DW(0.0f));
    m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, m_AlphaRef);
    ResetDevice(false);
    hr = m_pD3D8->GetAdapterDisplayMode(m_InitDesc.adapter, &mode);
    if (FAILED(hr)) {
        DX_ERROR(hr, 2583);
        return I3DERR_GENERIC;
    }
    m_D3DViewPort8.X = 0;
    m_D3DViewPort8.Y = 0;
    m_D3DViewPort8.Width = Scrn_sx();
    m_D3DViewPort8.Height = Scrn_sy();
    m_D3DViewPort8.MinZ = 0.0f;
    m_D3DViewPort8.MaxZ = 1.0f;

    // The texture formats
    if (IsTextureFormatOk(D3DFMT_R5G6B5, mode.Format))
        m_TexturePixelFormat = D3DFMT_R5G6B5;
    else if (IsTextureFormatOk(D3DFMT_X1R5G5B5, mode.Format))
        m_TexturePixelFormat = D3DFMT_X1R5G5B5;
    else if (IsTextureFormatOk(D3DFMT_A1R5G5B5, mode.Format))
        m_TexturePixelFormat = D3DFMT_A1R5G5B5;
    else
        dbgPrintf("IGraph::Init() - Error. Useful 16bit texture format not found !!");
    if (IsTextureFormatOk(D3DFMT_R5G6B5, mode.Format))
        m_TexturePixelFormat = D3DFMT_R5G6B5;
    else if (IsTextureFormatOk(D3DFMT_X1R5G5B5, mode.Format))
        m_TexturePixelFormat = D3DFMT_X1R5G5B5;
    else if (IsTextureFormatOk(D3DFMT_A1R5G5B5, mode.Format))
        m_TexturePixelFormat = D3DFMT_A1R5G5B5;
    else {
        dbgPrintf("IGraph::Init() - Error. Useful 16bit texture format not found !!");
        return I3DERR_BADTEXTUREFORMAT;
    }
    if (IsTextureFormatOk(D3DFMT_A1R5G5B5, mode.Format))
        m_Alpha1TexturePixelFormat = D3DFMT_A1R5G5B5;
    else {
        dbgPrintf("IGraph::Init() - Error. Useful 1bit Alpha texture format not found !!");
        return I3DERR_BADTEXTUREFORMAT;
    }
    if (IsTextureFormatOk(D3DFMT_A4R4G4B4, mode.Format))
        m_AlphaTexturePixelFormat = D3DFMT_A4R4G4B4;
    else if (IsTextureFormatOk(D3DFMT_A8R8G8B8, mode.Format))
        m_AlphaTexturePixelFormat = D3DFMT_A8R8G8B8;
    else {
        dbgPrintf("IGraph::Init() - Error. Useful Alpha texture format not found !!");
        return I3DERR_BADTEXTUREFORMAT;
    }
    if (IsTextureFormatOk(D3DFMT_A8R8G8B8, mode.Format))
        m_TrueColorTexturePixelFormat = D3DFMT_A8R8G8B8;
    if (!(m_ProblemFlags & 8)) {
        if (IsTextureFormatOk(D3DFMT_DXT1, mode.Format)) {
            m_HardwareCaps |= 8;
            m_DXTTexturePixelFormat = D3DFMT_DXT1;
        }
        if (IsTextureFormatOk(D3DFMT_DXT3, mode.Format)) {
            m_HardwareCaps |= 8;
            m_DXTAlphaTexturePixelFormat = D3DFMT_DXT3;
        }
    }
    dbgPrintf("\nMaximum available texture memory ( AGP + VRAM ): %i MB",
        m_pD3DDevice8->GetAvailableTextureMem() >> 20);
    dbgPrintf("\nUsing texture formats:");
    dbgPrintf("   16bit textures: %s", GetFormatName(m_TexturePixelFormat));
    dbgPrintf("   1bit Alpha textures: %s", GetFormatName(m_Alpha1TexturePixelFormat));
    dbgPrintf("   Alpha textures: %s", GetFormatName(m_AlphaTexturePixelFormat));
    dbgPrintf("   TrueColor textures: %s", GetFormatName(m_TrueColorTexturePixelFormat > 0 ?
        m_TrueColorTexturePixelFormat : (D3DFORMAT)-1));
    dbgPrintf("   DXT1 textures: %s", GetFormatName(m_DXTTexturePixelFormat > 0 ? m_DXTTexturePixelFormat :
        (D3DFORMAT)-1));
    dbgPrintf("   DXT3 textures: %s", GetFormatName(m_DXTAlphaTexturePixelFormat > 0 ? m_DXTAlphaTexturePixelFormat :
        (D3DFORMAT)-1));
    dbgPrintf("\n");

    hr = DirectInput8Create(m_InitDesc.hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8A, (void **)&m_pDI8, NULL);
    if (FAILED(hr)) {
        DX_ERROR(hr, 2708);
        return I3DERR_GENERIC;
    }
    m_LeftHandedMouse = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    ResetRenderProps();
    if (m_HardwareCaps & 1) {
        if (!cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0x18,
                15000, m_VB, 0, 0))
            dbgPrintf("Error creating Global Vertex Buffer !");
        if (!cIBMan.m_Alloc(D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 45000, m_IB))
            dbgPrintf("Error creating Global Index Buffer !");
    }
    if (caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) {
        m_HardwareCaps |= 0x20;
        LS3DGAMMARAMP ramp;
        GetGammaRamp(&ramp);
        // Whether the card's ramp has 8 bits only (GetGammaRamp then widens it)
        m_HardwareCaps |= 0x40;
        for (i = 256; i--; ) {
            if (ramp.red[i] > 0xff || ramp.green[i] > 0xff || ramp.blue[i] > 0xff)
                m_HardwareCaps &= ~0x40;
        }
    }
    if (m_InitDesc.flags & IGRAPH_RENDER_THREAD) {
        RestoreManagedTextures(m_InitDesc.flags);
        m_RenderThreadRunning = true;
        m_RenderThread = CreateThread(NULL, 0, RenderThreadProc, NULL, 0, &m_RenderThreadID);
        SetThreadPriority(m_RenderThread, THREAD_PRIORITY_NORMAL);
    }
    Clear(0, 1.0f, 3);
    Present();
    Clear(0, 1.0f, 3);
    Present();
    m_InitIsOK = true;
    return I3D_OK;
}

// qsort's comparison of the display modes: by width, then height, then format descending
// FUNCTION: LS3DF 0x1006fdc0
static int __cdecl SortModesCallback(const void *a, const void *b)
{
    const D3DDISPLAYMODE *m1 = (const D3DDISPLAYMODE *)a, *m2 = (const D3DDISPLAYMODE *)b;
    if (m1->Width < m2->Width)
        return -1;
    if (m1->Width > m2->Width)
        return 1;
    if (m1->Height < m2->Height)
        return -1;
    if (m1->Height > m2->Height)
        return 1;
    if ((int)m1->Format > (int)m2->Format)
        return -1;
    return (int)m1->Format < (int)m2->Format;
}

// FUNCTION: LS3DF 0x10070180
bool IGraph::IsInit()
{
    return m_InitIsOK;
}

// FUNCTION: LS3DF 0x10070190
IGRAPH_INIT_DESC *IGraph::GetResInitDesc()
{
    return &m_InitDesc;
}

// Releases everything Init made, and the textures still alive (which it lists in the log)
// FUNCTION: LS3DF 0x100701a0
void IGraph::Close()
{
    ClipCursor(NULL);
    if ((m_InitDesc.flags & IGRAPH_RENDER_THREAD) && m_RenderThreadRunning) {
        m_RenderThreadRunning = false;
        WaitForSingleObject(m_RenderThread, INFINITE);
        CloseHandle(m_RenderThread);
    }
    m_LeftHandedMouse = false;
    if (m_VB.vb) {
        cVBMan.m_Free(m_VB);
        memset(&m_VB, 0, sizeof(m_VB));
    }
    if (m_IB.ib) {
        cIBMan.m_Free(m_IB);
        memset(&m_IB, 0, sizeof(m_IB));
    }
    cVBMan.m_FreeAllUnusedVBuffers(NULL);
    cIBMan.m_FreeAllUnusedIBuffers();
    cVBMan.m_FreeAllVBuffers();
    cIBMan.m_FreeAllIBuffers();
    ClearMapDirs();
    int i;
    if (m_CachedTextureList.size() > 0) {
        dbgPrintf("\nWARNING! Releasing unreleased CACHED textures:");
        for (i = m_CachedTextureList.size(); i--; ) {
            dbgPrintf("   %s, %s, Refs:%i", &m_CachedTextureList[i]->file_name[0],
                &m_CachedTextureList[i]->alpha_file_name[0], m_CachedTextureList[i]->ref);
            while (m_CachedTextureList[i]->Release());
        }
        m_CachedTextureList.clear();
    }
    if (m_UncachedTextureList.size() > 0) {
        dbgPrintf("\nWARNING! Releasing unreleased UNCACHED textures:");
        for (i = m_UncachedTextureList.size(); i--; ) {
            dbgPrintf("   %s, %s, Refs:%i", &m_UncachedTextureList[i]->file_name[0],
                &m_UncachedTextureList[i]->alpha_file_name[0], m_UncachedTextureList[i]->ref);
            while (m_UncachedTextureList[i]->Release());
        }
        m_UncachedTextureList.clear();
    }
    m_DXTTexturePixelFormat = D3DFMT_UNKNOWN;
    m_DXTAlphaTexturePixelFormat = D3DFMT_UNKNOWN;
    m_TexturePixelFormat = D3DFMT_UNKNOWN;
    m_AlphaTexturePixelFormat = D3DFMT_UNKNOWN;
    m_Alpha1TexturePixelFormat = D3DFMT_UNKNOWN;
    m_TrueColorTexturePixelFormat = D3DFMT_UNKNOWN;
    m_AdapterModeList.clear();
    m_AdapterFormatList.clear();
    KeyboardClose();
    MouseClose();
    memset(&m_D3DPresentParams, 0, sizeof(m_D3DPresentParams));
    memset(&m_D3DViewPort8, 0, sizeof(m_D3DViewPort8));
    m_HardwareCaps = 0;
    m_ProblemFlags = 0;
    m_Modulate2X = 4;
    m_MaxTextureWidth = 0;
    m_MaxTextureHeight = 0;
    if (m_pDI8) {
        m_pDI8->Release();
        m_pDI8 = NULL;
    }
    if (m_pD3DDevice8) {
        m_pD3DDevice8->DeleteStateBlock(m_StateBlock);
        m_StateBlock = 0;
        if (m_InitDesc.flags & IGRAPH_RENDER_THREAD) {
            TerminateThread(m_RenderThread, 0);
            ReleaseManagedTextures();
        }
        m_RenderThread = NULL;
        m_pD3DDevice8->Release();
        m_pD3DDevice8 = NULL;
    }
    if (m_pD3D8) {
        m_pD3D8->Release();
        m_pD3D8 = NULL;
        CoUninitialize();
    }
    m_InitIsOK = false;
}

// FUNCTION: LS3DF 0x100704e0
int IGraph::Scrn_sx()
{
    return m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_InitDesc.child_rect.right : m_InitDesc.width;
}

// FUNCTION: LS3DF 0x10070500
int IGraph::Scrn_sy()
{
    return m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_InitDesc.child_rect.bottom : m_InitDesc.height;
}

// FUNCTION: LS3DF 0x10070520
int IGraph::BPP()
{
    return m_InitDesc.bpp;
}

// FUNCTION: LS3DF 0x10070530
D3DFORMAT IGraph::GetD3DFormat()
{
    return m_D3DPresentParams.BackBufferFormat;
}

// The image types GetImageInfo knows, by the extension's first letter (names guessed)
enum {
    IMAGE_BMP,
    IMAGE_TGA,
};

// Reads an image's size and depth from its header. With a handle the file stays open, at its start, for the caller to
// read. With flags & 1 nothing is read.
// FUNCTION: LS3DF 0x10070540
LS3D_RESULT IGraph::GetImageInfo(const char *name, ushort &sx, ushort &sy, uchar &bpp, uint flags, int *handle)
{
    if (flags & 1)
        return I3D_OK;
    const char *ext = strrchr(name, '.');
    if (!ext)
        return I3DERR_BADTEXTURE;
    int type;
    if (ext[1] == 'b' || ext[1] == 'B')
        type = IMAGE_BMP;
    else if (ext[1] == 't' || ext[1] == 'T')
        type = IMAGE_TGA;
    else
        return I3DERR_BADTEXTUREFORMAT;
    int h = dtaOpen(name, false);
    if (h == -1)
        return I3DERR_NOFILE;
    switch (type) {
    case IMAGE_BMP: {
        BITMAPFILEHEADER file_header;
        if ((int)dtaRead(h, &file_header, sizeof(file_header)) <= 0) {
            dtaClose(h);
            return I3DERR_FILESYSTEM;
        }
        BITMAPINFOHEADER info_header;
        if ((int)dtaRead(h, &info_header, sizeof(info_header)) <= 0) {
            dtaClose(h);
            return I3DERR_FILESYSTEM;
        }
        sx = (ushort)info_header.biWidth;
        sy = (ushort)info_header.biHeight;
        bpp = (uchar)info_header.biBitCount;
        break;
    }
    case IMAGE_TGA: {
        S_tga_header header;
        if ((int)dtaRead(h, &header, sizeof(header)) <= 0) {
            dtaClose(h);
            return I3DERR_FILESYSTEM;
        }
        sx = header.width;
        sy = header.height;
        bpp = header.bpp;
        break;
    }
    default:
        dbgPrintf("IGraph::GetImageInfo() - Unsupported file format %s !", name);
        dtaClose(h);
        return I3DERR_BADTEXTUREFORMAT;
    }
    if (handle) {
        *handle = h;
        dtaSeek(h, 0, FILE_BEGIN);
    } else
        dtaClose(h);
    return I3D_OK;
}

// The RLA file's numbers are big-endian (PC only, names guessed)
inline short Swap16(short x)
{
    return ((x & 0xff) << 8) | ((x >> 8) & 0xff);
}

inline uint Swap32(const int &x)
{
    return ((uint)x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24);
}

// A Wavefront RLA file's header (the format's own field names)
struct S_rla_header {
    struct {
        short left, right, bottom, top;
    } window, active_window;
    short frame;
    short storage_type;
    short num_chan;
    short num_matte;
    short num_aux;
    short revision;
    char gamma[16];
    char red_pri[24];
    char green_pri[24];
    char blue_pri[24];
    char white_pt[24];
    int job_num;
    char name[128];
    char desc[128];
    char program[64];
    char machine[32];
    char user[32];
    char date[20];
    char aspect[24];
    char aspect_ratio[8];
    char chan[32];
    short field;
    char time[12];
    char filter[32];
    short chan_bits;
    short matte_type;
    short matte_bits;
    short aux_type;
    short aux_bits;
    char aux[32];
    char space[36];
    int next;
};

// Run-length encodes one byte of each pixel, (pixel & mask) >> shift, as an RLA channel: a run of n (2 to 128) equal
// bytes as n - 1 and the byte, n (up to 128) other bytes as -n and the bytes. Returns the encoded length.
// (PC only, name guessed)
// FUNCTION: LS3DF 0x100706d0
short EncodeRLAChannel(const uint *src, uchar *dst, int count, uint mask, int shift)
{
    // The pointers are walked through copies, declared after len and left: with the parameters themselves, or in
    // another order, VC6 allocates other registers
    int len = 0;
    int left = count;
    const uint *in = src;
    uchar *out = dst;
    while (left > 0) {
        int n;
        if (left > 2 && ((in[0] & mask) >> shift) == ((in[1] & mask) >> shift) &&
                ((in[1] & mask) >> shift) == ((in[2] & mask) >> shift)) {
            for (n = 2; n < left; n++) {
                if (((in[n] & mask) >> shift) != ((in[n - 1] & mask) >> shift))
                    break;
                if (n >= 127)
                    break;
            }
            *out++ = n - 1;
            *out++ = (uchar)((in[0] & mask) >> shift);
            in += n;
            len += 2;
            left -= n;
        } else {
            for (n = 1; n < left; n++) {
                if (left - n > 2 && ((in[n] & mask) >> shift) == ((in[n + 1] & mask) >> shift) &&
                        ((in[n + 1] & mask) >> shift) == ((in[n + 2] & mask) >> shift))
                    break;
                if (n >= 127)
                    break;
            }
            *out++ = -n;
            len++;
            for (int i = 0; i < n; i++) {
                *out++ = (uchar)((*in++ & mask) >> shift);
                len++;
                left--;
            }
        }
    }
    return len;
}

// Saves the frame (or the part rect's right and bottom give) as a TGA file, as deep as the back buffer is. With flags
// 0x100000 it saves the depth instead, as an RLA file of the same name (.rla): the colors and the depth as floats. With
// flags 1 nothing is written to a file: the frame is copied bottom-up to the memory name points to (as 24 bits with
// 0x20000) and with 0x100000 the depth to z_buffer.
// FUNCTION: LS3DF 0x10070890
bool IGraph::SaveShot(const char *name, RECT *rect, uint flags, void *z_buffer)
{
    if (!(flags & 1) && !strstr(name, ".TGA") && !strstr(name, ".tga"))
        dbgPrintf("!!IGraph::SaveShot() - WARNING! File extension isn't .TGA !! %s", name);
    IDirect3DSurface8 *render_target = NULL;
    IDirect3DSurface8 *surface = NULL;
    IDirect3DSurface8 *depth = NULL;
    if (FAILED(m_pD3DDevice8->GetRenderTarget(&render_target)))
        return false;
    D3DSURFACE_DESC desc;
    if (FAILED(render_target->GetDesc(&desc))) {
        render_target->Release();
        return false;
    }
    if (!rect) {
        m_pD3DDevice8->CreateImageSurface(desc.Width, desc.Height, desc.Format, &surface);
        if (FAILED(m_pD3DDevice8->CopyRects(render_target, NULL, 0, surface, NULL))) {
            render_target->Release();
            return false;
        }
    } else {
        desc.Width = rect->right;
        desc.Height = rect->bottom;
        desc.Size = 0;
        m_pD3DDevice8->CreateImageSurface(desc.Width, desc.Height, desc.Format, &surface);
        POINT pt;
        pt.x = 0;
        pt.y = 0;
        if (FAILED(m_pD3DDevice8->CopyRects(render_target, rect, 1, surface, &pt))) {
            render_target->Release();
            return false;
        }
    }
    D3DLOCKED_RECT locked;
    if (FAILED(surface->LockRect(&locked, NULL, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK))) {
        render_target->Release();
        surface->Release();
        return false;
    }
    D3DLOCKED_RECT depth_locked;
    if ((flags & 0x100000) && !(flags & 1)) {
        m_pD3DDevice8->GetDepthStencilSurface(&depth);
        if (FAILED(depth->LockRect(&depth_locked, rect, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK))) {
            dbgPrintf("!!IGraph::SaveShot() - ERROR! Z-buffer pixel format isn't LOCKABLE !!");
            return false;
        }
        char file_name[250];
        for (int i = strlen(name) - 1; i >= 0; i--) {
            if (name[i] == '.') {
                strncpy(file_name, name, i + 1);
                file_name[i + 1] = 'r';
                file_name[i + 2] = 'l';
                file_name[i + 3] = 'a';
                file_name[i + 4] = 0;
                break;
            }
        }
        int sx, sy;
        if (!rect) {
            sx = Scrn_sx();
            sy = Scrn_sy();
        } else {
            sx = rect->right;
            sy = rect->bottom;
        }
        FILE *f = fopen(file_name, "wb");
        if (!f) {
            depth->UnlockRect();
            depth->Release();
            dbgPrintf("IGraph::SaveShot() - Cannot create file %s !!!", file_name);
            return false;
        }
        S_rla_header header;
        memset(&header, 0, sizeof(header));
        header.frame = Swap16(1);
        header.num_chan = Swap16(3);
        header.chan_bits = Swap16(8);
        header.revision = Swap16(-2);
        header.window.right = Swap16(sx - 1);
        header.active_window.right = header.window.right;
        header.window.top = Swap16(sy - 1);
        header.active_window.top = header.window.top;
        strcpy(header.gamma, "2.2");
        strcpy(header.red_pri, "0.670 0.330");
        strcpy(header.green_pri, "0.210 0.710");
        strcpy(header.blue_pri, "0.140 0.080");
        strcpy(header.white_pt, "0.310 0.316");
        strcpy(header.program, "LS3D engine ( Z )");
        strncpy(header.name, file_name, 127);
        strcpy(header.desc, "24bit RGB + 16bit Z-channel");
        strcpy(header.machine, "unknown");
        strcpy(header.user, "unknown");
        char date[16], time[16];
        _strdate(date);
        _strtime(time);
        sprintf(header.date, "%s  %s", date, time);
        sprintf(header.aspect, "%0.3f", (float)sx / sy);
        strcpy(header.chan, "rgb");
        fwrite(&header, sizeof(header), 1, f);
        int *offsets = new int[sy];
        if (!offsets) {
            depth->UnlockRect();
            depth->Release();
            dbgPrintf("IGraph::SaveShot() - Error saving depth !");
            fclose(f);
            return false;
        }
        fwrite(offsets, sizeof(int), sy, f);
        uchar *rle = new uchar[sx * 3];
        float *z = new float[sx];
        uint *colors = new uint[sx];
        locked.pBits = (BYTE *)locked.pBits + (sy - 1) * locked.Pitch;
        depth_locked.pBits = (BYTE *)depth_locked.pBits + (sy - 1) * depth_locked.Pitch;
        uint *pixels;
        if (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8) {
            pixels = colors;
            dbgPrintf("!!IGraph::SaveShot() - WARNING! Converting poor quality 16bit frame buffer to 32bit RLA !");
        }
        for (int y = 0; y < sy; y++) {
            int pos = ftell(f);
            offsets[y] = Swap32(pos);
            int x;
            switch (desc.Format) {
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
                for (x = 0; x < sx; x++) {
                    ushort p = ((ushort *)locked.pBits)[x];
                    ((uchar *)colors)[x * 4 + 2] = (p >> 7) & 0xf8;
                    ((uchar *)colors)[x * 4 + 1] = (p >> 2) & 0xf8;
                    ((uchar *)colors)[x * 4] = p << 3;
                }
                break;
            default:
                pixels = (uint *)locked.pBits;
                break;
            case D3DFMT_R5G6B5:
                for (x = 0; x < sx; x++) {
                    ushort p = ((ushort *)locked.pBits)[x];
                    ((uchar *)colors)[x * 4 + 2] = (p >> 8) & 0xf8;
                    ((uchar *)colors)[x * 4 + 1] = (p >> 3) & 0xfc;
                    ((uchar *)colors)[x * 4] = p << 3;
                }
                break;
            }
            short len;
            short len_be;
            len = EncodeRLAChannel(pixels, rle, sx, 0xff0000, 16);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            len = EncodeRLAChannel(pixels, rle, sx, 0xff00, 8);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            len = EncodeRLAChannel(pixels, rle, sx, 0xff, 0);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            for (x = 0; x < sx; x++) {
                float d = 1.0f - ((ushort *)depth_locked.pBits)[x] * (1.0f / 65535.0f);
                if (renderState[4])
                    d = 1.0f / d;
                z[x] = d * 10000.0f;
            }
            len = EncodeRLAChannel((uint *)z, rle, sx, 0xff000000, 24);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            len = EncodeRLAChannel((uint *)z, rle, sx, 0xff0000, 16);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            len = EncodeRLAChannel((uint *)z, rle, sx, 0xff00, 8);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            len = EncodeRLAChannel((uint *)z, rle, sx, 0xff, 0);
            len_be = Swap16(len);
            fwrite(&len_be, 2, 1, f);
            fwrite(rle, len, 1, f);
            locked.pBits = (BYTE *)locked.pBits - locked.Pitch;
            depth_locked.pBits = (BYTE *)depth_locked.pBits - depth_locked.Pitch;
        }
        fseek(f, sizeof(header), SEEK_SET);
        fwrite(offsets, sizeof(int), sy, f);
        fclose(f);
        delete[] offsets;
        delete[] rle;
        delete[] z;
        delete[] colors;
        depth->UnlockRect();
        depth->Release();
    } else {
        S_tga_header header;
        memset(&header, 0, sizeof(header));
        header.image_type = 2;
        switch (desc.Format) {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            header.bpp = 16;
            break;
        case D3DFMT_R8G8B8:
            header.bpp = 24;
            break;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            header.bpp = 32;
            break;
        }
        header.width = (short)desc.Width;
        header.height = (short)desc.Height;
        if ((flags & 1)) {
            if ((flags & 0x100000) && z_buffer) {
                m_pD3DDevice8->GetDepthStencilSurface(&depth);
                if (FAILED(depth->LockRect(&depth_locked, rect, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK))) {
                    dbgPrintf("!!IGraph::SaveShot() - ERROR! Z-buffer pixel format isn't LOCKABLE !!");
                    return false;
                }
                depth_locked.pBits = (BYTE *)depth_locked.pBits + (header.height - 1) * depth_locked.Pitch;
                for (int y = header.height; y; y--) {
                    memcpy(z_buffer, depth_locked.pBits, header.width * 2);
                    z_buffer = (uchar *)z_buffer + header.width * 2;
                    depth_locked.pBits = (BYTE *)depth_locked.pBits - depth_locked.Pitch;
                }
                depth->UnlockRect();
                depth->Release();
            }
            locked.pBits = (BYTE *)locked.pBits + (header.height - 1) * locked.Pitch;
            if ((flags & 0x20000) && desc.Format != D3DFMT_R8G8B8) {
                for (int y = header.height; y; y--) {
                    int x;
                    switch (desc.Format) {
                    case D3DFMT_R5G6B5:
                        for (x = 0; x < header.width; x++) {
                            ushort p = ((ushort *)locked.pBits)[x];
                            ((uchar *)name)[x * 3 + 2] = (p >> 8) & 0xf8;
                            ((uchar *)name)[x * 3 + 1] = (p >> 3) & 0xfc;
                            ((uchar *)name)[x * 3] = p << 3;
                        }
                        break;
                    case D3DFMT_X1R5G5B5:
                    case D3DFMT_A1R5G5B5:
                        for (x = 0; x < header.width; x++) {
                            ushort p = ((ushort *)locked.pBits)[x];
                            ((uchar *)name)[x * 3 + 2] = (p >> 7) & 0xf8;
                            ((uchar *)name)[x * 3 + 1] = (p >> 2) & 0xf8;
                            ((uchar *)name)[x * 3] = p << 3;
                        }
                        break;
                    case D3DFMT_A8R8G8B8:
                    case D3DFMT_X8R8G8B8:
                        for (x = 0; x < header.width; x++) {
                            ((uchar *)name)[x * 3] = ((BYTE *)locked.pBits)[x * 4];
                            ((uchar *)name)[x * 3 + 1] = ((BYTE *)locked.pBits)[x * 4 + 1];
                            ((uchar *)name)[x * 3 + 2] = ((BYTE *)locked.pBits)[x * 4 + 2];
                        }
                        break;
                    }
                    name += header.width * 3;
                    locked.pBits = (BYTE *)locked.pBits - locked.Pitch;
                }
            } else {
                for (int y = header.height; y; y--) {
                    memcpy((void *)name, locked.pBits, header.bpp / 8 * header.width);
                    name += header.bpp / 8 * header.width;
                    locked.pBits = (BYTE *)locked.pBits - locked.Pitch;
                }
            }
        } else {
            FILE *f = fopen(name, "wb");
            if (!f) {
                render_target->Release();
                surface->Release();
                dbgPrintf("IGraph::SaveShot() - Cannot create file %s !!!", name);
                return false;
            }
            fwrite(&header, sizeof(header), 1, f);
            locked.pBits = (BYTE *)locked.pBits + (header.height - 1) * locked.Pitch;
            if (desc.Format == D3DFMT_R5G6B5) {
                for (int y = header.height; y; y--) {
                    for (int x = header.width; x--; ) {
                        ushort p = ((ushort *)locked.pBits)[x];
                        ((ushort *)locked.pBits)[x] = ((p >> 1) & 0x7fe0) | (p & 0x1f);
                    }
                    fwrite(locked.pBits, 1, header.bpp / 8 * header.width, f);
                    locked.pBits = (BYTE *)locked.pBits - locked.Pitch;
                }
            } else {
                for (int y = header.height; y; y--) {
                    fwrite(locked.pBits, 1, header.bpp / 8 * header.width, f);
                    locked.pBits = (BYTE *)locked.pBits - locked.Pitch;
                }
            }
            fclose(f);
        }
    }
    surface->UnlockRect();
    surface->Release();
    render_target->Release();
    return true;
}

// Flags 1: exclusive, 2: the keys come from the window messages (ProcessWinMessages)
// FUNCTION: LS3DF 0x100716a0
LS3D_RESULT IGraph::KeyboardInit(uint flags)
{
    KeyboardClose();
    m_KBInitFlags = flags;
    HRESULT hr = m_pDI8->CreateDevice(GUID_SysKeyboard, &m_pDIKeyboard8, NULL);
    if (FAILED(hr)) {
        DX_ERROR(hr, 3786);
        return I3DERR_GENERIC;
    }
    hr = m_pDIKeyboard8->SetDataFormat(&c_dfDIKeyboard);
    if (FAILED(hr)) {
        DX_ERROR(hr, 3789);
        return I3DERR_GENERIC;
    }
    if (m_KBInitFlags & 1)
        hr = m_pDIKeyboard8->SetCooperativeLevel(m_MainHWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    else
        hr = m_pDIKeyboard8->SetCooperativeLevel(m_MainHWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    if (FAILED(hr)) {
        DX_ERROR(hr, 3799);
        return I3DERR_GENERIC;
    }
    m_pDIKeyboard8->Acquire();
    m_KeyboardInitIsOK = true;
    UpdateKeyboardData();
    memcpy(m_OldKeyboardData, m_KeyboardData, sizeof(m_KeyboardData));
    m_KBLastKey = 0;
    m_KBLastChar1 = 0;
    m_KBLastChar = 0;
    ClearCharBuffer();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100717b0
bool IGraph::IsKeyboardInit()
{
    return m_KeyboardInitIsOK;
}

// FUNCTION: LS3DF 0x100717c0
void IGraph::KeyboardClose()
{
    if (m_pDIKeyboard8) {
        m_pDIKeyboard8->Unacquire();
        m_pDIKeyboard8->Release();
        m_pDIKeyboard8 = NULL;
    }
    m_KeyboardInitIsOK = false;
}

// Reads the keys, acquiring the keyboard again if it was lost. Ctrl+Alt+Shift+F12 switches the frame rate on or off
// (and F11 another switch) unless the game sets 0x10000.
// FUNCTION: LS3DF 0x100717f0
LS3D_RESULT IGraph::UpdateKeyboardData()
{
    if (!m_pDIKeyboard8)
        return I3DERR_NOTINITIALIZED;
    memcpy(m_OldKeyboardData, m_KeyboardData, sizeof(m_KeyboardData));
    memset(m_KeyboardData, 0, sizeof(m_KeyboardData));
    if (FAILED(m_pDIKeyboard8->GetDeviceState(sizeof(m_KeyboardData), m_KeyboardData))) {
        while (m_pDIKeyboard8->Acquire() == DIERR_INPUTLOST)
            ProcessWinMessages();
        if (FAILED(m_pDIKeyboard8->GetDeviceState(sizeof(m_KeyboardData), m_KeyboardData))) {
            memset(m_KeyboardData, 0, sizeof(m_KeyboardData));
            return I3D_OK;
        }
    } else if (!(m_InitDesc.flags & 0x10000) && (m_KeyboardData[DIK_LSHIFT] & 0x80) &&
            (m_KeyboardData[DIK_LMENU] & 0x80) && (m_KeyboardData[DIK_LCONTROL] & 0x80)) {
        if ((m_KeyboardData[DIK_F12] & 0x80) && !(m_OldKeyboardData[DIK_F12] & 0x80))
            m_ShowFPS = !m_ShowFPS;
        if ((m_KeyboardData[DIK_F11] & 0x80) && !(m_OldKeyboardData[DIK_F11] & 0x80))
            unknown_59cd = !unknown_59cd;
    }
    return I3D_OK;
}

// Takes the oldest character typed out of the buffer
// FUNCTION: LS3DF 0x10071910
uint IGraph::ReadCharBuffer()
{
    memmove(m_KeyBuffer, m_KeyBuffer + 1, sizeof(m_KeyBuffer) - sizeof(m_KeyBuffer[0]));
    m_KeyBuffer[31] = 0;
    return m_KeyBuffer[0];
}

// FUNCTION: LS3DF 0x10071940
void IGraph::ClearCharBuffer()
{
    memset(m_KeyBuffer, 0, sizeof(m_KeyBuffer));
}

// The key pressed since the last update (a DIK_* code), not counting shift, control and alt. ReadChar then gives
// its character.
// FUNCTION: LS3DF 0x10071960
uint IGraph::ReadKey()
{
    if (m_KBInitFlags & 2) {
        uint key = m_KBLastKey;
        m_KBLastKey = 0;
        m_KBLastChar = m_KBLastChar1;
        m_KBLastChar1 = 0;
        return key;
    }
    for (int i = 0; i < 256; i++) {
        if (!(m_KeyboardData[i] & 0x80) || m_OldKeyboardData[i])
            continue;
        if (i == DIK_LCONTROL || i == DIK_RCONTROL || i == DIK_LSHIFT || i == DIK_RSHIFT || i == DIK_LMENU ||
                i == DIK_RMENU)
            continue;
        m_KBLastChar = TranslateKey(i, GetShiftKeys(false) & 0xd);
        return i;
    }
    return 0;
}

// FUNCTION: LS3DF 0x10071a10
uint IGraph::ReadChar()
{
    uint c = m_KBLastChar;
    m_KBLastChar = 0;
    return c;
}

// FUNCTION: LS3DF 0x10071a30
bool IGraph::TestKey(uchar key)
{
    return m_KeyboardData[key] >> 7;
}

// The keys ReadKeys reports, at most 64
// FUNCTION: LS3DF 0x10071a50
LS3D_RESULT IGraph::DefineKeys(const uchar *keys, uint count)
{
    if (!keys || count > 64)
        return I3DERR_INVALIDPARAMS;
    memset(m_ReadKeysBuffer, 0, sizeof(m_ReadKeysBuffer));
    memcpy(m_ReadKeysBuffer, keys, count);
    return I3D_OK;
}

// A bit for each key DefineKeys gave that is down
// FUNCTION: LS3DF 0x10071aa0
ulong IGraph::ReadKeys()
{
    ulong keys = 0;
    for (int i = 0; i < 64; i++) {
        if (m_ReadKeysBuffer[i] > 0 && (m_KeyboardData[m_ReadKeysBuffer[i]] & 0x80))
            keys |= (ulong)1 << i;
    }
    return keys;
}

// FUNCTION: LS3DF 0x10071af0
bool IGraph::KeyShift()
{
    if ((m_KeyboardData[DIK_LSHIFT] & 0x80) || (m_KeyboardData[DIK_RSHIFT] & 0x80))
        return true;
    return false;
}

// FUNCTION: LS3DF 0x10071b10
bool IGraph::KeyCtrl()
{
    if ((m_KeyboardData[DIK_LCONTROL] & 0x80) || (m_KeyboardData[DIK_RCONTROL] & 0x80))
        return true;
    return false;
}

// FUNCTION: LS3DF 0x10071b30
bool IGraph::KeyAlt()
{
    if ((m_KeyboardData[DIK_LMENU] & 0x80) || (m_KeyboardData[DIK_RMENU] & 0x80))
        return true;
    return false;
}

// Shift 1, control 4, alt 8; left and right shift 0x20 and 0x40, control 0x80 and 0x100, alt 0x200 and 0x400
// FUNCTION: LS3DF 0x10071b50
uint IGraph::GetShiftKeys(bool update)
{
    if (update)
        UpdateKeyboardData();
    uint keys = 0;
    if (KeyShift()) {
        keys = 1;
        if (m_KeyboardData[DIK_LSHIFT] & 0x80)
            keys |= 0x20;
        if (m_KeyboardData[DIK_RSHIFT] & 0x80)
            keys |= 0x40;
    }
    if (KeyCtrl()) {
        keys |= 4;
        if (m_KeyboardData[DIK_LCONTROL] & 0x80)
            keys |= 0x80;
        if (m_KeyboardData[DIK_RCONTROL] & 0x80)
            keys |= 0x100;
    }
    if (KeyAlt()) {
        keys |= 8;
        if (m_KeyboardData[DIK_LMENU] & 0x80)
            keys |= 0x200;
        if (m_KeyboardData[DIK_RMENU] & 0x80)
            keys |= 0x400;
    }
    return keys;
}

// A key's character, shifted with flags 1
// FUNCTION: LS3DF 0x10071c00
uchar IGraph::TranslateKey(uchar key, uint flags)
{
    for (S_key_char *p = key_table; p->key; p++) {
        if (p->key == key)
            return flags & 1 ? p->shift_c : p->c;
    }
    return 0;
}

// The key's name from DirectInput, a wide string
// FUNCTION: LS3DF 0x10071c40
const char *IGraph::GetKeyName(int key)
{
    if (!m_pDIKeyboard8)
        return NULL;
    memset(&m_KeyName, 0, sizeof(m_KeyName));
    m_KeyName.diph.dwSize = sizeof(m_KeyName);
    m_KeyName.diph.dwHeaderSize = sizeof(m_KeyName.diph);
    m_KeyName.diph.dwObj = key;
    m_KeyName.diph.dwHow = DIPH_BYOFFSET;
    return m_pDIKeyboard8->GetProperty(DIPROP_KEYNAME, &m_KeyName.diph) ? NULL : (const char *)m_KeyName.wsz;
}

// Mode 0: the cursor, 1: relative movement (exclusive), -1: only tells the current mode. Switching from the cursor to
// relative movement waits for the buttons to be released.
// FUNCTION: LS3DF 0x10071ca0
LS3D_RESULT IGraph::MouseInit(int mode)
{
    if (mode == -1)
        return (LS3D_RESULT)m_MouseMode;
    if (m_pDIMouse8 && !m_MouseMode && mode == 1) {
        do {
            ProcessWinMessages();
            UpdateMouseData();
        } while ((m_MouseData.rgbButtons[0] & 0x80) || (m_MouseData.rgbButtons[1] & 0x80) ||
            (m_MouseData.rgbButtons[2] & 0x80));
    }
    MouseClose();
    HRESULT hr = m_pDI8->CreateDevice(GUID_SysMouse, &m_pDIMouse8, NULL);
    if (FAILED(hr)) {
        DX_ERROR(hr, 4451);
        return I3DERR_GENERIC;
    }
    m_MouseCaps.dwSize = sizeof(m_MouseCaps);
    m_pDIMouse8->GetCapabilities(&m_MouseCaps);
    hr = m_pDIMouse8->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hr)) {
        DX_ERROR(hr, 4460);
        return I3DERR_GENERIC;
    }
    if (!mode)
        hr = m_pDIMouse8->SetCooperativeLevel(m_MainHWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    else
        hr = m_pDIMouse8->SetCooperativeLevel(m_MainHWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if (FAILED(hr)) {
        DX_ERROR(hr, 4470);
        return I3DERR_GENERIC;
    }
    m_pDIMouse8->Acquire();
    m_MouseMode = mode;
    m_MouseInitIsOK = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10071dc0
bool IGraph::IsMouseInit()
{
    return m_MouseInitIsOK;
}

// FUNCTION: LS3DF 0x10071dd0
void IGraph::MouseClose()
{
    if (m_pDIMouse8) {
        m_pDIMouse8->Unacquire();
        m_pDIMouse8->Release();
        m_pDIMouse8 = NULL;
    }
    // Before the memset: VC6 schedules the mode's store among the memset's
    m_MouseMode = -1;
    memset(&m_MouseData, 0, sizeof(m_MouseData));
    m_MouseInitIsOK = false;
}

// Reads the mouse. With the cursor, the window's position is computed but not used.
// FUNCTION: LS3DF 0x10071e20
LS3D_RESULT IGraph::UpdateMouseData()
{
    if (!m_pDIMouse8)
        return I3DERR_NOTINITIALIZED;
    if (FAILED(m_pDIMouse8->GetDeviceState(sizeof(m_MouseData), &m_MouseData))) {
        m_pDIMouse8->Acquire();
        if (FAILED(m_pDIMouse8->GetDeviceState(sizeof(m_MouseData), &m_MouseData))) {
            memset(&m_MouseData, 0, sizeof(m_MouseData));
            return I3DERR_GENERIC;
        }
    }
    if (m_MouseCaps.dwAxes < 3)
        m_MouseData.lZ = 0;
    if (!m_MouseMode) {
        POINT pt;
        GetCursorPos(&pt);
        RECT rc;
        if (m_InitDesc.flags & IGRAPH_CHILD_WINDOW)
            GetWindowRect(m_ChildHWnd, &rc);
        else if (m_InitDesc.flags & IGRAPH_FULLSCREEN)
            SetRect(&rc, 0, 0, Scrn_sx(), Scrn_sy());
        else {
            GetWindowRect(m_MainHWnd, &rc);
            int frame_x = GetSystemMetrics(SM_CXFIXEDFRAME);
            int frame_y = GetSystemMetrics(SM_CYFIXEDFRAME);
            int caption = GetSystemMetrics(SM_CYCAPTION);
            int menu = m_InitDesc.flags & IGRAPH_MENU ? GetSystemMetrics(SM_CYMENU) : 0;
        }
    } else {
        m_MousePos.x += m_MouseData.lX;
        m_MousePos.y += m_MouseData.lY;
    }
    if (m_LeftHandedMouse) {
        BYTE b = m_MouseData.rgbButtons[0];
        m_MouseData.rgbButtons[0] = m_MouseData.rgbButtons[1];
        m_MouseData.rgbButtons[1] = b;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10071f90
int IGraph::Mouse_rx()
{
    return m_MouseData.lX;
}

// FUNCTION: LS3DF 0x10071fa0
int IGraph::Mouse_ry()
{
    return m_MouseData.lY;
}

// FUNCTION: LS3DF 0x10071fb0
int IGraph::Mouse_rz()
{
    return m_MouseData.lZ;
}

// The position is the client area's; in a window the cursor goes to the screen position
// FUNCTION: LS3DF 0x10071fc0
void IGraph::SetMousePos(int x, int y)
{
    m_MousePos.x = x;
    m_MousePos.y = y;
    if (!(m_InitDesc.flags & IGRAPH_FULLSCREEN)) {
        RECT rc;
        if (m_InitDesc.flags & IGRAPH_CHILD_WINDOW)
            GetWindowRect(m_ChildHWnd, &rc);
        else {
            GetWindowRect(m_MainHWnd, &rc);
            int frame_x = GetSystemMetrics(SM_CXFIXEDFRAME);
            int frame_y = GetSystemMetrics(SM_CYFIXEDFRAME);
            int caption = GetSystemMetrics(SM_CYCAPTION);
            int menu = m_InitDesc.flags & IGRAPH_MENU ? GetSystemMetrics(SM_CYMENU) : 0;
            rc.left += frame_x;
            rc.top += menu + caption + frame_y;
        }
        m_MousePos.x += rc.left;
        m_MousePos.y += rc.top;
    }
    if (!m_MouseMode)
        SetCursorPos(m_MousePos.x, m_MousePos.y);
}

// FUNCTION: LS3DF 0x100720a0
int IGraph::Mouse_x()
{
    return m_MousePos.x;
}

// FUNCTION: LS3DF 0x100720b0
int IGraph::Mouse_y()
{
    return m_MousePos.y;
}

// Left 1, right 2, middle 4
// FUNCTION: LS3DF 0x100720c0
uint IGraph::GetMouseButtons()
{
    uint buttons = 0;
    if (m_MouseData.rgbButtons[0] & 0x80)
        buttons = 1;
    if (m_MouseData.rgbButtons[2] & 0x80)
        buttons |= 4;
    if (m_MouseData.rgbButtons[1] & 0x80)
        buttons |= 2;
    return buttons;
}

// FUNCTION: LS3DF 0x100720f0
void IGraph::NullTimer()
{
    m_InternalTimer = timeGetTime();
}

// The time since the last call, at least min_time (it sleeps for the rest) and at most max_time if that is set
// FUNCTION: LS3DF 0x10072100
uint IGraph::GetTimer(uint min_time, uint max_time)
{
    uint last = m_InternalTimer;
    NullTimer();
    uint time = m_InternalTimer - last;
    if (time < min_time) {
        uint rest = min_time - time;
        Sleep(rest);
        m_InternalTimer += rest;
        return min_time;
    }
    if (max_time > 0 && time > max_time)
        time = max_time;
    return time;
}

// The maps directory the last map was found in (PC only, names guessed)
// GLOBAL: LS3DF 0x101c59d8
char m_LastMapDir[256] = {0};
// GLOBAL: LS3DF 0x101c5ad8
bool m_LastMapDirOK = false;

// Finds a map file: as named if the name has a path, else in the directory the last one was found in, else in the
// maps directories, the last added first. Returns the full name, or an empty string. (PC only, name guessed)
// FUNCTION: LS3DF 0x10072150
C_str FindMapFile(C_list<char *> *list, const char *name)
{
    WIN32_FIND_DATA fd;
    if (strchr(name, '\\')) {
        HANDLE h = FindFirstFile(name, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            return name;
        }
    }
    char path[256];
    if (m_LastMapDirOK) {
        strcpy(path, m_LastMapDir);
        strcat(path, name);
        HANDLE h = FindFirstFile(path, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            return path;
        }
    }
    for (int i = list->size(); i--; ) {
        const char *dir = (*list)[i];
        strcpy(path, dir);
        strcat(path, name);
        HANDLE h = FindFirstFile(path, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            strcpy(m_LastMapDir, dir);
            m_LastMapDirOK = true;
            return path;
        }
    }
    m_LastMapDirOK = false;
    return NULL;
}

// Adds a directory to the list, with a backslash at its end, and with recursive its subdirectories too
// FUNCTION: LS3DF 0x100723e0
LS3D_RESULT m_AddNameToList(C_list<char *> *list, const char *name, bool recursive)
{
    int len = strlen(name);
    char *s = new char[len + 2];
    if (!s) {
        dbgPrintf(" !! Out of memory !! \tI3D_driver::m_AddNameToList()");
        return I3DERR_OUTOFMEM;
    }
    strcpy(s, name);
    if (name[len - 1] != '\\') {
        s[len] = '\\';
        s[len + 1] = 0;
    }
    list->push_back(s);
    if (recursive) {
        char path[256];
        WIN32_FIND_DATA fd;
        strcpy(path, name);
        strcat(path, "\\*.*");
        HANDLE h = FindFirstFile(path, &fd);
        if (h == INVALID_HANDLE_VALUE) {
            dbgPrintf("Could not open current directory %s  \tI3D_driver::m_AddNameToList()", name);
            return I3D_OK;
        }
        do {
            if (fd.cFileName[0] != '.' && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                strcpy(path, name);
                strcat(path, "\\");
                strcat(path, fd.cFileName);
                m_AddNameToList(list, path, true);
            }
        } while (FindNextFile(h, &fd));
        FindClose(h);
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100725f0
LS3D_RESULT IGraph::AddMapsDir(const char *dir, bool unknown)
{
    return m_AddNameToList(&m_MapsList, dir, unknown);
}

// Removes every directory whose name contains dir
// FUNCTION: LS3DF 0x10072610
LS3D_RESULT IGraph::DeleteMapsDir(const char *dir)
{
    for (int i = m_MapsList.size(); i; ) {
        char *name = m_MapsList[--i];
        if (strstr(name, dir)) {
            delete[] name;
            m_MapsList.erase(i);
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10072670
void IGraph::ClearMapDirs()
{
    for (int i = m_MapsList.size(); i; )
        delete[] m_MapsList[--i];
    m_MapsList.clear();
}

// FUNCTION: LS3DF 0x100726d0
ITexture **IGraph::GetCachedTextures()
{
    return &m_CachedTextureList[0];
}

// FUNCTION: LS3DF 0x100726e0
int IGraph::GetCachedTextureCnt()
{
    return m_CachedTextureList.size();
}

// FUNCTION: LS3DF 0x100726f0
ITexture **IGraph::GetUncachedTextures()
{
    return &m_UncachedTextureList[0];
}

// FUNCTION: LS3DF 0x10072700
int IGraph::GetUncachedTextureCnt()
{
    return m_UncachedTextureList.size();
}

// FUNCTION: LS3DF 0x10072710
bool IGraph::SetGammaRamp(LS3DGAMMARAMP *ramp, uint flags)
{
    if (!(m_HardwareCaps & 0x20))
        return false;
    m_pD3DDevice8->SetGammaRamp(flags != 0, (const D3DGAMMARAMP *)ramp);
    return true;
}

// FUNCTION: LS3DF 0x10072750
bool IGraph::GetGammaRamp(LS3DGAMMARAMP *ramp)
{
    if (!(m_HardwareCaps & 0x20))
        return false;
    m_pD3DDevice8->GetGammaRamp((D3DGAMMARAMP *)ramp);
    if (m_HardwareCaps & 0x40) {
        for (int i = 256; i--; ) {
            ramp->red[i] |= ramp->red[i] << 8;
            ramp->green[i] |= ramp->green[i] << 8;
            ramp->blue[i] |= ramp->blue[i] << 8;
        }
    }
    return true;
}

// Scales a 24- or 32-bit image to dst_sx * dst_sy into *dst (allocated if NULL). With filter, a smaller image averages
// the area each pixel covers (an empty area gives magenta); otherwise, and when enlarging, the nearest pixel is taken.
// The interior of the area advances its offset twice per pixel, as the original does.
// FUNCTION: LS3DF 0x100727d0
bool IGraph::ResizeImage(int sx, int sy, uint pitch, uchar *src, int dst_sx, int dst_sy, uint dst_pitch, uchar **dst,
    uint bpp, int filter)
{
    if (bpp != 4 && bpp != 3)
        dbgPrintf("IGraph::ResizeImage() - ERROR!! Stride must be 3 ( 24bit image ) or 4 ( 32bit image ) !!");
    if (!*dst) {
        *dst = new uchar[dst_sx * dst_sy * bpp];
        if (!*dst)
            return false;
    }
    uchar *d = *dst;
    if (filter && sx >= dst_sx && sy >= dst_sy) {
        float x_ratio = (float)(sx - 1) / dst_sx;
        float y_ratio = (float)(sy - 1) / dst_sy;
        for (int y = 0; y < dst_sy; y++) {
            for (int x = 0; x < dst_sx; x++) {
                float fy0 = y * y_ratio;
                float fy1 = (y + 1) * y_ratio;
                int iy0 = (int)fy0 + 1;
                int iy1 = (int)fy1;
                float r = 0, g = 0, b = 0, weight = 0;
                float a = 0;
                float wy0, wy1;
                if (iy0 > iy1) {
                    wy0 = fy1 - fy0;
                    wy1 = 0;
                } else {
                    wy0 = iy0 - fy0;
                    wy1 = fy1 - iy1;
                }
                int rows = max(0, iy1 - iy0);
                float fx0 = x * x_ratio;
                float fx1 = (x + 1) * x_ratio;
                int ix0 = (int)fx0 + 1;
                int ix1 = (int)fx1;
                float wx0, wx1;
                if (ix0 > ix1) {
                    wx0 = fx1 - fx0;
                    wx1 = 0;
                } else {
                    wx0 = ix0 - fx0;
                    wx1 = fx1 - ix1;
                }
                int cols = max(0, ix1 - ix0);
                uint pos;
                int i, j;
                // the row above the whole rows
                if (wy0 > MRG_ZERO) {
                    pos = (iy0 - 1) * pitch + (ix0 - 1) * bpp;
                    if (wx0 > MRG_ZERO) {
                        r = src[pos] * wx0 * wy0;
                        g = src[pos + 1] * wx0 * wy0;
                        b = src[pos + 2] * wx0 * wy0;
                        if (bpp == 4)
                            a = src[pos + 3] * wx0 * wy0;
                        weight = wx0 * wy0;
                    }
                    if (cols > 0) {
                        for (i = 1; i <= cols; i++) {
                            r += src[pos + i * bpp] * wy0;
                            g += src[pos + i * bpp + 1] * wy0;
                            b += src[pos + i * bpp + 2] * wy0;
                            if (bpp == 4)
                                a += src[pos + i * bpp + 3] * wy0;
                        }
                        weight += cols * wy0;
                    }
                    if (wx1 > MRG_ZERO) {
                        r += src[pos + (cols + 1) * bpp] * wx1 * wy0;
                        g += src[pos + (cols + 1) * bpp + 1] * wx1 * wy0;
                        b += src[pos + (cols + 1) * bpp + 2] * wx1 * wy0;
                        if (bpp == 4)
                            a += src[pos + (cols + 1) * bpp + 3] * wx1 * wy0;
                        weight += wx1 * wy0;
                    }
                }
                // the whole rows
                if (rows > 0) {
                    if (wx0 > MRG_ZERO) {
                        for (j = 0; j < rows; j++) {
                            pos = (iy0 + j) * pitch + (ix0 - 1) * bpp;
                            r += src[pos] * wx0;
                            g += src[pos + 1] * wx0;
                            b += src[pos + 2] * wx0;
                            if (bpp == 4)
                                a += src[pos + 3] * wx0;
                        }
                        weight += rows * wx0;
                    }
                    if (cols > 0) {
                        for (j = 0; j < rows; j++) {
                            for (i = 0; i < cols; i++) {
                                pos = (iy0 + j) * pitch + (ix0 + i) * bpp;
                                r += src[pos + i * bpp];
                                g += src[pos + i * bpp + 1];
                                b += src[pos + i * bpp + 2];
                                if (bpp == 4)
                                    a += src[pos + i * bpp + 3];
                            }
                        }
                        weight += cols * rows;
                    }
                    if (wx1 > MRG_ZERO) {
                        for (j = 0; j < rows; j++) {
                            pos = (iy0 + j) * pitch + (ix0 + cols) * bpp;
                            r += src[pos] * wx1;
                            g += src[pos + 1] * wx1;
                            b += src[pos + 2] * wx1;
                            if (bpp == 4)
                                a += src[pos + 3] * wx1;
                        }
                        weight += rows * wx1;
                    }
                }
                // the row below them
                if (wy1 > MRG_ZERO) {
                    pos = (iy0 + rows) * pitch + (ix0 - 1) * bpp;
                    if (wx0 > MRG_ZERO) {
                        r += src[pos] * wx0 * wy1;
                        g += src[pos + 1] * wx0 * wy1;
                        b += src[pos + 2] * wx0 * wy1;
                        if (bpp == 4)
                            a += src[pos + 3] * wx0 * wy1;
                        weight += wx0 * wy1;
                    }
                    if (cols > 0) {
                        for (i = 1; i <= cols; i++) {
                            r += src[pos + i * bpp] * wy1;
                            g += src[pos + i * bpp + 1] * wy1;
                            b += src[pos + i * bpp + 2] * wy1;
                            if (bpp == 4)
                                a += src[pos + i * bpp + 3] * wy1;
                        }
                        weight += cols * wy1;
                    }
                    if (wx1 > MRG_ZERO) {
                        r += src[pos + (cols + 1) * bpp] * wx1 * wy1;
                        g += src[pos + (cols + 1) * bpp + 1] * wx1 * wy1;
                        b += src[pos + (cols + 1) * bpp + 2] * wx1 * wy1;
                        if (bpp == 4)
                            a += src[pos + (cols + 1) * bpp + 3] * wx1 * wy1;
                        weight += wx1 * wy1;
                    }
                }
                if (weight <= MRG_ZERO) {
                    r = 255;
                    g = 0;
                    b = 255;
                    a = 255;
                } else {
                    float k = 1.0f / weight;
                    r *= k;
                    g *= k;
                    b *= k;
                    a *= k;
                }
                d[0] = FloatToInt(r);
                d[1] = FloatToInt(g);
                d[2] = FloatToInt(b);
                if (bpp == 4)
                    d[3] = FloatToInt(a);
                d += bpp;
            }
            d += dst_pitch - dst_sx * bpp;
        }
    } else {
        float x_ratio = (float)sx / dst_sx;
        float y_ratio = (float)sy / dst_sy;
        for (int y = 0; y < dst_sy; y++) {
            for (int x = 0; x < dst_sx; x++) {
                memcpy(d, src + (int)(y * y_ratio) * pitch + (int)(x * x_ratio) * bpp, bpp);
                d += bpp;
            }
            d += dst_pitch - dst_sx * bpp;
        }
    }
    return true;
}
