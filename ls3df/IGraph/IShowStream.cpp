// PC only: IShow's music streams, through the sound driver's stream functions (ISND_STREAM_FUNCS). At
// 0x10075890-0x10075c10 in the original, between IShow.cpp and ITexture.cpp, so the file's name sorts between theirs;
// the name is a guess. The header's zero vector has its $E at the end (0x10075bf0).
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"

// The IShow objects with a stream open, which the callback looks through
// GLOBAL: LS3DF 0x101c5db8
// GLOBAL: LS3DF11 0x101c2eb8
C_list<IShow *> stream_shows;

// The driver tells the end of a stream (ISND_STREAM_END)
// FUNCTION: LS3DF 0x100758e0
// FUNCTION: LS3DF11 0x10072770
static void __stdcall StreamCallback(int stream, int event)
{
    switch (event) {
    case ISND_STREAM_END:
        for (int i = stream_shows.size(); i--;) {
            IShow *show = stream_shows[i];
            if (stream == show->m_Stream)
                show->m_StreamDone = true;
        }
        break;
    }
}

// FUNCTION: LS3DF 0x10075920
// FUNCTION: LS3DF11 0x100727b0
LS3D_RESULT IShow::OpenStream(const char *name, int unused)
{
    CloseStream();
    if (FAILED(pISndDriver->streams->Init(0)))
        return I3DERR_UNSUPPORTED;
    m_StreamInit = true;
    if (FAILED(pISndDriver->streams->Open(0, &m_Stream, StreamCallback, name, 0)))
        return I3DERR_NOFILE;
    // Added once. The pointer and the count are set before the size is tested: assigned after it, they take other
    // registers.
    bool found = false;
    IShow **p = &stream_shows[0];
    int i = stream_shows.size();
    if (stream_shows.size()) {
        while (i--) {
            if (*p == this) {
                found = true;
                break;
            }
            p++;
        }
    }
    if (!found)
        stream_shows.push_back(this);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10075a20
// FUNCTION: LS3DF11 0x100728b0
void IShow::CloseStream()
{
    m_StreamPaused = false;
    m_StreamDone = false;
    stream_shows.remove(this);
    if (m_Stream != -1) {
        StopStream();
        if (FAILED(pISndDriver->streams->CloseStream(m_Stream)))
            return;
        m_Stream = -1;
    }
    if (m_StreamInit) {
        if (SUCCEEDED(pISndDriver->streams->Close(0)))
            m_StreamInit = false;
    }
}

// FUNCTION: LS3DF 0x10075ad0
// FUNCTION: LS3DF11 0x10072960
LS3D_RESULT IShow::PlayStream(bool loop)
{
    if (m_StreamPaused) {
        m_StreamPaused = false;
        if (FAILED(pISndDriver->streams->Resume(m_Stream)))
            return I3DERR_GENERIC;
    } else if (FAILED(pISndDriver->streams->Play(m_Stream, loop)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10075b30
// FUNCTION: LS3DF11 0x100729c0
LS3D_RESULT IShow::StopStream()
{
    return FAILED(pISndDriver->streams->Stop(m_Stream)) ? I3DERR_GENERIC : I3D_OK;
}

// FUNCTION: LS3DF 0x10075b60
// FUNCTION: LS3DF11 0x100729f0
LS3D_RESULT IShow::PauseStream()
{
    if (FAILED(pISndDriver->streams->Pause(m_Stream)))
        return I3DERR_GENERIC;
    m_StreamPaused = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10075b90
// FUNCTION: LS3DF11 0x10072a20
LS3D_RESULT IShow::SetStreamVolume(int volume)
{
    return FAILED(pISndDriver->streams->SetVolume(m_Stream, 0, volume)) ? I3DERR_GENERIC : I3D_OK;
}

// FUNCTION: LS3DF 0x10075bc0
// FUNCTION: LS3DF11 0x10072a50
LS3D_RESULT IShow::SetStreamPan(int pan)
{
    return FAILED(pISndDriver->streams->SetPan(m_Stream, 0, pan)) ? I3DERR_GENERIC : I3D_OK;
}
