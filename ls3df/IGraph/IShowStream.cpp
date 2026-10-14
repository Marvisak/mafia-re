// PC only: IShow's music streams, through the sound driver's stream functions (ISND_STREAM_FUNCS). At
// 0x10075890-0x10075c10 in the original, between IShow.cpp and ITexture.cpp, so the file's name sorts between theirs;
// the name is a guess. The header's zero vector has its $E at the end (0x10075bf0).
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"

// The IShow objects with a stream open, which the callback looks through
// GLOBAL: LS3DF 0x101c5db8
C_list<IShow *> stream_shows;

// The driver tells the end of a stream (event 1)
// FUNCTION: LS3DF 0x100758e0
static void __stdcall StreamCallback(int stream, int event)
{
    switch (event) {
    case 1:
        for (int i = stream_shows.size(); i--;) {
            IShow *show = stream_shows[i];
            if (stream == show->stream)
                show->stream_done = true;
        }
        break;
    }
}

// FUNCTION: LS3DF 0x10075920
LS3D_RESULT IShow::OpenStream(const char *name, int unused)
{
    CloseStream();
    if (FAILED(pISndDriver->streams->Init(0)))
        return I3DERR_UNSUPPORTED;
    stream_init = true;
    if (FAILED(pISndDriver->streams->Open(0, &stream, StreamCallback, name, 0)))
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
void IShow::CloseStream()
{
    stream_paused = false;
    stream_done = false;
    stream_shows.remove(this);
    if (stream != -1) {
        StopStream();
        if (FAILED(pISndDriver->streams->CloseStream(stream)))
            return;
        stream = -1;
    }
    if (stream_init) {
        if (SUCCEEDED(pISndDriver->streams->Close(0)))
            stream_init = false;
    }
}

// FUNCTION: LS3DF 0x10075ad0
LS3D_RESULT IShow::PlayStream(bool loop)
{
    if (stream_paused) {
        stream_paused = false;
        if (FAILED(pISndDriver->streams->Resume(stream)))
            return I3DERR_GENERIC;
    } else if (FAILED(pISndDriver->streams->Play(stream, loop)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10075b30
LS3D_RESULT IShow::StopStream()
{
    return FAILED(pISndDriver->streams->Stop(stream)) ? I3DERR_GENERIC : I3D_OK;
}

// FUNCTION: LS3DF 0x10075b60
LS3D_RESULT IShow::PauseStream()
{
    if (FAILED(pISndDriver->streams->Pause(stream)))
        return I3DERR_GENERIC;
    stream_paused = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10075b90
LS3D_RESULT IShow::SetStreamVolume(int volume)
{
    return FAILED(pISndDriver->streams->SetVolume(stream, 0, volume)) ? I3DERR_GENERIC : I3D_OK;
}

// FUNCTION: LS3DF 0x10075bc0
LS3D_RESULT IShow::SetStreamPan(int pan)
{
    return FAILED(pISndDriver->streams->SetPan(stream, 0, pan)) ? I3DERR_GENERIC : I3D_OK;
}
