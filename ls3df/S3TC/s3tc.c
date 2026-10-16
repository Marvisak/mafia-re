// Stands in for S3's S3TC library, which the original links (0x100813e0-0x10086a09, the encoder and its
// floating-point helpers). Not reconstructed yet: only what ITexture.cpp needs to link.
#include "S3TC/s3_intrf.h"

// Its body differs from every other function's: a plain return 0 is the base classes' return NOERROR, which the
// linker folds it into (and the PDB then names it after one of them). CompressTexture ignores the result.
// STUB: LS3DF 0x100813e0
int __cdecl S3TCencode(DDSURFACEDESC *lpDescIn, PALETTEENTRY *lpDescInPal, DDSURFACEDESC *lpDescOut,
    void *lpCodeBufferOut, unsigned int dwEncodeType, float *weight)
{
    return (int)dwEncodeType;
}
