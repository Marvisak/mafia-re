// Movies (PS2 IShow.cpp), at 0x100733a0-0x10075890 in the original: Bink videos, AVI files through DirectShow and the
// DirectX SDK's texture renderer, other files through DirectShow's own rendering, and, PC only, Ogg Vorbis files,
// decoded on a thread of their own into a DirectSound buffer. A movie is split into textures no larger than the card
// allows (the regions), which RenderCurrentFrame draws.
// Unlike the engine's other files, this one is compiled without /Gr: its helpers are __cdecl, and so are vorbisfile's
// functions and callbacks, which libvorbis' header declares without a calling convention.
// The file ends with the texture renderer's adjustor thunks, the header's zero vector's $E (0x10075800), IShow's inline
// functions and the base classes' IUnknown members. Under /YX (see tools/autopch.py) that takes the precompiled headers
// in this order: D3DX's inline math before the zero vector, which puts its $E after the whole file (the texture
// sample's Textures.h includes d3dx8.h; nothing here uses it), and the base classes after IGraph.h, which puts their
// members after IShow's. Without d3dx8.h the $E comes after m_PrepareVerts; with <streams.h> first the base classes'
// members come before the thunks.
#include <d3dx8.h>

#include "IGraph/IGraph.h"
#include "IGraph/bink.h"
// binkw32.lib is a default library: the original's import directory lists it after rw_data, which dta.h asks for, and
// RAD's bink.h asks for nothing
#pragma comment(lib, "binkw32.lib")
#include "IGraph/vorbisfile.h"
#include "ISound/ISound.h"
#include "dta.h"

#include <streams.h>

// IGraph.cpp
extern HWND m_ChildHWnd;
extern HWND m_MainHWnd;
extern uint m_CurrColorOp;
extern uint m_MaxTextureHeight;
extern uint m_CurrAlphaState;
extern uint m_CurrFogState;
extern uint m_CurrClipState;
extern uint m_MaxTextureWidth;
extern uint m_CurrColorOp1;
extern uint m_CurrAlphaOp;
extern IDirect3DDevice8 *m_pD3DDevice8;
extern D3DFORMAT m_TrueColorTexturePixelFormat;
extern D3DFORMAT m_TexturePixelFormat;

// I3d_driver.cpp
extern uint ZBufferEnable;

// The DirectX 8.1 SDK's texture renderer sample (samples\Multimedia\DirectShow\Players\Texture3D, DShowTextures.h),
// which renders an AVI file's frames into the regions' textures instead of the sample's one texture. The size is
// unsigned, unlike the sample's (IShow divides by it as unsigned); the members after it are PC additions with guessed
// names in the sample's m_ style. It derives from the base classes in ls3df/BaseClasses/: VC6's own STRMBASE.LIB
// (1997) is another build of them, with other vtables and no EH frame. The base classes' empty OnReceiveFirstSample and
// PrepareRender are folded with other empty functions: the original's slots point at the scene file's Update
// (0x1004e970) and at a copy among the base classes' functions (0x10088970).
struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_TextureRenderer;

// VTABLE: LS3DF 0x1009d788 CUnknown
// VTABLE: LS3DF11 0x1009a788 CUnknown
// VTABLE: LS3DF 0x1009d748 IBaseFilter
// VTABLE: LS3DF11 0x1009a748 IBaseFilter
// VTABLE: LS3DF 0x1009d734 IAMovieSetup
// VTABLE: LS3DF11 0x1009a734 IAMovieSetup
// VTABLE: LS3DF 0x1009d710 IQualProp
// VTABLE: LS3DF11 0x1009a710 IQualProp
// VTABLE: LS3DF 0x1009d6fc IQualityControl
// VTABLE: LS3DF11 0x1009a6fc IQualityControl
class CTextureRenderer : public CBaseVideoRenderer {
public:
    CTextureRenderer(LPUNKNOWN pUnk, HRESULT *phr);
    ~CTextureRenderer();

public:
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);

    uint m_lVidWidth;                   // 0x160
    uint m_lVidHeight;                  // 0x164
    uint m_lVidPitch;                   // 0x168
    IBaseFilter *m_pSoundRenderer;      // 0x16c, DirectShow's, for the file's sound
    IShow *m_pShow;                     // 0x170
    D3DFORMAT m_TextureFormat;          // 0x174, the regions' textures', from CreateRegions
};

// The Ogg Vorbis sound buffer: 11 notifications, one every 0xc000 bytes
#define OGG_BUFFER_SIZE 0x84000
#define OGG_NOTIFY_COUNT 11

DWORD WINAPI OggThreadProc(void *param);
static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource);
static int seek_func(void *datasource, ogg_int64_t offset, int whence);
static int close_func(void *datasource);
static long tell_func(void *datasource);

// FUNCTION: LS3DF 0x100733a0
// FUNCTION: LS3DF11 0x10070430
IShow::IShow() : m_RegionList(1)
{
    m_RefCount = 1;
    m_Stream = -1;
    m_DtaHandle = -1;
    m_Init = false;
    m_pGraphBuilder = NULL;
    m_pMediaControl = NULL;
    m_pMediaSeeking = NULL;
    m_pBasicAudio = NULL;
    m_pMediaEvent = NULL;
    m_pBink = NULL;
    m_StreamInit = false;
    m_StreamPaused = false;
    m_StreamDone = false;
    m_ThreadID = 0;
    m_Thread = NULL;
    m_Event = NULL;
    m_NotifyCount = 0;
    m_WritePos = 0;
    m_Filling = false;
    m_pDSBuffer8 = NULL;
    m_pDSNotify = NULL;
    m_BufferSize = 0;
    m_pVorbisFile = NULL;
    memset(&m_Rect, 0, sizeof(m_Rect));
    m_Volume = 1.0f;
    m_Flags = 0;
    m_pTextureRenderer = NULL;
    SetRect(NULL);
    m_DecodedFrameNum = 0;
}

// FUNCTION: LS3DF 0x100734c0
// FUNCTION: LS3DF11 0x10070550
IShow::~IShow()
{
    Close();
    CloseStream();
    if (m_pMediaSeeking)
        m_pMediaSeeking->Release();
    if (m_pMediaControl)
        m_pMediaControl->Release();
    if (m_pMediaEvent)
        m_pMediaEvent->Release();
    if (m_pBasicAudio)
        m_pBasicAudio->Release();
    if (m_pGraphBuilder)
        m_pGraphBuilder->Release();
}

// FUNCTION: LS3DF 0x10073570
// FUNCTION: LS3DF11 0x10070600
int IShow::AddRef()
{
    return ++m_RefCount;
}

// FUNCTION: LS3DF 0x10073580
// FUNCTION: LS3DF11 0x10070610
int IShow::Release()
{
    Close();
    if (--m_RefCount < 0) {
        dbgPrintf("!! m_RefCount  IShow this:0x%x", this);
        return 0;
    }
    if (!m_RefCount) {
        delete this;
        return 0;
    }
    return m_RefCount;
}

// Decodes the Ogg file into the sound buffer: all of it at the start, else from where it was filled up to the play
// position, wrapping around at the end. Past the file's end the buffer is filled with silence.
// play is declared at the top with the other address-taken locals: declared in its block, it takes the dead
// parameter's slot, where the original gives it one of its own and a spilled copy the parameter's.
// FUNCTION: LS3DF 0x100735d0
// FUNCTION: LS3DF11 0x10070660
void IShow::m_FillBuffer(bool first)
{
    while (m_Filling)
        Sleep(0);
    m_Filling = true;
    char *ptr;
    DWORD size;
    int bitstream;
    int play;
    if (first) {
        if (SUCCEEDED(m_pDSBuffer8->Lock(0, 0, (void **)&ptr, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER))) {
            char *buffer = ptr;
            int left = size;
            while (left > 0) {
                int n = ov_read(m_pVorbisFile, ptr, min(left, 0x1000), 0, 2, 1, &bitstream);
                ptr += n;
                left -= n;
                if (n <= 0) {
                    memset(ptr, 0, left);
                    m_Flags |= ISHOW_EOF;
                    break;
                }
            }
            m_pDSBuffer8->Unlock(buffer, size, NULL, 0);
            m_WritePos = 0;
        }
    } else {
        if (SUCCEEDED(m_pDSBuffer8->GetCurrentPosition((DWORD *)&play, NULL))) {
            if (play < m_WritePos) {
                if (FAILED(m_pDSBuffer8->Lock(m_WritePos, m_BufferSize - m_WritePos, (void **)&ptr, &size, NULL, NULL,
                        0)))
                    goto done;
                m_WritePos = 0;
                char *buffer = ptr;
                if (m_Flags & ISHOW_EOF)
                    memset(ptr, 0, size);
                else {
                    int left = size;
                    while (left > 0) {
                        int n = ov_read(m_pVorbisFile, ptr, min(left, 0x1000), 0, 2, 1, &bitstream);
                        ptr += n;
                        left -= n;
                        if (n <= 0) {
                            memset(ptr, 0, left);
                            m_Flags |= ISHOW_EOF;
                            break;
                        }
                    }
                }
                m_pDSBuffer8->Unlock(buffer, size, NULL, 0);
            }
            if (play > m_WritePos) {
                if (FAILED(m_pDSBuffer8->Lock(m_WritePos, play - m_WritePos, (void **)&ptr, &size, NULL, NULL, 0)))
                    goto done;
                m_WritePos = play;
                char *buffer = ptr;
                if (m_Flags & ISHOW_EOF)
                    memset(ptr, 0, size);
                else {
                    int left = size;
                    while (left > 0) {
                        int n = ov_read(m_pVorbisFile, ptr, min(left, 0x1000), 0, 2, 1, &bitstream);
                        ptr += n;
                        left -= n;
                        if (n <= 0) {
                            memset(ptr, 0, left);
                            m_Flags |= ISHOW_EOF;
                            break;
                        }
                    }
                }
                m_pDSBuffer8->Unlock(buffer, size, NULL, 0);
            }
        }
    }
done:
    m_Filling = false;
}

// Copies the Bink video's frame into the regions' textures
// FUNCTION: LS3DF 0x100738a0
// FUNCTION: LS3DF11 0x100709b0
void CopyFrameToAllTextures(IShow *show)
{
    for (int i = show->m_RegionList.size(); i--;) {
        S_show_region &region = show->m_RegionList[i];
        uint pitch;
        void *bits = region.texture->Lock(NULL, pitch, 0);
        U32 format = 0;
        switch (region.texture->m_Format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            format = BINKSURFACE32;
            break;
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            format = BINKSURFACE555;
            break;
        case D3DFMT_R5G6B5:
            format = BINKSURFACE565;
            break;
        default:
            dbgPrintf("CopyFrameToAllTextures() - ERROR! Unsupported destination texture format: %s !",
                pGlobalIGraph->GetFormatName(region.texture->m_Format));
        }
        BinkCopyToBufferRect(show->m_pBink, bits, pitch, region.sy, 0, 0, region.x, region.y, region.sx, region.sy,
            format | BINKCOPYALL);
        region.texture->Unlock();
        region.texture->AddDirtyRect(NULL);
    }
}

// Splits a movie of the given size into textures no larger than the card allows, each cleared to black, and returns
// their format (name guessed; the texture renderer calls it too)
// FUNCTION: LS3DF 0x100739b0
D3DFORMAT CreateRegions(int width, int height, IShow *show)
{
    int y = 0;
    int rows_left = height;
    while (rows_left > 0) {
        int sy;
        if (rows_left > (int)m_MaxTextureHeight)
            sy = m_MaxTextureHeight;
        else
            sy = rows_left;
        rows_left -= sy;
        int x = 0;
        int w = width;
        while (w > 0) {
            int sx = w > (int)m_MaxTextureWidth ? m_MaxTextureWidth : w;
            w -= sx;
            S_show_region region;
            region.x = x;
            region.y = y;
            region.sx = sx;
            region.sy = sy;
            ITexture *texture = pGlobalIGraph->CreateITexture();
            region.texture = texture;
            texture->Create(NULL, NULL, show->m_Flags & ISHOW_TRUECOLOR | ITC_EMPTY, sx, sy, 0);
            switch (texture->m_Format) {
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
                texture->Fill(NULL, 0xff000000, 0);
                break;
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5:
                texture->Fill(NULL, 0x8000, 0);
                break;
            case D3DFMT_R5G6B5:
                texture->Fill(NULL, 0, 0);
                break;
            }
            show->m_RegionList.push_back(region);
            x += sx;
        }
        y += sy;
    }
    return show->m_RegionList[0].texture->m_Format;
}

// FUNCTION: LS3DF 0x10073ba0
// FUNCTION: LS3DF11 0x10070ac0
LS3D_RESULT IShow::Open(const char *name, uint open_flags)
{
    Close();
    m_Flags = open_flags;
    if (!strstr(name, ".ogg") && !strstr(name, ".OGG")) {
        if (!strstr(name, ".bik") && !strstr(name, ".BIK")) {
            if (!m_pGraphBuilder) {
                CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
                    (void **)&m_pGraphBuilder);
                if (!m_pGraphBuilder)
                    return I3DERR_GENERIC;
                m_pGraphBuilder->QueryInterface(IID_IMediaSeeking, (void **)&m_pMediaSeeking);
                m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_pMediaControl);
                m_pGraphBuilder->QueryInterface(IID_IMediaEvent, (void **)&m_pMediaEvent);
                m_pGraphBuilder->QueryInterface(IID_IBasicAudio, (void **)&m_pBasicAudio);
            }
            WCHAR wname[250];
            MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 250);
            HRESULT hr;
            if (!strstr(name, ".avi") && !strstr(name, ".AVI"))
                hr = m_pGraphBuilder->RenderFile(wname, NULL);
            else
                hr = m_CreateCTextureRenderer(wname);
            if (FAILED(hr)) {
                dbgPrintf("ERROR! IShow::Open() - %s", name);
                return I3DERR_NOFILE;
            }
        } else {
            if (pISndDriver->IsInit())
                BinkSoundUseDirectSound(pISndDriver->lpDS8);
            m_pBink = BinkOpen(name, 0);
            // As an else, the failure shares the DirectShow failure's code
            if (m_pBink)
                CreateRegions(m_pBink->Width, m_pBink->Height, this);
            else {
                dbgPrintf("ERROR! IShow::Open() - %s", name);
                return I3DERR_NOFILE;
            }
        }
    } else {
        if (!pISndDriver->IsInit())
            return I3DERR_NOTINITIALIZED;
        m_DtaHandle = dtaOpen(name, false);
        // The failure is an else: it then shares the other failures' return
        if (m_DtaHandle >= 0) {
            m_pVorbisFile = new OggVorbis_File;
            ov_callbacks callbacks = {read_func, seek_func, close_func, tell_func};
            if (ov_open_callbacks(&m_DtaHandle, m_pVorbisFile, NULL, 0, callbacks) < 0)
                return I3DERR_FILECORRUPTED;
            vorbis_info *info = ov_info(m_pVorbisFile, -1);
            PCMWAVEFORMAT format;
            memset(&format, 0, sizeof(format));
            format.wf.wFormatTag = WAVE_FORMAT_PCM;
            format.wf.nChannels = info->channels;
            format.wBitsPerSample = 16;
            format.wf.nSamplesPerSec = info->rate;
            format.wf.nBlockAlign = format.wf.nChannels * format.wBitsPerSample / 8;
            format.wf.nAvgBytesPerSec = format.wf.nSamplesPerSec * format.wf.nBlockAlign;
            DSBUFFERDESC desc;
            memset(&desc, 0, sizeof(desc));
            m_BufferSize = OGG_BUFFER_SIZE;
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
            desc.dwBufferBytes = m_BufferSize;
            desc.lpwfxFormat = (WAVEFORMATEX *)&format;
            IDirectSoundBuffer *buffer;
            if (FAILED(pISndDriver->lpDS8->CreateSoundBuffer(&desc, &buffer, NULL)))
                return I3DERR_GENERIC;
            if (FAILED(buffer->QueryInterface(IID_IDirectSoundBuffer8, (void **)&m_pDSBuffer8)))
                return I3DERR_GENERIC;
            buffer->Release();
            if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (void **)&m_pDSNotify)))
                return I3DERR_GENERIC;
            m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
            DSBPOSITIONNOTIFY positions[OGG_NOTIFY_COUNT];
            for (uint i = 0; i < OGG_NOTIFY_COUNT; i++) {
                positions[i].dwOffset = (i + 1) * (OGG_BUFFER_SIZE / OGG_NOTIFY_COUNT) - 1;
                positions[i].hEventNotify = m_Event;
            }
            if (FAILED(m_pDSNotify->SetNotificationPositions(OGG_NOTIFY_COUNT, positions)))
                return I3DERR_GENERIC;
            m_FillBuffer(true);
            m_Flags |= ISHOW_FILLED;
            m_pDSBuffer8->SetCurrentPosition(0);
            m_NotifyCount = 0;
            m_WritePos = 0;
            m_Thread = CreateThread(NULL, 0, OggThreadProc, this, 0, &m_ThreadID);
            m_Flags |= ISHOW_OGG;
        } else
            return I3DERR_NOFILE;
    }
    m_Init = true;
    SetRect(NULL);
    SetVolume(m_Volume);
    return I3D_OK;
}

// Refills the Ogg sound buffer behind the play position at each notification, and stops it once all of it has
// played past the file's end
// FUNCTION: LS3DF 0x10073fe0
// FUNCTION: LS3DF11 0x10070f00
DWORD WINAPI OggThreadProc(void *param)
{
    IShow *show = (IShow *)param;
    for (;;) {
        Sleep(0);
        if (MsgWaitForMultipleObjects(1, &show->m_Event, FALSE, INFINITE, QS_ALLEVENTS) != WAIT_OBJECT_0)
            continue;
        if (show->m_NotifyCount >= OGG_NOTIFY_COUNT) {
            if (show->m_Flags & ISHOW_EOF) {
                show->m_pDSBuffer8->Stop();
                show->m_pDSBuffer8->SetCurrentPosition(0);
                show->m_Flags &= ~(ISHOW_EOF | ISHOW_PLAYING | ISHOW_FILLED);
            }
            show->m_NotifyCount = 0;
        }
        if (show->m_Flags & ISHOW_PLAYING) {
            show->m_FillBuffer(false);
            show->m_NotifyCount++;
        }
    }
}

// vorbisfile reads the Ogg file through rw_data; the data source is the handle. The names are those of Insanity3D's
// Ogg loader, ov_callbacks' fields.
// FUNCTION: LS3DF 0x10074060
// FUNCTION: LS3DF11 0x10070f80
static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    return dtaRead(*(int *)datasource, ptr, size * nmemb);
}

// FUNCTION: LS3DF 0x10074080
// FUNCTION: LS3DF11 0x10070fa0
static int seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    return dtaSeek(*(int *)datasource, (int)offset, whence) == -1 ? -1 : 0;
}

// FUNCTION: LS3DF 0x100740b0
// FUNCTION: LS3DF11 0x10070fd0
static int close_func(void *datasource)
{
    return dtaClose(*(int *)datasource);
}

// FUNCTION: LS3DF 0x100740c0
// FUNCTION: LS3DF11 0x10070fe0
static long tell_func(void *datasource)
{
    return dtaSeek(*(int *)datasource, 0, SEEK_CUR);
}

// FUNCTION: LS3DF 0x100740e0
bool IShow::IsInit()
{
    return m_Init;
}

// FUNCTION: LS3DF 0x100740f0
// FUNCTION: LS3DF11 0x10071000
void IShow::Close()
{
    Stop();
    for (int i = m_RegionList.size(); i--;)
        m_RegionList[i].texture->Release();
    m_RegionList.clear();
    if (m_pBink) {
        BinkClose(m_pBink);
        m_pBink = NULL;
    }
    if (m_Event) {
        TerminateThread(m_Thread, 0);
        m_Thread = NULL;
        m_ThreadID = 0;
        CloseHandle(m_Event);
        m_Event = NULL;
    }
    if (m_DtaHandle >= 0) {
        ov_clear(m_pVorbisFile);
        m_DtaHandle = -1;
    }
    if (m_pVorbisFile) {
        delete m_pVorbisFile;
        m_pVorbisFile = NULL;
    }
    m_DecodedFrameNum = 0;
    m_Init = false;
}

// FUNCTION: LS3DF 0x100741c0
// FUNCTION: LS3DF11 0x100710d0
LS3D_RESULT IShow::Play()
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink) {
        if (m_Flags & ISHOW_PAUSED) {
            BinkPause(m_pBink, 0);
            m_Flags &= ~ISHOW_PAUSED;
        }
        m_Flags |= ISHOW_PLAYING;
        return I3D_OK;
    }
    if (m_Flags & ISHOW_OGG) {
        if (!(m_Flags & ISHOW_PLAYING)) {
            if (!(m_Flags & ISHOW_FILLED)) {
                if (!(m_Flags & ISHOW_PAUSED))
                    SetCurrentPos(0);
                m_FillBuffer(true);
                m_Flags |= ISHOW_FILLED;
            }
            m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
            m_Flags &= ~(ISHOW_EOF | ISHOW_PAUSED);
            m_Flags |= ISHOW_PLAYING;
            return I3D_OK;
        }
    } else if (m_pMediaControl)
        m_pMediaControl->Run();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074270
// FUNCTION: LS3DF11 0x10071180
LS3D_RESULT IShow::Stop()
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink) {
        m_Flags &= ~(ISHOW_PLAYING | ISHOW_PAUSED);
        SetCurrentPos(0);
        return I3D_OK;
    }
    if (m_Flags & ISHOW_OGG) {
        m_pDSBuffer8->Stop();
        m_pDSBuffer8->SetCurrentPosition(0);
        m_Flags &= ~(ISHOW_EOF | ISHOW_PLAYING | ISHOW_PAUSED | ISHOW_FILLED);
        m_NotifyCount = 0;
        m_WritePos = 0;
        return I3D_OK;
    }
    m_pMediaControl->Stop();
    SetCurrentPos(0);
    m_DecodedFrameNum = 0;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074300
// FUNCTION: LS3DF11 0x100711f0
LS3D_RESULT IShow::Pause()
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink) {
        if (m_Flags & ISHOW_PLAYING) {
            BinkPause(m_pBink, 1);
            m_Flags &= ~ISHOW_PLAYING;
            m_Flags |= ISHOW_PAUSED;
            return I3D_OK;
        }
    } else if (m_Flags & ISHOW_OGG) {
        if (m_Flags & ISHOW_PLAYING) {
            Stop();
            m_Flags |= ISHOW_PAUSED;
            return I3D_OK;
        }
    } else
        m_pMediaControl->Pause();
    return I3D_OK;
}

// Places the regions' vertices: the movie is scaled to the rectangle
// FUNCTION: LS3DF 0x10074380
// FUNCTION: LS3DF11 0x10071270
void IShow::m_PrepareVerts()
{
    float scale_x, scale_y;
    if (m_pBink) {
        scale_x = (float)m_Rect.right / m_pBink->Width;
        scale_y = (float)m_Rect.bottom / m_pBink->Height;
    } else if (m_pTextureRenderer) {
        scale_x = (float)m_Rect.right / m_pTextureRenderer->m_lVidWidth;
        scale_y = (float)m_Rect.bottom / m_pTextureRenderer->m_lVidHeight;
    } else
        return;
    for (int i = m_RegionList.size(); i--;) {
        m_RegionList[i].verts[0].diffuse = 0xffffffff;
        m_RegionList[i].verts[0].x = m_RegionList[i].x * scale_x + m_Rect.left;
        m_RegionList[i].verts[0].y = m_RegionList[i].y * scale_y + m_Rect.top;
        m_RegionList[i].verts[0].z = 0.0f;
        m_RegionList[i].verts[0].rhw = 1.0f;
        m_RegionList[i].verts[0].u = 0.0f;
        m_RegionList[i].verts[0].v = 0.0f;
        m_RegionList[i].verts[1].diffuse = 0xffffffff;
        m_RegionList[i].verts[1].x = (m_RegionList[i].x + m_RegionList[i].sx) * scale_x + m_Rect.left;
        m_RegionList[i].verts[1].y = m_RegionList[i].verts[0].y;
        m_RegionList[i].verts[1].z = 0.0f;
        m_RegionList[i].verts[1].rhw = 1.0f;
        m_RegionList[i].verts[1].u = (float)m_RegionList[i].sx / m_RegionList[i].texture->m_Width;
        m_RegionList[i].verts[1].v = 0.0f;
        m_RegionList[i].verts[2].diffuse = 0xffffffff;
        m_RegionList[i].verts[2].x = m_RegionList[i].verts[0].x;
        m_RegionList[i].verts[2].y = (m_RegionList[i].y + m_RegionList[i].sy) * scale_y + m_Rect.top;
        m_RegionList[i].verts[2].z = 0.0f;
        m_RegionList[i].verts[2].rhw = 1.0f;
        m_RegionList[i].verts[2].u = 0.0f;
        m_RegionList[i].verts[2].v = (float)m_RegionList[i].sy / m_RegionList[i].texture->m_Height;
        m_RegionList[i].verts[3].diffuse = 0xffffffff;
        m_RegionList[i].verts[3].x = m_RegionList[i].verts[1].x;
        m_RegionList[i].verts[3].y = m_RegionList[i].verts[2].y;
        m_RegionList[i].verts[3].z = 0.0f;
        m_RegionList[i].verts[3].rhw = 1.0f;
        m_RegionList[i].verts[3].u = m_RegionList[i].verts[1].u;
        m_RegionList[i].verts[3].v = m_RegionList[i].verts[2].v;
    }
}

// NULL: the window's client area
// FUNCTION: LS3DF 0x100745a0
// FUNCTION: LS3DF11 0x10071490
LS3D_RESULT IShow::SetRect(const RECT *rc)
{
    HWND hwnd = m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_ChildHWnd : m_MainHWnd;
    RECT r = {0, 0, 640, 480};
    if (!rc)
        GetClientRect(hwnd, &r);
    else
        r = *rc;
    m_Rect = r;
    m_PrepareVerts();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074640
// FUNCTION: LS3DF11 0x10071530
LS3D_RESULT IShow::GetRect(RECT *rc)
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    *rc = m_Rect;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074680
// FUNCTION: LS3DF11 0x10071570
LS3D_RESULT IShow::GetVideoRect(RECT *rc)
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    rc->left = 0;
    rc->top = 0;
    rc->right = 0;
    rc->bottom = 0;
    if (m_pBink) {
        rc->right = m_pBink->Width;
        rc->bottom = m_pBink->Height;
        return I3D_OK;
    }
    if (!m_pTextureRenderer)
        return I3DERR_NOTINITIALIZED;
    rc->right = m_pTextureRenderer->m_lVidWidth;
    rc->bottom = m_pTextureRenderer->m_lVidHeight;
    return I3D_OK;
}

// 0 while it plays, 1 once it has stopped or ended, 2 while paused, -1 if nothing is open
// FUNCTION: LS3DF 0x10074700
// FUNCTION: LS3DF11 0x100715c0
int IShow::GetEvent()
{
    if (!m_Init)
        return -1;
    if (m_pBink) {
        if (m_Flags & ISHOW_PLAYING)
            return 0;
        return m_Flags & ISHOW_PAUSED ? 2 : 1;
    }
    if (m_Flags & ISHOW_OGG) {
        if (m_Flags & ISHOW_PLAYING)
            return 0;
        return m_Flags & ISHOW_PAUSED ? 2 : 1;
    }
    long code;
    LONG_PTR param1, param2;
    while (SUCCEEDED(m_pMediaEvent->GetEvent(&code, &param1, &param2, 0))) {
        m_pMediaEvent->FreeEventParams(code, param1, param2);
        switch (code) {
        case EC_COMPLETE:
        case EC_USERABORT:
            Stop();
            return 1;
        case EC_PAUSED:
            return 2;
        }
    }
    return 0;
}

// In milliseconds
// FUNCTION: LS3DF 0x10074800
// FUNCTION: LS3DF11 0x100716c0
LS3D_RESULT IShow::SetCurrentPos(uint pos)
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink) {
        BinkGoto(m_pBink, (U32)(m_pBink->unknown_2c0 * pos / (m_pBink->unknown_2bc * 0.001f)), 0);
        return I3D_OK;
    }
    if (m_Flags & ISHOW_OGG) {
        if (m_Flags & ISHOW_PLAYING)
            m_pDSBuffer8->Stop();
        if (!ov_time_seek(m_pVorbisFile, pos * 0.001f)) {
            m_Flags &= ~ISHOW_EOF;
            m_FillBuffer(true);
            m_Flags |= ISHOW_FILLED;
            m_pDSBuffer8->SetCurrentPosition(0);
            if (m_Flags & ISHOW_PLAYING)
                m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
            else
                m_Flags |= ISHOW_PAUSED;
            return I3D_OK;
        }
        return I3DERR_GENERIC;
    }
    LONGLONG time = pos * 10000;
    m_pMediaSeeking->SetPositions(&time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074960
// FUNCTION: LS3DF11 0x10071820
LS3D_RESULT IShow::GetCurrentPos(uint *pos)
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink) {
        *pos = (uint)(m_pBink->unknown_2bc * m_pBink->FrameNum * 0.001f / m_pBink->unknown_2c0);
        return I3D_OK;
    }
    if (m_Flags & ISHOW_OGG) {
        *pos = (uint)(ov_time_tell(m_pVorbisFile) * 1000.0);
        return I3D_OK;
    }
    LONGLONG time;
    m_pMediaSeeking->GetCurrentPosition(&time);
    *pos = (uint)(time / 10000);
    return I3D_OK;
}

// From 0 to 1: Bink's volume is linear, DirectSound's and DirectShow's in hundredths of a decibel
// FUNCTION: LS3DF 0x10074a30
// FUNCTION: LS3DF11 0x100718f0
LS3D_RESULT IShow::SetVolume(float vol)
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (!pISndDriver->IsInit())
        vol = 0.0f;
    m_Volume = vol;
    if (m_pBink) {
        BinkSetVolume(m_pBink, 0, (S32)(vol * 32768.0f));
        return I3D_OK;
    }
    int db = vol < MRG_ZERO ? DSBVOLUME_MIN : (int)(log(vol) * 866.0);
    if (db < DSBVOLUME_MIN)
        db = DSBVOLUME_MIN;
    if (m_Flags & ISHOW_OGG)
        m_pDSBuffer8->SetVolume(db);
    else
        m_pBasicAudio->put_Volume(db);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074ae0
// FUNCTION: LS3DF11 0x100719a0
int IShow::GetRegionCnt()
{
    return m_RegionList.size();
}

// FUNCTION: LS3DF 0x10074af0
// FUNCTION: LS3DF11 0x100719b0
S_show_region *IShow::GetRegions()
{
    Sleep(0);
    return &m_RegionList[0];
}

// Decodes the Bink video's next frame into the textures when it is time for it
// FUNCTION: LS3DF 0x10074b10
LS3D_RESULT IShow::ProcessBinkFrame()
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink && !BinkWait(m_pBink)) {
        BinkDoFrame(m_pBink);
        CopyFrameToAllTextures(this);
        if (m_pBink->FrameNum == m_pBink->Frames)
            Stop();
        else
            BinkNextFrame(m_pBink);
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074b70
// FUNCTION: LS3DF11 0x100719d0
LS3D_RESULT IShow::RenderCurrentFrame()
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    if (m_pBink && !BinkWait(m_pBink)) {
        BinkDoFrame(m_pBink);
        CopyFrameToAllTextures(this);
        if (m_pBink->FrameNum == m_pBink->Frames)
            Stop();
        else
            BinkNextFrame(m_pBink);
    }
    if (m_CurrAlphaOp != D3DTOP_MODULATE) {
        m_CurrAlphaOp = D3DTOP_MODULATE;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    }
    if (m_CurrColorOp != D3DTOP_MODULATE) {
        m_CurrColorOp = D3DTOP_MODULATE;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    }
    if (m_CurrColorOp1 != D3DTOP_DISABLE) {
        m_CurrColorOp1 = D3DTOP_DISABLE;
        m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    }
    if (m_CurrAlphaState != 0) {
        m_CurrAlphaState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    }
    if (m_CurrFogState != 0) {
        m_CurrFogState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    if (m_CurrClipState != 1) {
        m_CurrClipState = 1;
        m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
    }
    m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    for (int i = m_RegionList.size(); i--;) {
        pGlobalIGraph->SetTexture(m_RegionList[i].texture);
        pGlobalIGraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, m_RegionList[i].verts, LS3D_STREAM_TLVERTEX);
    }
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, ZBufferEnable);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074d70
// FUNCTION: LS3DF11 0x10071bd0
LS3D_RESULT IShow::RenderVideoUntilKeyPressOrEnd()
{
    if (!m_Init)
        return I3DERR_NOTINITIALIZED;
    Play();
    if (m_CurrAlphaOp != D3DTOP_MODULATE) {
        m_CurrAlphaOp = D3DTOP_MODULATE;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    }
    if (m_CurrColorOp != D3DTOP_MODULATE) {
        m_CurrColorOp = D3DTOP_MODULATE;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    }
    if (m_CurrColorOp1 != D3DTOP_DISABLE) {
        m_CurrColorOp1 = D3DTOP_DISABLE;
        m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    }
    if (m_CurrAlphaState != 0) {
        m_CurrAlphaState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    }
    if (m_CurrFogState != 0) {
        m_CurrFogState = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    int frame = m_DecodedFrameNum - 1;
    while (GetEvent() != 1) {
        pGlobalIGraph->ProcessWinMessages();
        pGlobalIGraph->UpdateKeyboardData();
        if (pGlobalIGraph->ReadKey()) {
            Stop();
            break;
        }
        if (m_pBink) {
            pGlobalIGraph->Clear(0, 1.0f, 3);
            pGlobalIGraph->BeginScene();
            RenderCurrentFrame();
            pGlobalIGraph->EndScene();
            pGlobalIGraph->Present();
        } else if (frame != m_DecodedFrameNum) {
            frame = m_DecodedFrameNum;
            pGlobalIGraph->Clear(0, 1.0f, 3);
            pGlobalIGraph->BeginScene();
            RenderCurrentFrame();
            pGlobalIGraph->EndScene();
            pGlobalIGraph->Present();
        } else
            Sleep(0);
    }
    return I3D_OK;
}

// Builds the DirectShow graph for an AVI file: its source filter connected to the texture renderer (the DirectX SDK
// sample's InitDShowTextureRenderer, which also ran the graph), then, if the AVI Splitter that brings in has a pin with
// an audio type, DirectShow's sound renderer connected to it
// FUNCTION: LS3DF 0x10074f40
// FUNCTION: LS3DF11 0x10071da0
HRESULT IShow::m_CreateCTextureRenderer(const WCHAR *name)
{
    // Statics, not locals: the original's .bss has them around the header's zero vector (0x101c5be8-0x101c5dab). And
    // the function's, not file-level globals: no consistent set of file-level names hashes into their order, natural
    // names of local statics do. The names are guesses, which their place in .bss rules out (tools/bsshash.py). The
    // splitter is never reset, so a later file without one still sees the last file's.
    static int pins_found;
    static AM_MEDIA_TYPE *media_types_array[20];
    static int types_found;
    static char name_buf[256];
    static IPin *pins_list[20];
    static IBaseFilter *splitter;
    static IEnumMediaTypes *enum_mt;
    static IEnumPins *pin_enum;

    HRESULT hr = -1;                    // what a failed new leaves
    IBaseFilter *pFTR;                  // Texture Renderer Filter
    IPin *pFTRPinIn = NULL;             // Texture Renderer Input Pin
    IBaseFilter *pFSrc = NULL;          // Source Filter
    IPin *pFSrcPinOut = NULL;           // Source Filter Output Pin

    m_pTextureRenderer = new CTextureRenderer(NULL, &hr);
    if (FAILED(hr)) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not create texture renderer object!  hr=0x%x", hr);
        return E_FAIL;
    }
    m_pTextureRenderer->m_pShow = this;

    pFTR = m_pTextureRenderer;
    if (FAILED(hr = m_pGraphBuilder->AddFilter(pFTR, L"TEXTURERENDERER"))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not add renderer filter to graph!  hr=0x%x", hr);
        return hr;
    }

    if (FAILED(hr = m_pGraphBuilder->AddSourceFilter(name, L"SOURCE", &pFSrc))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not create source filter to graph!  hr=0x%x", hr);
        return hr;
    }

    if (FAILED(hr = pFTR->FindPin(L"In", &pFTRPinIn))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not find input pin!  hr=0x%x", hr);
        return hr;
    }
    if (FAILED(hr = pFSrc->FindPin(L"Output", &pFSrcPinOut))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not find output pin!  hr=0x%x", hr);
        return hr;
    }

    if (FAILED(hr = m_pGraphBuilder->Connect(pFSrcPinOut, pFTRPinIn))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not connect pins!  hr=0x%x", hr);
        return hr;
    }
    pFTRPinIn->Release();
    pFSrcPinOut->Release();
    pFSrc->Release();

    // The connection brings in the AVI Splitter between the two. A block of its own: the original gives its locals'
    // slots to the pin loop's afterwards. Not matching yet: the original stores pEnum = NULL after the call's pushes,
    // ours after the first push (pEnum declared before the Releases puts the call right but the store too early).
    // Ours is VC6's usual order (I3d_sound.cpp's CreateSource(&src) has it too): the store before the vtable load.
    {
        IEnumFilters *pEnum = NULL;
        m_pGraphBuilder->EnumFilters(&pEnum);
        IBaseFilter *pFilter;
        ULONG fetched;
        while (pEnum->Next(1, &pFilter, &fetched) == S_OK) {
            FILTER_INFO info;
            pFilter->QueryFilterInfo(&info);
            WideCharToMultiByte(CP_ACP, 0, info.achName, -1, name_buf, sizeof(name_buf), NULL, NULL);
            if (!strcmp(name_buf, "AVI Splitter")) {
                splitter = pFilter;
                splitter->AddRef();
            }
            info.pGraph->Release();
            pFilter->Release();
        }
        pEnum->Release();
    }

    if (splitter) {
        // Its first pin with an audio type goes to the sound renderer
        splitter->EnumPins(&pin_enum);
        pin_enum->Next(20, pins_list, (ULONG *)&pins_found);
        IPin *audio_pin = NULL;
        for (int i = 0; i < pins_found; i++) {
            pins_list[i]->EnumMediaTypes(&enum_mt);
            enum_mt->Next(20, media_types_array, (ULONG *)&types_found);
            for (int j = 0; j < types_found; j++) {
                if (!audio_pin && media_types_array[j]->majortype == MEDIATYPE_Audio) {
                    audio_pin = pins_list[i];
                    audio_pin->AddRef();
                }
                DeleteMediaType(media_types_array[j]);
            }
            enum_mt->Release();
            pins_list[i]->Release();
        }
        pin_enum->Release();
        if (audio_pin) {
            CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter,
                (void **)&m_pTextureRenderer->m_pSoundRenderer);
            if (m_pTextureRenderer->m_pSoundRenderer) {
                hr = m_pGraphBuilder->AddFilter(m_pTextureRenderer->m_pSoundRenderer, L"SOUNDRENDERER");
                IPin *pSndPinIn;
                if (FAILED(hr = m_pTextureRenderer->m_pSoundRenderer->FindPin(L"Audio Input pin (rendered)",
                        &pSndPinIn))) {
                    dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not find audio input pin!  hr=0x%x",
                        hr);
                    return hr;
                }
                if (FAILED(hr = m_pGraphBuilder->Connect(audio_pin, pSndPinIn))) {
                    dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not connect pins!  hr=0x%x", hr);
                    return hr;
                }
                pSndPinIn->Release();
                audio_pin->Release();
            }
        }
        splitter->Release();
    }
    m_PrepareVerts();
    return S_OK;
}

// The sample's constructor, inlined into m_CreateCTextureRenderer (/OPT:REF drops its copy)
CTextureRenderer::CTextureRenderer(LPUNKNOWN pUnk, HRESULT *phr)
    : CBaseVideoRenderer(__uuidof(CLSID_TextureRenderer), NAME("Texture Renderer"), pUnk, phr)
{
    *phr = S_OK;
    m_TextureFormat = D3DFMT_UNKNOWN;
    m_pSoundRenderer = NULL;
}

// FUNCTION: LS3DF 0x10075400
// FUNCTION: LS3DF11 0x10072260
CTextureRenderer::~CTextureRenderer()
{
    if (m_pSoundRenderer)
        m_pSoundRenderer->Release();
}

// Accepts only RGB video in the textures' format: 32 bits for a true color movie if the card has a true color texture
// format, else the 16-bit format of the card's textures (the sample accepts RGB24)
// FUNCTION: LS3DF 0x10075480
// FUNCTION: LS3DF11 0x100722e0
HRESULT CTextureRenderer::CheckMediaType(const CMediaType *pmt)
{
    HRESULT hr = E_FAIL;
    VIDEOINFO *pvi;

    if (*pmt->FormatType() != FORMAT_VideoInfo)
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();
    if (m_pShow->m_Flags & ISHOW_TRUECOLOR && m_TrueColorTexturePixelFormat) {
        if (IsEqualGUID(*pmt->Type(), MEDIATYPE_Video) && IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB32))
            hr = S_OK;
    } else if (m_TexturePixelFormat == D3DFMT_R5G6B5) {
        if (IsEqualGUID(*pmt->Type(), MEDIATYPE_Video) && IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB565))
            hr = S_OK;
    } else {
        if (IsEqualGUID(*pmt->Type(), MEDIATYPE_Video) && IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_RGB555))
            hr = S_OK;
    }
    return hr;
}

// The connection is made: the movie's regions are created for the video's size
// FUNCTION: LS3DF 0x10075530
// FUNCTION: LS3DF11 0x10072390
HRESULT CTextureRenderer::SetMediaType(const CMediaType *pmt)
{
    VIDEOINFO *pviBmp = (VIDEOINFO *)pmt->Format();
    m_lVidWidth = pviBmp->bmiHeader.biWidth;
    m_lVidHeight = abs(pviBmp->bmiHeader.biHeight);
    // 16-bit rows are padded to 4 bytes. The odd width test is the original's (w - 1 ^ w) & ~1, not w & 1's test al, 1
    if (pviBmp->bmiHeader.biBitCount == 32)
        m_lVidPitch = m_lVidWidth * 4;
    else if ((m_lVidWidth - 1) / 2 == m_lVidWidth / 2)
        m_lVidPitch = m_lVidWidth * 2 + 2;
    else
        m_lVidPitch = m_lVidWidth * 2;
    m_TextureFormat = CreateRegions(m_lVidWidth, m_lVidHeight, m_pShow);
    return m_TextureFormat ? S_OK : -1;
}

// Copies the sample's frame into the regions' textures, the video's rows bottom up, and counts it. One src for both
// formats, as the original's frame has. Open: the original gives pitch the dead pSample's slot and pBmpBuffer one of
// its own, ours the reverse. The more referenced of the two gets the slot (one more use of pitch, an initializer or a
// message, gives the original's), ties go to pBmpBuffer whatever the declarations' or last uses' order, and references
// that leave no code (if (0) pitch++, (void)&pitch) don't count.
// FUNCTION: LS3DF 0x100755c0
// FUNCTION: LS3DF11 0x10072420
HRESULT CTextureRenderer::DoRenderSample(IMediaSample *pSample)
{
    BYTE *pBmpBuffer;
    BYTE *src;

    pSample->GetPointer(&pBmpBuffer);
    for (int i = m_pShow->m_RegionList.size(); i--;) {
        S_show_region &region = m_pShow->m_RegionList[i];
        uint pitch;
        void *bits = region.texture->Lock(NULL, pitch, 0);
        switch (m_TextureFormat) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8: {
            src = pBmpBuffer + (m_lVidHeight - region.y - 1) * m_lVidPitch + region.x * 4;
            int sx = region.sx;
            int sy = region.sy;
            int dst_add = pitch - sx * 4;
            int src_sub = sx * 4 + m_lVidPitch;
            __asm {
                mov esi, src
                mov edi, bits
                mov edx, sy
            row32:
                mov ecx, sx
                rep movsd
                add edi, dst_add
                sub esi, src_sub
                dec edx
                jnz row32
            }
            break;
        }
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5: {
            src = pBmpBuffer + (m_lVidHeight - region.y - 1) * m_lVidPitch + region.x * 2;
            int sx = region.sx;
            int sy = region.sy;
            int dst_add = pitch - sx * 2;
            int src_sub = sx * 2 + m_lVidPitch;
            __asm {
                mov esi, src
                mov edi, bits
                mov edx, sy
            row16:
                mov ecx, sx
                rep movsw
                add edi, dst_add
                sub esi, src_sub
                dec edx
                jnz row16
            }
            break;
        }
        default:
            dbgPrintf("IShow::DoRenderSample() - ERROR! Unsuppoted destination texture format: %s !",
                pGlobalIGraph->GetFormatName(m_TextureFormat));
        }
        region.texture->Unlock();
        region.texture->AddDirtyRect(NULL);
    }
    m_pShow->m_DecodedFrameNum++;
    return S_OK;
}
