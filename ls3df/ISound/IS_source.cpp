// Sound sources, at 0x1007d430-0x1007d9ce in the original (PS2 IS_source.cpp, same order): a WAV file read into a
// DirectSound buffer, which the sounds playing it duplicate. After the last function comes the header's zero vector
// (see IS_driver.cpp).
#include "ISound/ISound.h"
#include "dta.h"

// The chunks of a WAV file
struct S_riff_header {
    char id[4];                         // "RIFF"
    uint size;
    char type[4];                       // "WAVE"
};

struct S_chunk_header {
    char id[4];
    uint size;
};

struct S_fmt_chunk {
    S_chunk_header header;              // "fmt "
    PCMWAVEFORMAT format;
};

// FUNCTION: LS3DF 0x1007d430
ISND_source::ISND_source() : buffer(NULL)
{
    ref = 1;
    Close();
}

// FUNCTION: LS3DF 0x1007d480
ISND_source::~ISND_source()
{
    Close();
    pISndDriver->sources.remove(this);
}

// FUNCTION: LS3DF 0x1007d550
int ISND_source::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount<0  this:0x%x  ISND_source", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// FUNCTION: LS3DF 0x1007d590
LS3D_RESULT ISND_source::Open(const char *name, uint flags)
{
    Close();
    if (!pISndDriver->IsInit())
        return I3DERR_NOTINITIALIZED;
    open_flags = flags;
    int h = dtaOpen(name, false);
    if (h == -1) {
        dbgPrintf("ERROR!! IS_source::Open() - File not found ! %s", name);
        return I3DERR_NOFILE;
    }
    S_riff_header riff;
    dtaRead(h, &riff, sizeof(riff));
    S_fmt_chunk fmt;
    dtaRead(h, &fmt, sizeof(fmt));
    if (fmt.header.size > sizeof(PCMWAVEFORMAT))
        dtaSeek(h, fmt.header.size - sizeof(PCMWAVEFORMAT), FILE_CURRENT);
    wave_format.wFormatTag = fmt.format.wf.wFormatTag;
    wave_format.nChannels = fmt.format.wf.nChannels;
    wave_format.nSamplesPerSec = fmt.format.wf.nSamplesPerSec;
    wave_format.nAvgBytesPerSec = fmt.format.wf.nAvgBytesPerSec;
    wave_format.nBlockAlign = fmt.format.wf.nBlockAlign;
    wave_format.wBitsPerSample = fmt.format.wBitsPerSample;
    S_chunk_header chunk;
    dtaRead(h, &chunk, sizeof(chunk));
    while (strncmp(chunk.id, "data", 4)) {
        dtaSeek(h, chunk.size, FILE_CURRENT);
        dtaRead(h, &chunk, sizeof(chunk));
    }
    format.bytes_per_sample = fmt.format.wBitsPerSample / 8;
    format.num_channels = fmt.format.wf.nChannels;
    format.samples_per_sec = fmt.format.wf.nSamplesPerSec;
    format.size = chunk.size;
    bytes_per_ms = wave_format.nAvgBytesPerSec * 0.001f;
    // DirectSound positions only mono sounds in 3D
    if (wave_format.nChannels > 1)
        open_flags &= ~ISND_3DBUFFER;

    DSBUFFERDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | (open_flags & ISND_3DBUFFER ? DSBCAPS_CTRL3D : 0);
    if (!(pISndDriver->create_flags & ISNDINIT_NOHWCHANNELS) && !(flags & ISND_SYSMEM))
        desc.dwFlags |= DSBCAPS_LOCDEFER;
    else
        desc.dwFlags |= DSBCAPS_LOCSOFTWARE;
    desc.dwBufferBytes = chunk.size;
    desc.lpwfxFormat = &wave_format;
    if (desc.dwFlags & DSBCAPS_CTRL3D)
        desc.guid3DAlgorithm = DS3DALG_NO_VIRTUALIZATION;
    IDirectSoundBuffer *dsb;
    if (FAILED(pISndDriver->lpDS8->CreateSoundBuffer(&desc, &dsb, NULL))) {
        dtaClose(h);
        return I3DERR_GENERIC;
    }
    if (FAILED(dsb->QueryInterface(IID_IDirectSoundBuffer8, (void **)&buffer))) {
        dtaClose(h);
        return I3DERR_GENERIC;
    }
    dsb->Release();

    void *data;
    DWORD size;
    if (FAILED(buffer->Lock(0, 0, &data, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER))) {
        dtaClose(h);
        return I3DERR_GENERIC;
    }
    if (dtaRead(h, data, format.size) != format.size) {
        dtaClose(h);
        return I3DERR_GENERIC;
    }
    if (FAILED(buffer->Unlock(data, size, NULL, 0))) {
        dtaClose(h);
        return I3DERR_GENERIC;
    }
    dtaClose(h);
    filename = name;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d890
void ISND_source::Close()
{
    if (buffer) {
        buffer->Release();
        buffer = NULL;
    }
    memset(&format, 0, sizeof(format));
    bytes_per_ms = 0.0f;
}

// FUNCTION: LS3DF 0x1007d8c0
bool ISND_source::IsOpen() const
{
    return format.size > 0 ? true : false;
}

// In milliseconds
// FUNCTION: LS3DF 0x1007d8d0
uint ISND_source::GetPlayTime() const
{
    if (!format.size)
        return 0;
    return (uint)(format.size / bytes_per_ms);
}

// FUNCTION: LS3DF 0x1007d910
LS3D_RESULT ISND_source::GetFormat(S_wave_format *wf)
{
    *wf = format;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d940
LS3D_RESULT ISND_source::Lock(void **data)
{
    if (FAILED(buffer->Lock(0, 0, &save_lock_ptr1, &save_lick_size1, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
        return I3DERR_GENERIC;
    *data = save_lock_ptr1;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007d980
LS3D_RESULT ISND_source::Unlock()
{
    return SUCCEEDED(buffer->Unlock(save_lock_ptr1, save_lick_size1, NULL, 0)) ? I3D_OK : I3DERR_GENERIC;
}
