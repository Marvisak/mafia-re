// The 3D listener, at 0x1007b060-0x1007b36e in the original (PS2 IS_3Dlistener.cpp, same order). Its settings live in
// a DS3DLISTENER that Update hands to DirectSound; the EAX properties go to the driver's IKsPropertySet.
#include "ISound/ISound.h"
#include "ISound/eax.h"

// FUNCTION: LS3DF 0x1007b060
ISND_3Dlistener::ISND_3Dlistener()
{
    ref = 1;
    memset(&ds_data, 0, sizeof(ds_data));
    ds_data.flDistanceFactor = 1.0f;
    ds_data.flDopplerFactor = 1.0f;
    ds_data.flRolloffFactor = 1.0f;
    ds_data.dwSize = sizeof(ds_data);
}

// FUNCTION: LS3DF 0x1007b0a0
ISND_3Dlistener::~ISND_3Dlistener()
{
}

// FUNCTION: LS3DF 0x1007b0b0
int ISND_3Dlistener::AddRef()
{
    return ++ref;
}

// FUNCTION: LS3DF 0x1007b0c0
int ISND_3Dlistener::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount<0  this:0x%x  ISND_3Dlistener", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// FUNCTION: LS3DF 0x1007b100
LS3D_RESULT ISND_3Dlistener::SetPos(const S_vector &pos)
{
    ds_data.vPosition.x = pos.x;
    ds_data.vPosition.y = pos.y;
    ds_data.vPosition.z = pos.z;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b120
LS3D_RESULT ISND_3Dlistener::SetDir(const S_vector &dir, const S_vector &up)
{
    ds_data.vOrientFront.x = dir.x;
    ds_data.vOrientFront.y = dir.y;
    ds_data.vOrientFront.z = dir.z;
    ds_data.vOrientTop.x = up.x;
    ds_data.vOrientTop.y = up.y;
    ds_data.vOrientTop.z = up.z;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b160
LS3D_RESULT ISND_3Dlistener::SetVelocity(const S_vector &velocity)
{
    ds_data.vVelocity.x = velocity.x;
    ds_data.vVelocity.y = velocity.y;
    ds_data.vVelocity.z = velocity.z;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b180
LS3D_RESULT ISND_3Dlistener::SetDistanceFactor(float factor)
{
    ds_data.flDistanceFactor = factor;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b190
LS3D_RESULT ISND_3Dlistener::SetDopplerFactor(float factor)
{
    ds_data.flDopplerFactor = factor;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b1a0
LS3D_RESULT ISND_3Dlistener::SetRolloffFactor(float factor)
{
    ds_data.flRolloffFactor = factor;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b1b0
LS3D_RESULT ISND_3Dlistener::Update()
{
    return SUCCEEDED(pISndDriver->lpListener->SetAllParameters(&ds_data, DS3D_IMMEDIATE)) ? I3D_OK : I3DERR_GENERIC;
}

// Set takes the property's address: a scalar value is copied into a static for it, a vector is passed by address in
// value. The integer and float cases are separate groups, whose identical calls VC6 merges into one body before it
// builds the jump table (one group gives the same code with other registers: fewer references to property). Any split
// into two or more groups before the vectors gives the original; vectors before a scalar group put their body first.
// FUNCTION: LS3DF 0x1007b1e0
LS3D_RESULT ISND_3Dlistener::SetEAXProperty(int property, uint value)
{
    if (!pISndDriver->eax_property_set || !(pISndDriver->create_flags & ISNDINIT_EAX30))
        return I3DERR_UNSUPPORTED;
    // GLOBAL: LS3DF 0x101c62c0
    static uint data;
    data = value;
    switch (property) {
    case DSPROPERTY_EAXLISTENER_ENVIRONMENT:
    case DSPROPERTY_EAXLISTENER_ROOM:
    case DSPROPERTY_EAXLISTENER_ROOMHF:
    case DSPROPERTY_EAXLISTENER_ROOMLF:
    case DSPROPERTY_EAXLISTENER_REFLECTIONS:
    case DSPROPERTY_EAXLISTENER_REVERB:
    case DSPROPERTY_EAXLISTENER_FLAGS:
        pISndDriver->eax_property_set->Set(DSPROPSETID_EAX30_ListenerProperties, property, NULL, 0, &data,
            sizeof(long));
        break;
    case DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE:
    case DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION:
    case DSPROPERTY_EAXLISTENER_DECAYTIME:
    case DSPROPERTY_EAXLISTENER_DECAYHFRATIO:
    case DSPROPERTY_EAXLISTENER_DECAYLFRATIO:
    case DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY:
    case DSPROPERTY_EAXLISTENER_REVERBDELAY:
    case DSPROPERTY_EAXLISTENER_ECHOTIME:
    case DSPROPERTY_EAXLISTENER_ECHODEPTH:
    case DSPROPERTY_EAXLISTENER_MODULATIONTIME:
    case DSPROPERTY_EAXLISTENER_MODULATIONDEPTH:
    case DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF:
    case DSPROPERTY_EAXLISTENER_HFREFERENCE:
    case DSPROPERTY_EAXLISTENER_LFREFERENCE:
    case DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR:
        pISndDriver->eax_property_set->Set(DSPROPSETID_EAX30_ListenerProperties, property, NULL, 0, &data,
            sizeof(float));
        break;
    case DSPROPERTY_EAXLISTENER_REFLECTIONSPAN:
    case DSPROPERTY_EAXLISTENER_REVERBPAN:
        pISndDriver->eax_property_set->Set(DSPROPSETID_EAX30_ListenerProperties, property, NULL, 0, (void *)value,
            sizeof(EAXVECTOR));
        break;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b280
LS3D_RESULT ISND_3Dlistener::SetEAX3Property(uint property, void *data, uint size)
{
    if (!pISndDriver->eax_property_set || !(pISndDriver->create_flags & ISNDINIT_EAX30))
        return I3DERR_UNSUPPORTED;
    pISndDriver->eax_property_set->Set(DSPROPSETID_EAX30_ListenerProperties, property, NULL, 0, data, size);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b2d0
LS3D_RESULT ISND_3Dlistener::GetEAX3Property(uint property, void *data, uint size, uint *returned)
{
    if (!pISndDriver->eax_property_set || !(pISndDriver->create_flags & ISNDINIT_EAX30))
        return I3DERR_UNSUPPORTED;
    pISndDriver->eax_property_set->Get(DSPROPSETID_EAX30_ListenerProperties, property, NULL, 0, data, size,
        (ULONG *)returned);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1007b320
LS3D_RESULT ISND_3Dlistener::CommitDeferredSettings()
{
    return SUCCEEDED(pISndDriver->lpListener->CommitDeferredSettings()) ? I3D_OK : I3DERR_GENERIC;
}
