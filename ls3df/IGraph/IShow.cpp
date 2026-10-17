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
// names. It derives from the base classes in ls3df/BaseClasses/: VC6's own STRMBASE.LIB (1997) is another build of
// them, with other vtables and no EH frame. The base classes' empty OnReceiveFirstSample and PrepareRender are folded
// with other empty functions: the original's slots point at the scene file's Update (0x1004e970) and at a copy among
// the base classes' functions (0x10088970).
struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_TextureRenderer;

// VTABLE: LS3DF 0x1009d788 CUnknown
// VTABLE: LS3DF 0x1009d748 IBaseFilter
// VTABLE: LS3DF 0x1009d734 IAMovieSetup
// VTABLE: LS3DF 0x1009d710 IQualProp
// VTABLE: LS3DF 0x1009d6fc IQualityControl
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
    IBaseFilter *sound_renderer;        // 0x16c, DirectShow's, for the file's sound
    IShow *show;                        // 0x170
    D3DFORMAT format;                   // 0x174, the regions' textures', from CreateRegions
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
IShow::IShow() : regions(1)
{
    ref = 1;
    stream = -1;
    dta_handle = -1;
    init = false;
    graph = NULL;
    control = NULL;
    seeking = NULL;
    audio = NULL;
    media_event = NULL;
    bink = NULL;
    stream_init = false;
    stream_paused = false;
    stream_done = false;
    thread_id = 0;
    thread = NULL;
    event = NULL;
    notify_count = 0;
    write_pos = 0;
    filling = false;
    sound_buffer = NULL;
    notify = NULL;
    buffer_size = 0;
    vf = NULL;
    memset(&rect, 0, sizeof(rect));
    volume = 1.0f;
    flags = 0;
    renderer = NULL;
    SetRect(NULL);
    decoded_frame = 0;
}

// FUNCTION: LS3DF 0x100734c0
IShow::~IShow()
{
    Close();
    CloseStream();
    if (seeking)
        seeking->Release();
    if (control)
        control->Release();
    if (media_event)
        media_event->Release();
    if (audio)
        audio->Release();
    if (graph)
        graph->Release();
}

// FUNCTION: LS3DF 0x10073570
int IShow::AddRef()
{
    return ++ref;
}

// FUNCTION: LS3DF 0x10073580
int IShow::Release()
{
    Close();
    if (--ref < 0) {
        dbgPrintf("!! m_RefCount  IShow this:0x%x", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// Decodes the Ogg file into the sound buffer: all of it at the start, else from where it was filled up to the play
// position, wrapping around at the end. Past the file's end the buffer is filled with silence.
// play is declared at the top with the other address-taken locals: declared in its block, it takes the dead
// parameter's slot, where the original gives it one of its own and a spilled copy the parameter's.
// FUNCTION: LS3DF 0x100735d0
void IShow::m_FillBuffer(bool first)
{
    while (filling)
        Sleep(0);
    filling = true;
    char *ptr;
    DWORD size;
    int bitstream;
    int play;
    if (first) {
        if (SUCCEEDED(sound_buffer->Lock(0, 0, (void **)&ptr, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER))) {
            char *buffer = ptr;
            int left = size;
            while (left > 0) {
                int n = ov_read(vf, ptr, min(left, 0x1000), 0, 2, 1, &bitstream);
                ptr += n;
                left -= n;
                if (n <= 0) {
                    memset(ptr, 0, left);
                    flags |= ISHOW_EOF;
                    break;
                }
            }
            sound_buffer->Unlock(buffer, size, NULL, 0);
            write_pos = 0;
        }
    } else {
        if (SUCCEEDED(sound_buffer->GetCurrentPosition((DWORD *)&play, NULL))) {
            if (play < write_pos) {
                if (FAILED(sound_buffer->Lock(write_pos, buffer_size - write_pos, (void **)&ptr, &size, NULL, NULL,
                        0)))
                    goto done;
                write_pos = 0;
                char *buffer = ptr;
                if (flags & ISHOW_EOF)
                    memset(ptr, 0, size);
                else {
                    int left = size;
                    while (left > 0) {
                        int n = ov_read(vf, ptr, min(left, 0x1000), 0, 2, 1, &bitstream);
                        ptr += n;
                        left -= n;
                        if (n <= 0) {
                            memset(ptr, 0, left);
                            flags |= ISHOW_EOF;
                            break;
                        }
                    }
                }
                sound_buffer->Unlock(buffer, size, NULL, 0);
            }
            if (play > write_pos) {
                if (FAILED(sound_buffer->Lock(write_pos, play - write_pos, (void **)&ptr, &size, NULL, NULL, 0)))
                    goto done;
                write_pos = play;
                char *buffer = ptr;
                if (flags & ISHOW_EOF)
                    memset(ptr, 0, size);
                else {
                    int left = size;
                    while (left > 0) {
                        int n = ov_read(vf, ptr, min(left, 0x1000), 0, 2, 1, &bitstream);
                        ptr += n;
                        left -= n;
                        if (n <= 0) {
                            memset(ptr, 0, left);
                            flags |= ISHOW_EOF;
                            break;
                        }
                    }
                }
                sound_buffer->Unlock(buffer, size, NULL, 0);
            }
        }
    }
done:
    filling = false;
}

// Copies the Bink video's frame into the regions' textures
// FUNCTION: LS3DF 0x100738a0
void CopyFrameToAllTextures(IShow *show)
{
    for (int i = show->regions.size(); i--;) {
        S_show_region &region = show->regions[i];
        uint pitch;
        void *bits = region.texture->Lock(NULL, pitch, 0);
        U32 format = 0;
        switch (region.texture->format) {
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
                pGlobalIGraph->GetFormatName(region.texture->format));
        }
        BinkCopyToBufferRect(show->bink, bits, pitch, region.sy, 0, 0, region.x, region.y, region.sx, region.sy,
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
            texture->Create(NULL, NULL, show->flags & ISHOW_TRUECOLOR | ITC_EMPTY, sx, sy, 0);
            switch (texture->format) {
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
            show->regions.push_back(region);
            x += sx;
        }
        y += sy;
    }
    return show->regions[0].texture->format;
}

// FUNCTION: LS3DF 0x10073ba0
LS3D_RESULT IShow::Open(const char *name, uint open_flags)
{
    Close();
    flags = open_flags;
    if (!strstr(name, ".ogg") && !strstr(name, ".OGG")) {
        if (!strstr(name, ".bik") && !strstr(name, ".BIK")) {
            if (!graph) {
                CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&graph);
                if (!graph)
                    return I3DERR_GENERIC;
                graph->QueryInterface(IID_IMediaSeeking, (void **)&seeking);
                graph->QueryInterface(IID_IMediaControl, (void **)&control);
                graph->QueryInterface(IID_IMediaEvent, (void **)&media_event);
                graph->QueryInterface(IID_IBasicAudio, (void **)&audio);
            }
            WCHAR wname[250];
            MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 250);
            HRESULT hr;
            if (!strstr(name, ".avi") && !strstr(name, ".AVI"))
                hr = graph->RenderFile(wname, NULL);
            else
                hr = m_CreateCTextureRenderer(wname);
            if (FAILED(hr)) {
                dbgPrintf("ERROR! IShow::Open() - %s", name);
                return I3DERR_NOFILE;
            }
        } else {
            if (pISndDriver->IsInit())
                BinkSoundUseDirectSound(pISndDriver->lpDS8);
            bink = BinkOpen(name, 0);
            // As an else, the failure shares the DirectShow failure's code
            if (bink)
                CreateRegions(bink->Width, bink->Height, this);
            else {
                dbgPrintf("ERROR! IShow::Open() - %s", name);
                return I3DERR_NOFILE;
            }
        }
    } else {
        if (!pISndDriver->IsInit())
            return I3DERR_NOTINITIALIZED;
        dta_handle = dtaOpen(name, false);
        // The failure is an else: it then shares the other failures' return
        if (dta_handle >= 0) {
            vf = new OggVorbis_File;
            ov_callbacks callbacks = {read_func, seek_func, close_func, tell_func};
            if (ov_open_callbacks(&dta_handle, vf, NULL, 0, callbacks) < 0)
                return I3DERR_FILECORRUPTED;
            vorbis_info *info = ov_info(vf, -1);
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
            buffer_size = OGG_BUFFER_SIZE;
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
            desc.dwBufferBytes = buffer_size;
            desc.lpwfxFormat = (WAVEFORMATEX *)&format;
            IDirectSoundBuffer *buffer;
            if (FAILED(pISndDriver->lpDS8->CreateSoundBuffer(&desc, &buffer, NULL)))
                return I3DERR_GENERIC;
            if (FAILED(buffer->QueryInterface(IID_IDirectSoundBuffer8, (void **)&sound_buffer)))
                return I3DERR_GENERIC;
            buffer->Release();
            if (FAILED(sound_buffer->QueryInterface(IID_IDirectSoundNotify, (void **)&notify)))
                return I3DERR_GENERIC;
            event = CreateEvent(NULL, FALSE, FALSE, NULL);
            DSBPOSITIONNOTIFY positions[OGG_NOTIFY_COUNT];
            for (uint i = 0; i < OGG_NOTIFY_COUNT; i++) {
                positions[i].dwOffset = (i + 1) * (OGG_BUFFER_SIZE / OGG_NOTIFY_COUNT) - 1;
                positions[i].hEventNotify = event;
            }
            if (FAILED(notify->SetNotificationPositions(OGG_NOTIFY_COUNT, positions)))
                return I3DERR_GENERIC;
            m_FillBuffer(true);
            flags |= ISHOW_FILLED;
            sound_buffer->SetCurrentPosition(0);
            notify_count = 0;
            write_pos = 0;
            thread = CreateThread(NULL, 0, OggThreadProc, this, 0, &thread_id);
            flags |= ISHOW_OGG;
        } else
            return I3DERR_NOFILE;
    }
    init = true;
    SetRect(NULL);
    SetVolume(volume);
    return I3D_OK;
}

// Refills the Ogg sound buffer behind the play position at each notification, and stops it once all of it has
// played past the file's end
// FUNCTION: LS3DF 0x10073fe0
DWORD WINAPI OggThreadProc(void *param)
{
    IShow *show = (IShow *)param;
    for (;;) {
        Sleep(0);
        if (MsgWaitForMultipleObjects(1, &show->event, FALSE, INFINITE, QS_ALLEVENTS) != WAIT_OBJECT_0)
            continue;
        if (show->notify_count >= OGG_NOTIFY_COUNT) {
            if (show->flags & ISHOW_EOF) {
                show->sound_buffer->Stop();
                show->sound_buffer->SetCurrentPosition(0);
                show->flags &= ~(ISHOW_EOF | ISHOW_PLAYING | ISHOW_FILLED);
            }
            show->notify_count = 0;
        }
        if (show->flags & ISHOW_PLAYING) {
            show->m_FillBuffer(false);
            show->notify_count++;
        }
    }
}

// vorbisfile reads the Ogg file through rw_data; the data source is the handle. The names are those of Insanity3D's
// Ogg loader, ov_callbacks' fields.
// FUNCTION: LS3DF 0x10074060
static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    return dtaRead(*(int *)datasource, ptr, size * nmemb);
}

// FUNCTION: LS3DF 0x10074080
static int seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    return dtaSeek(*(int *)datasource, (int)offset, whence) == -1 ? -1 : 0;
}

// FUNCTION: LS3DF 0x100740b0
static int close_func(void *datasource)
{
    return dtaClose(*(int *)datasource);
}

// FUNCTION: LS3DF 0x100740c0
static long tell_func(void *datasource)
{
    return dtaSeek(*(int *)datasource, 0, SEEK_CUR);
}

// FUNCTION: LS3DF 0x100740e0
bool IShow::IsInit()
{
    return init;
}

// FUNCTION: LS3DF 0x100740f0
void IShow::Close()
{
    Stop();
    for (int i = regions.size(); i--;)
        regions[i].texture->Release();
    regions.clear();
    if (bink) {
        BinkClose(bink);
        bink = NULL;
    }
    if (event) {
        TerminateThread(thread, 0);
        thread = NULL;
        thread_id = 0;
        CloseHandle(event);
        event = NULL;
    }
    if (dta_handle >= 0) {
        ov_clear(vf);
        dta_handle = -1;
    }
    if (vf) {
        delete vf;
        vf = NULL;
    }
    decoded_frame = 0;
    init = false;
}

// FUNCTION: LS3DF 0x100741c0
LS3D_RESULT IShow::Play()
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink) {
        if (flags & ISHOW_PAUSED) {
            BinkPause(bink, 0);
            flags &= ~ISHOW_PAUSED;
        }
        flags |= ISHOW_PLAYING;
        return I3D_OK;
    }
    if (flags & ISHOW_OGG) {
        if (!(flags & ISHOW_PLAYING)) {
            if (!(flags & ISHOW_FILLED)) {
                if (!(flags & ISHOW_PAUSED))
                    SetCurrentPos(0);
                m_FillBuffer(true);
                flags |= ISHOW_FILLED;
            }
            sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
            flags &= ~(ISHOW_EOF | ISHOW_PAUSED);
            flags |= ISHOW_PLAYING;
            return I3D_OK;
        }
    } else if (control)
        control->Run();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074270
LS3D_RESULT IShow::Stop()
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink) {
        flags &= ~(ISHOW_PLAYING | ISHOW_PAUSED);
        SetCurrentPos(0);
        return I3D_OK;
    }
    if (flags & ISHOW_OGG) {
        sound_buffer->Stop();
        sound_buffer->SetCurrentPosition(0);
        flags &= ~(ISHOW_EOF | ISHOW_PLAYING | ISHOW_PAUSED | ISHOW_FILLED);
        notify_count = 0;
        write_pos = 0;
        return I3D_OK;
    }
    control->Stop();
    SetCurrentPos(0);
    decoded_frame = 0;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074300
LS3D_RESULT IShow::Pause()
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink) {
        if (flags & ISHOW_PLAYING) {
            BinkPause(bink, 1);
            flags &= ~ISHOW_PLAYING;
            flags |= ISHOW_PAUSED;
            return I3D_OK;
        }
    } else if (flags & ISHOW_OGG) {
        if (flags & ISHOW_PLAYING) {
            Stop();
            flags |= ISHOW_PAUSED;
            return I3D_OK;
        }
    } else
        control->Pause();
    return I3D_OK;
}

// Places the regions' vertices: the movie is scaled to the rectangle
// FUNCTION: LS3DF 0x10074380
void IShow::m_PrepareVerts()
{
    float scale_x, scale_y;
    if (bink) {
        scale_x = (float)rect.right / bink->Width;
        scale_y = (float)rect.bottom / bink->Height;
    } else if (renderer) {
        scale_x = (float)rect.right / renderer->m_lVidWidth;
        scale_y = (float)rect.bottom / renderer->m_lVidHeight;
    } else
        return;
    for (int i = regions.size(); i--;) {
        regions[i].verts[0].diffuse = 0xffffffff;
        regions[i].verts[0].x = regions[i].x * scale_x + rect.left;
        regions[i].verts[0].y = regions[i].y * scale_y + rect.top;
        regions[i].verts[0].z = 0.0f;
        regions[i].verts[0].rhw = 1.0f;
        regions[i].verts[0].u = 0.0f;
        regions[i].verts[0].v = 0.0f;
        regions[i].verts[1].diffuse = 0xffffffff;
        regions[i].verts[1].x = (regions[i].x + regions[i].sx) * scale_x + rect.left;
        regions[i].verts[1].y = regions[i].verts[0].y;
        regions[i].verts[1].z = 0.0f;
        regions[i].verts[1].rhw = 1.0f;
        regions[i].verts[1].u = (float)regions[i].sx / regions[i].texture->width;
        regions[i].verts[1].v = 0.0f;
        regions[i].verts[2].diffuse = 0xffffffff;
        regions[i].verts[2].x = regions[i].verts[0].x;
        regions[i].verts[2].y = (regions[i].y + regions[i].sy) * scale_y + rect.top;
        regions[i].verts[2].z = 0.0f;
        regions[i].verts[2].rhw = 1.0f;
        regions[i].verts[2].u = 0.0f;
        regions[i].verts[2].v = (float)regions[i].sy / regions[i].texture->height;
        regions[i].verts[3].diffuse = 0xffffffff;
        regions[i].verts[3].x = regions[i].verts[1].x;
        regions[i].verts[3].y = regions[i].verts[2].y;
        regions[i].verts[3].z = 0.0f;
        regions[i].verts[3].rhw = 1.0f;
        regions[i].verts[3].u = regions[i].verts[1].u;
        regions[i].verts[3].v = regions[i].verts[2].v;
    }
}

// NULL: the window's client area
// FUNCTION: LS3DF 0x100745a0
LS3D_RESULT IShow::SetRect(const RECT *rc)
{
    HWND hwnd = m_InitDesc.flags & IGRAPH_CHILD_WINDOW ? m_ChildHWnd : m_MainHWnd;
    RECT r = {0, 0, 640, 480};
    if (!rc)
        GetClientRect(hwnd, &r);
    else
        r = *rc;
    rect = r;
    m_PrepareVerts();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074640
LS3D_RESULT IShow::GetRect(RECT *rc)
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    *rc = rect;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074680
LS3D_RESULT IShow::GetVideoRect(RECT *rc)
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    rc->left = 0;
    rc->top = 0;
    rc->right = 0;
    rc->bottom = 0;
    if (bink) {
        rc->right = bink->Width;
        rc->bottom = bink->Height;
        return I3D_OK;
    }
    if (!renderer)
        return I3DERR_NOTINITIALIZED;
    rc->right = renderer->m_lVidWidth;
    rc->bottom = renderer->m_lVidHeight;
    return I3D_OK;
}

// 0 while it plays, 1 once it has stopped or ended, 2 while paused, -1 if nothing is open
// FUNCTION: LS3DF 0x10074700
int IShow::GetEvent()
{
    if (!init)
        return -1;
    if (bink) {
        if (flags & ISHOW_PLAYING)
            return 0;
        return flags & ISHOW_PAUSED ? 2 : 1;
    }
    if (flags & ISHOW_OGG) {
        if (flags & ISHOW_PLAYING)
            return 0;
        return flags & ISHOW_PAUSED ? 2 : 1;
    }
    long code;
    LONG_PTR param1, param2;
    while (SUCCEEDED(media_event->GetEvent(&code, &param1, &param2, 0))) {
        media_event->FreeEventParams(code, param1, param2);
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
LS3D_RESULT IShow::SetCurrentPos(uint pos)
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink) {
        BinkGoto(bink, (U32)(bink->unknown_2c0 * pos / (bink->unknown_2bc * 0.001f)), 0);
        return I3D_OK;
    }
    if (flags & ISHOW_OGG) {
        if (flags & ISHOW_PLAYING)
            sound_buffer->Stop();
        if (!ov_time_seek(vf, pos * 0.001f)) {
            flags &= ~ISHOW_EOF;
            m_FillBuffer(true);
            flags |= ISHOW_FILLED;
            sound_buffer->SetCurrentPosition(0);
            if (flags & ISHOW_PLAYING)
                sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
            else
                flags |= ISHOW_PAUSED;
            return I3D_OK;
        }
        return I3DERR_GENERIC;
    }
    LONGLONG time = pos * 10000;
    seeking->SetPositions(&time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074960
LS3D_RESULT IShow::GetCurrentPos(uint *pos)
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink) {
        *pos = (uint)(bink->unknown_2bc * bink->FrameNum * 0.001f / bink->unknown_2c0);
        return I3D_OK;
    }
    if (flags & ISHOW_OGG) {
        *pos = (uint)(ov_time_tell(vf) * 1000.0);
        return I3D_OK;
    }
    LONGLONG time;
    seeking->GetCurrentPosition(&time);
    *pos = (uint)(time / 10000);
    return I3D_OK;
}

// From 0 to 1: Bink's volume is linear, DirectSound's and DirectShow's in hundredths of a decibel
// FUNCTION: LS3DF 0x10074a30
LS3D_RESULT IShow::SetVolume(float vol)
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (!pISndDriver->IsInit())
        vol = 0.0f;
    volume = vol;
    if (bink) {
        BinkSetVolume(bink, 0, (S32)(vol * 32768.0f));
        return I3D_OK;
    }
    int db = vol < MRG_ZERO ? DSBVOLUME_MIN : (int)(log(vol) * 866.0);
    if (db < DSBVOLUME_MIN)
        db = DSBVOLUME_MIN;
    if (flags & ISHOW_OGG)
        sound_buffer->SetVolume(db);
    else
        audio->put_Volume(db);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074ae0
int IShow::GetRegionCnt()
{
    return regions.size();
}

// FUNCTION: LS3DF 0x10074af0
S_show_region *IShow::GetRegions()
{
    Sleep(0);
    return &regions[0];
}

// Decodes the Bink video's next frame into the textures when it is time for it
// FUNCTION: LS3DF 0x10074b10
LS3D_RESULT IShow::ProcessBinkFrame()
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink && !BinkWait(bink)) {
        BinkDoFrame(bink);
        CopyFrameToAllTextures(this);
        if (bink->FrameNum == bink->Frames)
            Stop();
        else
            BinkNextFrame(bink);
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074b70
LS3D_RESULT IShow::RenderCurrentFrame()
{
    if (!init)
        return I3DERR_NOTINITIALIZED;
    if (bink && !BinkWait(bink)) {
        BinkDoFrame(bink);
        CopyFrameToAllTextures(this);
        if (bink->FrameNum == bink->Frames)
            Stop();
        else
            BinkNextFrame(bink);
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
    for (int i = regions.size(); i--;) {
        pGlobalIGraph->SetTexture(regions[i].texture);
        pGlobalIGraph->DrawPrimitiveList(PT_TRIANGLESTRIP, 2, regions[i].verts, LS3D_STREAM_TLVERTEX);
    }
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, ZBufferEnable);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10074d70
LS3D_RESULT IShow::RenderVideoUntilKeyPressOrEnd()
{
    if (!init)
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
    int frame = decoded_frame - 1;
    while (GetEvent() != 1) {
        pGlobalIGraph->ProcessWinMessages();
        pGlobalIGraph->UpdateKeyboardData();
        if (pGlobalIGraph->ReadKey()) {
            Stop();
            break;
        }
        if (bink) {
            pGlobalIGraph->Clear(0, 1.0f, 3);
            pGlobalIGraph->BeginScene();
            RenderCurrentFrame();
            pGlobalIGraph->EndScene();
            pGlobalIGraph->Present();
        } else if (frame != decoded_frame) {
            frame = decoded_frame;
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

    renderer = new CTextureRenderer(NULL, &hr);
    if (FAILED(hr)) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not create texture renderer object!  hr=0x%x", hr);
        return E_FAIL;
    }
    renderer->show = this;

    pFTR = renderer;
    if (FAILED(hr = graph->AddFilter(pFTR, L"TEXTURERENDERER"))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not add renderer filter to graph!  hr=0x%x", hr);
        return hr;
    }

    if (FAILED(hr = graph->AddSourceFilter(name, L"SOURCE", &pFSrc))) {
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

    if (FAILED(hr = graph->Connect(pFSrcPinOut, pFTRPinIn))) {
        dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not connect pins!  hr=0x%x", hr);
        return hr;
    }
    pFTRPinIn->Release();
    pFSrcPinOut->Release();
    pFSrc->Release();

    // The connection brings in the AVI Splitter between the two. A block of its own: the original gives its locals'
    // slots to the pin loop's afterwards. Not matching yet: the original stores pEnum = NULL after the call's pushes,
    // ours after the first push (pEnum declared before the Releases puts the call right but the store too early).
    {
        IEnumFilters *pEnum = NULL;
        graph->EnumFilters(&pEnum);
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
                (void **)&renderer->sound_renderer);
            if (renderer->sound_renderer) {
                hr = graph->AddFilter(renderer->sound_renderer, L"SOUNDRENDERER");
                IPin *pSndPinIn;
                if (FAILED(hr = renderer->sound_renderer->FindPin(L"Audio Input pin (rendered)", &pSndPinIn))) {
                    dbgPrintf("IShow::m_CreateCTextureRenderer() - ERROR! Could not find audio input pin!  hr=0x%x",
                        hr);
                    return hr;
                }
                if (FAILED(hr = graph->Connect(audio_pin, pSndPinIn))) {
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
    format = D3DFMT_UNKNOWN;
    sound_renderer = NULL;
}

// FUNCTION: LS3DF 0x10075400
CTextureRenderer::~CTextureRenderer()
{
    if (sound_renderer)
        sound_renderer->Release();
}

// Accepts only RGB video in the textures' format: 32 bits for a true color movie if the card has a true color texture
// format, else the 16-bit format of the card's textures (the sample accepts RGB24)
// FUNCTION: LS3DF 0x10075480
HRESULT CTextureRenderer::CheckMediaType(const CMediaType *pmt)
{
    HRESULT hr = E_FAIL;
    VIDEOINFO *pvi;

    if (*pmt->FormatType() != FORMAT_VideoInfo)
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();
    if (show->flags & ISHOW_TRUECOLOR && m_TrueColorTexturePixelFormat) {
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
    format = CreateRegions(m_lVidWidth, m_lVidHeight, show);
    return format ? S_OK : -1;
}

// Copies the sample's frame into the regions' textures, the video's rows bottom up, and counts it. One src for both
// formats, as the original's frame has. Open: the original gives pitch the dead pSample's slot and pBmpBuffer one of
// its own, ours the reverse.
// FUNCTION: LS3DF 0x100755c0
HRESULT CTextureRenderer::DoRenderSample(IMediaSample *pSample)
{
    BYTE *pBmpBuffer;
    BYTE *src;

    pSample->GetPointer(&pBmpBuffer);
    for (int i = show->regions.size(); i--;) {
        S_show_region &region = show->regions[i];
        uint pitch;
        void *bits = region.texture->Lock(NULL, pitch, 0);
        switch (format) {
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
                pGlobalIGraph->GetFormatName(format));
        }
        region.texture->Unlock();
        region.texture->AddDirtyRect(NULL);
    }
    show->decoded_frame++;
    return S_OK;
}
