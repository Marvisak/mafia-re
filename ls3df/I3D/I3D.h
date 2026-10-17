#pragma once

#include "ls3df.h"
#include "I3D/I3d_math.h"

#include <d3d8.h>

// The 3D engine (PS2 I3d_*.cpp). The placeholders of the classes not recovered yet keep the members and virtual
// functions the written files use at their offsets and vtable slots, the sizes the driver allocates, and declare the
// constructors it calls (stubs, see I3d_driver.cpp). They are __declspec(novtable) so that the stubs don't need the
// virtual functions, none of which is written yet; the real classes will drop it.

// windows.h's DrawText macro would rename the driver's DrawText
#undef DrawText
#undef LoadImage

class I3D_driver;
class I3D_scene;
class I3D_dummy;
class I3D_material;
class ITexture;
class ISND_3Dlistener;
class I3D_frame;
class I3D_camera;
class I3D_sector;
class I3D_light;
class I3D_sound;
class I3D_animation_set;
class I3D_mesh_object;
struct S_vector4;
class C_IBuffer;
class C_VBuffer;

// A block of one of the index buffers, as the engine uses it (PS2 name; IBManager.h has the buffers)
struct COMPLETE_IB_STRUCT {
    C_IBuffer *buffer;                  // NULL if none
    IDirect3DIndexBuffer8 *ib;
    int base;                           // the block's first index
};

// A block of one of the vertex buffers, as the engine uses it (PS2 name; VBManager.h has the buffers)
struct COMPLETE_VB_STRUCT {
    C_VBuffer *buffer;                  // NULL if none
    IDirect3DVertexBuffer8 *vb;
    int base;                           // the block's first vertex
};

// The frame types (the names are FrameType2String's)
enum I3D_FRAME_TYPE {
    FRAME_NULL,
    FRAME_VISUAL,
    FRAME_LIGHT,
    FRAME_CAMERA,
    FRAME_SOUND,
    FRAME_SECTOR,
    FRAME_DUMMY,
    FRAME_TARGET,
    FRAME_USER,
    FRAME_MODEL,
    FRAME_JOINT,
    FRAME_VOLUME,
    FRAME_OCCLUDER,
    FRAME_SCENE,
    FRAME_AREA,
    FRAME_SHADOW,
    FRAME_LANDSCAPE,                    // PC only
    FRAME_EMITOR,
};

// The visual types, four-character codes (the names are VisualType2String's, LNDP has none). PS2's are 0 to 9.
enum I3D_VISUAL_TYPE {
    I3D_VISUAL_OBJECT = 0x5f4a424f,     // OBJ_
    I3D_VISUAL_LIT_OBJECT = 0x50414d4c, // LMAP
    I3D_VISUAL_BILLBOARD = 0x44524242,  // BBRD
    I3D_VISUAL_SINGLEMESH = 0x4d474e53, // SNGM
    I3D_VISUAL_LENSF = 0x534e454c,      // LENS
    I3D_VISUAL_MORPH = 0x4850524d,      // MRPH
    I3D_VISUAL_SINGLEMORPH = 0x524d4d53, // SMMR
    I3D_VISUAL_PROJECTOR = 0x4a4f5250,  // PROJ
    I3D_VISUAL_PART_BASE = 0x42545250,  // PRTB
    I3D_VISUAL_PART_ELEMENT = 0x45545250, // PRTE
    I3D_VISUAL_LANDPATCH = 0x50444e4c,  // LNDP, a landscape's patch (PC only, the name is a guess)
    I3D_VISUAL_MIRROR = 0x5252494d,     // MIRR
    I3D_VISUAL_SHADOW = 0x00444853,     // SHD
};

// The driver's render states (PS2 type, 63 on PC). The names of most aren't known; the ones here are guesses, after
// Insanity3D's where the meaning agrees.
enum I3D_RENDERSTATE {
    RS_LINEARFILTER = 0,                // the textures' minification and magnification filter
    RS_MIPMAP = 2,                      // the mip filter: 0 none, 1 point, 2 linear
    RS_USEZB = 3,                       // the z-buffer
    RS_WBUFFER = 4,                     // a w-buffer where there is a z-buffer (ZBufferEnable)
    RS_DITHER = 7,
    RS_MIPMAP_BIAS = 8,                 // the textures' mipmap LOD bias
    RS_WIREFRAME = 9,
    RS_RECOMPUTE_GEOMETRY = 18,         // sets RecomputeAllGeometry
    RS_FOG = 19,                        // fog (Insanity3D's name, the same number); refused where the hardware
                                        // lacks it (m_ProblemFlags 0x80)
    RS_LOD_QUALITY = 20,                // a float, 1.0 by default: the levels of detail's distance scale (Insanity3D's
                                        // name, the same number)
    RS_LOD_COARSER = 21,                // GetObjectLOD takes the next coarser level for VISF_UNKNOWN_10 visuals
    RS_SHADOW_SW = 48,                  // shadow textures are filtered in software
    RS_FAST_FIND = 49,                  // named frames go into the driver's fast find tree (the name is a guess)
    RS_LOD_SCALE = 50,                  // in percent
    RS_NONORTHO_TESSELLATION = 53,      // RsNonOrthoTessellation, inverted
    RS_ZBIAS_PROJECTOR = 54,            // RsZBiasProjector
    RS_ZBIAS_SHADOW = 55,               // RsZBiasShadow
    RS_ENVMAP_SCALE = 62,               // a float, 1.0 by default: the planar environment maps' scale (EnvProcess);
                                        // 666 frees the vertex buffers in video memory instead
    RS_LAST = 63,
};

// GetStats' index (PS2 type; the names are Insanity3D's, only the volume statistics exist)
enum I3DSTATSINDEX {
    I3DSTATS_VOLUME,
    I3DSTATS_RENDER,
    I3DSTATS_SCENE,
    I3DSTATS_VIDEO,
    I3DSTATS_MEMORY,
    I3DSTATS_SOUND,
    I3DSTATS_BSP,
};

// What an enumeration callback returns (PS2 type, Insanity3D's names)
enum I3DENUMRET {
    I3DENUMRET_OK,
    I3DENUMRET_SKIPCHILDREN,
    I3DENUMRET_CANCEL,
};

// EnumFrames' flags: the frame types to take, and how to match the names (Insanity3D's names; TARGET, AREA and SHADOW
// are guesses)
#define ENUMF_VISUAL 0x1
#define ENUMF_LIGHT 0x2
#define ENUMF_CAMERA 0x4
#define ENUMF_SOUND 0x8
#define ENUMF_SECTOR 0x10
#define ENUMF_DUMMY 0x20
#define ENUMF_TARGET 0x40
#define ENUMF_MODEL 0x80
#define ENUMF_USER 0x100
#define ENUMF_VOLUME 0x200
#define ENUMF_JOINT 0x400
#define ENUMF_OCCLUDER 0x800
#define ENUMF_AREA 0x1000
#define ENUMF_SHADOW 0x2000
#define ENUMF_ALL 0xffff
#define ENUMF_WILDMASK 0x10000 // '*' ends the name, '?' is any character
#define ENUMF_CASESENS 0x20000

typedef void __stdcall I3D_ENUMVISUALPROC(I3D_VISUAL_TYPE type, const char *name);
typedef I3DENUMRET __stdcall I3D_ENUMPROC(I3D_frame *frame, uint context);

// A bounding sphere (PS2 I3D_bsphere)
struct I3D_bsphere {
    S_vector pos;
    float radius;
};

// A triangle's vertex indices (PS2 I3D_triface)
struct I3D_triface {
    ushort vi[3];
};

// A vertex's texture coordinates (PS2 I3D_text_coor)
struct I3D_text_coor {
    float u, v;
};

// A float's bits as an int, for the render states that hold floats (Insanity3D's I3DFloatAsInt): the parameter is
// stored and read back, as the original's constants go through a stack slot
inline int I3DFloatAsInt(float f)
{
    return *(int *)&f;
}

// The other way (Insanity3D's I3DIntAsFloat), for the parameters that pass floats as their bits
inline float I3DIntAsFloat(int i)
{
    return *(float *)&i;
}

// I3D_frame::frm_flags (names mostly guessed)
#define FRMFLAGS_ON 0x1                 // visible (SetOn)
#define FRMFLAGS_ROT_VALID 0x4          // rot holds the local matrix's rotation
#define FRMFLAGS_QUAT_VALID 0x8         // rot holds the rotation the local matrix was made from
#define FRMFLAGS_MAT_VALID 0x10         // the local matrix is up to date
#define FRMFLAGS_WMAT_VALID 0x20        // the world matrix is up to date
#define FRMFLAGS_HR_BOUND_VALID 0x40    // bound (the hierarchy's bound volume) is up to date
#define FRMFLAGS_LOCKED 0x80            // the position doesn't change (SetPos)
#define FRMFLAGS_HR_BOUND_W_VALID 0x100 // bound_w is up to date
#define FRMFLAGS_CALLBACK_MASK 0x1e00   // the messages the callback gets
#define FRMFLAGS_CHANGED 0x2000         // moved since the last Update
#define FRMFLAGS_TARGET 0x4000          // a target's frame (I3D_target::DeleteFromAnyTarget)
#define FRMFLAGS_SHADOW 0x8000          // a shadow's frame (I3D_shadow::DeleteFromAnyShadow)
#define FRMFLAGS_HR_SECTORS 0x40000     // the hierarchy has sectors
#define FRMFLAGS_HR_OCCLUDERS 0x80000   // occluders
#define FRMFLAGS_HR_AREAS 0x100000      // areas
#define FRMFLAGS_CLIP 0x200000          // has to be clipped (the name is a guess)
#define FRMFLAGS_TICK 0x400000          // in its scene's tick list
#define FRMFLAGS_UNKNOWN_8000000 0x8000000 // set by the camera's constructor and Duplicate
#define FRMFLAGS_FFTREE 0x20000000      // in the driver's fast find tree
#define FRMFLAGS_UPDATE 0x40000000      // to update
#define FRMFLAGS_WEATHER 0x80000000     // a dummy in a scene's weather system (I3D_scene::WeatherSystemAddDummy)

// What a frame's callback is told (PS2 type; the names are guesses). SetCallback's flags are the messages to send.
enum LS3D_CALLBACK_MESSAGE {
    LS3D_CALLBACK_MESSAGE_0,
    LS3D_CALLBACK_LOD = 0x400,          // a visual's level of detail changed: the old one, the new one (GetObjectLOD)
};

// LinkTo's flags: the first is Insanity3D's I3DLINK_UPDMATRIX, the same value and role
#define I3DLINK_UPDMATRIX 0x1           // keep the world matrix: the local one is made again against the new parent

// A frame's callback (PS2's parameters; the calling convention isn't confirmed)
typedef uint __stdcall I3D_CALLBACK(I3D_frame *frame, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2);

// A bounding box and the sphere around it (PS2 I3D_bvolume)
struct I3D_bvolume {
    I3D_bbox bbox;
    I3D_bsphere bsphere;
};

// The base of the scene graph's nodes (PS2 I3D_frame, 0x140 bytes on PC, vtable 0x1009b9b8). The game reaches its
// members through the class's inline functions. Member names are Insanity3D's where the roles agree (ref, user_data,
// frm_flags, matrix, f_matrix, name, parent, type), the others are guesses.
// VTABLE: LS3DF 0x1009b9b8
class I3D_frame {
public:
    I3D_frame();

    virtual uint __stdcall Release();
    virtual void __stdcall SetWorldPos(const S_vector &pos);
    virtual void __stdcall SetWorldPosDir(const S_vector &pos, const S_vector &dir, float roll);
    virtual void __stdcall SetDir(const S_vector &dir, float roll);
    virtual void __stdcall SetWorldDir(const S_vector &dir, float roll);
    virtual void __stdcall Tick(int time) {}
    virtual void __stdcall Update();
    virtual void __stdcall SetCallback(I3D_CALLBACK *cb, uint flags);
    virtual LS3D_RESULT __stdcall SetProperty(const char *prop);
    virtual void __stdcall SetOn(bool on);
    virtual LS3D_RESULT __stdcall SetName(const char *new_name);
    virtual LS3D_RESULT __stdcall LinkTo(I3D_frame *frame, uint flags);
    virtual I3D_frame *__stdcall GetChild(int index) const;
    virtual LS3D_RESULT __stdcall EnumFrames(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const;
    virtual I3D_frame *__stdcall FindChildFrame(const char *frame_name, uint flags) const;
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual ~I3D_frame();
    virtual void UpdateV() {}
    virtual void SaveImage() {}
    virtual void LoadImage();

private:
    // The scene calls UpdateHRBoundVolW (Insanity3D's frame has the same friend, "for rendering and fast access")
    friend class I3D_scene;

    // Exported (the decorated names say private; UpdateWMatrixProc __thiscall, the others __stdcall)
    __declspec(dllexport) void UpdateWMatrixProc();
    __declspec(dllexport) void __stdcall UpdateHRBoundVol();
    __declspec(dllexport) void __stdcall UpdateHRBoundVolW();

public:
    // Makes the local matrix from the scale and rotation, or the rotation from the matrix (PS2 name). The dummy file's
    // copy is the one linked.
    // FUNCTION: LS3DF 0x1001a6e0
    void UpdateMatrix()
    {
        if (!(frm_flags & FRMFLAGS_MAT_VALID)) {
            frm_flags |= FRMFLAGS_MAT_VALID | FRMFLAGS_ROT_VALID;
            f_matrix.SetRot3(rot);
            f_matrix(0) *= scale.x;
            f_matrix(1) *= scale.y;
            f_matrix(2) *= scale.z;
        } else if (!(frm_flags & FRMFLAGS_ROT_VALID)) {
            if (!(frm_flags & FRMFLAGS_QUAT_VALID))
                rot.Make(f_matrix);
            frm_flags |= FRMFLAGS_ROT_VALID | FRMFLAGS_QUAT_VALID;
            f_matrix(0, 0) *= scale.x;
            f_matrix(0, 1) *= scale.x;
            f_matrix(0, 2) *= scale.x;
            f_matrix(1, 0) *= scale.y;
            f_matrix(1, 1) *= scale.y;
            f_matrix(1, 2) *= scale.y;
            f_matrix(2, 0) *= scale.z;
            f_matrix(2, 1) *= scale.z;
            f_matrix(2, 2) *= scale.z;
        }
    }

    // Insanity3D's name
    const S_matrix &GetLocalMatrix()
    {
        UpdateMatrix();
        return f_matrix;
    }

    // Insanity3D's name
    void SetRot(const S_quat &q)
    {
        rot = q;
        frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    }

    // Insanity3D's name
    void SetPos(const S_vector &pos)
    {
        if (frm_flags & FRMFLAGS_LOCKED)
            return;
        f_matrix(3) = pos;
        frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID)) | FRMFLAGS_UPDATE;
    }

    // The scale, by reference (the game's out-of-line copy is at 0x47aec0 in Game.exe)
    const S_vector &GetScale() const
    {
        return scale;
    }

    // Sets the scale; the matrix is made again from it and the rotation, which is taken from the matrix first if it
    // isn't up to date (the game inlines it too)
    void SetScale(const S_vector &s)
    {
        scale = s;
        if (!(frm_flags & FRMFLAGS_QUAT_VALID))
            rot.Make(f_matrix);
        frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
    }

    // PS2 name
    void UpdateWMatrix()
    {
        if (!(frm_flags & FRMFLAGS_WMAT_VALID))
            UpdateWMatrixProc();
    }

    const S_matrix &GetMatrix()
    {
        UpdateWMatrix();
        return matrix;
    }

    const char *GetName() const
    {
        return name ? name : "";
    }

    I3D_FRAME_TYPE GetType() const
    {
        return type;
    }

    int NumChildren() const
    {
        return num_children;
    }

    void DeleteFromOwner();
    void RemoveFromFFTree();
    void AddToFFTree();

    // The fast find tree (PS2 OFFD_*): the number of frames in the branch from this one, and the list of them in name
    // order
    int OFFD_CountFrames();
    void AlphabeticalListOfFrames_();

    LS3D_RESULT SetModelName(const char *model_name);
    LS3D_RESULT ResetModelName();
    I3D_frame *AddChild(I3D_frame *frame);
    I3D_frame *RemoveChild(I3D_frame *frame);
    LS3D_RESULT EnumSubtree(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const;
    I3D_frame *GetChildFrame(const char *frame_name, uint flags);
    void UpdateChildren();
    void DiscardHRBoundVol();
    void SetBoundVolume(const I3D_bbox &bbox);

private:
    void OFFD_CountFrames_();

public:
    int ref;                            // 0x04
    uint user_data;                     // 0x08
    char *property;                     // 0x0c, SetProperty's
    S_matrix matrix;                    // 0x10, the world matrix
    S_matrix f_matrix;                  // 0x50, the local one
    S_vector scale;                     // 0x90
    S_quat rot;                         // 0x9c
    uint frm_flags;                     // 0xac, FRMFLAGS_*
    I3D_bvolume bound;                  // 0xb0, the hierarchy's, in local space
    I3D_bvolume bound_w;                // 0xd8, in world space
    char *name;                         // 0x100, "model.frame" in a model
    const char *orig_name;              // 0x104, the frame's own name, inside name
    I3D_CALLBACK *callback;             // 0x108
    I3D_frame *owner;                   // 0x10c, the model or scene the frame is in
    I3D_FRAME_TYPE type;                // 0x110
    float world_scale;                  // 0x114
    uint update_count;                  // 0x118, counts the updates that moved it
    int num_children;                   // 0x11c
    I3D_frame *parent;                  // 0x120
    I3D_frame *child;                   // 0x124, the first child
    I3D_frame *prev, *next;             // 0x128, the siblings
    uint unknown_130;                   // 0x130
    I3D_frame *ff_left, *ff_right;      // 0x134, the fast find tree (PS2 0x114, 0x118)
    int render_number;                  // 0x13c, the driver's RenderNumber when the frame was last rendered, -1
};

// SYNTHETIC: LS3DF 0x1001a980
// I3D_frame::`scalar deleting destructor'

// The function-local statics of I3d_frame.cpp (PS2 names): AddToFFTree's count of the frames added, Update's guard
// against updating the targets twice. VC6 puts a function's static into the PDB with its plain name, so they have
// by-name markers.
// GLOBAL: LS3DF 0x100c0044
// addcounter

// GLOBAL: LS3DF 0x100c0048
// updating

class __declspec(novtable) I3D_sound : public I3D_frame {
public:
    I3D_sound();

    // PS2 name (its PS2 parameters; the result type is a guess)
    virtual LS3D_RESULT __stdcall Open(const char *file_name, uint flags);
    // PS2 name, 0x100597f0
    virtual bool __stdcall IsPlaying(bool unknown) const;

    uint sound_flags;                   // 0x140
    uchar unknown_144[0xa8];
};

// A sector's sound environment (PS2 S_sector_sound_properties, 0x68 bytes): the EAX 3.0 listener properties, scaled to
// 0..1 or in milliseconds, which the scene converts (I3D_scene::m_UpdateSoundProperties). The members are named after
// the EAX properties they give; the sector file has SetupDefaults and MakeBlend (PS2 names).
struct S_sector_sound_properties {
    float room_lf;                      // 0x00
    float room_hf;                      // 0x04
    int decay_time;                     // 0x08, in ms
    float decay_hf_ratio;               // 0x0c
    float reflections;                  // 0x10
    int reflections_delay;              // 0x14, in ms
    float reverb;                       // 0x18
    int reverb_delay;                   // 0x1c, in ms
    float environment_diffusion;        // 0x20
    float unknown_24;                   // 0x24
    float decay_lf_ratio;               // 0x28
    float room;                         // 0x2c
    int echo_time;                      // 0x30, in ms
    float echo_depth;                   // 0x34
    int modulation_time;                // 0x38, in ms
    float modulation_depth;             // 0x3c
    float room_rolloff_factor;          // 0x40
    float air_absorption_hf;            // 0x44
    float hf_reference;                 // 0x48
    float lf_reference;                 // 0x4c
    float reflections_pan[3];           // 0x50
    float reverb_pan[3];                // 0x5c

    S_sector_sound_properties() {}

    void SetupDefaults();
    // The blend of a and b at t (b's at 1)
    void MakeBlend(const S_sector_sound_properties &a, const S_sector_sound_properties &b, float t, bool unknown);
};

// I3D_sector::sct_flags (names guessed; PRIMARY to NEAR are the scene's four sectors)
#define SCTF_PRIMARY 0x200
#define SCTF_SOUND_CHANGED 0x800        // the sound properties are to be given to the listener
#define SCTF_BACKDROP 0x1000
#define SCTF_OMNI 0x2000
#define SCTF_NEAR 0x4000

class __declspec(novtable) I3D_sector : public I3D_frame {
public:
    I3D_sector();

    // PS2 names, in PS2's order
    virtual void __stdcall AddLight(I3D_light *light);
    virtual void __stdcall DeleteLight(I3D_light *light);
    virtual void __stdcall AddSound(I3D_sound *sound);
    virtual void __stdcall DeleteSound(I3D_sound *sound);
    virtual void __stdcall SetSoundData(uint index, uint data);
    virtual uint __stdcall GetSoundData(uint index) const;
    // Whether the point is inside the sector
    virtual bool __stdcall CheckPoint(const S_vector &pos);

    // PS2 names
    void AddToRemoveFromSceneRing(I3D_scene *scene);
    void MakeReady();

    uint lights_changed;                // 0x140, counts the changes to its lights (the name is a guess)
    C_list<I3D_light *> lights;         // 0x144, the lights that reach into it
    uint unknown_154;
    C_list<I3D_sound *> sounds;         // 0x158
    S_sector_sound_properties sound_props; // 0x168
    uint sct_flags;                     // 0x1d0, SCTF_*
    uchar unknown_1d4[0x14];
    uint unknown_1e8;                   // 0x1e8
    uint unknown_1ec;                   // 0x1ec
    I3D_light *fog_light;               // 0x1f0
    uchar unknown_1f4[0x20];
    I3D_scene *scene;                   // 0x214
    uchar unknown_218[8];
    I3D_sector *next_in_scene;          // 0x220, the scene's ring of sectors
    uchar unknown_224[0x10];
};

// A portal between sectors (PS2 I3D_portal; not a frame). The scene keeps a ring of them.
class I3D_portal {
public:
    // PS2 name
    void UpdatePortal();

    uchar unknown_00[0x104];
    I3D_portal *next_in_scene;          // 0x104
};

// One of the interpolator set's animations (0x1c bytes; the names are guesses)
struct S_ipol_anim {
    I3D_animation_set *set;
    bool on;                            // 0x04
    uint unknown_08;
    int time;                           // 0x0c
    int time_1;                         // 0x10, set with time
    uint options;                       // 0x14
    float blend;                        // 0x18
};

class I3D_interpolator;

// The animations a scene or a model plays (PS2 I3D_IPolSet, 0xf8 bytes on PC; I3d_interpolator.cpp isn't written yet)
class I3D_IPolSet {
public:
    // Out of line on PS2, inlined on PC
    I3D_IPolSet(I3D_frame *f) : frame(f)
    {
        memset(anims, 0, sizeof(anims));
    }

    ~I3D_IPolSet();

    // PS2 names
    void Tick(int time);
    LS3D_RESULT SetAnimation(I3D_animation_set *set, uint index, float blend, uint options);

    I3D_frame *frame;
    bool changed;                       // 0x04, an animation's blend or time was set
    S_ipol_anim anims[8];               // 0x08
    C_list<I3D_interpolator *> interpolators; // 0xe8
};

// What a scene's callback is told (PS2 type; the PC's messages aren't known)
enum SCN_CALLBACK_MESSAGE {
    SCN_CALLBACK_MESSAGE_0,
};

typedef uint __stdcall SCN_CALLBACK(SCN_CALLBACK_MESSAGE message, uint param1, uint param2);

// GetLightness' flags (the name is a guess): only what reaches the point unoccluded
#define I3D_LIGHTNESS_TESTCOL 0x1

// EnumFrames' and FindFrame's flags: the scene's other sectors to take besides the primary one (names guessed)
#define ENUMF_BACKDROP 0x40000
#define ENUMF_OMNI 0x80000
#define ENUMF_NEAR 0x100000

// The weather system's parameters (PS2 type; the names are guesses after their roles): its switches, the particles'
// colors, the density, size and speed, the area around the camera they fill, their count and the wind
enum WS_PARAM {
    WS_PARAM_ON,                        // the weather system works
    WS_PARAM_FLAG_2,
    WS_PARAM_FLAG_4,
    WS_PARAM_COLOR,                     // ARGB
    WS_PARAM_COLOR_2,
    WS_PARAM_DENSITY,                   // per 1000
    WS_PARAM_SIZE,
    WS_PARAM_SPEED,                     // half of it is stored
    WS_PARAM_RANGE_X,
    WS_PARAM_RANGE_Y,
    WS_PARAM_COUNT,                     // of the particles
    WS_PARAM_WIND_X,
    WS_PARAM_WIND_Y,
    WS_PARAM_WIND_Z,
    WS_PARAM_SNOW,                      // snow instead of rain
    WS_PARAM_MODE,                      // 1 or 2
    WS_PARAM_LAST,
};

// The scene's weather flags (names guessed)
#define WSF_ON 0x1
#define WSF_FLAG_2 0x2
#define WSF_FLAG_4 0x4
#define WSF_SNOW 0x40
#define WSF_MODE_1 0x200
#define WSF_MODE_2 0x400

// A weather region the scene keeps for a dummy (PS2 DUMMY_STRUCT, 0x54 bytes on PC): the dummy's box in world space,
// three of its faces as planes, its center and radius (names guessed)
struct DUMMY_STRUCT {
    I3D_dummy *dummy;
    uint update_count;                  // 0x04, the dummy's, when this was made
    S_plane planes[3];                  // 0x08
    S_vector center;                    // 0x38
    float radius_2;                     // 0x44, squared
    float size[3];                      // 0x48, the box's edges' lengths
};

// A weather particle (PS2 WE_STRUCT, 0x48 bytes on PC; names guessed)
struct WE_STRUCT {
    S_vector v[4];                      // 0x00, the shape's corners around the position
    S_vector pos;                       // 0x30
    float min_y;                        // 0x3c, it comes back when it falls below
    float angle;                        // 0x40, of the shape around z
    uint phase;                         // 0x44, of a snow flake's sway, random
};

class I3D_shadow;
class I3D_mirror;

// The scene (PS2 I3D_scene, 0x33c bytes on PC; I3d_scene.cpp). Its virtual functions are in PS2's order; the PC lacks
// PS2's UpdatePortals, the watched frames, two of four GetSector overloads and one of two SetFrameSectorPos, and its
// Render takes nothing. Besides the primary sector it has three of its own (backdrop, omni, near; LS3DF names all four),
// and a weather system of rain or snow particles around the camera.
// VTABLE: LS3DF 0x1009c8f8
class I3D_scene : public I3D_frame {
public:
    I3D_scene();
    ~I3D_scene();

    virtual void __stdcall SetDir(const S_vector &dir, float roll) {}
    virtual void __stdcall Tick(int time);
    virtual void __stdcall Update() {}
    virtual void __stdcall SetCallback(I3D_CALLBACK *cb, uint flags) {}
    virtual void __stdcall SetOn(bool on) {}
    virtual LS3D_RESULT __stdcall SetName(const char *new_name) { return I3D_OK; }
    virtual LS3D_RESULT __stdcall LinkTo(I3D_frame *frame, uint flags) { return I3DERR_UNSUPPORTED; }
    virtual LS3D_RESULT __stdcall EnumFrames(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const;
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame) { return I3D_OK; }

    // PS2 names
    virtual SCN_CALLBACK *__stdcall SetCallback(SCN_CALLBACK *cb, uint context);
    virtual LS3D_RESULT __stdcall Render();
    virtual I3D_frame *__stdcall FindFrame(const char *name, uint flags) const;
    virtual void __stdcall AddFrame(I3D_frame *frame);
    virtual LS3D_RESULT __stdcall DeleteFrame(I3D_frame *frame);
    // Every light of the primary sector's SetDiffuse
    virtual void __stdcall SetGlobalColor(const S_vector &color);
    // The lights' brightness at the point, 0 to 1
    virtual float __stdcall GetLightness(const S_vector &pos, I3D_sector *sector, uint flags, I3D_frame *frame);
    virtual void __stdcall SetActiveCamera(I3D_camera *cam);
    virtual LS3D_RESULT __stdcall UnmapScreenPoint(int x, int y, S_vector &pos, S_vector &dir) const;
    virtual LS3D_RESULT __stdcall TransformPoints(const S_vector *in, S_vector4 *out, uint num) const;
    virtual bool __stdcall TestVisibility(I3D_frame *frame, I3D_camera *cam);
    virtual LS3D_RESULT __stdcall SetAnimation(I3D_animation_set *set, uint index, float blend, uint options);
    virtual I3D_animation_set *__stdcall GetAnimationSet(uint index) const;
    virtual void __stdcall StopAnimation(uint index);
    virtual LS3D_RESULT __stdcall SetAnimBlend(uint index, float blend);
    virtual float __stdcall GetAnimBlend(uint index) const;
    virtual LS3D_RESULT __stdcall SetAnimTime(uint index, int time);
    virtual int __stdcall GetAnimTime(uint index) const;
    virtual bool __stdcall AnimIsOn(uint index) const;
    virtual int __stdcall NumInterpolators() const;
    virtual I3D_interpolator **__stdcall GetInterpolators() const;
    virtual void __stdcall AddTickFrame(I3D_frame *frame);
    virtual LS3D_RESULT __stdcall DeleteTickFrame(I3D_frame *frame);
    virtual I3D_sector *__stdcall CreateSector();
    // The sector at pos, looked for from hint first (the camera's)
    virtual I3D_sector *__stdcall GetSector(const S_vector &pos, I3D_sector *hint) const;
    // The last of the sectors that holds pos
    virtual I3D_sector *__stdcall GetSector(const S_vector &pos, I3D_sector **list, int num) const;
    virtual void __stdcall SetBackdropRange(float near1, float far1);
    virtual LS3D_RESULT __stdcall SetFrameSectorPos(I3D_frame *frame, const S_vector &pos) const;
    virtual LS3D_RESULT __stdcall Open(const char *file_name, uint flags, void *cb, void *context);
    virtual void __stdcall Close();
    // i3d_collision.cpp (not written yet): a ray from from along dir against the geometry; texel_cb is asked about the
    // texels it passes through (TexelCallback), with cb_context (the parameter types are guesses, the callbacks' after
    // TexelCallback's). TestColGeo1 and the others are PS2's names, but their parameters aren't checked.
    virtual int __stdcall TestColGeo1(const S_vector &from, const S_vector &dir, uint flags, S_vector *pos,
        S_vector *normal, void *cb, void *cb_context, void *texel_cb, int x) const;
    virtual int __stdcall TestColGeo(const S_vector &from, const S_vector &dir, uint flags, float *dist, S_vector *normal,
        void *cb, void *cb_context, int (__stdcall *texel_cb)(const S_vector &, float, I3D_frame *, void *, void *),
        I3D_frame *frm, int x) const;
    virtual int __stdcall TestColHierarchy() const;
    virtual int __stdcall TestColSphere() const;
    virtual int __stdcall TestColSphereHierarchy() const;
    virtual int __stdcall TestColSphereVisual() const;
    virtual int __stdcall SphereToSphereD() const;
    // Every object's and lightmapped object's SetSpace
    virtual void __stdcall SetSpace(bool world);
    virtual void __stdcall SetWeatherSystemParam(WS_PARAM param, uint value);
    virtual uint __stdcall GetWeatherSystemParam(WS_PARAM param);
    virtual void __stdcall WeatherSystemReset();
    virtual void __stdcall WeatherSystemAddDummy(I3D_dummy *dummy);
    virtual void __stdcall WeatherSystemDelDummy(I3D_dummy *dummy);
    virtual int __stdcall WeatherSystemGetDummyCnt();
    virtual DUMMY_STRUCT *__stdcall WeatherSystemGetDumies();
    virtual LS3D_RESULT __stdcall SetViewport(uint x, uint y, uint sx, uint sy);
    // PC only, empty (the name is a guess)
    virtual void __stdcall vfunc_66(uint a, uint b) {}

    // PS2 names
    void MakeReadySectors();
    void SetViewProjMatrix(const S_matrix &m);
    void m_UpdateSoundProperties(I3D_sector *sector, I3D_sector *prev_sector, float blend);
    void m_UpdateWeatherSystem();
    void m_RenderWeatherSystem();

    // PC only: the shadows and mirrors in the scene (the names are guesses after DeleteShadow's and DeleteMirror's
    // messages)
    void AddShadow(I3D_shadow *shadow);
    LS3D_RESULT DeleteShadow(I3D_shadow *shadow);
    void AddMirror(I3D_mirror *mirror);
    LS3D_RESULT DeleteMirror(I3D_mirror *mirror);

    SCN_CALLBACK *callback;             // 0x140
    uint cb_context;                    // 0x144
    C_list<I3D_frame *> tick_frames;    // 0x148, the frames the scene ticks (FRMFLAGS_TICK)
    C_list<I3D_shadow *> shadows;       // 0x158
    C_list<I3D_mirror *> mirrors;       // 0x168
    I3D_IPolSet *ipol_set;              // 0x178, made by the first SetAnimation
    I3D_camera *current_camera;         // 0x17c, SetActiveCamera's (Insanity3D's name)
    float viewport_center[2];           // 0x180, Insanity3D's names
    S_matrix m_view_proj_screen;        // 0x188, the camera's view and projection scaled to the viewport
    uchar unknown_1c8[0x40];
    float viewport_size_half[2];        // 0x208, its height's negative
    I3D_sector *primary_sector;         // 0x210, the sector outside all others (Insanity3D's name; LS3DF names it
                                        // "Primary sector" too)
    I3D_sector *backdrop_sector;        // 0x214 (Insanity3D's name)
    I3D_sector *omni_sector;            // 0x218
    I3D_sector *near_sector;            // 0x21c
    S_vector unknown_220;               // 0x220
    C_list<I3D_frame *> owned_frames;   // 0x22c, the frames AddFrame took
    RECT viewport;                      // 0x23c, x, y and the size (Insanity3D's name)
    float backdrop_ncp, backdrop_fcp;   // 0x24c, SetBackdropRange's (the names are guesses)
    int num_portals;                    // 0x254, the ring of portals
    I3D_portal *portals;                // 0x258
    uint unknown_25c;
    int num_sectors;                    // 0x260, the ring of sectors
    I3D_sector *sectors;                // 0x264
    uint unknown_268;
    uint unknown_26c;
    uint unknown_270;
    bool unknown_274;
    bool unknown_275;
    uint unknown_278;
    uint unknown_27c;
    bool unknown_280;
    ISND_3Dlistener *listener;          // 0x284
    C_list<I3D_frame *> list_288;       // 0x288, released by Close
    uint ws_flags;                      // 0x298, WSF_*
    C_list<WE_STRUCT> ws_elements;      // 0x29c, the particles
    float ws_density;                   // 0x2ac
    float ws_size;                      // 0x2b0
    float ws_speed;                     // 0x2b4
    float ws_range_x;                   // 0x2b8
    float ws_range_y;                   // 0x2bc
    int ws_count;                       // 0x2c0
    uint ws_unknown_2c4;                // 0x2c4
    S_vector ws_wind;                   // 0x2c8
    S_vector ws_dir;                    // 0x2d4, the wind when it was reset
    S_matrix ws_matrix;                 // 0x2e0, along ws_dir
    uint ws_color;                      // 0x320
    uint ws_color_2;                    // 0x324
    float ws_range;                     // 0x328, of ws_range_x and ws_range_y
    C_list<DUMMY_STRUCT> ws_dummies;    // 0x32c
};

// SYNTHETIC: LS3DF 0x10048eb0
// I3D_scene::`scalar deleting destructor'

// The destructors the scene file's thunks register for WSNearSectorList and WSNearDummyList, and the one
// m_UpdateWeatherSystem's CamOldPos registers (it has nothing to destroy). VC6 numbers them per file: Globals.cpp has
// a $E13 and a $E18 too, IGraph.cpp both. reccmp pairs the annotations of one name with our functions of that name in
// address order, so the scene's (the first in link order) and Globals.cpp's $E13 (I3d_math.h) are annotated, while
// IGraph.cpp's stay unmatched. The constructor's bigBox registers $E26, which I3d_driver.cpp has before it.
// SYNTHETIC: LS3DF 0x10048a30
// $E13

// SYNTHETIC: LS3DF 0x10048a80
// $E18

// SYNTHETIC: LS3DF 0x1004ddc0
// $E29

// The destructor render.cpp's thunk registers for CurrentActiveAreaList: the first $E10 in link order (Globals.cpp,
// ITexture.cpp and IS_sound.cpp have one too)
// SYNTHETIC: LS3DF 0x10061d00
// $E10

// The in-class functions' copies: Update, SetDir, SetName and LinkTo. The original has them at the end of the scene's
// file, ours come earlier (the headers have less inline code before them than the original's, see autopch.py). All but
// LinkTo are folded with other empty functions of their shape, so the markers are by name, the names our PDB keeps for
// the copies (Update's is the visual's m_RenderFGroup, which our linker keeps).
// FUNCTION: LS3DF 0x1004e970
// ?m_RenderFGroup@I3D_visual@@UAEXPAURENDER_FG_HELP_STRUCT@@@Z

// FUNCTION: LS3DF 0x1004e980
// ?vfunc_66@I3D_scene@@UAGXII@Z

// FUNCTION: LS3DF 0x1004e990
// ?Duplicate@I3D_scene@@UAG?AW4LS3D_RESULT@@PAVI3D_frame@@@Z

// FUNCTION: LS3DF 0x1004e9a0
// ?LinkTo@I3D_scene@@UAG?AW4LS3D_RESULT@@PAVI3D_frame@@I@Z

// The weather system's lists, out of line after the scene's functions
// TEMPLATE: LS3DF 0x1004e6e0
// C_list<WE_STRUCT>::reserve

// TEMPLATE: LS3DF 0x1004e7a0
// C_list<WE_STRUCT>::push_back

// TEMPLATE: LS3DF 0x1004e880
// C_list<DUMMY_STRUCT>::push_back

// The helper these lists' new[] construct their elements' vector arrays with; the original's copy is in the first I3D
// file, ours in the scene's
// SYNTHETIC: LS3DF 0x10001340
// `vector constructor iterator'

// The scene's function-local statics (VC6 writes them into the PDB under their plain names, but not
// m_UpdateSoundProperties' last_time at 0x101b4e70, which is in a block). The names are guesses, and their order in .bss
// isn't the original's yet: CamOldPos and LastUpdateTime come before the header's zero vector and WSNearSectorList
// there, the constructor's guard after the weather system's, bigBox after InitialSoundProp.
// GLOBAL: LS3DF 0x101b4ee0
// bigBox

// GLOBAL: LS3DF 0x101b4e48
// LastUpdateTime

// GLOBAL: LS3DF 0x101b4e30
// CamOldPos

// The scene file's copy of I3d_math.h's identity matrix (m_RenderWeatherSystem's world matrix)
// GLOBAL: LS3DF 0x1009c8b8
// IdentityMatrixData

// I3D_dummy::dummy_flags (names guessed)
#define DUMMYF_NORMALIZED 0x1           // Normalize made the box (0, 0, 0) to (1, 1, 1)
#define DUMMYF_NORMALIZED_CENTER 0x2    // Normalize made the box (-1, -1, -1) to (1, 1, 1)
#define DUMMYF_INV_MAT_VALID 0x10       // PC only: inv_matrix is up to date (cleared when the frame moves or is
                                        // normalized; after Insanity3D's FRMFLAGS_INV_MAT_VALID)

// A frame with a box (PS2 I3D_dummy, 0x19c bytes on PC; I3d_dummy.cpp). The game marks places and regions with them,
// and the scenes' weather system takes its regions from them.
// VTABLE: LS3DF 0x1009b920
class I3D_dummy : public I3D_frame {
public:
    I3D_dummy();
    ~I3D_dummy();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();
    // Moves the box into the frame's position and scale, which leaves a unit box: (-1, -1, -1) to (1, 1, 1) around the
    // box's center for mode 2, (0, 0, 0) to (1, 1, 1) from its min for the others
    virtual void __stdcall Normalize(uint mode);

    // The inverse of the world matrix (PC only; Insanity3D's frames have it under this name. T&l.cpp's copy is the one
    // linked)
    // FUNCTION: LS3DF 0x10068840
    const S_matrix &GetInvMatrix()
    {
        if (!(dummy_flags & DUMMYF_INV_MAT_VALID)) {
            dummy_flags |= DUMMYF_INV_MAT_VALID;
            inv_matrix.Inverse(GetMatrix());
        }
        return inv_matrix;
    }

    I3D_bbox bbox;                      // 0x140, in local space
    S_matrix inv_matrix;                // 0x158, PC only
    uint dummy_flags;                   // 0x198, DUMMYF_*
};

// SYNTHETIC: LS3DF 0x1001a240
// I3D_dummy::`scalar deleting destructor'

// The other frames, by the sizes the driver allocates (the landscape's name is a guess)
#define I3D_PLACEHOLDER_FRAME(name, size) \
    class __declspec(novtable) name : public I3D_frame { \
    public: \
        name(); \
        uchar unknown_140[size - 0x140]; \
    };

// A light's type (PS2 type; the values are Insanity3D's I3DLIGHT_*, whose order the PC's lighting code agrees with)
enum I3D_LIGHTTYPE {
    I3DLIGHT_NULL,
    I3DLIGHT_POINT,
    I3DLIGHT_SPOT,
    I3DLIGHT_DIRECTIONAL,
    I3DLIGHT_AMBIENT,
    I3DLIGHT_FOG,
    I3DLIGHT_POINTAMBIENT,
    I3DLIGHT_POINTFOG,
};

// I3D_light::mode: what the light lights (Insanity3D's names; PrepareLights is given the first, the lightmaps' code
// tests the second)
#define I3DLIGHTMODE_VERTEX 0x1
#define I3DLIGHTMODE_LIGHTMAP 0x2
#define I3DLIGHTMODE_LIGHTNESS 0x40     // taken by I3D_scene::GetLightness (the name is a guess)

// A light (PS2 I3D_light, 0x1f0 bytes on PC; I3d_light.cpp isn't written yet). The members are the ones T&l.cpp and the
// scene read, named after Insanity3D's where the roles agree; the light's UpdateV computes the world values from the
// others. Its virtual functions are PS2's, in PS2's order (SetDiffuse's overloads in reverse, as MSVC orders them).
class __declspec(novtable) I3D_light : public I3D_frame {
public:
    I3D_light();

    virtual void __stdcall SetLightType(I3D_LIGHTTYPE type);
    virtual void __stdcall SetDiffuse(const S_vector &color);
    virtual void __stdcall SetDiffuse(float r, float g, float b);

    uchar unknown_140[0x10];
    uint mode;                          // 0x150, I3DLIGHTMODE_*
    I3D_LIGHTTYPE ltype;                // 0x154
    float power;                        // 0x158
    S_vector diffuse;                   // 0x15c, SetDiffuse's
    float near_range, far_range;        // 0x168, a point light fades out between them (GetLightness); a fog's
                                        // density is near_range
    uchar unknown_170[8];
    S_vector w_pos;                     // 0x178, in world space
    S_vector w_dir;                     // 0x184, in world space
    S_vector color;                     // 0x190, the power applied
    float range_n;                      // 0x19c
    float outer_cos;                    // 0x1a0, of the spot's cone
    uchar unknown_1a4[0xc];
    float inner_cos;                    // 0x1b0
    uchar unknown_1b4[0xc];
    float cone_scale;                   // 0x1c0, 1 / (inner_cos - outer_cos)
    uchar unknown_1c4[0xc];
    float range_f;                      // 0x1d0
    float range_delta;                  // 0x1d4, range_f - range_n
    float range_f_2;                    // 0x1d8, its square
    uint unknown_1dc;                   // 0x1dc
    I3D_bsphere bsphere;                // 0x1e0, what a spot lights
};

// Planes that bound what a camera sees (PC 0x50 bytes; the name is a guess): unit normals pointing inside and their
// distances (normal.Dot(p) + d = 0 on the plane). The first is the view direction's, the others are the sides'. The
// normals have no constructor: the camera's constructor constructs nothing for them, as an S_vector array would need.
struct S_frustum {
    D3DVECTOR normal[5];
    float d[5];
};

// A camera (PS2 I3D_camera, 0x4f4 bytes on PC; I3d_camera.cpp). Besides the view it projects the scene's backdrop (with
// the scene's backdrop range) and things near the viewer (the near field of view and range, PS2's SetNearFOV and
// SetNearRange). Names are Insanity3D's where it has the member (fov_a, ncp, fcp, ortho_scale, m_view, m_proj; the
// scene's aspect_ratio, the orthogonal flag, curr_sector's role), the others are guesses.
// VTABLE: LS3DF 0x1009b6d0
class I3D_camera : public I3D_frame {
public:
    I3D_camera();
    ~I3D_camera();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    // PS2 names
    virtual void __stdcall SetFOV(float fov);
    virtual void __stdcall SetNearFOV(float fov);
    virtual void __stdcall SetRange(float near1, float far1);
    virtual void __stdcall SetNearRange(float near1, float far1);
    virtual void __stdcall SetOrthoScale(float scale);
    virtual void __stdcall SetOrthogonal(bool b);
    virtual void __stdcall SetAspectRatio(float aspect);

    // PS2 names
    void UpdateProjMatrix();
    void UpdateSector();

    float fov_a;                        // 0x140, the field of view
    float ncp, fcp;                     // 0x144, the clipping planes
    float near_fov_a;                   // 0x14c, the near projection's
    float near_ncp, near_fcp;           // 0x150
    float aspect_ratio;                 // 0x158, the screen's width / height
    bool orthogonal;                    // 0x15c
    float ortho_scale;                  // 0x160
    S_matrix m_view;                    // 0x164, the inverse of the world matrix
    S_matrix m_proj;                    // 0x1a4
    S_matrix m_view_proj;               // 0x1e4
    S_matrix view_inv;                  // 0x224, the inverse of m_view: the billboards align to it
    S_matrix m_bd_view;                 // 0x264, the backdrop's: m_view without the translation
    S_matrix m_bd_view_inv;             // 0x2a4, view_inv without the translation
    S_matrix m_bd_proj;                 // 0x2e4
    S_matrix m_bd_view_proj;            // 0x324
    S_matrix m_near_proj;               // 0x364, the near projection, with m_view
    S_matrix m_near_view_proj;          // 0x3a4
    float ncp_x, ncp_y;                 // 0x3e4, half the near clipping plane's width and height
    float near_ncp_x, near_ncp_y;       // 0x3ec, the same for the near projection
    I3D_scene *scene;                   // 0x3f4, the scene the camera is active in (I3D_scene::current_camera)
    I3D_sector *sector;                 // 0x3f8, the sector it is in
    uint unknown_3fc;                   // 0x3fc
    uint unknown_400;                   // 0x400
    S_frustum frustums[3];              // 0x404, of the view, the backdrop and the near projection
};

// SYNTHETIC: LS3DF 0x1000b250
// I3D_camera::`scalar deleting destructor'

// A frame that carries the game's data (PS2 I3D_user, 0x144 bytes on PC). Insanity3D's has the one member.
class __declspec(novtable) I3D_user : public I3D_frame {
public:
    I3D_user();

    uint add_data;                      // 0x140
};

I3D_PLACEHOLDER_FRAME(I3D_joint, 0x20c)
I3D_PLACEHOLDER_FRAME(I3D_volume, 0x174)
I3D_PLACEHOLDER_FRAME(I3D_landscape, 0x62144)
// A particle emitter (PS2 I3D_emitor, 0x1f8 bytes on PC; the file isn't written yet)
class __declspec(novtable) I3D_emitor : public I3D_frame {
public:
    I3D_emitor();

    uchar unknown_140[8];
    I3D_mesh_object *mesh;              // 0x148, its particles' (I3D_object::Duplicate)
    uchar unknown_14c[0x1f8 - 0x14c];
};

// The frames whose members the frames call (PS2 names)
class __declspec(novtable) I3D_target : public I3D_frame {
public:
    I3D_target();

    void UpdateFrames();
    static void DeleteFromAnyTarget(I3D_frame *frame);

    uchar unknown_140[0x14];
};

class __declspec(novtable) I3D_model : public I3D_frame {
public:
    I3D_model();

    virtual void __stdcall vfunc_20(); virtual void __stdcall vfunc_21(); virtual void __stdcall vfunc_22();
    virtual void __stdcall DeleteFrame(I3D_frame *frame);

    uchar unknown_140[0x18];
};

class __declspec(novtable) I3D_occluder : public I3D_frame {
public:
    I3D_occluder();

    void AddToRemoveFromSceneRing(I3D_scene *scene);

    uchar unknown_140[0x4ac];
};

class __declspec(novtable) I3D_area : public I3D_frame {
public:
    I3D_area();

    void AddToRemoveFromSceneRing(I3D_scene *scene);

    uchar unknown_140[0x3c];
};

class __declspec(novtable) I3D_shadow : public I3D_frame {
public:
    I3D_shadow();

    void SetTo(I3D_frame *frame);
    static void DeleteFromAnyShadow(I3D_frame *frame);

    uchar unknown_140[0x48];
    I3D_scene *scene;                   // 0x188, the scene's AddShadow
    uchar unknown_18c[0x88];
};

// The weights of a visual's levels and regions (PS2 I3D_void_interface; the parameters' names are guesses). The
// visuals derive from it, the morphs implement it. Its functions are I3d_visual.cpp's; NumLevels' and NumRegions'
// (return 0) are folded with a DirectShow base class function (0x10087520), so they have no markers.
// VTABLE: LS3DF 0x1009d108
class I3D_void_interface {
public:
    virtual int __stdcall NumLevels() const;
    virtual int __stdcall NumRegions() const;
    virtual LS3D_RESULT __stdcall SetWeight(int level, int region, float weight);
    virtual float __stdcall GetWeight(int level, int region) const;
};

// What SetShadowMode and SetProjectorMode take (PS2 type; the values' names aren't known). The shadows and projectors
// fall on the visuals with mode 1 or 2; nothing in LS3DF tells 2 from 1.
enum SHADOW_MODE {
    SHADOW_MODE_0,
    SHADOW_MODE_1,                      // the visuals' default
    SHADOW_MODE_2,
};

// I3D_visual::vis_flags (names guessed; BOUND_VALID and INV_MAT_VALID after Insanity3D's VISF_BOUNDS_VALID and
// FRMFLAGS_INV_MAT_VALID)
#define VISF_UNKNOWN_10 0x10            // the level of detail follows the textures loaded, and the lighting takes
                                        // the first lod's cache, not the current one's
#define VISF_WORLD_SPACE 0x40           // the vertices are in world space (SetSpace): a move invalidates nothing
#define VISF_UNKNOWN_80 0x80            // the lighting gives every vertex their average color
#define VISF_SHADOW 0x200               // SHADOW_MODE 1 or 2: shadows fall on it
#define VISF_SHADOW_2 0x400             // SHADOW_MODE 2
#define VISF_UNKNOWN_800 0x800          // set by the objects' constructor, SetMesh and Duplicate
#define VISF_PROJECTOR 0x2000           // projector mode 1 or 2: projectors fall on it
#define VISF_PROJECTOR_2 0x4000         // projector mode 2
#define VISF_PROXY 0x10000              // renders the face groups of the visual in user_data (the name is a guess)
#define VISF_BOUND_VALID 0x20000        // vis_bound is up to date (UpdateBoundProc)
#define VISF_WBOUND_VALID 0x40000       // vis_bound_w is up to date (UpdateWBoundProc)
#define VISF_UNKNOWN_80000 0x80000      // cleared with VISF_WBOUND_VALID when the frame moves; nothing in LS3DF tests it
#define VISF_INV_MAT_VALID 0x100000     // inv_matrix is up to date
#define VISF_LOD_CHANGED 0x2000000      // GetObjectLOD changed a VISF_UNKNOWN_10 visual's level

struct RENDER_FG_HELP_STRUCT;

// A frame with something to render (PS2 I3D_visual, 0x1dc bytes on PC; I3d_visual.cpp), the base of the visuals the
// driver creates. Its own bound volume is in vis_bound, which the derived classes' UpdateBoundProc computes; the
// frame's bound volume takes its box from there (I3D_frame::UpdateHRBoundVol). PS2's virtual GetVisualType and
// SetBoundBox aren't in LS3DF, and SetBoundVolume isn't virtual on PC.
// VTABLE: LS3DF 0x1009d098 I3D_frame
// VTABLE: LS3DF 0x1009d088 I3D_void_interface
class I3D_visual : public I3D_frame, public I3D_void_interface {
public:
    I3D_visual();
    ~I3D_visual();

    virtual void UpdateV();
    // Only the derived classes that can move their vertices support a change of space; the others print an error
    virtual void __stdcall SetSpace(bool world);
    virtual void __stdcall SetShadowMode(SHADOW_MODE mode);
    virtual SHADOW_MODE __stdcall GetShadowMode() const;
    virtual void __stdcall SetProjectorMode(SHADOW_MODE mode);
    virtual SHADOW_MODE __stdcall GetProjectorMode() const;
    // Computes vis_bound (PS2 name); the base only marks it valid
    virtual void __stdcall UpdateBoundProc();
    // Empty (PS2 inlines both; the linker folded the copies with other files' empty functions)
    virtual void m_AddFGroups() {}
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help) {}

private:
    // Exported (the decorated name says private)
    __declspec(dllexport) void __stdcall UpdateWBoundProc();

public:
    void SetBoundVolume(const I3D_bvolume &bvol);

    // The visual's bound volume in world space, updated first if it isn't (the name is a guess; the lighting and the
    // visuals' rendering inline it)
    const I3D_bvolume &GetWBoundVolume()
    {
        if (!(vis_flags & VISF_WBOUND_VALID))
            UpdateWBoundProc();
        return vis_bound_w;
    }

    // The inverse of the world matrix, identity in world space (Insanity3D's name; other files inline it)
    const S_matrix &GetInvMatrix()
    {
        if (vis_flags & VISF_WORLD_SPACE)
            return IdentityMatrix;
        if (!(vis_flags & VISF_INV_MAT_VALID)) {
            vis_flags |= VISF_INV_MAT_VALID;
            inv_matrix.Inverse(GetMatrix());
        }
        return inv_matrix;
    }

    // The matrix the vertices are rendered with: the world matrix, identity in world space (the name is a guess)
    const S_matrix &GetRenderMatrix()
    {
        if (vis_flags & VISF_WORLD_SPACE)
            return IdentityMatrix;
        return GetMatrix();
    }

    S_matrix inv_matrix;                // 0x144, the inverse of the world matrix (identity in world space)
    I3D_bvolume vis_bound_w;            // 0x184, the visual's own, in world space
    I3D_bvolume vis_bound;              // 0x1ac, in local space
    I3D_VISUAL_TYPE visual_type;        // 0x1d4, set by the derived classes
    uint vis_flags;                     // 0x1d8, VISF_*
};

// SYNTHETIC: LS3DF 0x1005a350
// I3D_visual::`scalar deleting destructor'

#define I3D_PLACEHOLDER_VISUAL(name, size) \
    class __declspec(novtable) name : public I3D_visual { \
    public: \
        name(); \
        uchar unknown_1dc[size - 0x1dc]; \
    };

// An object's lighting state for one of its mesh's levels (PC only; the object file's "lod::Create()" names the
// class): its own vertex buffer, which the object renders from
// VTABLE: LS3DF 0x1009c3d8
class lod {
public:
    lod();
    virtual ~lod();

    // Copies the texture coordinates at uv (stride bytes apart, offset by u_ofs and v_ofs) into the vertices of format
    // 1 or 2 (one or two coordinate pairs), as m_HardwareCaps' T&L bit lays them out (the name is a guess)
    void SetUV(uint format, int num, const I3D_text_coor *uv, uint stride, float u_ofs, float v_ofs);

    // Gives the lod a vertex buffer of the format (flags' low bits; 4 needs hardware T&L) if it has none, and fills in
    // the positions (with flag 0x100, hardware T&L only) and texture coordinates given; false if out of memory
    bool Create(uint flags, uint num, const S_vector *pos, uint pos_stride, const I3D_text_coor *uv, uint uv_stride,
        const I3D_text_coor *uv2, uint uv2_stride, float u_ofs, float v_ofs);

    uint lights_changed;                // 0x04, the sector's lights_changed when it was lit, -1
    uint update_count;                  // 0x08, the object's update_count then, -1
    uint light_key;                     // 0x0c, PrepareLights' key of the lights it was lit by, -1
    COMPLETE_VB_STRUCT vb;              // 0x10
    float u_ofs, v_ofs;                 // 0x1c, SetUV's
    float env_u_ofs, env_v_ofs;         // 0x24, the mesh's when its environment map was computed
    bool dirty;                         // 0x2c, to update (I3D_object::UpdateVertices)
    bool alpha_dirty;                   // 0x2d, the alpha changed (SetTransparency; the name is a guess)
    uchar alpha;                        // 0x2e, of the colors PrepareLights wrote, 0xff
};

// SYNTHETIC: LS3DF 0x10036cb0
// lod::`scalar deleting destructor'

// A visual made of a mesh (PS2 I3D_object, 0x21c bytes on PC; I3d_object.cpp): a level of the mesh is rendered from a
// lod of the object's own. Its billboards, lens flares, single meshes, morphs, particles and lit objects derive from it.
// VTABLE: LS3DF 0x1009c348 I3D_frame
// VTABLE: LS3DF 0x1009c338 I3D_void_interface
class I3D_object : public I3D_visual {
public:
    I3D_object();
    ~I3D_object();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();
    virtual void __stdcall SetSpace(bool world);
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);
    // keep_lods keeps the lods if the meshes are compatible
    virtual void __stdcall SetMesh(I3D_mesh_object *m, bool keep_lods);
    // Gives the object a copy of its mesh if the mesh is shared
    virtual LS3D_RESULT __stdcall DetachMesh();
    // Marks the lods to update (PS2 names; VC6 puts the second at the first's slot)
    virtual void __stdcall UpdateVertices();
    virtual void __stdcall UpdateVertices(int index);
    virtual void __stdcall SetLightness(float l);
    virtual void __stdcall SetTransparency(float t);
    // Has the lods lit again (PC only; Insanity3D's name for the same, which returns early without a mesh too)
    virtual void __stdcall ResetLightInfo();
    // A lod's vertices, locked (PC only, the name is a guess; the flags aren't used)
    virtual uchar *__stdcall LockVertices(int index, uint flags);

    // Like SetTransparency, for fade (PC only, the name is a guess)
    void SetFade(float f);

    float sort_dist;                    // 0x1dc, orders the transparent face groups (set by the rendering)
    I3D_mesh_object *mesh;              // 0x1e0
    lod *lods[10];                      // 0x1e4, one per level of the mesh
    uint cur_lod;                       // 0x20c
    float lightness;                    // 0x210, scales the lights (PS2 SetLightness)
    float transparency;                 // 0x214, 1.0 opaque
    float fade;                         // 0x218, scales the transparency (the name is a guess)
};

// SYNTHETIC: LS3DF 0x10036820
// I3D_object::`scalar deleting destructor'

// A visual that turns to the camera (PS2 I3D_billboard, 0x268 bytes on PC; I3d_billboard.cpp): its mesh is rendered
// with g_matrix, which ProductBillMatrix computes for the camera. Names are Insanity3D's (rot_axis, g_matrix) or guesses.
// VTABLE: LS3DF 0x1009b520 I3D_frame
// VTABLE: LS3DF 0x1009b510 I3D_void_interface
class I3D_billboard : public I3D_object {
public:
    I3D_billboard();
    ~I3D_billboard();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();
    // The vertices stay in local space
    virtual void __stdcall SetSpace(bool world);
    virtual void __stdcall UpdateBoundProc();
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);
    virtual void __stdcall SetMesh(I3D_mesh_object *m, bool keep_lods);

    // The matrix the billboard is rendered with, turned to the camera at cam_pos (PS2 name)
    void ProductBillMatrix(S_vector &cam_pos, S_matrix *m);

    uint rot_axis;                      // 0x21c, 0 x, 1 y, 2 z
    int axis_mode;                      // 0x220, 0 to 3, see ProductBillMatrix (0 faces the camera)
    S_matrix g_matrix;                  // 0x224, the matrix rendered with
    I3D_frame *env_dummy;               // 0x264, the parent's first dummy child, EnvProcess' mode 5 maps into its box
};

// SYNTHETIC: LS3DF 0x10005df0
// I3D_billboard::`scalar deleting destructor'

// A glow of a lens flare (PS2 GLOW_STRUCT; the PC's is 8 bytes)
struct GLOW_STRUCT {
    float pos;                          // where on the line from the light through the screen's center
    I3D_material *mat;
};

// A light's flare on the screen (PS2 I3D_lensflare, 0x27c bytes on PC; I3d_billboard.cpp): its glows, drawn by Draw
// (PS2 name), which the scene calls for each lens flare after the rendering
// VTABLE: LS3DF 0x1009b5c8 I3D_frame
// VTABLE: LS3DF 0x1009b5b8 I3D_void_interface
class I3D_lensflare : public I3D_object {
public:
    I3D_lensflare();
    ~I3D_lensflare();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();
    virtual void __stdcall Draw(bool test);

    // Names guessed from Draw's use
    float intensity;                    // 0x21c, scales the glows' alpha
    float size;                         // 0x220, scales the glows' textures' size in pixels
    uint unknown_224;                   // 0x224
    uint test_interval;                 // 0x228, ms between the occlusion tests, 150
    uint last_check_col_time;           // 0x22c, timeGetTime() of the last one (Insanity3D's name)
    uint last_update;                   // 0x230, of the last fade step
    float visibility;                   // 0x234, fades to target_visibility
    float target_visibility;            // 0x238, 0 if occluded
    S_vector color;                     // 0x23c, fades to target_color
    S_vector target_color;              // 0x248, what the occlusion test saw (TexelCallback)
    float fade_in, fade_out;            // 0x254, per second, 15 and 8
    C_list<GLOW_STRUCT> glows;          // 0x25c
    float near_dist;                    // 0x26c, within it a single glow grows (by near_scale)
    float far_dist;                     // 0x270, beyond it a single glow shrinks to nothing at range
    float range;                        // 0x274, 300
    float near_scale;                   // 0x278
};

// SYNTHETIC: LS3DF 0x10009610
// I3D_lensflare::`scalar deleting destructor'

// TEMPLATE: LS3DF 0x1000af20
// C_list<GLOW_STRUCT>::reserve

I3D_PLACEHOLDER_VISUAL(I3D_lit_object, 0x370)
I3D_PLACEHOLDER_VISUAL(I3D_singlemesh, 0x22c)
I3D_PLACEHOLDER_VISUAL(I3D_morph, 0x238)
I3D_PLACEHOLDER_VISUAL(I3D_singlemorph, 0x248)
I3D_PLACEHOLDER_VISUAL(I3D_projector, 0x264)
I3D_PLACEHOLDER_VISUAL(I3D_part_base, 0x284)
I3D_PLACEHOLDER_VISUAL(I3D_part_element, 0x310)
I3D_PLACEHOLDER_VISUAL(I3D_landpatch, 0x620)
// A mirror (PS2 I3D_mirror, 0x2ec bytes on PC; the file isn't written yet)
class __declspec(novtable) I3D_mirror : public I3D_visual {
public:
    I3D_mirror();

    uchar unknown_1dc[4];
    I3D_scene *scene;                   // 0x1e0, the scene's AddMirror
    uchar unknown_1e4[0x2ec - 0x1e4];
};

// I3D_material::flags (names guessed)
#define MATF_ENVMAP_BLEND 0x700         // how the environment map is blended (1 to 5, I3D_object::m_RenderFGroup)
#define MATF_LOD0 0x800                 // the objects render the face group from their first lod
#define MATF_ENVMAP_MODE 0x7000         // with MATF_ENVMAP, a level's MESHF_ENVMAP_MODE value
#define MATF_TEXTURE 0x40000            // texture is used; gives its level MESHF_TEXTURE
#define MATF_ENVMAP 0x80000             // env_texture is used; gives its level MESHF_ENVMAP, and the low byte divides
                                        // the level's env_scale
#define MATF_ADDRESS_U_CLAMP 0x10000    // the texture is clamped, not wrapped
#define MATF_ADDRESS_V_CLAMP 0x20000
#define MATF_ANIMATED 0x4000000         // UpdateTexture picks the frame
#define MATF_KEEP_DIFFUSE 0x8000000     // textured, the lights still take the diffuse color: gives its level
                                        // MESHF_KEEP_DIFFUSE (after Insanity3D's loader's keep_diffuse)
#define MATF_TWO_SIDED 0x10000000       // not culled
// The last three are Insanity3D's names for the same roles (its values differ)
#define MATF_CKEY_ALPHA 0x20000000      // alpha tested against m_AlphaRef
#define MATF_TXT_ALPHA 0x40000000       // the texture has alpha: blended (I3D_driver::GetIcon sets it)
#define MATF_ADD_MODE 0x80000000        // blended additively

// A material (PS2 I3D_material, 0x40 bytes on PC), reference counted
class __declspec(novtable) I3D_material {
public:
    I3D_material();

    virtual int __stdcall Release();
    // PS2 names (0x1002af20 to 0x1002af80); each returns I3D_OK
    virtual LS3D_RESULT __stdcall SetAmbient(const S_vector &color);
    virtual LS3D_RESULT __stdcall SetDiffuse(const S_vector &color);
    virtual LS3D_RESULT __stdcall SetEmissive(const S_vector &color);
    virtual LS3D_RESULT __stdcall SetAlpha(float a);
    virtual void __stdcall SetTexture(ITexture *texture);
    // PS2 name: makes this material a copy of mat
    virtual LS3D_RESULT __stdcall Duplicate(I3D_material *mat);
    // Sets MATF_TWO_SIDED (0x1002b120; Insanity3D's name, whose version takes a bool)
    virtual LS3D_RESULT __stdcall Set2Sided();
    // PS2 name: an animated material's frame
    virtual void __stdcall UpdateTexture();
    // PS2 name: the same for the environment map
    virtual void __stdcall UpdateEnvTexture();
    // PS2 name (0x1002b040): the environment map, with its MATF_ENVMAP_BLEND and MATF_ENVMAP_MODE values and env_alpha;
    // a NULL texture clears MATF_ENVMAP
    virtual LS3D_RESULT __stdcall SetEnvTexture(ITexture *texture, uint blend, uint mode, float a);

    int ref;                            // 0x04
    uint flags;                         // 0x08, MATF_*
    ITexture *texture;                  // 0x0c
    ITexture *env_texture;              // 0x10
    S_vector ambient;                   // 0x14
    S_vector diffuse;                   // 0x20
    S_vector emissive;                  // 0x2c
    float alpha;                        // 0x38, 1.0 opaque
    float env_alpha;                    // 0x3c, the environment map's blend factor (MATF_ENVMAP_BLEND 1; PS2's
                                        // SetEnvAlpha)

    void AddRef()
    {
        ref++;
    }

    ITexture *GetTexture()
    {
        if (flags & MATF_ANIMATED)
            UpdateTexture();
        return texture;
    }

    ITexture *GetEnvTexture()
    {
        if (flags & MATF_ANIMATED)
            UpdateEnvTexture();
        return env_texture;
    }
};

// An animated material (PS2 I3D_anim_material)
class __declspec(novtable) I3D_anim_material : public I3D_material {
public:
    I3D_anim_material();

    uchar unknown_40[0x48];
};

// The meshes (PS2 I3d_mesh.cpp; I3d_mesh.cpp): an object's mesh has up to 10 levels of detail, each a vertex buffer and
// a chain of face groups, one per material, each an index buffer. The buffers are blocks of the vertex and index buffer
// managers' (VBManager.h, IBManager.h). Class and function names are PS2's unless noted.

class I3D_face_group;

// A mesh's vertex (PS2 name): D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1
struct I3D_vertex {
    S_vector pos;
    S_vector normal;
    I3D_text_coor tex;
};

#define I3D_VERTEX_FVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

// What the meshes' GetStats fill in (PC only, names guessed)
struct I3D_mesh_stats {
    uint size;                          // of the mesh's object
    uint num_vertices;
    uint vertex_bytes;
    uint num_faces;
    uint face_bytes;
};

// A face group: a material's triangles of a mesh level (0x1c bytes on PC). Its destructor is I3d_mesh.cpp's.
// VTABLE: LS3DF 0x1009bfe0
class I3D_face_group {
public:
    I3D_face_group();
    ~I3D_face_group();

    virtual void __stdcall SetMaterial(I3D_material *mat);
    // A copy of the faces, NULL to free them (PC only, the name is a guess)
    virtual void __stdcall SetFaces(const I3D_triface *faces, int num);
    // Returns the new faces, locked
    virtual I3D_triface *__stdcall CreateFaces(int num);
    // Locks the faces (PS2 GetFaces takes a lock flag)
    virtual I3D_triface *__stdcall GetFaces();
    // PC only, the name is a guess
    virtual void __stdcall UnlockFaces();
    virtual LS3D_RESULT __stdcall Duplicate(I3D_face_group *fg);
    // Gives the face group a material of its own if the material is shared (PC only)
    virtual LS3D_RESULT __stdcall DetachMaterial();

    I3D_material *material;             // 0x04
    int num_faces;                      // 0x08
    I3D_face_group *next;               // 0x0c, the level's next one
    COMPLETE_IB_STRUCT ib;              // 0x10
};

// SYNTHETIC: LS3DF 0x10030960
// I3D_face_group::`scalar deleting destructor'

// I3D_mesh_level::mesh_flags (names guessed; the bits come from the face groups' materials, see AddFGroup)
#define MESHF_ENVMAP_MODE 0x7           // a material's MATF_ENVMAP_MODE value: EnvProcess's mode
#define MESHF_ENVMAP 0x8                // a material has an environment map: the lods have a second pair of coordinates
#define MESHF_TEXTURE 0x10              // a material has a texture
#define MESHF_KEEP_DIFFUSE 0x20         // a material has MATF_KEEP_DIFFUSE

// A mesh level: a level of detail's vertices and face groups (0x44 bytes on PC), reference counted. Its destructor is
// I3d_mesh.cpp's.
// VTABLE: LS3DF 0x1009bffc
class I3D_mesh_level {
public:
    I3D_mesh_level();
    ~I3D_mesh_level();

    virtual int __stdcall Release();
    // Bit 0 of flags locks for reading only
    virtual I3D_vertex *__stdcall LockVertices(uint flags);
    virtual void __stdcall UnlockVertices();
    virtual LS3D_RESULT __stdcall Duplicate(I3D_mesh_level *level);
    virtual int __stdcall NumFaces() const;
    virtual I3D_face_group *__stdcall GetFGroup(int index);
    // PC only, the name is a guess
    virtual LS3D_RESULT __stdcall GetStats(I3D_mesh_stats &stats);
    // At the chain's end if last, or if the material is transparent, else at its start
    virtual void __stdcall AddFGroup(I3D_face_group *fg, bool last);
    virtual LS3D_RESULT __stdcall SetVertexMesh(const I3D_vertex *verts, int num);
    // Returns the new vertices, locked
    virtual I3D_vertex *__stdcall CreateVertexMesh(int num);
    virtual void __stdcall UpdateBBox();

    int ref;                            // 0x04
    int num_vertices;                   // 0x08
    COMPLETE_VB_STRUCT vb;              // 0x0c
    uint num_fgroups;                   // 0x18
    I3D_face_group *fgroups;            // 0x1c
    I3D_bbox bbox;                      // 0x20
    float lod_dist;                     // 0x38, the squared distance up to which the level is rendered (GetObjectLOD)
    uint mesh_flags;                    // 0x3c, MESHF_*
    float env_scale;                    // 0x40, the environment map's (EnvProcess), 2 * sqrt(2) by default

    // The flags (the name is a guess; the visuals' m_AddFGroups read them through it, which their inlining shows)
    uint GetFlags() const
    {
        return mesh_flags;
    }
};

// A mesh: up to 10 levels of detail and their bound volume (0x6c bytes on PC), reference counted. Its destructor is
// I3d_mesh.cpp's.
// VTABLE: LS3DF 0x1009c030
class I3D_mesh_object {
public:
    I3D_mesh_object();
    ~I3D_mesh_object();

    virtual int __stdcall Release();
    virtual I3D_mesh_level *__stdcall GetLOD(int index) const;
    virtual LS3D_RESULT __stdcall SetLOD(I3D_mesh_level *level, int index);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_mesh_object *mesh);
    // PC only, the name is a guess
    virtual LS3D_RESULT __stdcall GetStats(I3D_mesh_stats &stats);
    virtual void __stdcall UpdateBoundVolume();
    // Every face group's DetachMaterial (PC only, the name is a guess)
    virtual LS3D_RESULT __stdcall DetachMaterials();
    // Whether the meshes have as many levels with as many vertices (PC only, the name is a guess)
    virtual bool __stdcall IsCompatible(I3D_mesh_object *mesh);

    void AddRef()
    {
        ref++;
    }

    int ref;                            // 0x04
    uint num_lods;                      // 0x08
    I3D_mesh_level *levels[10];         // 0x0c
    I3D_bvolume bvol;                   // 0x34
    float u_ofs, v_ofs;                 // 0x5c, added to the texture coordinates (lod::Create)
    float env_u_ofs, env_v_ofs;         // 0x64, to the environment map's (EnvProcess)
};

// The other parts the driver makes (PS2 names)
#define I3D_PLACEHOLDER(name, size) \
    class name { \
    public: \
        name(); \
        uchar unknown_00[size]; \
    };

I3D_PLACEHOLDER(I3D_animation_set, 0x14)
I3D_PLACEHOLDER(I3D_pose, 0x38)
I3D_PLACEHOLDER(I3D_pose_set, 0x18)

// The base classes' empty CBaseRenderer::OnRenderEnd, which the linker folds with I3D_driver::m_RecomputeUsedVRAM:
// the original kept the base classes' copy. Our map lists our copy under the driver's object, so crtannotate leaves
// it out of library_strmbase.h; the name is the one our PDB keeps (tools/foldfix.py)
// LIBRARY: LS3DF 0x10087ec0
// ?m_RecomputeUsedVRAM@I3D_driver@@UAGXXZ

// The function-local statics of I3d_driver.cpp: VC6 puts a function's static into the PDB as a module-level symbol
// with its plain name, where reccmp doesn't look for one, so by-name markers match them. FrameType2StringFunc's
// buffer comes first, VisualType2String's second.

// RenderShadowTexture's list of the points in texture coordinates, its guard and the destructor its first call
// registers with atexit. VC6 numbers the last two per file (after the zero vector's three numbers), so the names
// hold only while no other file has them.
// GLOBAL: LS3DF 0x100bfc88
// points

// GLOBAL: LS3DF 0x100bffc0
// $S23

// SYNTHETIC: LS3DF 0x100165e0
// $E24

// DrawSphere's matrix
// GLOBAL: LS3DF 0x100afc28
// m

// GLOBAL: LS3DF 0x100af8e8
// str

// GLOBAL: LS3DF 0x100bffa8
// si

// GLOBAL: LS3DF 0x100bffc8
// str

// Stubs in I3d_driver.cpp, I3d_frame.cpp and I3d_scene.cpp, after their files' functions though their addresses come
// before them, so their markers are here, by name

// STUB: LS3DF 0x10001370
// FreeClipArrays

// STUB: LS3DF 0x10004890
// I3D_animation_set::I3D_animation_set

// STUB: LS3DF 0x100051a0
// I3D_area::I3D_area

// STUB: LS3DF 0x10005440
// I3D_area::AddToRemoveFromSceneRing

// STUB: LS3DF 0x1000dec0
// I3D_scene::TestColGeo

// STUB: LS3DF 0x10011430
// I3D_scene::TestColGeo1

// STUB: LS3DF 0x100118e0
// I3D_scene::TestColHierarchy

// STUB: LS3DF 0x10011d60
// I3D_scene::TestColSphere

// STUB: LS3DF 0x10014aa0
// I3D_scene::TestColSphereHierarchy

// STUB: LS3DF 0x10014e50
// I3D_scene::TestColSphereVisual

// STUB: LS3DF 0x100153b0
// I3D_scene::SphereToSphereD

// STUB: LS3DF 0x1001e280
// I3D_IPolSet::~I3D_IPolSet

// STUB: LS3DF 0x1001e470
// I3D_IPolSet::Tick

// STUB: LS3DF 0x1001e580
// I3D_IPolSet::SetAnimation

// STUB: LS3DF 0x100419e0
// I3D_portal::UpdatePortal

// An untransformed vertex with a color, as DrawLines and DrawTriangles pass them (PS2 LVERTEX, D3DFVF_XYZ |
// D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 without the specular color)
struct LVERTEX {
    float x, y, z;
    uint color;
    float u, v;
};

// A transformed vertex (PS2 TLVERTEX)
struct TLVERTEX {
    float x, y, z, rhw;
    uint color, specular;
    float u, v;
};

// The same with a second pair of texture coordinates, as the objects' lods have them (the names are guesses)
struct LVERTEX2 {
    float x, y, z;
    uint color;
    float u, v;
    float u2, v2;
};

struct TLVERTEX2 {
    float x, y, z, rhw;
    uint color, specular;
    float u, v;
    float u2, v2;
};

// The draws' lists' reserve, out of line where VC6 didn't inline it, after I3d_driver.cpp's functions
// TEMPLATE: LS3DF 0x1001a100
// C_list<TLVERTEX>::reserve
// TEMPLATE: LS3DF 0x1001a170
// C_list<LVERTEX>::reserve

// The pointer lists' push_back, one copy for every pointer type the linker folded; the original keeps it among
// I3D files written later (0x10041700)
// TEMPLATE: LS3DF 0x10041700
// ?push_back@?$C_list@PAVC_VBuffer@@@@QAEXABQAVC_VBuffer@@@Z

// The frame's empty Tick, folded with the other empty functions of its shape (29 vtables' slots; our PDB names the
// copy after the scene's SetOn); the original keeps the area file's copy
// FUNCTION: LS3DF 0x10005240
// ?SetOn@I3D_scene@@UAGX_N@Z

// The 3D driver (PS2 I3d_driver.cpp), 0x5c bytes, created by Globals.cpp's BreBre. Its virtual functions are in
// PS2's order; the PC lacks PS2's DeleteReleasedObjects, SetLightness, SetTransparency, GetHRBbox and GetHRBboxEx,
// and its Render takes the scene only.
// VTABLE: LS3DF 0x1009b830
class I3D_driver {
public:
    I3D_driver();
    ~I3D_driver();

    virtual LS3D_RESULT __stdcall Init(uint flags);
    virtual LS3D_RESULT __stdcall Close();
    virtual void __stdcall Tick(int time);
    virtual int __stdcall GetTickTime() const;
    virtual LS3D_RESULT __stdcall GetStats(I3DSTATSINDEX index, void *data);
    virtual LS3D_RESULT __stdcall SetState(I3D_RENDERSTATE state, int value);
    virtual int __stdcall GetState(I3D_RENDERSTATE state) const;
    virtual LS3D_RESULT __stdcall Render(I3D_scene *scene);
    // The debugging draws: the alphas are transparencies (0 opaque), DrawBox's has the lines' in its high byte
    virtual void __stdcall DrawPoint(const S_vector &pos, const S_vector &color, uint alpha);
    virtual void __stdcall DrawPoint2D(int x, int y, const S_vector &color, uint alpha);
    virtual void __stdcall DrawLine(const S_vector &from, const S_vector &to, const S_vector &color, uint alpha);
    virtual void __stdcall DrawLines(const S_vector *verts, uint num_verts, const ushort *indx, uint num_indx,
        const S_vector &color, uint vstride, uint alpha);
    virtual LS3D_RESULT __stdcall DrawTriangles(const void *verts, uint num_verts, const ushort *indx, uint num_indx,
        const S_vector &color, uint vstride, uint flags, const I3D_text_coor *tx);
    virtual void __stdcall DrawCircle2D(const S_vector2 &pos, float radius, const S_vector &color, int num_segments,
        uint alpha);
    virtual void __stdcall DrawText(const S_vector &pos, const char *text, uchar alpha, float size);
    virtual void __stdcall DrawText2D(int x, int y, const char *text, uchar alpha, float size);
    virtual void __stdcall DrawSprite(const S_vector &pos, I3D_material *mat, uchar alpha, float size);
    virtual void __stdcall DrawBox(const I3D_bbox &bbox, const S_vector *line_color, const S_vector *fill_color,
        ushort alpha);
    virtual void __stdcall DrawSphere(const S_matrix &m, const I3D_bsphere &bsphere, const S_vector &color,
        ushort alpha);
    virtual I3D_visual *__stdcall CreateVisual(I3D_VISUAL_TYPE type);
    virtual I3D_frame *__stdcall CreateFrame(I3D_FRAME_TYPE type);
    virtual I3D_material *__stdcall CreateMaterial();
    virtual I3D_anim_material *__stdcall CreateAnimMaterial();
    virtual I3D_triface *__stdcall CreateTrifaceArray(int num);
    virtual I3D_face_group *__stdcall CreateFaceGroup();
    virtual I3D_mesh_level *__stdcall CreateMeshLevel();
    virtual I3D_mesh_object *__stdcall CreateMesh();
    virtual I3D_animation_set *__stdcall CreateAnimationSet();
    virtual I3D_pose *__stdcall CreatePose();
    virtual I3D_pose_set *__stdcall CreatePoseSet();
    virtual const char *__stdcall FrameType2String(I3D_FRAME_TYPE type) const;
    virtual const char *__stdcall VisualType2String(I3D_VISUAL_TYPE type) const;
    virtual LS3D_RESULT __stdcall EnumVisualTypes(I3D_ENUMVISUALPROC *proc, bool all) const;
    virtual I3D_frame *__stdcall FindFrameFast(I3D_scene *scene, const char *name) const;
    virtual void __stdcall OptimalizeFastFindData();
    virtual LS3D_RESULT __stdcall EnumFFTree(I3D_ENUMPROC *proc, uint context) const;
    virtual void __stdcall DeleteFromSceneTickList(I3D_frame *frame);
    virtual void __stdcall DeleteFromAllShadows(I3D_frame *frame);
    virtual void __stdcall DeleteFromAllTargets(I3D_frame *frame);
    virtual void __stdcall BuildAllVBuffers(I3D_scene *scene);
    // PS2 name. Empty on PS2 and PC: the original's copy is folded with an empty function of the DirectShow base
    // classes (0x10087ec0). The buffer managers call it whenever they create or release a buffer.
    virtual void __stdcall m_RecomputeUsedVRAM();

    // PC only: a material whose texture is one of LS3DF's bitmaps (Insanity3D's GetIcon), and the shadow textures'
    // rasterizer (names guessed), which fills a texture with the shadow's triangles, projected onto the box (x, y, -,
    // x2, y2)
    I3D_material *GetIcon(uint res_id);
    void DrawShadowTriangles(const int *verts, const ushort *indx, uint num_faces, void *dst, int pitch);
    void DrawShadowTrianglesSW(const int *verts, const ushort *indx, uint num_faces);
    LS3D_RESULT RenderShadowTexture(const float *verts, uint num_verts, const ushort *indx, uint num_faces,
        ITexture *texture, const float *box);

    // The fast find tree's search for a frame, which remembers the parent it came from in pFFLastUp_ (PS2 name, with
    // its underscore). The frames' code calls it.
    I3D_frame *FindFrameFast_(const char *name, I3D_frame *frame);

private:
    // render.cpp (PS2 names)
    void initRender();
    void closeRender();

    // The fast find tree's rebuild (PS2 name)
    void OFFD_RebuildBranchOfFFTree_(I3D_frame **branch, int first, int num);

public:
    uint init_flags;                    // 0x04, Init's
    C_list<ITexture *> textures;        // 0x08, the loader's
    C_list<void *> list_18;             // 0x18, the landscape's
    C_list<I3D_sector *> sectors;       // 0x28
    C_list<I3D_sound *> sounds;         // 0x38, CreateFrame's
    C_list<I3D_scene *> scenes;         // 0x48, CreateFrame's
    ushort unknown_58;                  // the particles'
};

// IGraph.cpp's render states, which the I3D code sets through the helpers below
extern IDirect3DDevice8 *m_pD3DDevice8;
extern uint m_CurrColorOp;
extern uint m_CurrAlphaOp;
extern uint m_CurrAlphaState;
extern uint m_CurrBlendState;
extern uint m_CurrCullState;
extern uint m_CurrVertexFormat;
extern DWORD m_VertexVS;
extern DWORD m_LVertexVS;
extern DWORD m_LVertex2VS;
extern uint m_CurrClipState;
extern uint m_CurrColorOp1;
extern uint m_CurrFogState;
extern uint m_CurrFogColor;
extern uint m_CullMode;
// The color op that modulates by the texture: D3DTOP_MODULATE2X, or D3DTOP_MODULATE without it
extern uint m_Modulate2X;
// What Init found the hardware lacks (bit 0x10, 0x20, 0x40: the environment blends 1, 2 and 5 in one pass)
extern uint m_ProblemFlags;
// What the device can do: 1 hardware T&L, 4 table fog, 0x80 single pass (bits named in IGraph::Init's messages)
extern uint m_HardwareCaps;

extern uint m_AlphaRef;
extern D3DVIEWPORT8 m_D3DViewPort8;

// Functions that set a render state where it changed (names guessed). PS2 inlines them all. On PC they are inline:
// VC6 inlines them where the inline budget allows it, and the copies the linker kept are in the billboard file's code.
// SetBlendState and SetVertexFormat are defined in render.h, which I3d_driver.cpp doesn't include, so its draws call
// them.
void SetBlendState(uint state);
void SetVertexFormat(uint format);

// FUNCTION: LS3DF 0x1000aa30
inline void SetAlphaState(uint state)
{
    if (m_CurrAlphaState != state) {
        m_CurrAlphaState = state;
        switch (state) {
        case 0:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            break;
        case 1:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 1);
            break;
        case 3:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, m_AlphaRef);
            break;
        case 2:
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            m_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 0);
            break;
        }
    }
}

// FUNCTION: LS3DF 0x1000ac30
inline void SetColorOp(uint op)
{
    if (m_CurrColorOp != op) {
        m_CurrColorOp = op;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_COLOROP, op);
    }
}

// FUNCTION: LS3DF 0x1000ac60
inline void SetAlphaOp(uint op)
{
    if (m_CurrAlphaOp != op) {
        m_CurrAlphaOp = op;
        m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ALPHAOP, op);
    }
}

// I3d_driver.cpp's globals (PS2 names)

// The values the driver set last, by I3D_RENDERSTATE
extern int renderState[RS_LAST];

// Whether the driver is initialised (set by its Init, cleared by its Close)
extern bool I3DIsInit;

// The driver's volume statistics, which its GetStats copies out and IGraph::Init clears
extern uint volumeStats[8];

// The z-buffer type of RS_USEZB, D3DZB_TRUE or with RS_WBUFFER D3DZB_USEW
extern uint ZBufferEnable;

// T&l.cpp: the lights' scale of the vertex colors, 128 or 255 without MODULATE2X (Insanity3D's driver member for the
// same, 0.5 or 1.0 there)
extern float vertex_light_mult;
void LightingSetSector(I3D_sector *sector);
bool PrepareLights(uchar *dst, uint dst_stride, I3D_mesh_level *level, const uchar *src, uint src_stride,
    I3D_object *obj, const S_matrix &m, uint mode, bool force, lod *rec);
void SetVertexAlpha(uchar *dst, uint dst_stride, I3D_mesh_level *level, I3D_object *obj, lod *rec);
void EnvProcess(void *dst, uint dst_stride, const void *src, uint src_stride, uint count, const S_vector &cam_pos,
    const S_matrix &m, float scale, float u_ofs, float v_ofs, int mode, I3D_frame *frm);
void TransformVertices(void *dst, const void *src, uint count, const S_matrix &m, const S_matrix &mn);
void TransformVerticesBlend(void *dst, const void *src, uint count, const S_matrix &m1, const S_matrix &mn1,
    const float *weights, const S_matrix &m2, const S_matrix &mn2);
void FogProcess(uchar *dst, const float *src, uint stride, uint count, I3D_light *light, int mode);
void SwapVertexBytes(uchar *a, uchar *b, uint stride, const I3D_triface *faces, uint num_faces, int restore);
int __stdcall TexelCallback(const S_vector &pos, float f, I3D_frame *frm, void *color, void *context);

// render.cpp's camera, which the 3D texts and sprites face and the fog is scaled by
extern I3D_camera *pCurrentCamera;
// render.cpp's: the camera's view and projection
extern S_matrix *pViewProjMatrix;

// A face group to render, as the visuals' m_AddFGroups queue them for their m_RenderFGroup (PS2 name, 0x20 bytes on PC)
struct RENDER_FG_HELP_STRUCT {
    I3D_visual *vis;
    uint sort_value;                    // 0x04, the transparent ones' 0x80000000 | the distance, else the texture's
                                        // uid (0x40000000 | it for MATF_CKEY_ALPHA), 0 untextured (Insanity3D's name)
    uint clip;                          // 0x08, the frame's FRMFLAGS_CLIP
    uint unknown_0c;
    I3D_face_group *fg;                 // 0x10
    I3D_sector *sector;                 // 0x14, lit by
    ITexture *texture;                  // 0x18
    ITexture *env_texture;              // 0x1c
};

// What the rendering gives the visuals' m_AddFGroups (PS2 name; the PC's members differ). Its constructor is out of line
// in render.cpp.
struct ADD_FG_HELP_STRUCT {
    S_vector cam_pos;                   // 0x00, the camera's position
    uint unknown_0c;
    I3D_camera *camera;                 // 0x10, pCurrentCamera
    I3D_sector *sector;                 // 0x14, the visual's
    I3D_scene *scene;                   // 0x18
    C_list<RENDER_FG_HELP_STRUCT> fgroups; // 0x1c, the face groups to render
    uint unknown_2c[3];                 // 0x2c
    uint num_opaque;                    // 0x38, counts the face groups queued
    uint num_transparent;               // 0x3c

    ADD_FG_HELP_STRUCT() : fgroups(0x400)
    {
        unknown_2c[0] = 0;
        unknown_2c[1] = 0;
        unknown_2c[2] = 0;
        unknown_0c = 0;
    }
};

// render.cpp
extern I3D_scene *pCurrentScene;
extern ADD_FG_HELP_STRUCT AddFGHlpStruct;
extern bool renderCameraChanged;
extern float RSLodQuality;
int GetObjectLOD(I3D_object *obj, const float &dist_2);

// I3d_driver.cpp: RenderNumber's previous value
extern uint RenderNumberMinus1;

// The loader's (load4ds.cpp defines them): the frames and materials loaded so far, the file and the error
extern I3D_frame *LoadFramy[10000];
extern I3D_material *LoadMaterialy[5000];
extern int LoadMaterialu;
extern LS3D_RESULT ImageErrFlg;
extern int LoadImageHandle;
extern const char *ImageFileName;
I3D_mesh_object *LoadMeshImage();
