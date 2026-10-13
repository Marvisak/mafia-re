// 3D sounds, at 0x1007b370-0x1007b687 in the original (PS2 IS_3Dsound.cpp, same order). After the last function come
// the header's zero vector (see IS_driver.cpp) and the out-of-line ISND_sound::GetSource.
#include "isound.h"

// FUNCTION: LS3DF 0x1007b370
ISND_3Dsound::ISND_3Dsound() : is3d_init(false), lpDS3DBuffer(NULL)
{
}

// FUNCTION: LS3DF 0x1007b3b0
ISND_3Dsound::~ISND_3Dsound()
{
    Close();
}

// FUNCTION: LS3DF 0x1007b420
bool ISND_3Dsound::Is3DInit() const
{
    return is3d_init;
}

// FUNCTION: LS3DF 0x1007b430
LS3D_RESULT ISND_3Dsound::SetPos(const S_vector &pos)
{
    if (lpDS3DBuffer && FAILED(lpDS3DBuffer->SetPosition(pos.x, pos.y, pos.z, DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b470
LS3D_RESULT ISND_3Dsound::SetDir(const S_vector &dir)
{
    if (lpDS3DBuffer && FAILED(lpDS3DBuffer->SetConeOrientation(dir.x, dir.y, dir.z, DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b4b0
LS3D_RESULT ISND_3Dsound::SetVelocity(const S_vector &velocity)
{
    if (lpDS3DBuffer && FAILED(lpDS3DBuffer->SetVelocity(velocity.x, velocity.y, velocity.z, DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b4f0
LS3D_RESULT ISND_3Dsound::SetDistances(float min, float max)
{
    if (lpDS3DBuffer) {
        if (FAILED(lpDS3DBuffer->SetMinDistance(min, DS3D_IMMEDIATE)))
            return I3DERR_GENERIC;
        if (FAILED(lpDS3DBuffer->SetMaxDistance(max, DS3D_IMMEDIATE)))
            return I3DERR_GENERIC;
    }
    return I3D_OK;
}

// The angles are in radians, DirectSound's in degrees
// FUNCTION: LS3DF 0x1007b540
LS3D_RESULT ISND_3Dsound::SetCone(float inside, float outside)
{
    if (lpDS3DBuffer
        && FAILED(lpDS3DBuffer->SetConeAngles((DWORD)(inside * 57.295784f), (DWORD)(outside * 57.295784f),
            DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b590
LS3D_RESULT ISND_3Dsound::SetOutVolume(float volume)
{
    if (lpDS3DBuffer
        && FAILED(lpDS3DBuffer->SetConeOutsideVolume((LONG)((1.0f - volume) * -10000.0f), DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b5d0
LS3D_RESULT ISND_3Dsound::Open(ISND_source *source, uint flags)
{
    Close();
    LS3D_RESULT result = ISND_sound::Open(source, flags);
    if (result != I3D_OK)
        return result;
    if ((flags & ISND_3D) && FAILED(buffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&lpDS3DBuffer)))
        return I3DERR_GENERIC;
    is3d_init = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b630
LS3D_RESULT ISND_3Dsound::Close()
{
    is3d_init = false;
    if (lpDS3DBuffer) {
        lpDS3DBuffer->Release();
        lpDS3DBuffer = NULL;
    }
    ISND_sound::Close();
    return I3D_OK;
}
