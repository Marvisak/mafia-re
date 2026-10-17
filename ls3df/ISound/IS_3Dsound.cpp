// 3D sounds, at 0x1007b370-0x1007b687 in the original (PS2 IS_3Dsound.cpp, same order). After the last function come
// the header's zero vector (see IS_driver.cpp) and the out-of-line ISND_sound::GetSource.
#include "ISound/ISound.h"

// FUNCTION: LS3DF 0x1007b370
ISND_3Dsound::ISND_3Dsound() : is3d_init(false), lpDSB3D8(NULL)
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
    if (lpDSB3D8 && FAILED(lpDSB3D8->SetPosition(pos.x, pos.y, pos.z, DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b470
LS3D_RESULT ISND_3Dsound::SetDir(const S_vector &dir)
{
    if (lpDSB3D8 && FAILED(lpDSB3D8->SetConeOrientation(dir.x, dir.y, dir.z, DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b4b0
LS3D_RESULT ISND_3Dsound::SetVelocity(const S_vector &velocity)
{
    if (lpDSB3D8 && FAILED(lpDSB3D8->SetVelocity(velocity.x, velocity.y, velocity.z, DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b4f0
LS3D_RESULT ISND_3Dsound::SetDistances(float min, float max)
{
    if (lpDSB3D8) {
        if (FAILED(lpDSB3D8->SetMinDistance(min, DS3D_IMMEDIATE)))
            return I3DERR_GENERIC;
        if (FAILED(lpDSB3D8->SetMaxDistance(max, DS3D_IMMEDIATE)))
            return I3DERR_GENERIC;
    }
    return I3D_OK;
}

// The angles are in radians, DirectSound's in degrees
// FUNCTION: LS3DF 0x1007b540
LS3D_RESULT ISND_3Dsound::SetCone(float in_angle, float out_angle)
{
    if (lpDSB3D8
        && FAILED(lpDSB3D8->SetConeAngles((DWORD)(in_angle * 57.295784f), (DWORD)(out_angle * 57.295784f),
            DS3D_IMMEDIATE)))
        return I3DERR_GENERIC;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b590
LS3D_RESULT ISND_3Dsound::SetOutVolume(float out_vol)
{
    if (lpDSB3D8
        && FAILED(lpDSB3D8->SetConeOutsideVolume((LONG)((1.0f - out_vol) * -10000.0f), DS3D_IMMEDIATE)))
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
    if ((flags & ISND_3DBUFFER) && FAILED(lpDSB8->QueryInterface(IID_IDirectSound3DBuffer, (void **)&lpDSB3D8)))
        return I3DERR_GENERIC;
    is3d_init = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b630
LS3D_RESULT ISND_3Dsound::Close()
{
    is3d_init = false;
    if (lpDSB3D8) {
        lpDSB3D8->Release();
        lpDSB3D8 = NULL;
    }
    ISND_sound::Close();
    return I3D_OK;
}
