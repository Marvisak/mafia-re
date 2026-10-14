#pragma once

// S3's S3TC library, which ITexture's CompressTexture compresses DXT textures with. Its code is linked into the
// original (0x100813e0-0x10086a09, before the DSOUND thunk); its SDK isn't public, so this declares what the engine
// uses, with the SDK's header name.

#include <ddraw.h>

// S3TCencode's dwEncodeType: the color encoding, ORed with the alpha encoding
#define S3TC_ENCODE_RGB_FULL 0x0
#define S3TC_ENCODE_RGB_ALPHA_COMPARE 0x2
#define S3TC_ENCODE_ALPHA_EXPLICIT 0x100

#ifdef __cplusplus
extern "C" {
#endif

// Encodes a surface described by lpDescIn into lpCodeBufferOut, weighting the color channels for the error
int __cdecl S3TCencode(DDSURFACEDESC *lpDescIn, PALETTEENTRY *lpDescInPal, DDSURFACEDESC *lpDescOut,
    void *lpCodeBufferOut, unsigned int dwEncodeType, float *weight);

#ifdef __cplusplus
}
#endif
