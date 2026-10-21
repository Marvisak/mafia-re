// The materials (PS2 I3d_material.cpp), at 0x1002ade0-0x1002cda0 in the original: I3D_material from its constructor,
// then I3D_anim_material from its constructor, each in PS2's order with the PC's additions (Set2Sided, SetEnvScale), and
// the two LoadImage functions last, as on PS2. The header's zero vector has its $E after them (0x1002c8a0), followed
// by std::vector's out-of-line functions and the material's in-class IsCkey and IsAnimated, which its vtable needs.
// Names are the PS2 build's unless noted.
// The PC's animated material keeps its frames in std::vectors where PS2 has C_lists, and its UpdateTexture counts the
// time against a frame delay in its own way; the 4ds data and the texture flags LoadImage makes are the same.
// Two of the class's functions are folded with other files' copies, which the original kept: SetEnvAlpha with
// ISND_3Dlistener::SetDistanceFactor, the in-class UpdateTexture and UpdateEnvTexture with the scene file's Update.
// Open: I3D_anim_material::Duplicate (see there), and so the original's out-of-line vector::insert(P, x)
// (0x1002c900), which only its first push_back calls. Our IsCkey and IsAnimated come between vector::size and erase,
// the original's after std::copy.
#include "I3D/mesh/I3d_material.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// I3d_driver.cpp's
extern int driverTickTime;

// FUNCTION: LS3DF 0x1002ade0
I3D_material::I3D_material()
{
    ref = 1;
    flags = 1;
    texture = NULL;
    env_texture = NULL;
    ambient = diffuse = S_vector(1.0f, 1.0f, 1.0f);
    emissive = S_vector(0.0f, 0.0f, 0.0f);
    alpha = 1.0f;
    env_alpha = 1.0f;
}

// FUNCTION: LS3DF 0x1002aec0
I3D_material::~I3D_material()
{
    if (texture)
        texture->Release();
    if (env_texture)
        env_texture->Release();
}

// FUNCTION: LS3DF 0x1002aef0
int I3D_material::Release()
{
    if (--ref < 0) {
        dbgPrintf("!! RefCount<0  this:0x%x  (I3D_material)", this);
        return 0;
    }
    if (!ref) {
        delete this;
        return 0;
    }
    return ref;
}

// FUNCTION: LS3DF 0x1002af20
LS3D_RESULT I3D_material::SetAmbient(const S_vector &color)
{
    ambient = color;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002af40
LS3D_RESULT I3D_material::SetDiffuse(const S_vector &color)
{
    diffuse = color;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002af60
LS3D_RESULT I3D_material::SetEmissive(const S_vector &color)
{
    emissive = color;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002af80
LS3D_RESULT I3D_material::SetAlpha(float a)
{
    alpha = a;
    return I3D_OK;
}

// The texture's alpha decides how the material is drawn: a color key alone is tested, with mipmaps blended
// FUNCTION: LS3DF 0x1002af90
LS3D_RESULT I3D_material::SetTexture(ITexture *tex)
{
    if (tex != texture) {
        if (texture)
            texture->Release();
        flags &= ~(MATF_TEXTURE | MATF_ANIMATED | MATF_CKEY_ALPHA | MATF_TXT_ALPHA | MATF_ADD_MODE);
        texture = tex;
        if (tex) {
            tex->ref++;
            if (texture->flags & (ITC_ALPHA | ITC_USE_ALPHA))
                flags |= MATF_TXT_ALPHA;
            if (texture->flags & ITC_COLORKEY) {
                if (texture->flags & ITC_MIPMAP)
                    flags |= MATF_CKEY_ALPHA | MATF_TXT_ALPHA;
                else
                    flags |= MATF_CKEY_ALPHA;
            }
            flags |= MATF_TEXTURE;
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b000
LS3D_RESULT I3D_material::SetEnvTexture(ITexture *tex)
{
    if (tex != env_texture) {
        if (env_texture)
            env_texture->Release();
        env_texture = tex;
        if (tex) {
            tex->ref++;
            flags |= MATF_ENVMAP;
        } else
            flags &= ~MATF_ENVMAP;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b040
LS3D_RESULT I3D_material::SetEnvTexture(ITexture *tex, uint blend, uint mode, float a)
{
    if (tex != env_texture) {
        if (env_texture)
            env_texture->Release();
        env_texture = tex;
        if (tex)
            tex->ref++;
    }
    env_alpha = a;
    if (tex)
        flags = (flags & ~(MATF_ENVMAP_BLEND | MATF_ENVMAP_MODE)) | (blend << 8) | (mode << 12) | MATF_ENVMAP;
    else
        flags &= ~MATF_ENVMAP;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b0b0
LS3D_RESULT I3D_material::Duplicate(I3D_material *mat)
{
    if (!mat)
        return I3DERR_INVALIDPARAMS;
    flags = mat->flags & ~MATF_ANIMATED;
    SetAlpha(mat->alpha);
    SetAmbient(mat->ambient);
    SetDiffuse(mat->diffuse);
    SetEmissive(mat->emissive);
    env_alpha = mat->env_alpha;
    SetTexture(mat->texture);
    SetEnvTexture(mat->env_texture);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b120
LS3D_RESULT I3D_material::Set2Sided()
{
    flags |= MATF_TWO_SIDED;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b130
LS3D_RESULT I3D_material::SetEnvType(uint type)
{
    flags = (flags & ~MATF_ENVMAP_BLEND) | (type << 8);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b150
LS3D_RESULT I3D_material::SetEnvMode(uint mode)
{
    flags = (flags & ~MATF_ENVMAP_MODE) | (mode << 12);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002b170
LS3D_RESULT I3D_material::SetEnvScale(uint scale)
{
    flags = (flags & ~MATF_ENVMAP_SCALE) | scale;
    return I3D_OK;
}

LS3D_RESULT I3D_material::SetEnvAlpha(float a)
{
    env_alpha = a;
    return I3D_OK;
}

// The first frame of each is NULL until LoadImage or Duplicate sets them
// FUNCTION: LS3DF 0x1002b190
I3D_anim_material::I3D_anim_material()
{
    textures.push_back(NULL);
    elapsed = 0;
    last_time = driverTickTime;
    frame = 0;
    loop_frame = 0;
    frame_delay = 0;
    env_textures.push_back(NULL);
    env_elapsed = 0;
    env_last_time = driverTickTime;
    env_frame = 0;
    env_loop_frame = 0;
    env_frame_delay = 0;
    flags |= MATF_ANIMATED;
}

// FUNCTION: LS3DF 0x1002b450
I3D_anim_material::~I3D_anim_material()
{
    int i;
    for (i = textures.size(); i--; ) {
        if (textures[i])
            textures[i]->Release();
    }
    textures.clear();
    texture = NULL;
    for (i = env_textures.size(); i--; ) {
        if (env_textures[i])
            env_textures[i]->Release();
    }
    env_textures.clear();
    env_texture = NULL;
}

// The texture of the frame the time since the last change has reached. The random frame's branch ends in its own copy
// of the last two statements: VC6 copied the join into it, a return there gives other code.
// FUNCTION: LS3DF 0x1002b580
void I3D_anim_material::UpdateTexture()
{
    if (textures.size() < 2 || !frame_delay) {
        texture = textures[0];
        return;
    }
    if (!last_time) {
        texture = textures[frame];
        last_time = driverTickTime;
        elapsed = 0;
        return;
    }
    int time = driverTickTime;
    if (last_time != time) {
        int delta = time - last_time;
        if (loop_frame == -1) {
            if (delta < frame_delay)
                return;
            frame = rand() % textures.size();
        } else {
            elapsed += delta;
            while (elapsed >= frame_delay) {
                elapsed -= frame_delay;
                frame++;
                if (frame >= textures.size())
                    frame = loop_frame + frame % (textures.size() - loop_frame);
            }
        }
        last_time = time;
    }
    texture = textures[frame];
}

// FUNCTION: LS3DF 0x1002b6d0
void I3D_anim_material::UpdateEnvTexture()
{
    if (env_textures.size() < 2 || !env_frame_delay) {
        env_texture = env_textures[0];
        return;
    }
    if (!env_last_time) {
        env_texture = env_textures[env_frame];
        env_last_time = driverTickTime;
        env_elapsed = 0;
        return;
    }
    int time = driverTickTime;
    if (env_last_time != time) {
        int delta = time - env_last_time;
        if (env_loop_frame == -1) {
            if (delta < env_frame_delay)
                return;
            env_frame = rand() % env_textures.size();
        } else {
            env_elapsed += delta;
            while (env_elapsed >= env_frame_delay) {
                env_elapsed -= env_frame_delay;
                env_frame++;
                if (env_frame >= env_textures.size())
                    env_frame = env_loop_frame + env_frame % (env_textures.size() - env_loop_frame);
            }
        }
        env_last_time = time;
    }
    env_texture = env_textures[env_frame];
}

// Takes mat's frames if it is animated too, else its texture and environment map as the only frames.
// Not matching yet: the original leaves the first push_back's insert(P, x) out of line and inlines vector::size in the
// last push_back's insert, which ours call. Both fit more pending inline calls right after the first push_back and a
// little more source (four calls of an empty inline function there give the original's code, as a probe); inline
// setters for the timing fields come close, a release helper for the frames doesn't.
// FUNCTION: LS3DF 0x1002b830
LS3D_RESULT I3D_anim_material::Duplicate(I3D_material *mat)
{
    int i;
    for (i = textures.size(); i--; ) {
        if (textures[i])
            textures[i]->Release();
    }
    textures.clear();
    for (i = env_textures.size(); i--; ) {
        if (env_textures[i])
            env_textures[i]->Release();
    }
    env_textures.clear();
    elapsed = 0;
    last_time = driverTickTime;
    frame = 0;
    texture = NULL;
    env_elapsed = 0;
    env_last_time = driverTickTime;
    env_frame = 0;
    env_texture = NULL;
    I3D_material::Duplicate(mat);
    if (!(mat->flags & MATF_ANIMATED)) {
        textures.push_back(texture);
        loop_frame = 0;
        frame_delay = 0;
        env_textures.push_back(env_texture);
        env_loop_frame = 0;
        env_frame_delay = 0;
        return I3D_OK;
    }
    I3D_anim_material *amat = (I3D_anim_material *)mat;
    flags |= MATF_ANIMATED;
    frame_delay = amat->frame_delay;
    loop_frame = amat->loop_frame;
    env_frame_delay = amat->env_frame_delay;
    env_loop_frame = amat->env_loop_frame;
    if (texture)
        texture->Release();
    if (env_texture)
        env_texture->Release();
    uint j;
    if (amat->textures.size() > 0) {
        for (j = 0; j < amat->textures.size(); j++) {
            textures.push_back(amat->textures[j]);
            if (textures[j])
                textures[j]->ref++;
        }
        texture = textures[0];
    } else {
        textures.push_back(NULL);
        texture = NULL;
    }
    if (amat->env_textures.size() > 0) {
        for (j = 0; j < amat->env_textures.size(); j++) {
            env_textures.push_back(amat->env_textures[j]);
            if (env_textures[j])
                env_textures[j]->ref++;
        }
        env_texture = env_textures[0];
    } else {
        env_textures.push_back(NULL);
        env_texture = NULL;
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1002bc50
bool I3D_anim_material::SetCurrentFrame(int f)
{
    if (f >= (int)textures.size())
        f %= textures.size();
    if (f >= 0) {
        frame = f;
        last_time = 0;
    } else {
        last_time = driverTickTime;
        frame = 0;
    }
    texture = textures[frame];
    return true;
}

// FUNCTION: LS3DF 0x1002bcd0
bool I3D_anim_material::SetEnvCurrentFrame(int f)
{
    if (f > (int)env_textures.size() || !env_textures.size())
        return false;
    if (f >= 0) {
        env_frame = f;
        env_last_time = 0;
    } else {
        env_last_time = driverTickTime;
        env_frame = 0;
    }
    env_texture = env_textures[env_frame];
    return true;
}

// The texture's ITexture::Create flags, from the material's flags and the driver's render states. An inline function,
// as in the original: written out in LoadImage, the result stays in a register where the original's frame keeps a copy
// of it, and the animated LoadImage inlines one push_back more than the original. The flags come by reference: by value
// (or the material by pointer) the MATF_TXT_ALPHA update after the call reuses the register they were loaded into,
// where the original updates them in memory.
static inline uint GetTextureFlags(const uint &flags)
{
    uint tflags = flags & MATF_LOD0 ? ITC_MANAGED : 0;
    if (renderState[16])
        tflags |= ITC_DITHER;
    if (renderState[17] || flags & MATF_TRUECOLOR)
        tflags |= ITC_TRUECOLOR;
    else if (renderState[25] && !(flags & MATF_NO_DXT))
        tflags |= ITC_DXT;
    if (flags & MATF_MIPMAP && renderState[13])
        tflags |= ITC_MIPMAP;
    if (renderState[RS_LOD_COARSER] && !(flags & MATF_FULL_SIZE))
        tflags |= ITC_HALF;
    if (flags & MATF_KEEP_PIXELS)
        tflags |= ITC_KEEP_PIXELS;
    if (flags & MATF_CKEY_ALPHA) {
        if (flags & MATF_IMAGE_ALPHA)
            tflags |= ITC_ALPHA | ITC_COLORKEY;
        else
            tflags |= ITC_COLORKEY;
    } else if (flags & MATF_TXT_ALPHA) {
        if (flags & MATF_IMAGE_ALPHA)
            tflags |= ITC_ALPHA;
        else
            tflags |= ITC_USE_ALPHA;
    }
    return tflags;
}

// An environment map's flags: no color key, alpha, dithering or kept pixels, mipmaps by the render state alone
#define ENV_TEXTURE_FLAGS(tflags)                                                                                     \
    ((tflags & ~(ITC_KEEP_PIXELS | ITC_DITHER | ITC_MANAGED | ITC_COLORKEY | ITC_USE_ALPHA | ITC_ALPHA)) |           \
        ITC_MANAGED_DXT | (renderState[13] ? ITC_MIPMAP : 0))

// The colors (env_alpha too with an environment map), then each texture's name: a length byte and the characters.
// A color-keyed or additive material, or one whose image has alpha, has no alpha image name.
// FUNCTION: LS3DF 0x1002bd50
void I3D_material::LoadImage()
{
    char name[256], alpha_name[256];
    uint tflags = GetTextureFlags(flags);
    if ((tflags & (ITC_MIPMAP | ITC_COLORKEY)) == (ITC_MIPMAP | ITC_COLORKEY))
        flags |= MATF_TXT_ALPHA;
    if (flags & MATF_ENVMAP) {
        dtaRead(LoadImageHandle, &ambient, 0x2c);
        int len = 0;
        dtaRead(LoadImageHandle, &len, 1);
        if (len) {
            dtaRead(LoadImageHandle, name, len);
            name[len] = 0;
            LS3D_RESULT r = pGlobalIGraph->CreateITexture(name, NULL, ENV_TEXTURE_FLAGS(tflags), &env_texture);
            if (r < 0) {
                dbgPrintf(" !! ERROR creating env. texture %s  result:%s   ?::Open()  %s", name, I3DGetResultMessage(r),
                    ImageFileName);
            }
        }
    } else
        dtaRead(LoadImageHandle, &ambient, 0x28);
    int len = 0;
    dtaRead(LoadImageHandle, &len, 1);
    if (len) {
        dtaRead(LoadImageHandle, name, len);
        name[len] = 0;
        if (flags & MATF_TXT_ALPHA && !(flags & (MATF_ADD_MODE | MATF_CKEY_ALPHA | MATF_IMAGE_ALPHA))) {
            int alpha_len = 0;
            dtaRead(LoadImageHandle, &alpha_len, 1);
            if (alpha_len)
                dtaRead(LoadImageHandle, alpha_name, alpha_len);
            else
                tflags &= ~ITC_USE_ALPHA;
            alpha_name[alpha_len] = 0;
        } else
            alpha_name[0] = 0;
        LS3D_RESULT r = pGlobalIGraph->CreateITexture(name, alpha_name, tflags, &texture);
        if (r < 0) {
            flags &= ~MATF_LOD0;
            dbgPrintf(" !! ERROR creating texture %s,%s  result:%s   ?::Open()  %s", name, alpha_name,
                I3DGetResultMessage(r), ImageFileName);
        }
    } else
        flags &= ~MATF_LOD0;
}

// As I3D_material's, with the frames' counts and the timing after the names. A material with more than one frame
// numbers its images: the two characters before the extension's six (".tga" and two digits) become 00, 01 and so on.
// The environment frames get the texture's flags as they are (the single environment map gets ENV_TEXTURE_FLAGS).
// FUNCTION: LS3DF 0x1002c000
void I3D_anim_material::LoadImage()
{
    char alpha_name[256], name[256], env_name[256];
    if (flags & MATF_ENVMAP) {
        dtaRead(LoadImageHandle, &ambient, 0x2c);
        int len = 0;
        dtaRead(LoadImageHandle, &len, 1);
        if (len)
            dtaRead(LoadImageHandle, env_name, len);
        env_name[len] = 0;
    } else
        dtaRead(LoadImageHandle, &ambient, 0x28);
    int len = 0;
    dtaRead(LoadImageHandle, &len, 1);
    if (len) {
        dtaRead(LoadImageHandle, name, len);
        name[len] = 0;
        if (flags & MATF_TXT_ALPHA && !(flags & (MATF_ADD_MODE | MATF_CKEY_ALPHA | MATF_IMAGE_ALPHA))) {
            int alpha_len = 0;
            dtaRead(LoadImageHandle, &alpha_len, 1);
            if (alpha_len)
                dtaRead(LoadImageHandle, alpha_name, alpha_len);
            alpha_name[alpha_len] = 0;
        } else
            alpha_name[0] = 0;
    } else {
        name[0] = 0;
        alpha_name[0] = 0;
    }
    int num_frames = 0;
    dtaRead(LoadImageHandle, &num_frames, 1);
    int env_num_frames = 0;
    dtaRead(LoadImageHandle, &env_num_frames, 1);
    dtaRead(LoadImageHandle, &loop_frame, 16);
    uint tflags = GetTextureFlags(flags);
    if ((tflags & (ITC_MIPMAP | ITC_COLORKEY)) == (ITC_MIPMAP | ITC_COLORKEY))
        flags |= MATF_TXT_ALPHA;
    if (flags & MATF_ENVMAP) {
        env_textures.pop_back();
        if (env_num_frames < 2) {
            LS3D_RESULT r = pGlobalIGraph->CreateITexture(env_name, NULL, ENV_TEXTURE_FLAGS(tflags), &env_texture);
            if (r < 0) {
                dbgPrintf(" !! ERROR creating env. texture %s  result:%s   ?::Open()  %s", env_name,
                    I3DGetResultMessage(r), ImageFileName);
            }
            env_textures.push_back(env_texture);
        } else {
            int n = strlen(env_name) - 6;
            if (n < 0) {
                dbgPrintf("!! ERROR Anim. environmental texture - invalid name: %s  %s", env_name, ImageFileName);
                env_textures.push_back(NULL);
            } else {
                char *num = env_name + n;
                for (int i = 0; i < env_num_frames; i++) {
                    sprintf(num, "%02i", i);
                    num[2] = '.';
                    ITexture *tex = NULL;
                    LS3D_RESULT r = pGlobalIGraph->CreateITexture(env_name, NULL, tflags, &tex);
                    if (r < 0) {
                        dbgPrintf(" !! ERROR creating env. texture %s  result:%s   ?::Open()  %s", env_name,
                            I3DGetResultMessage(r), ImageFileName);
                    }
                    env_textures.push_back(tex);
                }
            }
        }
    }
    if (name[0] || alpha_name[0]) {
        textures.pop_back();
        if (num_frames < 2) {
            LS3D_RESULT r = pGlobalIGraph->CreateITexture(name, alpha_name, tflags, &texture);
            if (r < 0) {
                dbgPrintf(" !! ERROR creating texture %s,%s  result:%s   ?::Open()  %s", name, alpha_name,
                    I3DGetResultMessage(r), ImageFileName);
            }
            textures.push_back(texture);
        } else {
            int n = strlen(name) - 6;
            if (n < 0) {
                dbgPrintf("!! ERROR Anim. texture - invalid name: %s  %s", name, ImageFileName);
                textures.push_back(NULL);
            } else {
                char *num = name + n;
                char *alpha_num = NULL;
                if (flags & MATF_ANIM_ALPHA && !(flags & (MATF_ADD_MODE | MATF_CKEY_ALPHA | MATF_IMAGE_ALPHA))) {
                    int alpha_n = strlen(alpha_name) - 6;
                    if (alpha_n < 0) {
                        dbgPrintf("!! ERROR Anim. texture - invalid alpha name: %s  (name:%s) %s", alpha_name, name,
                            ImageFileName);
                    } else
                        alpha_num = alpha_name + alpha_n;
                }
                for (int i = 0; i < num_frames; i++) {
                    sprintf(num, "%02i", i);
                    num[2] = '.';
                    if (alpha_num) {
                        sprintf(alpha_num, "%02i", i);
                        alpha_num[2] = '.';
                    }
                    ITexture *tex = NULL;
                    LS3D_RESULT r = pGlobalIGraph->CreateITexture(name, alpha_name, tflags, &tex);
                    if (r < 0) {
                        dbgPrintf(" !! ERROR creating anim. texture %s,%s  result:%s   ?::Open()  %s", name, alpha_name,
                            I3DGetResultMessage(r), ImageFileName);
                    }
                    textures.push_back(tex);
                }
            }
        }
    }
    SetCurrentFrame(-1);
    SetEnvCurrentFrame(-1);
}
