#pragma once

// S3's S3TC library, which ITexture's CompressTexture compresses DXT textures with. The original links it prebuilt
// (0x100813e0-0x10086a09, before the DSOUND thunk), and so does the build: s3tc.lib holds the original's two objects,
// s3tc.obj and S3_quant.obj. This declares what the engine uses, as S3's s3_intrf.h does.

#include <ddraw.h>

// S3TCencode's dwEncodeType: the color encoding, ORed with the alpha encoding
#define S3TC_ENCODE_RGB_FULL 0x0
#define S3TC_ENCODE_RGB_ALPHA_COMPARE 0x2
#define S3TC_ENCODE_ALPHA_EXPLICIT 0x100

#ifdef __cplusplus
extern "C" {
#endif

// Encodes the surface lpSrc describes into lpDestBuf, weighting the color channels for the error
void __cdecl S3TCencode(DDSURFACEDESC *lpSrc, PALETTEENTRY *lpPal, DDSURFACEDESC *lpDest, void *lpDestBuf,
    unsigned int dwEncodeType, float *weight);

#ifdef __cplusplus
}
#endif
