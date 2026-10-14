#pragma once

// What IShow.cpp uses of the Xiph.Org Foundation's vorbisfile.dll (libvorbis 1.0), with libvorbis' names. The engine
// calls the functions through the import library's thunks (no dllimport). They are __cdecl.

typedef __int64 ogg_int64_t;

typedef struct vorbis_info {
    int version;
    int channels;
    long rate;
    long bitrate_upper;
    long bitrate_nominal;
    long bitrate_lower;
    long bitrate_window;
    void *codec_setup;
} vorbis_info;

// The decoder's state, which the engine allocates; its members are vorbisfile's own
typedef struct OggVorbis_File {
    char opaque[0x2d0];
} OggVorbis_File;

// How vorbisfile reads the stream
typedef struct {
    size_t(__cdecl *read_func)(void *ptr, size_t size, size_t nmemb, void *datasource);
    int(__cdecl *seek_func)(void *datasource, ogg_int64_t offset, int whence);
    int(__cdecl *close_func)(void *datasource);
    long(__cdecl *tell_func)(void *datasource);
} ov_callbacks;

extern "C" {
int __cdecl ov_clear(OggVorbis_File *vf);
int __cdecl ov_open_callbacks(void *datasource, OggVorbis_File *vf, char *initial, long ibytes,
    ov_callbacks callbacks);
vorbis_info *__cdecl ov_info(OggVorbis_File *vf, int link);
long __cdecl ov_read(OggVorbis_File *vf, char *buffer, int length, int bigendianp, int word, int sgned,
    int *bitstream);
int __cdecl ov_time_seek(OggVorbis_File *vf, double pos);
double __cdecl ov_time_tell(OggVorbis_File *vf);
}
