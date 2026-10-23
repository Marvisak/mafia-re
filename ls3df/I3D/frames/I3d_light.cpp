// The lights (PS2 I3d_light.cpp), at 0x100201f0-0x10021570 in the original: I3D_light in PS2's order, without PS2's
// specular, ambient and attenuation setters and SetFogRange. The header's zero vector has its $E at the end
// (0x10021550). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "dta.h"

// <new> declares operator delete throw(), so VC6 sets no EH state before the sector list's destructor in the light's
#include <new>

// Computes what UpdateV and the setters would, for a directional light shining down at an angle.
// Not matching yet: the inline budget. The original inlines the Normalize calls of GetDir and GetWorldDir, ours leaves
// both out of line. About 27 dead statements' worth of source more (if (0) { power = 1.0f; ... } at the top) gives
// the original's code exactly, so its source had more budget here or fewer inline calls after GetDir (the color's
// constructor, the bound sphere's four). Neither moving the color, nor the frame's inline functions spelled with
// UpdateWMatrix(), nor GetWorldDir's body written out changes it; GetDir's written out inlines both, but with a local
// instead of the return value the branches end differently.
// FUNCTION: LS3DF 0x100201f0
// FUNCTION: LS3DF11 0x1001fd20
I3D_light::I3D_light()
{
    diffuse = S_vector(1.0f, 1.0f, 1.0f);
    m_Type = FRAME_LIGHT;
    ltype = I3DLIGHT_DIRECTIONAL;
    power = 1.0f;
    near_range = 1.0f;
    far_range = 100.0f;
    SetDir(S_vector(1.0f, -1.0f, 1.0f), 0.0f);
    cone_in = 20.0f * PI / 180.0f;
    cone_out = 40.0f * PI / 180.0f;
    // Every mode but 0x4, 0x10 and 0x80
    mode = 0xff6b;
    w_pos = S_vector(0.0f, 0.0f, 0.0f);
    w_dir = GetDir();
    range_n = near_range * m_WorldScale;
    color = S_vector(1.0f, 1.0f, 1.0f);
    range_f = far_range * m_WorldScale;
    range_f_2 = range_f * range_f;
    range_delta = range_f - range_n;
    outer_cos = (float)cos(cone_out * .5);
    inner_cos = (float)cos(cone_in * .5);
    float d = inner_cos - outer_cos;
    if (d > 0.0f)
        cone_scale = 1.0f / d;
    else
        cone_scale = 0.0f;
    // PrepareLights adds it to the key of the lights it lit with
    m_UpdateCount = rand();
    cone_radius = far_range / (2.0f * sin((PI - cone_out * .5f) * .5f));
    bsphere.radius = cone_radius * m_WorldScale;
    bsphere.pos = GetWorldPos() + GetWorldDir() * bsphere.radius;
    Update();
}

// FUNCTION: LS3DF 0x10020760
// FUNCTION: LS3DF11 0x10020290
I3D_light::~I3D_light()
{
    I3D_sector **p = &sectors[0];
    for (int i = sectors.size(); i--; p++)
        (*p)->DeleteLight(this);
}

// The fog types are their sectors' fog_light (UpdateV): a light becoming one or ceasing to be one is taken out of its
// sectors and put back
// FUNCTION: LS3DF 0x10020800
// FUNCTION: LS3DF11 0x10020330
void I3D_light::SetLightType(I3D_LIGHTTYPE type)
{
    if (ltype == type)
        return;
    m_UpdateCount++;
    if (ltype != I3DLIGHT_FOG && ltype != I3DLIGHT_POINTAMBIENT && ltype != I3DLIGHT_POINTFOG) {
        ltype = type;
        if (type != I3DLIGHT_FOG && type != I3DLIGHT_POINTAMBIENT && type != I3DLIGHT_POINTFOG) {
            for (int i = sectors.size(); i--; )
                sectors[i]->lights_changed++;
        } else {
            for (int i = sectors.size(); i--; )
                sectors[i]->AddLight(this);
        }
        if (ltype == I3DLIGHT_SPOT)
            cone_radius = far_range / (2.0f * sin((PI - cone_out * .5f) * .5f));
    } else {
        int i;
        for (i = sectors.size(); i--; )
            sectors[i]->DeleteLight(this);
        ltype = type;
        for (i = sectors.size(); i--; )
            sectors[i]->AddLight(this);
    }
}

// FUNCTION: LS3DF 0x10020930
// FUNCTION: LS3DF11 0x10020460
void I3D_light::SetDiffuse(const S_vector &c)
{
    diffuse = c;
    color = diffuse * power;
    for (int i = sectors.size(); i--; )
        sectors[i]->lights_changed++;
    m_UpdateCount++;
}

// FUNCTION: LS3DF 0x100209e0
// FUNCTION: LS3DF11 0x10020510
void I3D_light::SetDiffuse(float r, float g, float b)
{
    diffuse.x = r;
    diffuse.y = g;
    diffuse.z = b;
    color = diffuse * power;
    for (int i = sectors.size(); i--; )
        sectors[i]->lights_changed++;
    m_UpdateCount++;
}

// FUNCTION: LS3DF 0x10020a90
// FUNCTION: LS3DF11 0x100205c0
void I3D_light::SetDiffusePower(float p)
{
    power = p;
    color = diffuse * power;
    for (int i = sectors.size(); i--; )
        sectors[i]->lights_changed++;
    m_UpdateCount++;
}

// The fog types' ranges are in percent
// FUNCTION: LS3DF 0x10020b20
// FUNCTION: LS3DF11 0x10020650
void I3D_light::SetRange(float near1, float far1)
{
    near_range = near1;
    far_range = far1 > near1 ? far1 : near1;
    if (ltype != I3DLIGHT_FOG && ltype != I3DLIGHT_POINTAMBIENT && ltype != I3DLIGHT_POINTFOG) {
        range_n = near_range * m_WorldScale;
        range_f = far_range * m_WorldScale;
        range_f_2 = range_f * range_f;
        range_delta = range_f - range_n;
    } else {
        range_n = near_range * .01f;
        range_f = far_range * .01f;
    }
    for (int i = sectors.size(); i--; )
        sectors[i]->lights_changed++;
    m_UpdateCount++;
    if (ltype == I3DLIGHT_SPOT)
        cone_radius = far_range / (2.0f * sin((PI - cone_out * .5f) * .5f));
}

// FUNCTION: LS3DF 0x10020c30
// FUNCTION: LS3DF11 0x10020760
void I3D_light::SetCone(float inner, float outer)
{
    cone_in = inner;
    cone_out = outer;
    outer_cos = (float)cos(cone_out * .5);
    inner_cos = (float)cos(cone_in * .5);
    float d = inner_cos - outer_cos;
    if (d > 0.0f)
        cone_scale = 1.0f / d;
    else
        cone_scale = 0.0f;
    for (int i = sectors.size(); i--; )
        sectors[i]->lights_changed++;
    m_UpdateCount++;
    if (ltype == I3DLIGHT_SPOT)
        cone_radius = far_range / (2.0f * sin((PI - cone_out * .5f) * .5f));
}

// A fog light that is on becomes its sectors' fog_light; one turned off gives the place to another fog light of the
// sector that is on. The lists are read through their members, not size() and operator[]: those are inline calls,
// and with them after the GetWorldDir calls VC6 leaves the Normalize calls in those out of line (with them only in the
// first loop, it orders the directional light's sum of squares differently).
// FUNCTION: LS3DF 0x10020d10
// FUNCTION: LS3DF11 0x10020840
void I3D_light::UpdateV()
{
    m_UpdateCount++;
    for (int i = 0; i < sectors.used_size; i++)
        sectors.array[i]->lights_changed++;
    switch (ltype) {
    case I3DLIGHT_POINT:
        w_pos = GetWorldPos();
        range_n = near_range * m_WorldScale;
        range_f = far_range * m_WorldScale;
        range_f_2 = range_f * range_f;
        range_delta = range_f - range_n;
        break;
    case I3DLIGHT_SPOT:
        {
            w_pos = GetWorldPos();
            w_dir = GetWorldDir();
            range_n = near_range * m_WorldScale;
            range_f = far_range * m_WorldScale;
            range_f_2 = range_f * range_f;
            range_delta = range_f - range_n;
            outer_cos = (float)cos(cone_out * .5);
            inner_cos = (float)cos(cone_in * .5);
            float d = inner_cos - outer_cos;
            if (d > 0.0f)
                cone_scale = 1.0f / d;
            else
                cone_scale = 0.0f;
            bsphere.radius = cone_radius * m_WorldScale;
            bsphere.pos = GetWorldPos() + GetWorldDir() * bsphere.radius;
        }
        break;
    case I3DLIGHT_DIRECTIONAL:
        w_dir = GetWorldDir();
        break;
    case I3DLIGHT_FOG:
    case I3DLIGHT_POINTAMBIENT:
    case I3DLIGHT_POINTFOG:
        range_n = near_range * .01;
        range_f = far_range * .01;
        if (m_FrmFlags & FRMFLAGS_ON) {
            for (int i = 0; i < sectors.used_size; i++)
                sectors.array[i]->fog_light = this;
        } else {
            for (int i = 0; i < sectors.used_size; i++) {
                I3D_sector *s = sectors.array[i];
                if (s->fog_light == this) {
                    s->fog_light = NULL;
                    I3D_light **lp = s->lights.array;
                    for (int j = s->lights.used_size; j--; lp++) {
                        if (((*lp)->m_FrmFlags & FRMFLAGS_ON) && ((*lp)->ltype == I3DLIGHT_FOG ||
                            (*lp)->ltype == I3DLIGHT_POINTAMBIENT || (*lp)->ltype == I3DLIGHT_POINTFOG)) {
                            s->fog_light = *lp;
                            break;
                        }
                    }
                }
            }
        }
        break;
    }
}

// FUNCTION: LS3DF 0x10021400
// FUNCTION: LS3DF11 0x10020f30
void I3D_light::SetOn(bool on)
{
    if (on) {
        if (m_FrmFlags & FRMFLAGS_ON)
            return;
        m_FrmFlags |= FRMFLAGS_ON;
    } else {
        if (!(m_FrmFlags & FRMFLAGS_ON))
            return;
        m_FrmFlags &= ~FRMFLAGS_ON;
    }
    UpdateV();
}

// FUNCTION: LS3DF 0x10021440
// FUNCTION: LS3DF11 0x10020f70
void I3D_light::SetMode(uint m)
{
    if (mode == m)
        return;
    mode = m;
    for (int i = sectors.size(); i--; )
        sectors[i]->lights_changed++;
    m_UpdateCount++;
}

// FUNCTION: LS3DF 0x10021490
// FUNCTION: LS3DF11 0x10020fc0
LS3D_RESULT I3D_light::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_LIGHT)
        return I3D_OK;
    I3D_light *lt = (I3D_light *)frame;
    diffuse = lt->diffuse;
    ltype = lt->ltype;
    power = lt->power;
    near_range = lt->near_range;
    far_range = lt->far_range;
    cone_in = lt->cone_in;
    cone_out = lt->cone_out;
    UpdateV();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10021530
// FUNCTION: LS3DF11 0x10021060
void I3D_light::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &mode, 0x28);
}
