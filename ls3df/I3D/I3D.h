#pragma once

#include "ls3df.h"
#include "I3D/I3d_math.h"
#include "I3D/render/RadixSort.h"

#include <d3d8.h>

// The 3D engine (PS2 I3d_*.cpp).

// windows.h's DrawText macro would rename the driver's DrawText
#undef DrawText
#undef LoadImage

class I3D_driver;
class I3D_scene;
class I3D_dummy;
class I3D_material;
class ITexture;
class ISND_3Dlistener;
class ISND_3Dsound;
class ISND_source;
class I3D_frame;
class I3D_visual;
class I3D_lensflare;
class I3D_camera;
class I3D_sector;
class I3D_light;
class I3D_sound;
class I3D_area;
class I3D_animation_set;
class I3D_mesh_object;
class I3D_mesh_level;
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
    RS_CLEAR = 5,                       // the rendering clears the screen first
    RS_CLEAR_ZBUFFER = 6,               // and the z-buffer (and stencil, where there is one)
    RS_DITHER = 7,
    RS_MIPMAP_BIAS = 8,                 // the textures' mipmap LOD bias
    RS_WIREFRAME = 9,
    RS_DRAWVISUALS = 18,                // the rendering draws the sectors' visuals (Insanity3D's name, the same number);
                                        // setting it sets RecomputeAllGeometry
    RS_FOG = 19,                        // fog (Insanity3D's name, the same number); refused where the hardware
                                        // lacks it (m_ProblemFlags 0x80)
    RS_LOD_QUALITY = 20,                // a float, 1.0 by default: the levels of detail's distance scale (Insanity3D's
                                        // name, the same number)
    RS_LOD_COARSER = 21,                // GetObjectLOD takes the next coarser level for VISF_UNKNOWN_10 visuals
    RS_LMTRUECOLOR = 34,                // the lightmaps' textures in the true color format, where there is one
                                        // (Insanity3D's name, its number is 35)
    RS_LMDITHER = 35,                   // the 16-bit lightmaps dithered (Insanity3D's name, its number is 36)
    RS_LOD_INDEX = 38,                  // -1 by default; the rendering copies it to ForceLodIndex, which nothing reads
                                        // (Insanity3D's name for its forced level of detail, -1 its default too)
    RS_USE_OCCLUSION = 39,              // the occluders hide what is behind them (Insanity3D's name for its own
                                        // state 47)
    RS_LM_AA_RATIO = 40,                // the lightmaps' samples per texel and axis (Construct; the name is a guess)
    RS_USESHADOWS = 43,                // the shadows are rendered (Insanity3D's name for its own state 13)
    RS_SHADOW_SW = 48,                  // shadow textures are filtered in software
    RS_FAST_FIND = 49,                  // named frames go into the driver's fast find tree (the name is a guess)
    RS_LOD_SCALE = 50,                  // in percent
    RS_USEPROJECTORS = 51,              // the projectors are rendered (the name is a guess, after RS_USESHADOWS)
    RS_NONORTHO_TESSELLATION = 53,      // RsNonOrthoTessellation, inverted
    RS_ZBIAS_PROJECTOR = 54,            // RsZBiasProjector
    RS_ZBIAS_SHADOW = 55,               // RsZBiasShadow
    RS_DRAWMIRRORS = 59,                // the mirrors render what they reflect (Insanity3D's name for its own
                                        // state 12); 1 by default
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

// A vertex's texture coordinates (PS2 I3D_text_coor)
struct I3D_text_coor {
    float u, v;
};

// A float's bits as an int, for the render states that hold floats (Insanity3D's I3DFloatAsInt): the value is
// copied to a local and read back, as the original's constants go through a stack slot. By reference: the lit
// object's Construct calls its out-of-line copy (0x100268b0) with the float's address in ecx.
inline int I3DFloatAsInt(const float &f)
{
    float t = f;
    return *(int *)&t;
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
#define FRMFLAGS_UNKNOWN_10000 0x10000  // an occluder hides nothing without it (I3D_occluder::UpdateShadow)
// The hierarchy has areas, sectors, occluders: each of these frames has its own from its constructor on, and LinkTo
// keeps it in a parent of the type
#define FRMFLAGS_HR_AREAS 0x40000
#define FRMFLAGS_HR_SECTORS 0x80000
#define FRMFLAGS_HR_OCCLUDERS 0x100000
#define FRMFLAGS_CLIP 0x200000          // has to be clipped (the name is a guess)
#define FRMFLAGS_TICK 0x400000          // in its scene's tick list
#define FRMFLAGS_UNKNOWN_8000000 0x8000000 // set by the camera's constructor and Duplicate
#define FRMFLAGS_INTERPOLATOR 0x10000000 // has an interpolator (I3D_IPolSet::GetFrameInterpolator)
#define FRMFLAGS_FFTREE 0x20000000      // in the driver's fast find tree
#define FRMFLAGS_UPDATE 0x40000000      // to update
#define FRMFLAGS_WEATHER 0x80000000     // a dummy in a scene's weather system (I3D_scene::WeatherSystemAddDummy)

// What a frame's callback is told (PS2 type; the names are guesses). SetCallback's flags are the messages to send.
enum LS3D_CALLBACK_MESSAGE {
    LS3D_CALLBACK_MESSAGE_0,
    CBM_PROGRESS = 8,                   // a loader's progress (I3D_LOAD_CB_PROC; Insanity3D's name, its value is 0)
    LS3D_CALLBACK_NOTE = 0x200,         // an animation passed a note key: the note, its time in ms
                                        // (I3D_interpolator::Update)
    LS3D_CALLBACK_LOD = 0x400,          // a visual's level of detail changed: the old one, the new one (GetObjectLOD)
    LS3D_CALLBACK_ANIM_END = 0x800,     // an animation reached an end: its index (I3D_IPolSet::Tick)
    LS3D_CALLBACK_NOTE_STRING = 0x1000, // an animation passed a string note key: the string, its time in ms
};

// LinkTo's flags: the first is Insanity3D's I3DLINK_UPDMATRIX, the same value and role
#define I3DLINK_UPDMATRIX 0x1           // keep the world matrix: the local one is made again against the new parent

// A frame's callback (PS2's parameters; the calling convention isn't confirmed)
typedef uint __stdcall I3D_CALLBACK(I3D_frame *frame, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2);

// The callback the Open functions' loaders report to (Insanity3D's name, PS2's parameters; Load.cpp): CBM_PROGRESS with
// the progress from 0 to 1 as a float's bits and 0 at the start, 1 on the way and 2 at the end, then the caller's
// context. The flags of the Open functions say what to report (Insanity3D's names and values).
typedef void __stdcall I3D_LOAD_CB_PROC(LS3D_CALLBACK_MESSAGE msg, uint prm1, uint prm2, void *context);
#define I3DLOAD_LOG 0x1                 // kept without a callback
#define I3DLOAD_PROGRESS 0x2

// A bounding box and the sphere around it (PS2 I3D_bvolume)
struct I3D_bvolume {
    I3D_bbox bbox;
    I3D_bsphere bsphere;
};

// The base of the scene graph's nodes (PS2 I3D_frame, 0x140 bytes on PC, vtable 0x1009b9b8). The game reaches its
// members through the class's inline functions. Member names are Insanity3D's where the roles agree (ref, user_data,
// frm_flags, matrix, f_matrix, name, parent, type), m_pOwner DeleteFromOwner's message's, the others are guesses.
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
    // Empty: the linker folds them with other empty functions, and the original's slots point at the base classes'
    // copy (CBaseVideoRenderer::OnWaitEnd, 0x10088540, defined out of class in renbase.cpp), where ours point at the
    // first object's in-class copy (I3d_billboard.cpp's). Defined out of class here (with I3D_visual::m_AddFGroups),
    // they fold with that copy's group, but our linker then keeps I3d_visual.cpp's copy, neither the first object's
    // nor the last's: which copy LINK keeps isn't understood yet (2026-10-22).
    virtual void UpdateV() {}
    virtual void SaveImage() {}
    virtual void LoadImage();

private:
    // The scene, the sector's CheckPoint and the mirrors' search for what they reflect call UpdateHRBoundVolW
    // (Insanity3D's frame has the scene as a friend, "for rendering and fast access")
    friend class I3D_scene;
    friend class I3D_sector;
    friend class I3D_mirror;

    // Exported (the decorated names say private; UpdateWMatrixProc __thiscall, the others __stdcall)
    __declspec(dllexport) void UpdateWMatrixProc();
    __declspec(dllexport) void __stdcall UpdateHRBoundVol();
    __declspec(dllexport) void __stdcall UpdateHRBoundVolW();

public:
    // Makes the local matrix from the scale and rotation, or the rotation from the matrix (PS2 name). The dummy file's
    // copy is the one linked. Both branches scale the rows: the volume's UpdateV calls S_vector::operator*= in each.
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
            f_matrix(0) *= scale.x;
            f_matrix(1) *= scale.y;
            f_matrix(2) *= scale.z;
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

    // Makes m the local matrix, with its scale (the name is a guess; the targets, sectors and the mirrors' file inline
    // it, at 0x10059e99, 0x1004fc29 and 0x100330c9)
    void SetLocalMatrix(const S_matrix &m)
    {
        f_matrix = m;
        scale = f_matrix.GetScale();
        frm_flags = (frm_flags & ~(FRMFLAGS_WMAT_VALID | FRMFLAGS_QUAT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID;
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

    // The scale in world space: the frame's own at the top of the hierarchy, else the world matrix's (the name is a
    // guess). Returned from each branch, so VC6 writes the result where it goes; a conditional expression selects a
    // pointer to either and copies after (the collision tests' billboards in mode 2).
    S_vector GetWorldScale()
    {
        if (!parent)
            return scale;
        return GetMatrix().GetScale();
    }

    // The hierarchy's bound volume in world space, updated first if it isn't (the name is a guess, after the visuals'
    // GetWBoundVolume; the collision tests, free functions, reach the private UpdateHRBoundVolW through it)
    const I3D_bvolume &GetHRBoundVolumeW()
    {
        if (!(frm_flags & FRMFLAGS_HR_BOUND_W_VALID))
            UpdateHRBoundVolW();
        return bound_w;
    }

    // Whether the frame is on (the name is a guess; the projectors' CollectReceivers reads the flags through it, so VC6
    // loads them again for the bound volume's test after it)
    bool IsOn() const
    {
        return frm_flags & FRMFLAGS_ON;
    }

    // PS2 name
    const S_vector &GetWorldPos()
    {
        return GetMatrix()(3);
    }

    // The local z axis, normalized unless the matrix is made from the rotation here (PS2 name; the game's out-of-line
    // copy is at 0x47a890 in Game.exe)
    S_vector GetDir()
    {
        if (!(frm_flags & FRMFLAGS_MAT_VALID)) {
            frm_flags |= FRMFLAGS_MAT_VALID;
            rot.Normalize();
            f_matrix.SetRot3(rot);
            return f_matrix(2);
        }
        S_vector dir = f_matrix(2);
        dir.Normalize();
        return dir;
    }

    // The world z axis, normalized (PS2 name; the game's out-of-line copy is at 0x47aa40 in Game.exe)
    S_vector GetWorldDir()
    {
        S_vector dir = GetMatrix()(2);
        dir.Normalize();
        return dir;
    }

    const char *GetName() const
    {
        return name ? name : "";
    }

    // The name without the model's (Insanity3D's name)
    const char *GetOrigName() const
    {
        return orig_name ? orig_name : "";
    }

    // Insanity3D's name. The single meshes' proxies reach their visual through it; written as the member, VC6 inlines
    // more of I3D_singlemesh::UpdateLodVertices' calls than the original.
    uint GetData() const
    {
        return user_data;
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
    I3D_frame *m_pOwner;                // 0x10c, the model or scene the frame is in
    I3D_FRAME_TYPE type;                // 0x110
    float world_scale;                  // 0x114
    uint update_count;                  // 0x118, counts the updates that moved it
    int num_children;                   // 0x11c
    I3D_frame *parent;                  // 0x120
    I3D_frame *child;                   // 0x124, the first child
    I3D_frame *prev, *next;             // 0x128, the siblings
    I3D_visual *next_visual;            // 0x130, a sector's visuals to render, linked from the sector's own (the
                                        // rendering's; the name is a guess)
    I3D_frame *ff_left, *ff_right;      // 0x134, the fast find tree (PS2 0x114, 0x118)
    int render_number;                  // 0x13c, the driver's RenderNumber when the frame was last rendered, -1
};

// SYNTHETIC: LS3DF 0x1001a980
// I3D_frame::`scalar deleting destructor'

// The destructor I3d_frame.cpp's thunk registers for UpdateTargetList: the second $E5 in link order, after
// clipper.cpp's. reccmp pairs the annotations of one name with our functions of that name in address order, so the
// landscape, model, target and 4ds loader files' are annotated too, the later files' aren't.
// SYNTHETIC: LS3DF 0x1001c7b0
// $E5

// The function-local statics of I3d_frame.cpp (PS2 names): AddToFFTree's count of the frames added, Update's guard
// against updating the targets twice. VC6 puts a function's static into the PDB with its plain name, so they have
// by-name markers.
// GLOBAL: LS3DF 0x100c0044
// addcounter

// GLOBAL: LS3DF 0x100c0048
// updating

// A sound's type (PS2 type; the names are Insanity3D's, which has the same first four values)
enum I3D_SOUNDTYPE {
    I3DSOUND_NULL,                      // silent: SetOn switches it off
    I3DSOUND_POINT,                     // fades out with the distance
    I3DSOUND_SPOT,                      // and outside its cone
    I3DSOUND_AMBIENT,                   // everywhere at its volume; the only type for a sound without a 3D buffer
};

// I3D_sound::Open's flags (Insanity3D's name for the same value and role)
#define I3DLOAD_SOUND_FREQUENCY 0x8     // the source's frequency can be changed (ISND_FREQUENCY)

// I3D_sound::snd_flags (the names are guesses): what m_UpdateSound has to give the voice, and the playing's own state
#define SOUNDF_POS 0x1                  // the position
#define SOUNDF_DIR 0x2                  // the direction
#define SOUNDF_RANGE 0x4                // SetRange: the scaled ranges are to be computed again
#define SOUNDF_VOLUME 0x8               // SetVolume: the volume is given to the voice even if it changed little
#define SOUNDF_OUTVOL 0x10              // SetOutVol
#define SOUNDF_FREQUENCY 0x20           // SetFrequency
#define SOUNDF_CONE 0x40                // SetCone
#define SOUNDF_SCALE 0x80               // the scaled ranges are to be computed again
#define SOUNDF_TYPE 0x100               // SetSoundType: the voice's 3D mode is to be set
#define SOUNDF_CAMERA_SECTOR 0x200      // the camera is in one of its sectors (SetOn)
#define SOUNDF_FADE_IN 0x400            // fade's direction, in its areas (SetOn)
#define SOUNDF_FADE_OUT 0x800
#define SOUNDF_PLAY 0x1000              // the voice is to be started by the next m_UpdateSound
#define SOUNDF_VELOCITY 0x2000          // SetVelocity

// I3D_sound::state (the names are guesses; PS2's m_SetState sets it)
enum {
    SOUNDSTATE_STOP = 1,
    SOUNDSTATE_PLAY,
    SOUNDSTATE_PAUSE,                   // out of range: the voice is stopped, the time keeps running
    SOUNDSTATE_RESUME,                  // m_UpdateSound switches it on again
    SOUNDSTATE_NOCAMERA,                // switched on without a camera: m_UpdateSound switches it on again
};

// A sound in the scene (PS2 I3D_sound, 0x1ec bytes on PC; I3d_sound.cpp), which plays a source through a 3D sound of
// the sound driver, its voice. Insanity3D's I3D_sound gives the names of the members whose roles agree (voice,
// snd_src, snd_flags, snd_type, open_flags, the ranges, volumes, cone and pause members); the others are guesses.
// VTABLE: LS3DF 0x1009ce78
class I3D_sound : public I3D_frame {
public:
    I3D_sound();
    ~I3D_sound();

    // The game's (PS2 names). In PS2's order but for SetVelocity, which comes after GetRange and GetCone there.
    virtual LS3D_RESULT __stdcall Open(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context);
    // Whether the voice plays, or with real false whether the sound would (it keeps time while paused)
    virtual bool __stdcall IsPlaying(bool real) const;
    virtual void __stdcall SetSoundType(I3D_SOUNDTYPE type);
    // The volume is 1 up to range_n, falls to mid_vol at range_mid of the way to range_f and to 0 at range_f
    virtual void __stdcall SetRange(float near_range, float far_range, float mid, float mid_vol);
    virtual void __stdcall SetCone(float in_angle, float out_angle);
    virtual void __stdcall SetOutVol(float vol);
    virtual void __stdcall SetVolume(float vol);
    virtual void __stdcall SetLoop(bool loop);
    virtual LS3D_RESULT __stdcall SetCurrTime(uint time);
    virtual void __stdcall SetFrequency(float freq);
    virtual void __stdcall GetRange(float &near_range, float &far_range, float &mid, float &mid_vol) const;
    virtual void __stdcall GetCone(float &in_angle, float &out_angle) const;
    virtual uint __stdcall GetCurrTime() const;
    virtual uint __stdcall GetPlayTime() const;
    virtual void __stdcall SetOn(bool on, bool update);
    virtual void __stdcall SetVelocity(const S_vector &v);
    virtual void __stdcall ForceUpdate();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();

    // Out of line at the file's end, for the vtable
    // FUNCTION: LS3DF 0x10059420
    virtual void __stdcall SetOn(bool on)
    {
        SetOn(on, false);
    }

    // PS2 names
    void m_Pause();
    void m_UpdateSound();
    void m_SetState(uchar new_state);

    float GetRangeVolume(float dist) const;

    uint snd_flags;                     // 0x140, SOUNDF_*
    uchar state;                        // 0x144, SOUNDSTATE_*
    uint priority;                      // 0x148, the voice's Play's
    uint unknown_14c;                   // 0x14c
    uint start_time;                    // 0x150, timeGetTime when it played from the start
    uint pause_time;                    // 0x154, timeGetTime when it paused
    uint pause_pos;                     // 0x158, the position it paused at
    ISND_3Dsound *voice;                // 0x15c
    ISND_source *snd_src;               // 0x160
    bool loop;                          // 0x164
    float range_n;                      // 0x168
    float scaled_range_n;               // 0x16c, scaled by the world scale
    float range_f;                      // 0x170
    float scaled_range_f;               // 0x174
    float scaled_range_f_2;             // 0x178, its square
    float range_mid;                    // 0x17c, of the way from range_n to range_f
    float scaled_range_mid;             // 0x180, the distance
    float range_mid_vol;                // 0x184, the volume there
    float volume_out;                   // 0x188, outside the cone
    float volume;                       // 0x18c
    float virtual_volume;               // 0x190, the voice's
    float frequency;                    // 0x194, of the source's
    uint open_flags;                    // 0x198, ISND_*: the source's
    I3D_SOUNDTYPE snd_type;             // 0x19c
    float cone_in;                      // 0x1a0
    float cos_cone_in;                  // 0x1a4, of half of it
    float cone_out;                     // 0x1a8
    float cos_cone_out;                 // 0x1ac
    I3D_sector *fade_sector;            // 0x1b0, whose blend it follows (SCTF_SOUND_FADE_IN)
    float fade;                         // 0x1b4, its fading's volume, 0 to 1, -1 when it doesn't fade
    uint fade_time;                     // 0x1b8, timeGetTime at its last step
    uint fade_length;                   // 0x1bc, in ms
    S_vector velocity;                  // 0x1c0
    C_list<I3D_sector *> sectors;       // 0x1cc, the sectors it sounds in (I3D_sector::AddSound)
    C_list<I3D_area *> areas;           // 0x1dc, the areas it sounds in (I3D_area::AddSound)
};

// SYNTHETIC: LS3DF 0x100577c0
// I3D_sound::`scalar deleting destructor'

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
    int blend_time;                     // 0x24, in ms: how long the renderer blends into the sector's environment
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
    S_vector reflections_pan;           // 0x50
    S_vector reverb_pan;                // 0x5c

    S_sector_sound_properties() {}

    void SetupDefaults();
    // The blend of a and b at t (b's at 1)
    void MakeBlend(const S_sector_sound_properties &a, const S_sector_sound_properties &b, float t, bool unknown);
};

// SetSoundData's and GetSoundData's index (PS2 type): the members of S_sector_sound_properties in order, the two pans
// last (the names are guesses)
enum I3D_SECTOR_SOUND_DATA_INDEX {
    SSD_ROOM_LF,
    SSD_ROOM_HF,
    SSD_DECAY_TIME,
    SSD_DECAY_HF_RATIO,
    SSD_REFLECTIONS,
    SSD_REFLECTIONS_DELAY,
    SSD_REVERB,
    SSD_REVERB_DELAY,
    SSD_ENVIRONMENT_DIFFUSION,
    SSD_BLEND_TIME,
    SSD_DECAY_LF_RATIO,
    SSD_ROOM,
    SSD_ECHO_TIME,
    SSD_ECHO_DEPTH,
    SSD_MODULATION_TIME,
    SSD_MODULATION_DEPTH,
    SSD_ROOM_ROLLOFF_FACTOR,
    SSD_AIR_ABSORPTION_HF,
    SSD_HF_REFERENCE,
    SSD_LF_REFERENCE,
    SSD_REFLECTIONS_PAN,                // through a const S_vector *
    SSD_REVERB_PAN,
    SSD_LAST,
};

// I3D_sector::sct_flags (names guessed; PRIMARY to NEAR are the scene's four sectors)
#define SCTF_VISIBLE 0x1                // the portals let the camera see into it this frame (ParsePortalsInSector; the
                                        // scene's weather system shows only where the primary sector has it)
#define SCTF_IN_WALK 0x2                // on the portal walk's stack (ParsePortalsInSector)
#define SCTF_PREPARED 0x4               // MakeReady was called: the sector has its hull's planes
#define SCTF_CULLED 0x10                // its frames were culled this frame (CullThruPortal's CTP_EnumProcFirst)
#define SCTF_OCCLUDER 0x40              // MakeReady makes an occluder of the hull
#define SCTF_UNKNOWN_80 0x80            // the walk came back into it through a portal without a twin: the occluder
                                        // made of its hull hides nothing (ProcessOccluders)
#define SCTF_UNKNOWN_100 0x100          // Duplicate keeps it
#define SCTF_PRIMARY 0x200
#define SCTF_SOUND_FADE_IN 0x400        // the renderer blends into the sound environment, not out of it
#define SCTF_SOUND_CHANGED 0x800        // the sound properties are to be given to the listener
#define SCTF_BACKDROP 0x1000
#define SCTF_OMNI 0x2000
#define SCTF_NEAR 0x4000

class I3D_portal;
class I3D_occluder;

// A part of the scene closed by its hull (PS2 I3D_sector, 0x234 bytes on PC; I3d_sector.cpp), which holds the lights
// and sounds that reach into it and the portals that lead out of it. The member names are guesses.
// VTABLE: LS3DF 0x1009ca88
class I3D_sector : public I3D_frame {
public:
    I3D_sector();
    ~I3D_sector();

    virtual LS3D_RESULT __stdcall SetName(const char *new_name);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();

    // PS2 names, in PS2's order, without its CheckPointDepth
    virtual void __stdcall AddLight(I3D_light *light);
    virtual LS3D_RESULT __stdcall DeleteLight(I3D_light *light);
    virtual void __stdcall AddSound(I3D_sound *sound);
    virtual LS3D_RESULT __stdcall DeleteSound(I3D_sound *sound);
    virtual LS3D_RESULT __stdcall SetSoundData(I3D_SECTOR_SOUND_DATA_INDEX index, uint data);
    virtual uint __stdcall GetSoundData(I3D_SECTOR_SOUND_DATA_INDEX index) const;
    // Whether the point is inside the sector: its world box and its hull
    virtual bool __stdcall CheckPoint(const S_vector &pos);

    // PS2 name. Out of line after the file's marker, for the vtable.
    // FUNCTION: LS3DF 0x100508b0
    virtual void __stdcall SetGroupBits(uint bits)
    {
        group_bits = bits;
        lights_changed++;
    }

    // PS2 names
    void UpdateCHPlanesW();
    void AddToRemoveFromSceneRing(I3D_scene *scene);
    void MakeReady();

    uint lights_changed;                // 0x140, counts the changes to its lights (the name is a guess)
    C_list<I3D_light *> lights;         // 0x144, the lights that reach into it
    int num_fog_lights;                 // 0x154, of the fog types
    C_list<I3D_sound *> sounds;         // 0x158
    S_sector_sound_properties sound_props; // 0x168
    uint sct_flags;                     // 0x1d0, SCTF_*
    uint group_bits;                    // 0x1d4
    int num_verts;                      // 0x1d8, the hull's
    int num_faces;                      // 0x1dc
    S_vector *verts;                    // 0x1e0
    ushort *faces;                      // 0x1e4, three vertex indices each
    float unknown_1e8;                  // 0x1e8, the renderer's blend into the sound environment, 0 to 1
    uint unknown_1ec;                   // 0x1ec, the time of its last step, in ms
    I3D_light *fog_light;               // 0x1f0, the fog light of the lights that is on
    C_list<I3D_portal *> portals;       // 0x1f4, its own, leading out
    C_list<I3D_portal *> portals_in;    // 0x204, the other sectors' that lead into it
    I3D_scene *scene;                   // 0x214
    int num_planes;                     // 0x218, the hull's in world space (UpdateCHPlanesW)
    S_plane *planes;                    // 0x21c
    I3D_sector *next_in_scene;          // 0x220, the scene's ring of sectors
    I3D_sector *prev_in_scene;          // 0x224
    I3D_occluder *occluders;            // 0x228, the occluders in it, linked through their next_in_sector
                                        // (OCCL_UpdateOcclLists)
    I3D_occluder *occluder;             // 0x22c, made of the hull by MakeReady (SCTF_OCCLUDER)
    I3D_frame *last_visual;             // 0x230, the last of its visuals to render (next_visual), the sector itself
                                        // while there are none (the portals' CullThruPortal_First)
};

// SYNTHETIC: LS3DF 0x1004ee60
// I3D_sector::`scalar deleting destructor'

// SetName's buffer, a function-local static: VC6 puts it into the PDB under its plain name
// GLOBAL: LS3DF 0x100a86e0
// name

// I3D_portal::portal_flags (the names are guesses; the loader reads them)
#define PORTALF_OCCLUDED 0x2            // an occluder hides it (OCCL_PrepareSector2)
#define PORTALF_ON 0x4                  // the walk goes through it (SetOn; set by the constructor)
#define PORTALF_PASSED 0x8              // the walk went through it this frame
#define PORTALF_RANGE_STOP 0x10         // farther than its range, the walk doesn't go through it, else it goes on with
                                        // the frames' children only in the sectors' own hierarchies
#define PORTALF_DIVIDES 0x20            // reported: it divides its sector (ReportBadPortals)
#define PORTALF_IN_PORTAL 0x80          // it has the place of another portal of its sector

// I3D_portal::parse_flags (the names are guesses): ParsePortal's vertices' sides of the camera
#define PORTALPF_SOME_BEHIND 0x1        // a vertex is behind the camera
#define PORTALPF_ALL_BEHIND 0x2         // all are

// A portal between sectors (PS2 I3D_portal, 0x150 bytes on PC; not a frame; I3d_portal.cpp). The scene keeps a ring of
// them. The portals of two sectors that have the same place are twins, each leading into the other's sector; one
// without a twin leads into the sector its first vertex is in (UpdatePortalInterconnection). The member names are
// guesses.
// VTABLE: LS3DF 0x1009c738
class I3D_portal {
public:
    // PS2 names
    I3D_portal();
    ~I3D_portal();
    void AddToSceneRing(I3D_scene *scene);
    void UpdatePortal();
    void LoadImage();

    // PC only (the names are guesses): the game switches a portal and its twin
    virtual void __stdcall SetOn(bool on);
    virtual bool __stdcall IsOn() const;

    int num_verts;                      // 0x04
    uint portal_flags;                  // 0x08, PORTALF_*
    float range_2;                      // 0x0c, the square of the distance from the camera, at the field of view's
                                        // correction, out to which it is all seen (PORTALF_RANGE_STOP), 0 for any
    uchar unknown_10[0x14];
    D3DVECTOR verts[8];                 // 0x24, in its sector's space (a constructor would construct them)
    D3DVECTOR verts_w[8];               // 0x84, in world space (UpdatePortal)
    S_plane plane;                      // 0xe4, in world space, facing out of its sector
    I3D_sector *sector;                 // 0xf4, the sector it leads out of
    I3D_sector *connect_sector;         // 0xf8, the one it leads into
    I3D_portal *twin;                   // 0xfc, the other sector's portal at its place: an occluder doesn't hide that
    I3D_portal *prev_in_scene;          // 0x100, the scene's ring of portals
    I3D_portal *next_in_scene;          // 0x104
    uchar parse_flags;                  // 0x108, PORTALPF_*
    I3D_scene *scene;                   // 0x10c
    S_vector center;                    // 0x110, of its box in world space
    float radius;                       // 0x11c, of the sphere around it
    S_vector rect_min, rect_max;        // 0x120, its vertices' box on the screen (x and y, -1 to 1; ParsePortal)
    S_vector clip_min, clip_max;        // 0x138, rect cut down to the box of the portals the walk came through
};

// The initializer of the portal file's CTP_VFnormals, empty (S_vector's constructor is): the fifth $E5 in link order,
// after the model file's
// SYNTHETIC: LS3DF 0x10041de0
// $E5

// The sectors' file's counts of the sectors' and the portals' changes (PS2 names)
extern int portalVersion;
extern int sectorVersion;

// An animation's keys for one frame (I3D_animation_set's data, names guessed): the ANIMF_* flags (a word, padded to
// 4 bytes), then a track for each flag, in the order rotation, position, scale, notes. A track is the number of keys
// (a word), their times (words, in 1/25 s) and their values: S_quats, S_vectors, notes (words) or string notes
// (pointers). Each but the notes' ends aligned to 4 bytes, and the string notes' values start aligned.
#define ANIMF_POS 0x2
#define ANIMF_ROT 0x4
#define ANIMF_SCALE 0x8
#define ANIMF_NOTE 0x10
#define ANIMF_NOTE_STRING 0x20

// An animation set's animation of one frame (the name is a guess)
struct S_frame_anim {
    const char *frame_name;
    const ushort *keys;                 // 0x04, see ANIMF_*
};

// An animation file's data as the loader leaves it, in one block (the names are guesses): the number of frames and the
// end time (in 1/25 s), then each frame's name and keys. The pointers are offsets into the block, which Open turns
// into addresses, the string notes' relative to the end of their track.
struct S_anim_set_data {
    ushort num_anims;
    ushort end_time;                    // 0x02
    S_frame_anim anims[1];              // 0x04
};

// An animation file's frames' keys (PS2 I3D_animation_set, 0x14 bytes on PC), reference counted. The PS2 build's has
// no vtable and keeps the file's name; the PC's has neither the name nor the PS2's Close and destructor out of line.
// The key functions take a frame's index (a "link", as their messages say) and a key's.
// VTABLE: LS3DF 0x1009b3e0
class I3D_animation_set {
public:
    I3D_animation_set();
    ~I3D_animation_set();
    void Close();

    // PS2 names but GetAnimLink (Insanity3D's), NumRotKeys, GetRotKey, NumPosKeys, NumSclKeys, GetSclKey and
    // GetNoteKeyStr (their messages')
    virtual int __stdcall Release();
    virtual LS3D_RESULT __stdcall Open(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context);
    virtual int __stdcall GetEndTime() const;
    virtual int __stdcall NumAnimations() const;
    virtual const char *__stdcall GetAnimLink(int index) const;
    virtual int __stdcall GetAnimIndex(const char *frame_name) const;
    virtual int __stdcall NumPosKeys(uint link) const;
    virtual LS3D_RESULT __stdcall GetPosKey(uint link, uint key, int &time, S_vector &pos) const;
    virtual int __stdcall NumRotKeys(uint link) const;
    virtual LS3D_RESULT __stdcall GetRotKey(uint link, uint key, int &time, S_quat &rot) const;
    virtual int __stdcall NumSclKeys(uint link) const;
    virtual LS3D_RESULT __stdcall GetSclKey(uint link, uint key, int &time, S_vector &scale) const;
    virtual int __stdcall NumNoteKeys(uint link, int *is_note) const;
    virtual LS3D_RESULT __stdcall GetNoteKey(uint link, uint key, int &time, short &note) const;
    virtual LS3D_RESULT __stdcall GetNoteKeyStr(uint link, uint key, int &time, const char *&note) const;

    void AddRef()
    {
        ref++;
    }

    // PS2 name: the animations
    const S_frame_anim *GetAnims() const;

    int ref;                            // 0x04
    uint options;                       // 0x08, the animations' I3DANIMOP_* where SetAnimation gets none
    uint unknown_0c;                    // the PS2's name is here; the PC's constructor leaves it alone
    S_anim_set_data *data;              // 0x10
};

// I3D_pose::pose_flags (names guessed): what the pose sets
#define POSEF_POS 0x1
#define POSEF_SCALE 0x2
#define POSEF_ROT_ADD 0x4               // rot turns the animation's rotation
#define POSEF_ROT 0x8                   // rot replaces it

// A frame's pose, which a model's or scene's interpolator blends into its animations (PS2 I3D_pose, 0x38 bytes on PC;
// I3d_pose.cpp), reference counted. The PS2's has no vtable; the PC's has its setters, for the game. The member names
// are guesses.
// VTABLE: LS3DF 0x1009c790
class I3D_pose {
public:
    I3D_pose();
    ~I3D_pose();

    virtual int __stdcall Release();
    // Insanity3D's names (I3D_anim_pose). Clear unsets all; a setter's NULL pointer unsets its value.
    virtual void __stdcall Clear();
    virtual void __stdcall SetPos(const S_vector *p);
    virtual void __stdcall SetScale(const S_vector *s);
    // A rotation that turns the animation's (POSEF_ROT_ADD), and one that replaces it (POSEF_ROT; the name is a guess)
    virtual void __stdcall SetRot(const S_quat *q);
    virtual void __stdcall SetRotAbs(const S_quat *q);
    // The same from a direction and a roll (PS2 name, the reference's): a name's overloads take its first one's slot in
    // reverse declaration order, so the vtable has the reference's first
    virtual void __stdcall SetDir(const S_vector *dir, float roll);
    virtual void __stdcall SetDir(const S_vector &dir, float roll);
    virtual void __stdcall SetDirAbs(const S_vector &dir, float roll);

    // Names the frame it is for; the pose set's AddPose calls it (the name is its message's)
    void SetName(const char *n);

    // The name of the frame it is for
    const char *GetName() const
    {
        return name ? name : "";
    }

    // Insanity3D's names (its pose has the position's and rotation's). The interpolator reads the values through them,
    // as their inline calls show (I3D_interpolator::Update).
    const S_vector &GetPos1() const
    {
        return pos;
    }

    const S_vector &GetScale1() const
    {
        return scale;
    }

    const S_quat &GetRot1() const
    {
        return rot;
    }

    int ref;                            // 0x04
    char *name;                         // 0x08
    uint pose_flags;                    // 0x0c, POSEF_*
    S_vector pos;                       // 0x10
    S_vector scale;                     // 0x1c
    S_quat rot;                         // 0x28
};

// Poses for several frames (PS2 I3D_pose_set, 0x18 bytes on PC; I3d_pose.cpp), reference counted. The PS2's has no
// vtable. The virtual functions' names are guesses.
// VTABLE: LS3DF 0x1009c7b4
class I3D_pose_set {
public:
    I3D_pose_set();
    ~I3D_pose_set();

    virtual int __stdcall Release();
    // Keeps the pose, for the frame of the name
    virtual void __stdcall AddPose(I3D_pose *pose, const char *frame_name);
    virtual LS3D_RESULT __stdcall DeletePose(I3D_pose *pose);
    // The name of the frame of a pose, "" past the end
    virtual const char *__stdcall GetFrameName(int index) const;

    int ref;                            // 0x04
    C_list<I3D_pose *> poses;           // 0x08
};

// The animation options (PS2 type I3D_ANIMOPTIONS; the name is a guess)
#define I3DANIMOP_LOOP 2                // starts again at the other end

// One of the interpolator set's animations (0x1c bytes; the names are guesses)
struct S_ipol_anim {
    I3D_animation_set *set;
    bool on;                            // 0x04
    uint options;                       // 0x08, I3DANIMOP_*, the model's SetAnimOptions
    int time;                           // 0x0c, in ms
    int last_time;                      // 0x10, the time when the interpolators were updated last
    int delta;                          // 0x14, the last tick's, negative backwards
    float blend;                        // 0x18
};

class I3D_IPolSet;

// One animation's keys of an interpolator's frame (0x1c bytes; the names are guesses): the tracks of the animation's
// data (ANIMF_*), each its key times followed by the values. flags is 0 where the animation doesn't move the frame.
struct S_anim_keys {
    const ushort *pos;                  // 0x00
    const ushort *rot;                  // 0x04
    const ushort *scale;                // 0x08
    const ushort *note;                 // 0x0c
    ushort num_pos;                     // 0x10
    ushort num_rot;                     // 0x12
    ushort num_scale;                   // 0x14
    ushort num_note;                    // 0x16
    ushort flags;                       // 0x18, ANIMF_*
};

// Moves one frame by the animations of its set and a pose (PS2 I3D_interpolator, 0xf4 bytes on PC;
// I3d_interpolator.cpp). The PC's has a vtable, for the game (I3D_model::GetInterpolators). The member names are
// guesses.
// VTABLE: LS3DF 0x1009ba58
class I3D_interpolator {
public:
    I3D_interpolator(I3D_IPolSet *set);
    ~I3D_interpolator();

    // Forgets the keys of one of the set's animations (the name is a guess)
    virtual LS3D_RESULT __stdcall ClearAnimation(uint index);
    // PS2 names
    virtual void __stdcall SetPose(I3D_pose *p);
    virtual void __stdcall SetPoseBlend(float blend);

    // PS2 name
    void Update();

    I3D_pose *pose;                     // 0x04
    float pose_blend;                   // 0x08
    S_anim_keys keys[8];                // 0x0c, by the set's animations
    I3D_frame *frame;                   // 0xec
    I3D_IPolSet *ipol_set;              // 0xf0
};

// The animations a scene or a model plays (PS2 I3D_IPolSet, 0xf8 bytes on PC; I3d_interpolator.cpp): an interpolator
// for each frame that an animation or a pose moves
class I3D_IPolSet {
public:
    // Out of line on PS2, inlined on PC
    I3D_IPolSet(I3D_frame *f) : frame(f)
    {
        memset(anims, 0, sizeof(anims));
    }

    ~I3D_IPolSet();

    // PS2 names, in PS2's order
    void Clear();
    void Tick(const int *times);
    void Tick(int time);
    LS3D_RESULT SetAnimation(I3D_animation_set *set, uint index, float blend, uint options);
    LS3D_RESULT SetPose(I3D_frame *frame, I3D_pose *pose);
    LS3D_RESULT SetPose(const char *frame_name, I3D_pose *pose);
    LS3D_RESULT SetPose(I3D_pose_set *set);
    void SetPoseBlend(float blend);
    LS3D_RESULT SetPoseBlend(float blend, I3D_pose_set *set);
    float GetPoseBlend() const;
    void ResetAllPoses();
    void m_releaseUnusedInterpolators();
    I3D_interpolator *GetFrameInterpolator(I3D_frame *frm);
    I3D_interpolator *GetFrameInterpolator(const char *frame_name);

    I3D_frame *frame;                   // the model or scene
    bool changed;                       // 0x04, an animation's blend or time was set
    S_ipol_anim anims[8];               // 0x08
    C_list<I3D_interpolator *> interpolators; // 0xe8
};

// What a scene's callback is told (PS2 type; the names are guesses): the rendering's stages, each sent if its bit is in
// the scene's cb_flags
enum SCN_CALLBACK_MESSAGE {
    SCN_CALLBACK_PRE_RENDER,            // the scene began, before anything is drawn
    SCN_CALLBACK_POST_SCENE,            // the scene is drawn, before the near sector and the lens flares
    SCN_CALLBACK_POST_RENDER,           // everything is drawn, before the scene ends
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
class I3D_face_group;
class I3D_visual;

// What a collision test's callback answers about a hit (PS2 type; the names are guesses)
enum COLL_ENUMRET {
    COLL_ACCEPT,                        // the hit counts
    COLL_ACCEPT_STOP,                   // the hit counts, and the test stops
    COLL_IGNORE,                        // the hit doesn't count
    COLL_IGNORE_CHILDREN,               // nor do the visual's children
};

// A collision test's callback, asked about each hit: the point (or the zero vector), its distance, the frame and the
// test's context (PS2's parameters)
typedef COLL_ENUMRET __stdcall I3D_COLLISION_CALLBACK(const S_vector &pos, float dist, I3D_frame *frm, void *context);

// The texel callback of a ray's test (COLF_COLORKEY), asked about each face hit: the point, its distance, the frame,
// the test's context and the texel (an S_texel_info). It answers 0 for an opaque texel, 2 for a transparent one, which
// the ray passes, 1 else (PS2's parameters; T&l.cpp's TexelCallback)
typedef int __stdcall I3D_TEXEL_CALLBACK(const S_vector &pos, float dist, I3D_frame *frm, void *context, void *texel);

// What a face's texel callback is given about the texel hit (the name is a guess)
struct S_texel_info {
    float u, v;                         // the texel's coordinates
    I3D_material *mat;                  // sampled
};

// TestColHierarchy's result (PS2 I3D_COLLISION, 0x1c bytes on PC; the member names are guesses)
struct I3D_COLLISION {
    float dist;
    S_vector normal;                    // 0x04, in world space
    ushort v[3];                        // 0x10, the face's vertices
    I3D_face_group *fgroup;             // 0x18, the face's group
};

// The collision tests' flags (the names are guesses but COLF_COLORKEY's, after Insanity3D's I3DCOL_COLORKEY)
#define COLF_BOX 0x1                    // a ray: a visual's box is its hit, not its faces
#define COLF_ACCEPT_ANY 0x10000         // a ray: a hit counts though a nearer one was found, and an opaque one clears
                                        // the context's color (the lights' shadow rays)
#define COLF_COLORKEY 0x20000           // a ray: the faces' texels are tested (the texel callback)
#define COLF_INFINITE 0x40000           // a ray: from goes on beyond from + dir
#define COLF_TWO_SIDED 0x100000         // a ray: every face is two-sided
#define COLF_FIRST_HIT 0x1000000        // a ray: the first hit ends the test
#define COLF_FRAME_LOD 0x2000000        // the frame given is tested at the level of detail given, the others at 0
#define COLF_UNKNOWN_4000000 0x4000000  // the visuals with VISF_UNKNOWN_1000 aren't tested
#define COLF_HIDDEN 0x8000000           // the frames switched off are tested too
#define COLF_EXCLUDE_SUBTREE 0x10000000 // the frame given and its children aren't tested
#define COLF_OMNI_SECTOR 0x20000000     // the scene's omni sector is tested too
#define COLF_NEAR_SECTOR 0x40000000     // and its near sector
#define COLF_FIRST_FACE 0x80000000      // a visual's first face hit is its hit

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
    virtual SCN_CALLBACK *__stdcall SetCallback(SCN_CALLBACK *cb, uint flags);
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
    virtual LS3D_RESULT __stdcall Open(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context);
    virtual void __stdcall Close();
    // i3d_collision.cpp (PS2's names and parameters). A ray from from along dir, as long as dir, against the visuals'
    // faces in the primary sector (and the near and omni ones, COLF_NEAR_SECTOR, COLF_OMNI_SECTOR): the nearest
    // frame hit, the hit's distance or point and its normal. cb is asked about each hit, texel_cb about its texel
    // (COLF_COLORKEY, DefaultTexelCallback without one), both with cb_context. frm is a frame or subtree to leave
    // out or the one to test (the flags say which), lod the mesh level to test.
    virtual I3D_frame *__stdcall TestColGeo1(const S_vector &from, const S_vector &dir, uint flags, S_vector *pos,
        S_vector *normal, I3D_COLLISION_CALLBACK *cb, void *cb_context, I3D_TEXEL_CALLBACK *texel_cb, int lod) const;
    virtual I3D_frame *__stdcall TestColGeo(const S_vector &from, const S_vector &dir, uint flags, float *dist,
        S_vector *normal, I3D_COLLISION_CALLBACK *cb, void *cb_context, I3D_TEXEL_CALLBACK *texel_cb, I3D_frame *frm,
        int lod) const;
    // The ray against the hierarchy of root only, its face in col
    virtual I3D_frame *__stdcall TestColHierarchy(const S_vector &from, const S_vector &dir, I3D_frame *root,
        I3D_COLLISION &col, uint flags, I3D_frame *frm, int lod, I3D_COLLISION_CALLBACK *cb) const;
    // A sphere of radius moved from from along dir: the nearest frame hit and the point
    virtual I3D_frame *__stdcall TestColSphere(const S_vector &from, const S_vector &dir, float radius, uint flags,
        S_vector *pos, I3D_frame *frm, I3D_COLLISION_CALLBACK *cb) const;
    virtual I3D_frame *__stdcall TestColSphereHierarchy(const S_vector &from, const S_vector &dir, float radius,
        I3D_frame *root, uint flags, S_vector *pos, I3D_frame *frm, I3D_COLLISION_CALLBACK *cb) const;
    virtual I3D_frame *__stdcall TestColSphereVisual(const S_vector &from, const S_vector &dir, float radius,
        I3D_visual *vis, uint flags, S_vector *pos) const;
    // Two spheres moving from from1 to to1 and from from2 to to2: the fraction of the move at which they touch, -1 if
    // they don't, 0 if their relative move is shorter than the sum of the radii
    virtual float __stdcall SphereToSphereD(float radius1, const S_vector &from1, const S_vector &to1, float radius2,
        const S_vector &from2, const S_vector &to2) const;
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
    uint cb_flags;                      // 0x144, the messages the rendering sends: bit n for message n
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
    S_vector bg_color;                  // 0x220, the rendering clears the screen to it (the name is a guess)
    C_list<I3D_frame *> owned_frames;   // 0x22c, the frames AddFrame took
    RECT viewport;                      // 0x23c, x, y and the size (Insanity3D's name)
    float backdrop_ncp, backdrop_fcp;   // 0x24c, SetBackdropRange's (the names are guesses)
    int num_portals;                    // 0x254, the ring of portals
    I3D_portal *portals;                // 0x258
    int portal_version;                 // 0x25c, portalVersion when the portals were connected (ProcessPortals)
    int num_sectors;                    // 0x260, the ring of sectors
    I3D_sector *sectors;                // 0x264
    int sector_version;                 // 0x268, sectorVersion when the sectors were made ready
    int num_occluders;                  // 0x26c, the ring of occluders
    I3D_occluder *occluders;            // 0x270
    bool unknown_274;                   // set when an occluder joins the ring
    bool occluders_changed;             // 0x275, the sectors' lists of occluders are to be made again
                                        // (OCCL_UpdateOcclLists)
    int num_areas;                      // 0x278, the ring of areas
    I3D_area *areas;                    // 0x27c
    bool areas_changed;                 // 0x280, set when an area joins or leaves the ring
    ISND_3Dlistener *listener;          // 0x284
    C_list<I3D_lensflare *> lensflares; // 0x288, the lens flares the rendering has met, until they fade out (they
                                        // hold a reference; Close releases them)
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

// The collision file's $Es, the first of each name in link order (the scene's, the shadows' and render.cpp's pair with
// theirs after them): the empty initializers of normala, lineBase and lineDir, the destructor TestColHierarchy registers
// for its static retNormal (it has nothing to destroy), and the empty initializers of sphBox, lineDest, colPointSph and
// kolBod. An empty initializer uses no global, so reccmp can pair it only by name.
// SYNTHETIC: LS3DF 0x1000de70
// $E9

// SYNTHETIC: LS3DF 0x1000de80
// $E12

// SYNTHETIC: LS3DF 0x1000de90
// $E15

// SYNTHETIC: LS3DF 0x10011d10
// $E18

// SYNTHETIC: LS3DF 0x10011d20
// $E20

// SYNTHETIC: LS3DF 0x10011d30
// $E23

// SYNTHETIC: LS3DF 0x10011d40
// $E26

// SYNTHETIC: LS3DF 0x10011d50
// $E29

// The collision file's functions whose markers would be out of address order there: its static helpers are defined
// before their callers, as PS2 has them (the zero vector's $E comes due before TestColGeo only so), and VC6 emits each
// after its first caller. The static ones have no public symbol, so their names are the PDB's procedure names.
// FUNCTION: LS3DF 0x1000dec0
// ?TestColGeo@I3D_scene@@UBGPAVI3D_frame@@ABUS_vector@@0IPAMPAU3@P6G?AW4COLL_ENUMRET@@0MPAV2@PAX@Z4P6GH0M344@Z3H@Z

// FUNCTION: LS3DF 0x1000e780
// testVisual

// FUNCTION: LS3DF 0x10011d60
// ?TestColSphere@I3D_scene@@UBGPAVI3D_frame@@ABUS_vector@@0MIPAU3@PAV2@P6G?AW4COLL_ENUMRET@@0M2PAX@Z@Z

// FUNCTION: LS3DF 0x10012520
// sphTestVisual

// FUNCTION: LS3DF 0x100152a0
// ?SphereIntersect@@YI_NABUI3D_bsphere@@@Z

// The destructors the scene file's thunks register for WSNearSectorList and WSNearDummyList, and the one
// m_UpdateWeatherSystem's CamOldPos registers (it has nothing to destroy). VC6 numbers them per file, and other files
// have these names too. reccmp pairs the annotations of one name with our functions of that name in address order, so
// every file's up to the last one that needs it is annotated: the scene's are the third $E13 in link order (after the
// lit objects' and the projector file's) and the second $E18 (after the collision file's).
// SYNTHETIC: LS3DF 0x10048a30
// $E13

// SYNTHETIC: LS3DF 0x10048a80
// $E18

// The scene's empty initializer, the second $E23 in link order, after the collision file's
// SYNTHETIC: LS3DF 0x10048aa0
// $E23

// The destructor the constructor's static bigBox registers (it has nothing to destroy): the second $E26 in link order,
// after the collision file's
// SYNTHETIC: LS3DF 0x10048ea0
// $E26

// SYNTHETIC: LS3DF 0x1004ddc0
// $E29

// The destructors render.cpp's thunks register for CurrentActiveAreaList, SoundsForDeactivation, AddFGHlpStruct and
// ParticleElementsList: the fifth $E13 in link order (after the lit objects', the projector file's, the scene's and the
// shadows', before Globals.cpp's), the fourth $E18 (after the collision file's, the scene's and the shadows'), the
// third $E23 (after the collision file's and the scene's) and the first $E28 (before IGraph.cpp's)
// SYNTHETIC: LS3DF 0x10061d00
// $E13

// SYNTHETIC: LS3DF 0x10061d50
// $E18

// SYNTHETIC: LS3DF 0x10061d90
// $E23

// SYNTHETIC: LS3DF 0x10061df0
// $E28

// The in-class functions' copies: Update, SetDir, SetName and LinkTo. The original has them at the end of the scene's
// file, ours come earlier (the headers have less inline code before them than the original's, see autopch.py). All but
// LinkTo are folded with other empty functions of their shape, so the markers are by name, the names our PDB keeps for
// the copies, which change with the link (tools/foldfix.py): Update's is its own, another class's or a base class's
// (CBaseRenderer::OnReceiveFirstSample's from I3d_light.cpp to I3d_area.cpp, from I3d_material.cpp, whose
// I3D_material::UpdateTexture and UpdateEnvTexture are in the group too, to I3d_sound.cpp and from I3d_singlemesh.cpp
// to I3d_singlemorph.cpp and again in I3d_portal.cpp, I3D_visual::m_RenderFGroup's in I3d_projector.cpp, in
// I3d_occluder.cpp and since I3d_particle.cpp, UpdateTexture's in render.cpp), and SetDir's and SetName's are scene
// functions of their shape or the shadow visual's (its SetCallback and Duplicate).
// FUNCTION: LS3DF 0x1004e970
// ?UpdateEnvTexture@I3D_material@@UAGXXZ

// FUNCTION: LS3DF 0x1004e980
// ?SetDir@I3D_scene@@UAGXABUS_vector@@M@Z

// FUNCTION: LS3DF 0x1004e990
// ?SetName@I3D_scene@@UAG?AW4LS3D_RESULT@@PBD@Z

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
    I3DLIGHT_UNKNOWN_8,                 // only the shadows know it: it adds to their lights' sum as a point light
                                        // does, but never gives their direction (Insanity3D has its layered fog here)
};

// I3D_light::mode: what the light lights (Insanity3D's names; PrepareLights is given the first, the lightmaps' code
// tests the second)
#define I3DLIGHTMODE_VERTEX 0x1
#define I3DLIGHTMODE_LIGHTMAP 0x2
#define I3DLIGHTMODE_DYNAMIC_SHADOW 0x8 // the shadows (I3D_shadow) take it (the name is a guess)
#define I3DLIGHTMODE_SHADOW 0x20        // the lightmaps test rays to the light (the name is a guess)
#define I3DLIGHTMODE_LIGHTNESS 0x40     // taken by I3D_scene::GetLightness (the name is a guess)

// A light (PS2 I3D_light, 0x1f0 bytes on PC; I3d_light.cpp). The PC's has only PS2's diffuse color and one power: its
// virtual functions are PS2's in PS2's order without the specular, ambient and attenuation setters and SetFogRange
// (SetRange takes both ranges, as Insanity3D's), SetDiffuse's overloads in reverse, as MSVC orders them. Member names
// are Insanity3D's where the roles agree (ltype, power, cone_in, cone_out, inner_cos, outer_cos), the others are
// guesses. UpdateV computes the world values from the others; the setters compute the ones they change themselves.
// VTABLE: LS3DF 0x1009bc88
class I3D_light : public I3D_frame {
public:
    I3D_light();
    ~I3D_light();

    virtual void __stdcall SetOn(bool on);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();
    // PS2 names (SetDiffusePower is Insanity3D's SetPower)
    virtual void __stdcall SetLightType(I3D_LIGHTTYPE type);
    virtual void __stdcall SetDiffuse(const S_vector &color);
    virtual void __stdcall SetDiffuse(float r, float g, float b);
    virtual void __stdcall SetDiffusePower(float power);
    virtual void __stdcall SetMode(uint mode);
    virtual void __stdcall SetRange(float near1, float far1);
    virtual void __stdcall SetCone(float inner, float outer);

    C_list<I3D_sector *> sectors;       // 0x140, the sectors it lights (Insanity3D's light_sectors)
    // LoadImage reads the members from mode to cone_out
    uint mode;                          // 0x150, I3DLIGHTMODE_*
    I3D_LIGHTTYPE ltype;                // 0x154
    float power;                        // 0x158
    S_vector diffuse;                   // 0x15c, SetDiffuse's
    float near_range, far_range;        // 0x168, a point light fades out between them (GetLightness); a fog's
                                        // density is near_range
    float cone_in, cone_out;            // 0x170, the spot's cone angles
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
    float cone_radius;                  // 0x1dc, of the sphere around the spot's cone, unscaled
    I3D_bsphere bsphere;                // 0x1e0, what a spot lights
};

// SYNTHETIC: LS3DF 0x10020740
// I3D_light::`scalar deleting destructor'

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

// A frame that carries the game's data (PS2 I3D_user, 0x144 bytes on PC; I3d_user.cpp). Insanity3D's has the one
// member, under this name.
// VTABLE: LS3DF 0x1009cff8
class I3D_user : public I3D_frame {
public:
    I3D_user();
    ~I3D_user();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);

    uint add_data;                      // 0x140
};

// SYNTHETIC: LS3DF 0x1005a170
// I3D_user::`scalar deleting destructor'

class I3D_singlemesh;

// I3D_joint::joint_flags (names guessed)
#define JOINTF_LOADED 0x1               // bone_matrix and index came from the file (LoadImage)
#define JOINTF_UNKNOWN_4 0x4            // cleared with JOINTF_JMAT_VALID when the joint moves
#define JOINTF_UNKNOWN_8 0x8
#define JOINTF_JMAT_VALID 0x10          // joint_matrix is up to date (PS2's GetJointMatrix tests the same bit)

// A bone of a single mesh (PS2 I3D_joint, 0x20c bytes on PC; I3d_joint.cpp): the single mesh's vertices follow it. The
// file gives it a matrix and its index among the single mesh's joints. Names are guesses; PS2's joint has only the
// index, the flags and the single mesh (at 0x160, 0x164, 0x168).
// VTABLE: LS3DF 0x1009baa8
class I3D_joint : public I3D_frame {
public:
    I3D_joint();
    ~I3D_joint();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();

    // joint_matrix, made again if the joint moved (PS2 name; defined after the single meshes' classes)
    const S_matrix &GetJointMatrix();

    S_matrix bone_matrix;               // 0x140, the file's; a scale of 0.01 by default
    uint index;                         // 0x180, among the single mesh's joints; loaded with bone_matrix
    S_matrix matrix_184;                // 0x184, bone_matrix at first
    S_matrix joint_matrix;              // 0x1c4, the local matrices of the joint and its parents up to the single mesh
    uint joint_flags;                   // 0x204, JOINTF_*
    I3D_singlemesh *singlemesh;         // 0x208, whose joints it is in
};

// SYNTHETIC: LS3DF 0x1001edc0
// I3D_joint::`scalar deleting destructor'

// A collision volume (PS2 I3D_volume, 0x174 bytes on PC; I3d_volume.cpp). Its world matrix is its local one, made
// again in every UpdateV. LS3DF uses none of its members but the type (the 4ds loader reads a volume as a frame), so
// only the type and the box have names (Insanity3D's volume has both; its other members aren't the PC's).
// VTABLE: LS3DF 0x1009d158
class I3D_volume : public I3D_frame {
public:
    I3D_volume();
    ~I3D_volume();

    virtual void UpdateV();
    // PS2 name
    virtual uint __stdcall GetVolumeType() const;

    // The type: UpdateV keeps 1 at scale 1, 4 at y scale 1, and gives 5 and 6 x's scale on all axes (Insanity3D's
    // types, sphere, rectangle, box, cylinder and capped cylinder, end at 5, so its names aren't taken)
    uint volume_type;                   // 0x140
    uint unknown_144;
    uint unknown_148;
    uint unknown_14c;
    uint unknown_150;
    I3D_bbox bbox;                      // 0x154
    uint unknown_16c;
    uint unknown_170;
};

// SYNTHETIC: LS3DF 0x1005a820
// I3D_volume::`scalar deleting destructor'

// I3D_emitor's parameters (PS2 type; PS2's emitter is another design, so the names are guesses after their roles).
// The times are in ms, the angles in radians, the rates per second.
enum PARTICLE_PARAM {
    PP_EMIT_TIME,                       // the time between two particles
    PP_EMIT_TIME_RND,                   // a random part of it
    PP_LIFE_TIME,                       // a particle's life
    PP_LIFE_TIME_RND,
    PP_GROW_1,                          // a particle's scale grows by a rate between these two
    PP_GROW_2,
    PP_POS_RND,                         // the size of the box around the emitter a particle starts in
    PP_DIR_RND,                         // how far its end direction strays
    PP_DIR_TIME,                        // the time after which it strays again, 0 never
    PP_DIR_TIME_RND,
    PP_DIR_TIME_STRAY,                  // how far it strays then
    PP_FADE_IN,                         // the part of its life it fades in over
    PP_FADE_OUT,                        // the part of its life after which it fades out
    PP_VELOCITY_X,                      // its velocity at its start
    PP_VELOCITY_Y,
    PP_VELOCITY_Z,
    PP_VELOCITY_END_X,                  // and at its end
    PP_VELOCITY_END_Y,
    PP_VELOCITY_END_Z,
    PP_FLAGS,                           // the particles' part_flags (3 and 0x600)
    PP_MAX_PARTICLES,                   // the emitter switches itself off after so many, 0 never; turns it on
    PP_VELOCITY_WORLD,                  // the velocities are in world space, not turned with the emitter
    PP_VELOCITY_END_WORLD,
    PP_PRE_TICK,                        // a time to run the emitter for at once
    PP_ROT_SPEED_1,                     // a particle turns at a speed between these two
    PP_ROT_SPEED_2,
    PP_SCALE,                           // a particle's scale, times the emitter's
    PP_UNUSED_27,
    PP_BILLBOARD,                       // the particles turn to the camera
    PP_COLOR,                           // their color at their start (a pointer to an S_vector)
    PP_COLOR_END,                       // and at their end
    PP_OMNI_SECTOR,                     // they go into the scene's omni sector, not the emitter's sector
    PP_ROT_AXIS,                        // their billboards' rot_axis
    PP_AXIS_MODE,                       // and axis_mode
    PP_LAST,
};

class I3D_part_element;

// A particle emitter (PS2 I3D_emitor, 0x1f8 bytes on PC; I3d_particle.cpp): a frame that creates a particle
// (I3D_part_element) with its mesh every so often in its sector, which the particle leaves when it dies. Its
// parameters are SetParam's. PS2's is another design (its particles are its own elements in a vertex buffer), so
// all but the class's and SetParam's names are guesses.
// VTABLE: LS3DF 0x1009c538
class I3D_emitor : public I3D_frame {
public:
    I3D_emitor();
    ~I3D_emitor();

    virtual void __stdcall Tick(int time);
    virtual void __stdcall SetOn(bool on);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();
    // The mesh of the particles it creates (PS2 SetTexture's slot)
    virtual void __stdcall SetMesh(I3D_mesh_object *m);
    virtual LS3D_RESULT __stdcall SetParam(PARTICLE_PARAM param, uint value);
    virtual uint __stdcall GetParam(PARTICLE_PARAM param) const;
    // The parameters' defaults
    virtual void __stdcall Reset();
    // Kills its particles (PS2 name)
    virtual void __stdcall KillElements();
    // Ticks its particles
    virtual void __stdcall TickElements(int time);
    // Switches its particles on or off
    virtual void __stdcall SetElementsOn(bool on);

    ushort id;                          // 0x140, from the driver: the particles' sort_group with 0x600
    uint part_flags;                    // 0x144, PP_FLAGS
    I3D_mesh_object *mesh;              // 0x148, its particles' (I3D_object::Duplicate)
    C_list<I3D_part_element *> elements; // 0x14c, the living particles it created
    uint time;                          // 0x15c, since the last particle
    uint emit_time;                     // 0x160, until the next one
    int num_emitted;                    // 0x164
    I3D_sector *sector;                 // 0x168, the first sector above it, where the particles go
    uint p_emit_time;                   // 0x16c, the parameters: PP_EMIT_TIME
    uint p_emit_time_rnd;               // 0x170
    uint p_life_time;                   // 0x174
    uint p_life_time_rnd;               // 0x178
    float p_scale;                      // 0x17c
    float p_grow_1;                     // 0x180
    float p_grow_2;                     // 0x184
    float p_pos_rnd;                    // 0x188
    float p_dir_rnd;                    // 0x18c
    float p_fade_in;                    // 0x190
    float p_fade_out;                   // 0x194
    uint p_dir_time;                    // 0x198
    uint p_dir_time_rnd;                // 0x19c
    float p_dir_time_stray;             // 0x1a0
    S_vector p_velocity;                // 0x1a4
    S_vector p_velocity_end;            // 0x1b0
    int p_max_particles;                // 0x1bc
    uchar p_velocity_world;             // 0x1c0
    uchar p_velocity_end_world;         // 0x1c1
    float p_rot_speed_1;                // 0x1c4
    float p_rot_speed_2;                // 0x1c8
    uint p_pre_tick;                    // 0x1cc
    bool p_billboard;                   // 0x1d0
    S_vector p_color;                   // 0x1d4
    S_vector p_color_end;               // 0x1e0
    bool p_omni_sector;                 // 0x1ec
    uint p_rot_axis;                    // 0x1f0
    int p_axis_mode;                    // 0x1f4
};

// SYNTHETIC: LS3DF 0x1003df00
// I3D_emitor::`scalar deleting destructor'

// I3D_target::target_flags: what the target does to its frames (names guessed). TARGETF_LOOKAT, the default, turns
// them to it; without it they take the parts of its world matrix the others name.
#define TARGETF_LOOKAT 0x1
#define TARGETF_ROT 0x2
#define TARGETF_POS 0x4
#define TARGETF_SCALE 0x8

// A frame that other frames follow (PS2 I3D_target, 0x154 bytes on PC; I3d_target.cpp): its frames, which are in no
// other target's list (FRMFLAGS_TARGET), turn to it or take its world position, rotation or scale. A target that moved
// goes into UpdateTargetList, whose targets I3D_frame::Update updates once the frames are (UpdateFrames). The member
// names are guesses.
// VTABLE: LS3DF 0x1009cf58
class I3D_target : public I3D_frame {
public:
    I3D_target();
    ~I3D_target();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();
    // PS2 names
    virtual void __stdcall AddToTarget(I3D_frame *frame);
    virtual LS3D_RESULT __stdcall DeleteFromTarget(I3D_frame *frame);
    virtual void __stdcall SetTargetFlags(uint flags);

    // PS2 names: the target whose list has the frame, NULL if none
    static I3D_target *GetTarget(I3D_frame *frame);
    static void DeleteFromAnyTarget(I3D_frame *frame);
    void UpdateFrames();

    C_list<I3D_frame *> frames;         // 0x140, the frames it moves
    uint target_flags;                  // 0x150, TARGETF_*
};

// SYNTHETIC: LS3DF 0x10059590
// I3D_target::`scalar deleting destructor'

// The destructor the target file's thunk registers for globalTargetList: the sixth $E5 in link order, after the
// portal file's
// SYNTHETIC: LS3DF 0x10059470
// $E5

// I3d_frame.cpp's: the targets to update after the frames (PS2 name)
extern C_list<I3D_target *> UpdateTargetList;

// A frame and a target (PS2 type; the names are guesses): a model's duplication keeps the copies of the frames that had
// a target with their targets, to give the copies the copied targets
struct TARGET_ITEM {
    I3D_frame *frame;
    I3D_target *target;
};

// A hierarchy of frames loaded from a file, which it owns, and its animations (PS2 I3D_model, 0x158 bytes on PC)
// VTABLE: LS3DF 0x1009c160
class I3D_model : public I3D_frame {
public:
    I3D_model();

    virtual LS3D_RESULT __stdcall Open(const char *name, uint flags, I3D_LOAD_CB_PROC *cb, void *context);
    virtual void __stdcall Close();
    virtual void __stdcall AddFrame(I3D_frame *frame);
    virtual LS3D_RESULT __stdcall DeleteFrame(I3D_frame *frame);
    virtual LS3D_RESULT __stdcall SetAnimation(I3D_animation_set *set, uint index, float blend, uint options);
    virtual I3D_animation_set *__stdcall GetAnimationSet(uint index) const;
    virtual void __stdcall StopAnimation(uint index);
    virtual void __stdcall StartAnimation(uint index);
    virtual LS3D_RESULT __stdcall SetAnimBlend(uint index, float blend);
    virtual float __stdcall GetAnimBlend(uint index) const;
    virtual LS3D_RESULT __stdcall SetAnimTime(uint index, int time);
    virtual int __stdcall GetAnimTime(uint index) const;
    virtual LS3D_RESULT __stdcall SetAnimOptions(uint index, uint options);
    virtual uint __stdcall GetAnimOptions(uint index) const;
    virtual bool __stdcall AnimIsOn(uint index) const;
    virtual void __stdcall Tick(int time);
    virtual void __stdcall Tick(const int *times);
    // A name's new overloads take its first one's slot in reverse declaration order: the vtable has SetPose(I3D_pose_set
    // *) and SetPoseBlend(float, I3D_pose_set *) first
    virtual LS3D_RESULT __stdcall SetPose(I3D_frame *frame, I3D_pose *pose);
    virtual LS3D_RESULT __stdcall SetPose(const char *frame_name, I3D_pose *pose);
    virtual LS3D_RESULT __stdcall SetPose(I3D_pose_set *set);
    virtual LS3D_RESULT __stdcall SetPoseBlend(float blend);
    virtual LS3D_RESULT __stdcall SetPoseBlend(float blend, I3D_pose_set *set);
    virtual float __stdcall GetPoseBlend() const;
    virtual void __stdcall ResetAllPoses();
    virtual int __stdcall NumInterpolators() const;
    virtual I3D_interpolator **__stdcall GetInterpolators() const;
    virtual LS3D_RESULT __stdcall DuplicateHierarchy(I3D_model *model);
    virtual LS3D_RESULT __stdcall SetName(const char *new_name);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual ~I3D_model();

    // Gives the frames the model's name, or their own if it has none
    LS3D_RESULT SetName();

    C_list<I3D_frame *> frames;         // 0x140, the frames it owns
    I3D_IPolSet *ipol_set;              // 0x150
    char *file_name;                    // 0x154, Open's, with a reference count in the int before it: the copies share
                                        // it
};

// SYNTHETIC: LS3DF 0x10033460
// I3D_model::`scalar deleting destructor'

// The destructors the model file's thunks register for DuplTargetList and DuplShadowList: the fourth $E5 in link order,
// after I3d_landscape.cpp's, and the second $E10, after the driver's and before render.cpp's
// SYNTHETIC: LS3DF 0x10033b80
// $E5

// SYNTHETIC: LS3DF 0x10033bd0
// $E10

// I3D_occluder::occl_flags (the names are guesses)
#define OCCLF_ACTIVE 0x2                // UpdateShadow made its volume this frame, and nothing hides it
#define OCCLF_PORTAL 0x4                // the camera is on both sides of its sector's portals: it hides no portal

// A convex body that hides what is behind it (PS2 I3D_occluder, 0x5ec bytes on PC; I3d_occluder.cpp). For the camera
// UpdateShadow makes the volume it hides: the planes of the faces turned to the camera, the planes through the camera
// and the silhouette's edges, and a plane along the view through the silhouette's farthest vertex. The scene keeps a
// ring of them, each sector a list of those in it (OCCL_UpdateOcclLists). The member names are guesses.
// VTABLE: LS3DF 0x1009c420
class I3D_occluder : public I3D_frame {
public:
    I3D_occluder();
    ~I3D_occluder();

    virtual LS3D_RESULT __stdcall LinkTo(I3D_frame *frame, uint flags);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();

    // PS2 name and parameters: copies the faces and the vertices, which are stride bytes apart
    virtual LS3D_RESULT __stdcall Build(const S_vector *verts, uint num_verts, const ushort *faces, uint num_faces,
        uint stride);

    // PS2 names
    void AddToRemoveFromSceneRing(I3D_scene *scene);
    LS3D_RESULT Build();
    void UpdateShadow();
    bool TestVertListW(int num, S_vector *verts);

    uint num_verts;                     // 0x140
    uint num_faces;                     // 0x144
    S_vector *verts;                    // 0x148, in local space
    S_vector *verts_w;                  // 0x14c, in world space (UpdateV)
    ushort *faces;                      // 0x150, three vertex indices each
    I3D_occluder *prev_in_scene;        // 0x154, the scene's ring of occluders
    I3D_occluder *next_in_scene;        // 0x158
    ushort *edge_faces;                 // 0x15c, three for each face: the face across each edge (Build)
    uchar *edge_sides;                  // 0x160, and which of that face's edges it is
    S_plane *planes;                    // 0x164, one for each face, in world space (UpdateV)
    ushort *face_flags;                 // 0x168, one for each face: 1 if it faces the camera
    S_plane side_planes[35];            // 0x16c, through the camera and the silhouette's edges (UpdateShadow)
    int num_side_planes;                // 0x39c
    S_plane face_planes[33];            // 0x3a0, of the faces turned to the camera
    int num_face_planes;                // 0x5b0
    S_plane depth_plane;                // 0x5b4, along the view, through the silhouette's farthest vertex
    uint occl_flags;                    // 0x5c4, OCCLF_*
    I3D_scene *scene;                   // 0x5c8
    S_vector center;                    // 0x5cc, of the vertices' box in world space (UpdateV)
    float radius;                       // 0x5d8, of the sphere around it; Build sets 1, 0 means never built
    float radius_2;                     // 0x5dc, its square
    I3D_occluder *next_in_sector;       // 0x5e0, the sector's list (I3D_sector::occluders)
    I3D_sector *sector;                 // 0x5e4, the sector whose hull it is (I3D_sector::MakeReady)
    float dist;                         // 0x5e8, from the camera: CullOcclsEachOther takes the nearer ones first
};

// SYNTHETIC: LS3DF 0x1003a500
// I3D_occluder::`scalar deleting destructor'

// The occluder file's static functions, defined before their callers as PS2 has them: VC6 emits them after
// ProcessOccluders, their first caller, so their markers would be out of address order there. They have no public
// symbol, so their names are the PDB's procedure names.
// FUNCTION: LS3DF 0x1003c2e0
// CullOcclsEachOther

// FUNCTION: LS3DF 0x1003c480
// POIS_EnumProc

// A part of the scene closed by its faces (PS2 I3D_area, 0x17c bytes on PC; I3d_area.cpp), which knows the sounds
// that play in it. The scene keeps a ring of them, and render.cpp the areas the camera is in (CheckPoint). The member
// names are guesses.
// VTABLE: LS3DF 0x1009b460
class I3D_area : public I3D_frame {
public:
    I3D_area();
    ~I3D_area();

    virtual LS3D_RESULT __stdcall LinkTo(I3D_frame *frame, uint flags);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void SaveImage();
    virtual void LoadImage();

    // PS2 names; the PC's vtable has CheckPoint after the sounds' functions, where PS2's source has it before them.
    // Build's last parameter is the distance from one vertex to the next.
    virtual LS3D_RESULT __stdcall Build(const S_vector *verts, uint num_verts, const ushort *faces, uint num_faces,
        uint stride);
    virtual void __stdcall AddSound(I3D_sound *sound);
    virtual LS3D_RESULT __stdcall DeleteSound(I3D_sound *sound);
    // Whether the point is behind all the faces' planes
    virtual bool __stdcall CheckPoint(const S_vector &pos);

    // PS2 names, out of line after the constructor, for the vtable. The time is at least 1.
    // FUNCTION: LS3DF 0x10005250
    virtual void __stdcall SetSwitchFadeTime(uint time)
    {
        switch_fade_time = time > 1 ? time : 1;
    }

    // FUNCTION: LS3DF 0x10005280
    virtual uint __stdcall GetSwitchFadeTime()
    {
        return switch_fade_time;
    }

    // PS2 name
    void AddToRemoveFromSceneRing(I3D_scene *new_scene);

    int num_verts;                      // 0x140
    S_vector *verts;                    // 0x144, in local space
    S_vector *verts_w;                  // 0x148, in world space (UpdateV)
    int num_faces;                      // 0x14c
    ushort *faces;                      // 0x150, three vertex indices each
    I3D_area *prev_in_scene;            // 0x154, the scene's ring of areas
    I3D_area *next_in_scene;            // 0x158
    S_plane *planes;                    // 0x15c, one for each face, in world space (UpdateV)
    I3D_scene *scene;                   // 0x160
    int unknown_164;                    // 0x164, -1 when the sounds change
    C_list<I3D_sound *> sounds;         // 0x168, the sounds that play in it
    uint switch_fade_time;              // 0x178, 500 by default
};

// SYNTHETIC: LS3DF 0x10005290
// I3D_area::`scalar deleting destructor'

// LoadImage is folded with I3D_sound's, the same code. The original keeps the area file's copy; which one our linker
// keeps changes with the link, so the marker is by the name our PDB keeps (tools/foldfix.py updates it).
// FUNCTION: LS3DF 0x10005ce0
// ?LoadImage@I3D_area@@UAEXXZ

// A part of a shadow's geometry (PC only, 0xc bytes; the names are guesses): the vertices and faces that the frame of
// its name, under the frame the shadow is set to, moves
struct S_shadow_part {
    char *name;                         // 0x00, the frame's name without the model's
    int num_verts;                      // 0x04
    int num_faces;                      // 0x08
};

// A shadow's geometry as its file (.6ds, Load.cpp) has it, shared by the copies of a shadow, reference counted (PC
// only; "!! RefCount  this:0x%x  ShadowDef" names the class). Each part's vertices are in its frame's space, its faces
// index them from the part's first vertex.
class ShadowDef {
public:
    int ref;                            // 0x00
    C_list<S_vector> verts;             // 0x04
    C_list<ushort> faces;               // 0x14, three vertex indices each
    C_list<S_shadow_part> parts;        // 0x24

    ShadowDef() : verts(10), faces(64), parts(10)
    {
        ref = 1;
        parts.reserve(20);
        verts.reserve(160);
        faces.reserve(240);
    }

    ~ShadowDef();

    void Release()
    {
        if (--ref < 0)
            dbgPrintf("!! RefCount  this:0x%x  ShadowDef", this);
        else if (!ref)
            delete this;
    }
};

// A frame of a shadow's hierarchy that a part of its geometry follows, and the part's vertices (PC only, the names are
// guesses)
struct S_shadow_frame {
    I3D_frame *frame;                   // NULL once the frame is destroyed (DeleteFromAnyShadow)
    int num_verts;
};

// What a shadow keeps from frame to frame (SetShadowLock's type, PS2's name; the values' names are guesses)
enum I3D_LOCK {
    I3DLOCK_NONE,                       // everything is computed again each frame
    I3DLOCK_VERTICES,                   // the vertices once transformed
    I3DLOCK_LIGHT,                      // and the light's direction and the texture once rendered
    I3DLOCK_RECEIVERS,                  // and the faces it falls on, in the buffer managers' buffers
};

// I3D_shadow::shd_flags (PC only, the names are guesses). Prepare clears the changes first, then the steps set them.
#define SHDF_TEXTURE_CHANGED 0x1        // RenderTexture rendered the texture again
#define SHDF_DIR_CHANGED 0x2            // the light's direction changed (light_matrix)
#define SHDF_VERTS_CHANGED 0x4          // UpdateVertices transformed the vertices again
#define SHDF_OPACITY_CHANGED 0x8
#define SHDF_RECEIVERS_CHANGED 0x10     // ProcessShadow found other receivers
#define SHDF_TEXTURE_VALID 0x400
#define SHDF_LIGHT_VALID 0x800          // the light and its direction are up to date
#define SHDF_VERTS_VALID 0x1000
#define SHDF_RECEIVERS_VALID 0x2000     // the receivers' faces are clipped (ProcessShadow)
#define SHDF_NONE 0x8000                // no light casts it, or it has no geometry

class I3D_shadow_visual;

// A shadow (PS2 I3D_shadow, 0x214 bytes on PC; I3d_shadow.cpp). PS2's is a stub; the PC's casts the geometry of its
// ShadowDef, moved by the frames of the frame it is set to, from the strongest light of its sector, renders it into a
// texture and projects that onto the visuals around it (the receivers) through its visual. Its virtual functions are
// PS2's in PS2's order. Names are PS2's where it has the member (orthogonal, range, max_texture_distance, color,
// reverse_factor, shadow_lock, the texture size), the others are guesses.
// VTABLE: LS3DF 0x1009cb40
class I3D_shadow : public I3D_frame {
public:
    I3D_shadow();
    ~I3D_shadow();

    virtual LS3D_RESULT __stdcall LinkTo(I3D_frame *frame, uint flags);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    // PS2 names
    virtual LS3D_RESULT __stdcall Open(const char *name, uint flags, I3D_LOAD_CB_PROC *cb, void *context);
    virtual void __stdcall Close();
    virtual LS3D_RESULT __stdcall SetTextureSize(int x, int y);
    virtual void __stdcall SetRange(float r);
    virtual LS3D_RESULT __stdcall SetMaxTextureDistance(float d);
    virtual void __stdcall SetShadowLock(I3D_LOCK lock);
    virtual void __stdcall SetReverseFactor(float f);
    virtual void __stdcall SetColor(const S_vector &c);
    virtual void __stdcall SetOrthogonal(bool b);

    // The name is a guess: Duplicate copies the range through it (fld, fstp), as the value goes through a parameter
    void SetVisibleRange(float r)
    {
        visible_range = r;
    }

    // PS2 names
    void SetTo(I3D_frame *frame);
    static void DeleteFromAnyShadow(I3D_frame *frame);

    // What the rendering calls for each shadow of the scene before it queues the visuals (render.cpp), and its steps
    // (PC only, the names are guesses)
    void Prepare();
    bool UpdateVertices();
    bool UpdateLight();
    bool RenderTexture();

    bool orthogonal;                    // 0x140, PS2's
    ShadowDef *def;                     // 0x144
    S_vector *verts;                    // 0x148, the parts' vertices, in their frames' spaces
    S_vector *verts_w;                  // 0x14c, the same in world space, in the same block
    float *points;                      // 0x150, in the light's space: x, y pairs
    int num_verts;                      // 0x154
    ushort *faces;                      // 0x158, three indices into the vertices each
    int num_faces;                      // 0x15c
    ITexture *texture;                  // 0x160
    I3D_bbox bbox;                      // 0x164, of the points, in the light's space
    S_vector color;                     // 0x17c, PS2's
    I3D_scene *scene;                   // 0x188, the scene's AddShadow
    I3D_sector *sector;                 // 0x18c, the one it is in
    S_shadow_frame *frames;             // 0x190
    int num_frames;                     // 0x194
    I3D_shadow_visual *visual;          // 0x198
    int size_x, size_y;                 // 0x19c, the texture's, PS2's
    float range;                        // 0x1a4, PS2's: how far it reaches beyond the box
    S_vector light_dir;                 // 0x1a8, from the light to the shadow
    S_matrix light_matrix;              // 0x1b4, from world space into the light's
    I3D_LOCK shadow_lock;               // 0x1f4, PS2's
    float opacity;                      // 0x1f8, by the lights' strength, 0 without a light
    float max_texture_distance;         // 0x1fc, PS2's: how far the box reaches towards the light
    float visible_range;                // 0x200, scaled by FOVDistCorrection: farther from the camera it isn't
                                        // rendered (100 by default; only Duplicate sets it)
    int verts_count;                    // 0x204, the frames' update counts when the vertices were transformed
    uint shd_flags;                     // 0x208, SHDF_*
    int lights_count;                   // 0x20c, the sector's lights_changed and the shadow's update count then
    float reverse_factor;               // 0x210, PS2's
};

// SYNTHETIC: LS3DF 0x10050b70
// I3D_shadow::`scalar deleting destructor'

// The destructors the shadow file's thunks register for globalShadowList, SetTo's FoundFrames and ShadowVisualQueue.
// reccmp pairs the annotations of one name with our functions of that name in address order: the shadow file's $E13
// comes between the scene's and Globals.cpp's (I3d_math.h), its $E18 after the collision file's and the scene's.
// SYNTHETIC: LS3DF 0x10050930
// $E13

// SYNTHETIC: LS3DF 0x10050d60
// $E18

// SYNTHETIC: LS3DF 0x10052f60
// $E24

// The weights of a visual's levels and regions (PS2 I3D_void_interface; the parameters' names are the morph's
// messages'). The visuals derive from it, the morphs implement it. Its functions are I3d_visual.cpp's; NumLevels' and
// NumRegions' (return 0) are folded with a DirectShow base class function (0x10087520), so they have no markers.
// VTABLE: LS3DF 0x1009d108
class I3D_void_interface {
public:
    virtual int __stdcall NumLevels() const;
    virtual int __stdcall NumRegions() const;
    virtual LS3D_RESULT __stdcall SetWeight(int region, int level, float weight);
    virtual float __stdcall GetWeight(int region, int level) const;
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
#define VISF_COLOR_CHANGED 0x4          // a particle's color or transparency changed: m_AddFGroups writes them into
                                        // its lod's vertices
#define VISF_UNKNOWN_10 0x10            // the level of detail follows the textures loaded, and the lighting takes
                                        // the first lod's cache, not the current one's
#define VISF_UNKNOWN_20 0x20            // the mirrors don't reflect it
#define VISF_WORLD_SPACE 0x40          // the vertices are in world space (SetSpace): a move invalidates nothing
#define VISF_UNKNOWN_80 0x80            // the lighting gives every vertex their average color
#define VISF_SHADOW 0x200               // SHADOW_MODE 1 or 2: shadows fall on it
#define VISF_SHADOW_2 0x400             // SHADOW_MODE 2
#define VISF_UNKNOWN_800 0x800          // set by the objects' constructor, SetMesh and Duplicate
#define VISF_UNKNOWN_1000 0x1000        // the collision tests leave it out with COLF_UNKNOWN_4000000
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
    // Empty (PS2 inlines both; the linker folded the copies with other files' empty functions). The original's slots
    // point at a copy among the base classes' functions (0x10088970) and at the scene file's Update (0x1004e970).
    virtual void m_AddFGroups() {}
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help) {}

private:
    // Exported (the decorated name says private)
    __declspec(dllexport) void __stdcall UpdateWBoundProc();

public:
    void SetBoundVolume(const I3D_bvolume &bvol);

    // Insanity3D's name (its GetVisualType1)
    I3D_VISUAL_TYPE GetVisualType() const
    {
        return visual_type;
    }

    // The visual's bound volume in world space, updated first if it isn't (the name is a guess; the lighting and the
    // visuals' rendering inline it)
    const I3D_bvolume &GetWBoundVolume()
    {
        if (!(vis_flags & VISF_WBOUND_VALID))
            UpdateWBoundProc();
        return vis_bound_w;
    }

    // The visual's own bound volume, updated first if it isn't (the name is a guess, after GetWBoundVolume; the mirrors
    // inline it)
    const I3D_bvolume &GetBoundVolume()
    {
        if (!(vis_flags & VISF_BOUND_VALID))
            UpdateBoundProc();
        return vis_bound;
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

    // The matrix the vertices are rendered with: the world matrix, identity in world space (the name is a guess). The
    // copy is the lit object file's.
    // FUNCTION: LS3DF 0x1002ad00
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

// An object's lighting state for one of its mesh's levels (PC only; the object file's "lod::Create()" names the
// class): its own vertex buffer, which the object renders from
// VTABLE: LS3DF 0x1009c3d8
class lod {
public:
    lod();
    virtual ~lod();

    // Copies the texture coordinates at uv (stride bytes apart, offset by u_ofs and v_ofs) into the vertices of format
    // 1 or 2 (one or two coordinate pairs), as HWCAPS_HWTL lays them out (the name is a guess)
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

    // Insanity3D's name
    I3D_mesh_object *GetMesh() const
    {
        return mesh;
    }

    // The level the visual's mesh is rendered at, the first with VISF_UNKNOWN_10 (the name is a guess). The shadows'
    // receivers read it through this: written out, VC6 reads the flags once for two uses where the original reads
    // them again.
    int GetMeshLOD() const
    {
        return vis_flags & VISF_UNKNOWN_10 ? 0 : cur_lod;
    }

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

// SetSpace is folded with the morphs' and the single meshes', the same empty function. The original keeps the
// billboard file's copy, our linker the single meshes' from I3d_singlemesh.cpp and in render.cpp, the morphs' in
// I3d_projector.cpp, from I3d_occluder.cpp to I3d_particle.cpp and since I3d_lit_object.cpp, the billboard's from
// Load.cpp to lightmap.cpp, so the marker is by the name our PDB keeps (tools/foldfix.py updates it).
// FUNCTION: LS3DF 0x10005e30
// ?SetSpace@I3D_morph@@UAGX_N@Z

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

class LightMap;
struct RECTANGLE_INFO;

// A lit vertex: a vertex of the mesh's level in one of the lightmaps' rectangles (PC only; postProcess' message names
// pVI->rect, the rest is guessed). A vertex of faces in different rectangles has a lit vertex in each.
struct VERTEX_INFO {
    ushort vertex;                      // 0x00, the level's
    ushort rect;                        // 0x02, the rectangle's index
};

// A lit object's lighting of one level of its mesh (PS2 litLod, 0x74 bytes on PC; I3d_lit_object.cpp): a lod whose
// vertices take their colors from lightmaps, or from colors per vertex. The lightmaps' vertices are the lit vertices,
// rendered through an index buffer of their own; the colors are the level's vertices'. The m_ names are the
// messages' (m_pUV, m_pLitUV, m_numRectangles, m_numPIndex), the others are guessed in their style.
// VTABLE: LS3DF 0x1009be5c
class litLod : public lod {
public:
    litLod()
    {
        m_numLVertices = 0;
        m_pUV = NULL;
        m_pLitUV = NULL;
        unknown_3c = NULL;
        update_count = 0;
        m_pLM = NULL;
        m_numPIndex = 0;
        m_pPIndex = NULL;
        m_pRectangles = NULL;
        m_numRectangles = 0;
        memset(&m_IB, 0, sizeof(m_IB));
        m_pVI = NULL;
        memset(&m_PosVB, 0, sizeof(m_PosVB));
        m_MemSize = 0;
        dirty = true;
    }
    ~litLod();

    // Gives the lod its vertex buffer from the level's vertices: colored per vertex (flags bit 0), or the lit
    // vertices with their lightmap coordinates (PS2 name; the PC's parameters)
    bool Create(I3D_mesh_level *level, I3D_mesh_object *mesh, uint flags);
    // Packs the rectangles into the lightmap's texture and maps the lit vertices into it. Loaded, the index buffer was
    // read; else it is built from the faces' rectangles (PS2 name; the messages say postLoadProcess).
    bool postProcess(I3D_mesh_level *level, bool loaded);

    // The lit vertex of the vertex in the rectangle (PS2 name, inlined)
    int m_GetLVertex(ushort &vertex, ushort &rect)
    {
        VERTEX_INFO *vi = m_pVI;
        for (int i = 0; i < m_numLVertices; i++, vi++) {
            if (vi->vertex == vertex && vi->rect == rect)
                return i;
        }
        dbgPrintf("!! vertex not found GetLVertex() vertex:%d rectangle:%d", vertex, rect);
        return 0;
    }

    // The face group's faces' rectangles (PS2 name, inlined)
    void AddFGindex(ushort *indices, int index)
    {
        if (index >= m_numPIndex)
            dbgPrintf("!! Illegal order:%d  m_numPIndex:%d   litLod::AddFGindex()", index, m_numPIndex);
        else
            m_pPIndex[index] = indices;
    }

    // Room for the face groups' rectangles, cleared (PS2 name, inlined)
    bool SetNumFGI(int num)
    {
        uint size = num * sizeof(ushort *);
        m_pPIndex = (ushort **)new uchar[size];
        if (!m_pPIndex)
            return false;
        m_numPIndex = num;
        memset(m_pPIndex, 0, size);
        return true;
    }

    int m_numLVertices;                 // 0x30, the lit vertices, or the level's vertices with colors
    I3D_text_coor *m_pUV;               // 0x34, the lit vertices' texture coordinates (the level's, postProcess)
    I3D_text_coor *m_pLitUV;            // 0x38, their lightmap coordinates, in their rectangles until postProcess
    void *unknown_3c;                   // 0x3c, freed only
    VERTEX_INFO *m_pVI;                 // 0x40
    union {
        uint *m_pColors;                // 0x44, the colors, one per vertex
        COMPLETE_VB_STRUCT m_PosVB;     // 0x44, the lightmaps': the lit vertices' positions, without hardware T&L
    };
    RECTANGLE_INFO *m_pRectangles;      // 0x50
    int m_numRectangles;                // 0x54
    LightMap *m_pLM;                    // 0x58, the rectangles packed into a texture
    int m_numPIndex;                    // 0x5c, the level's face groups
    ushort **m_pPIndex;                 // 0x60, for each face group its faces' rectangles
    COMPLETE_IB_STRUCT m_IB;            // 0x64, the faces in lit vertices
    uint m_MemSize;                     // 0x70, the lightmap's memory (m_CreateLM's), 0 for colors
};

// SYNTHETIC: LS3DF 0x10029f60
// litLod::`scalar deleting destructor'

// One of a lit object's lightings (PC only, the names are guessed). PS2 has the same with three lods.
struct S_lit_level {
    bool constructed;                   // 0x00, by Construct or Load
    uint mode;                          // 0x04, 1 colors per vertex, 2 lightmaps
    float resolution;                   // 0x08, Construct's, 5.0
    float angle;                        // 0x0c, 45.0
    litLod *lods[10];                   // 0x10, one per level of the mesh
};

// A visual whose mesh is lit by lightmaps, or by colors per vertex (PS2 I3D_lit_object, 0x370 bytes on PC;
// I3d_lit_object.cpp): it has six lightings, one of them active, which Construct computes from the scene's lights or
// Load reads. Its own virtual functions are in PS2's order.
// VTABLE: LS3DF 0x1009bd70 I3D_frame
// VTABLE: LS3DF 0x1009bd5c I3D_void_interface
class I3D_lit_object : public I3D_object {
public:
    I3D_lit_object();
    ~I3D_lit_object();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);
    virtual void __stdcall SetMesh(I3D_mesh_object *m, bool keep_lods);
    virtual void __stdcall UpdateVertices();
    virtual void __stdcall UpdateVertices(int index);
    virtual LS3D_RESULT __stdcall Construct(I3D_scene *scene, S_vector &color, float resolution, uint flags,
        I3D_LOAD_CB_PROC *cb, void *context);
    virtual LS3D_RESULT __stdcall Destruct(uint flags);
    virtual LS3D_RESULT __stdcall Load(int handle);
    virtual LS3D_RESULT __stdcall Save(int handle);
    virtual LS3D_RESULT __stdcall SetMode(uint mode);
    virtual void __stdcall SetActiveLevel(int index);
    virtual uint __stdcall GetTotalLMMemory();
    virtual int __stdcall GetNumRectInfos(int lod_index, int index);
    virtual RECTANGLE_INFO *__stdcall GetRectInfos(int lod_index, int index);
    virtual void __stdcall UpdateLMFromRectInfos(int lod_index, int index);
    virtual int __stdcall GetRectInfoFlags(int lod_index, int index, int flags);

    int active_level;                   // 0x21c, of levels
    S_lit_level levels[6];              // 0x220
};

// SYNTHETIC: LS3DF 0x100215e0
// I3D_lit_object::`scalar deleting destructor'

// The destructor m_AddFGroups registers for its static list of PrepareLights' colors (0x101b42f8, in a block, so not
// in the PDB): the first $E13 in link order (see the scene's)
// SYNTHETIC: LS3DF 0x10028d20
// $E13

// The constructor of Construct's lists of the faces in each rectangle, out of line for the vector constructor iterator
// TEMPLATE: LS3DF 0x1002ad30
// C_list<NEW_FACE_STRUCT>::C_list<NEW_FACE_STRUCT>

// The single meshes (PS2 I3d_singlemesh.cpp; I3d_singlemesh.cpp): a mesh whose vertices the joints below it move.
// For each level of the mesh the file gives the joints' part of its vertices, joint by joint in the order of their
// indices: the vertices the joint moves alone, then those it shares with another joint by their weights; the vertices
// after all joints' stay where the mesh has them. PS2 keeps one SMeshDef in the single mesh, the PC one per lod.

// A joint's part of a level (PS2 SM_INFO, 0x68 bytes on PC; m_vertNum is LoadImage's message's, the other names are
// guesses). The file has it up to weights.
struct SM_INFO {
    S_matrix matrix;                    // 0x00, from the mesh's space into the joint's
    int num_verts;                      // 0x40, the vertices the joint moves alone
    int m_vertNum;                      // 0x44, the vertices after them, blended with blend_joint's moves
    int blend_joint;                    // 0x48, the other joint's index + 1, 0 for the single mesh itself
    I3D_bbox bbox;                      // 0x4c, of the joint's vertices, in its space
    float *weights;                     // 0x64, the joint's share of each of the m_vertNum vertices
};

// The joints' parts of a level, shared by the copies of a single mesh (PS2 SMeshDef, 0x28 bytes on PC), reference
// counted
struct SMeshDef {
    int ref;                            // 0x00
    int num_joints;                     // 0x04, of info
    SM_INFO *info;                      // 0x08
    int num_rigid;                      // 0x0c, the vertices at the level's end that no joint moves
    I3D_bbox bbox;                      // 0x10, of the level

    SMeshDef()
    {
        ref = 1;
        num_joints = 0;
        info = NULL;
        num_rigid = 0;
        bbox.Invalidate();
    }

    ~SMeshDef()
    {
        SM_INFO *p = info;
        for (int i = num_joints; i--; p++)
            delete[] p->weights;
        delete[] info;
    }

    void Release()
    {
        if (--ref >= 0 && !ref)
            delete this;
    }
};

// A single mesh's vertex as the joints move it (0x18 bytes): its position and normal. The lods' vertex buffers are
// transformed and lit from them, and the shadows and projectors clip them (clipper.h).
struct S_clip_sm_vertex {
    S_vector pos;
    S_vector normal;
};

// A single mesh's lod (PC only, the name is a guess): an object's lod with the joints' part of its level and the
// level's vertices as the joints move them. Its constructor is inlined where the single meshes create one; its
// destructor is defined inline in I3d_singlemesh.cpp, which puts the copy after the scalar deleting destructor.
// VTABLE: LS3DF 0x1009cd38
class SMeshLod : public lod {
public:
    SMeshLod()
    {
        def = NULL;
        memset(&vb_skin, 0, sizeof(vb_skin));
        joints_changed = (uint)-2000;
    }
    ~SMeshLod();

    SMeshDef *def;                      // 0x30
    COMPLETE_VB_STRUCT vb_skin;         // 0x34, the moved vertices (S_clip_sm_vertex)
    uint joints_changed;                // 0x40, the single mesh's when they were moved
};

// SYNTHETIC: LS3DF 0x10056ad0
// SMeshLod::`scalar deleting destructor'

// A mesh moved by joints (PS2 I3D_singlemesh, 0x22c bytes on PC): an object whose lods are SMeshLods. Its joints are
// the I3D_joints among the frames below it (DBuild), by their index. The single morph is one too: the model's
// duplication builds both.
// VTABLE: LS3DF 0x1009cc98 I3D_frame
// VTABLE: LS3DF 0x1009cc88 I3D_void_interface
class I3D_singlemesh : public I3D_object {
public:
    I3D_singlemesh();
    ~I3D_singlemesh();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();
    // Empty: the vertices stay in local space. The linker folded it with the billboard's (0x10005e30).
    virtual void __stdcall SetSpace(bool world);
    virtual void __stdcall UpdateBoundProc();
    virtual void m_AddFGroups();
    // The joints are moved again too (PS2 names; VC6 puts the second at the first's slot)
    virtual void __stdcall UpdateVertices();
    virtual void __stdcall UpdateVertices(int index);
    // A joint's box in its space, or the first level's (index -1), from the first lod (PS2 name)
    virtual const I3D_bbox *__stdcall GetJointBBox(int index) const;
    // The lod's moved vertices, locked, moved again first if the joints moved (PS2 name; PS2's returns the positions,
    // in a buffer of its own)
    virtual const S_clip_sm_vertex *__stdcall GetVertexPos(int lod_index);
    // The joint that moves the lod's vertex (PS2 name)
    virtual I3D_joint *__stdcall GetVertexJoint(int vertex, int lod_index);
    // Moves the lod's vertices by the joints (PC only, the name is a guess)
    virtual void UpdateLodVertices(int lod_index);

    // PS2 names
    LS3D_RESULT DuplicateSinglemeshDef(I3D_singlemesh *sm);
    void UpdateBVolumeProc();
    // Finds the joints again; false if it has none
    bool DBuild();
    // The positions of the lod's vertices into its vertex buffer, stride bytes apart: those no joint moves from the
    // mesh, then the moved ones (PC only, the names are guesses)
    void CopyRigidPositions(uchar *dst, uint stride, int lod_index);
    void CopyMovedPositions(uchar *dst, uint stride, int lod_index);

    int num_joints;                     // 0x21c
    I3D_joint **joints;                 // 0x220, by their index (PS2 0x214)
    uint joints_changed;                // 0x224, counts the moves of its joints (PS2 0x218; the name is a guess)
    uint bvol_joints_changed;           // 0x228, joints_changed when the bound volume was computed
};

// SYNTHETIC: LS3DF 0x10054290
// I3D_singlemesh::`scalar deleting destructor'

// I3D_joint::GetJointMatrix: its local matrix times its parents' up to the single mesh's level (the first visual,
// sector or model), or up to a parent joint whose own is up to date. Here, after SMeshLod: the single meshes' file has
// the out-of-line copy, which the precompiled header defers to the end of the file, after SMeshLod's scalar deleting
// destructor and destructor, as in the original, only if the header has it after that class.
// Not matching yet: the original's copy calls S_vector::operator*= out of line for four of the first UpdateMatrix's
// row scalings and inlines the loop's, ours inlines almost all of the first. Five empty inline calls after the first
// UpdateMatrix (a probe) give the original's calls, so the original had that many more inline calls pending there;
// no source for them is known. The callers that inline the function don't change with them.
// FUNCTION: LS3DF 0x10056ba0
inline const S_matrix &I3D_joint::GetJointMatrix()
{
    if (!(joint_flags & JOINTF_JMAT_VALID)) {
        joint_flags |= JOINTF_JMAT_VALID;
        UpdateMatrix();
        joint_matrix = f_matrix;
        for (I3D_frame *frm = parent; frm; frm = frm->parent) {
            switch (frm->GetType()) {
            case FRAME_VISUAL:
            case FRAME_SECTOR:
            case FRAME_MODEL:
                return joint_matrix;
            case FRAME_JOINT:
                if (((I3D_joint *)frm)->joint_flags & JOINTF_JMAT_VALID) {
                    joint_matrix = joint_matrix * ((I3D_joint *)frm)->joint_matrix;
                    return joint_matrix;
                }
            default:
                frm->UpdateMatrix();
                joint_matrix = joint_matrix * frm->f_matrix;
            }
        }
    }
    return joint_matrix;
}

// The morphs (PS2 I3d_morph.cpp; I3d_morph.cpp): a mesh whose vertices are a weighted sum of its levels. A morph's
// vertices are split into regions, each with a weight for every level; m_pMorphDef holds the levels' positions and
// normals for each region of each lod of the mesh, and the morph sums them into the lod's vertices when the weights
// change (UpdateMorphLod). Class names are PS2's, member names the messages' (m_numLevels, m_numRegions, m_pMorphDef)
// or guesses, the classes' in the messages' style.

// A region's vertices in one lod (PS2 morph_region, 0xc bytes)
struct morph_region {
    morph_region();
    ~morph_region();

    int num_verts;
    ushort *indices;                    // 0x04, the mesh vertex each one moves
    S_vector *verts;                    // 0x08, for each vertex a position and a normal per level, 16-byte aligned
};

// The levels of a morph, shared by its copies (PS2 MorphDef, 0x38 bytes on PC), reference counted
class MorphDef {
public:
    MorphDef(int lods, int regs);
    ~MorphDef();
    int Release();

    int m_numLods;                      // 0x00
    int m_numRegions;                   // 0x04
    morph_region *m_pRegions;           // 0x08, m_numRegions for each lod
    int m_RefCount;                     // 0x0c
    I3D_bvolume m_BVolume;              // 0x10, the morph's (LoadImageB reads it)
};

// What morphBase::unknown_18 points to (PC only): nothing in LS3DF creates one, the morphs only release it. Its
// objects' types aren't known; Release is their first virtual function, as the mesh levels'.
struct S_morph_unknown {
    I3D_mesh_level *unknown_00[8];
    I3D_mesh_level *unknown_20[16];

    ~S_morph_unknown();
};

// morphBase::m_Flags (names guessed; PS2's are other bits, a dirty bit for each lod, 0x10000 and 0x20000)
#define MORPHF_DIRTY 0x1                // the weights changed: UpdateMorphLod moves the vertices
#define MORPHF_LOADED 0x2               // m_pMorphDef holds the levels (LoadImageB, or Duplicate's copy)
#define MORPHF_UNKNOWN_4 0x4            // set by the constructor, cleared by LoadImage and Duplicate; nothing tests it

// What a morph is besides its visual (PS2 morphBase, 0x1c bytes on PC): the weights and the levels. The PS2's is
// polymorphic, the PC's isn't. Its destructor is the PS2's BogusDestructor.
class morphBase {
public:
    morphBase();
    ~morphBase()
    {
        Clear();
    }

    // Frees the weights and the levels
    void Clear();
    LS3D_RESULT SetWeightB(int region, int level, float weight);
    float GetWeightB(int region, int level) const;
    // m_pNormWeights from m_pWeights: each region's scaled to a sum of 1, or all on the first level if they are all
    // zero
    void NormalizeWeights();
    // Sums the levels of the lod's regions into its vertices, if the weights changed (the second one also into the
    // positions at pos, stride bytes apart: a lod's vertices)
    void UpdateMorphLod(int lod, I3D_mesh_object *mesh);
    void UpdateMorphLod(void *pos, uint stride, int lod, I3D_mesh_object *mesh);
    // Shares mb's levels and copies its weights
    LS3D_RESULT DuplicateMorphDef(morphBase *mb);
    void LoadImageB();

    uint m_Flags;                       // 0x00, MORPHF_*
    int m_numLevels;                    // 0x04
    int m_numRegions;                   // 0x08
    float *m_pWeights;                  // 0x0c, m_numLevels for each region; m_pNormWeights is in the same block
    float *m_pNormWeights;              // 0x10
    MorphDef *m_pMorphDef;              // 0x14
    S_morph_unknown *unknown_18;        // 0x18
};

// A morph (PS2 I3D_morph, 0x238 bytes on PC): an object whose mesh is its own copy of the original's, which the
// weights move
// VTABLE: LS3DF 0x1009c268 I3D_frame
// VTABLE: LS3DF 0x1009c258 I3D_void_interface
class I3D_morph : public I3D_object, public morphBase {
public:
    I3D_morph();
    ~I3D_morph();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();
    // Empty: the vertices stay in local space. The linker folded it with the billboard's (0x10005e30).
    virtual void __stdcall SetSpace(bool world);
    virtual void m_AddFGroups();
    virtual void __stdcall SetMesh(I3D_mesh_object *m, bool keep_lods);
    virtual int __stdcall NumLevels() const;
    virtual int __stdcall NumRegions() const;
    virtual LS3D_RESULT __stdcall SetWeight(int region, int level, float weight);
    virtual float __stdcall GetWeight(int region, int level) const;
};

// SYNTHETIC: LS3DF 0x10034e90
// I3D_morph::`scalar deleting destructor'

// A single morph (PS2 I3D_singlemorph, 0x248 bytes on PC; I3d_singlemorph.cpp): a single mesh whose levels are summed
// by the weights before the joints move the vertices. Its copies share the mesh; Duplicate gives it a copy of a morph's
// or single morph's own.
// VTABLE: LS3DF 0x1009cd90 I3D_frame
// VTABLE: LS3DF 0x1009cd80 I3D_void_interface
class I3D_singlemorph : public I3D_singlemesh, public morphBase {
public:
    I3D_singlemorph();
    ~I3D_singlemorph();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void LoadImage();
    virtual void __stdcall UpdateBoundProc();
    virtual void __stdcall SetMesh(I3D_mesh_object *m, bool keep_lods);
    virtual int __stdcall NumLevels() const;
    virtual int __stdcall NumRegions() const;
    virtual LS3D_RESULT __stdcall SetWeight(int region, int level, float weight);
    virtual float __stdcall GetWeight(int region, int level) const;
    // The levels summed into the mesh first, if the weights changed
    virtual void UpdateLodVertices(int lod_index);
};

// SYNTHETIC: LS3DF 0x10056ef0
// I3D_singlemorph::`scalar deleting destructor'

// I3D_part_base::part_flags (names guessed)
#define PARTF_OPAQUE 0x1                // rendered opaque while not faded, else blended
#define PARTF_ALPHA 0x2                 // with 3: blended by alpha (else added), sort_dist in the rendering's key
#define PARTF_MESH 0x200                // the particles' renderer draws the mesh, else a quad
#define PARTF_RENDERER 0x600            // queued for the particles' renderer (VisualProc)

// The particles (PS2 I3D_part_base and I3D_part_element, 0x284 and 0x310 bytes on PC; I3d_particle.cpp): billboards
// of one color, which turn to the camera only with billboard set. The rendering queues them for the particles'
// renderer with a key from sort_group and sort_dist (VisualProc), or renders them itself. An element is an emitter's
// particle, which moves, turns, grows and fades over its life. The names but the classes' are guesses.
// VTABLE: LS3DF 0x1009c660 I3D_frame
// VTABLE: LS3DF 0x1009c650 I3D_void_interface
class I3D_part_base : public I3D_billboard {
public:
    I3D_part_base();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);
    // The sector it is lit by, else the rendering's
    // FUNCTION: LS3DF 0x100417d0
    virtual void __stdcall SetSector(I3D_sector *s)
    {
        sector = s;
    }
    // FUNCTION: LS3DF 0x100417f0
    virtual I3D_sector *__stdcall GetSector()
    {
        return sector;
    }

    ushort sort_group;                  // 0x268, the high word of the rendering's key
    uint part_flags;                    // 0x26c, PARTF_*
    S_vector color;                     // 0x270
    bool billboard;                     // 0x27c, turned to the camera
    I3D_sector *sector;                 // 0x280
};

// SYNTHETIC: LS3DF 0x10040740
// I3D_part_base::`scalar deleting destructor'

// The implicit destructor, a jump to the billboard's
// SYNTHETIC: LS3DF 0x10040760
// I3D_part_base::~I3D_part_base

// VTABLE: LS3DF 0x1009c5b8 I3D_frame
// VTABLE: LS3DF 0x1009c5a4 I3D_void_interface
class I3D_part_element : public I3D_part_base {
public:
    I3D_part_element();
    ~I3D_part_element();

    virtual void __stdcall Tick(int time);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);

    I3D_emitor *emitor;                 // 0x284, which created it, until it dies
    uint life;                          // 0x288
    int life_max;                       // 0x28c, the emitter's longest
    uint unknown_290;
    S_vector dir_start;                 // 0x294, the start velocity's direction
    S_vector dir_end;                   // 0x2a0, the end velocity's
    S_vector dir;                       // 0x2ac, where it heads, dir_end strayed
    uint dir_time;                      // 0x2b8, the emitter's PP_DIR_TIME
    uint dir_time_rnd;                  // 0x2bc
    float dir_time_stray;               // 0x2c0
    float rot_speed;                    // 0x2c4, per ms
    float speed_start;                  // 0x2c8
    float speed_end;                    // 0x2cc
    uint fade_in;                       // 0x2d0, ms
    uint fade_out;                      // 0x2d4
    float grow;                         // 0x2d8, per ms
    S_vector color_start;               // 0x2dc
    S_vector color_end;                 // 0x2e8
    S_vector dir_prev;                  // 0x2f4, where it headed before dir
    int age;                            // 0x300
    float speed;                        // 0x304
    int dir_period;                     // 0x308, until it strays again, 0 never
    int dir_counter;                    // 0x30c
};

// SYNTHETIC: LS3DF 0x1003fbe0
// I3D_part_element::`scalar deleting destructor'

// The landscape (PC only; I3d_landscape.cpp) is Bryan Turner's ROAM demo ("ROAM Simplistic Implementation", January
// 2000, from his Gamasutra article on ROAM) in the engine's classes: a height map of bytes, MAP_SIZE on a side, in
// NUM_PATCHES_PER_SIDE patches a side, each a pair of binary triangle trees that a frame splits from a pool of nodes.
// The demo's names are kept where the roles agree; the PC's additions have guessed names.
#define MAP_SIZE 1024
#define NUM_PATCHES_PER_SIDE 16
#define PATCH_SIZE (MAP_SIZE / NUM_PATCHES_PER_SIDE)
// The nodes for all patches' trees (the demo has 25000)
#define POOL_SIZE 50000
// The depth of a patch's variance trees
#define VARIANCE_DEPTH 9

// A triangle of a patch's trees: its children once split, and the triangles across its three edges (the demo's)
struct TriTreeNode {
    TriTreeNode *LeftChild;
    TriTreeNode *RightChild;
    TriTreeNode *BaseNeighbor;
    TriTreeNode *LeftNeighbor;
    TriTreeNode *RightNeighbor;
};

// A square of the landscape (the demo's Patch, 0x620 bytes; I3D_VISUAL_LANDPATCH): a visual whose leaves the
// landscape's Render draws. The virtual functions after the visual's are the demo's, in its order. The PC's has no
// visibility flag (the landscape tests render_number) and no SetVisibility.
// VTABLE: LS3DF 0x1009bb48 I3D_frame
// VTABLE: LS3DF 0x1009bb38 I3D_void_interface
class I3D_landpatch : public I3D_visual {
public:
    I3D_landpatch();
    ~I3D_landpatch();

    // Draws nothing. Not in the class: LINK folds an identical function only with others of its kind, in-class or
    // out of line, and the original's slot points at the copy of the out-of-line ones (0x10087ec0, with
    // I3D_driver::m_RecomputeUsedVRAM), not at the visual's (0x1004e970). The original keeps the base classes' copy of
    // the group; which one our linker keeps changes with the link (see the markers of folded base class functions).
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);

    TriTreeNode *GetBaseLeft()
    {
        return &m_BaseLeft;
    }

    TriTreeNode *GetBaseRight()
    {
        return &m_BaseRight;
    }

    char isDirty()
    {
        return m_VarianceDirty;
    }

    virtual void Init(int heightX, int heightY, int worldX, int worldY, uchar *hMap);
    virtual void Reset();
    virtual void Tessellate();
    virtual void Render();
    virtual void ComputeVariance();

    virtual void Split(TriTreeNode *tri);
    virtual void RecursTessellate(TriTreeNode *tri, int leftX, int leftY, int rightX, int rightY, int apexX, int apexY,
        int node);
    virtual void RecursRender(TriTreeNode *tri, int leftX, int leftY, int rightX, int rightY, int apexX, int apexY);
    virtual uchar RecursComputeVariance(int leftX, int leftY, uchar leftZ, int rightX, int rightY, uchar rightZ,
        int apexX, int apexY, uchar apexZ, int node);

    uint unknown_1dc;
    uchar *m_HeightMap;                 // 0x1e0, at the patch's corner in the landscape's map
    int m_WorldX, m_WorldY;             // 0x1e4, the patch's place in the map
    uchar m_VarianceLeft[1 << VARIANCE_DEPTH]; // 0x1ec, by node, one more than the variance
    uchar m_VarianceRight[1 << VARIANCE_DEPTH]; // 0x3ec
    uchar *m_CurrentVariance;           // 0x5ec, the tree's whose triangle is being worked on
    uchar m_VarianceDirty;              // 0x5f0, the variance trees are to be computed
    TriTreeNode m_BaseLeft;             // 0x5f4, the trees' roots
    TriTreeNode m_BaseRight;            // 0x608
    ITexture *texture;                  // 0x61c, rendered with; nothing in LS3DF sets it
};

// SYNTHETIC: LS3DF 0x1001efa0
// I3D_landpatch::`scalar deleting destructor'

// A landscape (the demo's Landscape, 0x62144 bytes; FRAME_LANDSCAPE): its patches are its children. The driver keeps
// the landscapes in a list, and its Render resets, tessellates and renders each in turn.
// VTABLE: LS3DF 0x1009bbe8
class I3D_landscape : public I3D_frame {
public:
    I3D_landscape();
    ~I3D_landscape();

    // Takes the height map, loads the textures, makes the patches and the vertex buffer. With size 0, name is the map,
    // which the landscape keeps; otherwise the map's file, MAP_SIZE + 1 bytes on a side, read whole (size isn't used
    // otherwise; the names are guesses).
    virtual void __stdcall Init(const char *name, uint size);

    // The height at a place of the map (the name is a guess). Out of line for the vtable, after the file's $E for the
    // zero vector.
    // FUNCTION: LS3DF 0x100201d0
    virtual uchar __stdcall GetHeight(int x, int y)
    {
        return m_HeightMap[y * MAP_SIZE + x];
    }

    // A node from the pool, NULL when it is used up. The pool and its count are the file's globals, not the demo's
    // static members: VC6 gives each static data member declared in a header one of the $E numbers of every file
    // that includes it, which would rename the other files' $Es.
    static TriTreeNode *AllocateTri();

    // Not virtual, unlike the demo's
    void Reset();
    void Tessellate();
    void Render();

    I3D_landpatch m_Patches[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE]; // 0x140
    uchar *m_HeightMap;                 // 0x62140, Init's
};

// SYNTHETIC: LS3DF 0x1001fca0
// I3D_landscape::`scalar deleting destructor'

// The initializer of I3d_landscape.cpp's gViewPos, empty (S_vector's constructor is): the third $E5 in link order,
// after I3d_frame.cpp's
// SYNTHETIC: LS3DF 0x1001ef60
// $E5

// I3D_mirror::mirror_flags (names guessed)
#define MIRF_VIEW_BOX 0x1               // view_box is set
#define MIRF_PREPARED 0x4               // bsphere, inv_view_box and mirror_matrix are up to date (until it moves)
#define MIRF_VISIBLE 0x100              // the camera sees its front (UpdateVisibility)
#define MIRF_REFLECT 0x200              // near enough to render what it reflects

// A mirror (PS2 I3D_mirror, 0x2ec bytes on PC; I3d_mirror.cpp): its faces, which the rendering draws in its color, or,
// near the camera, as a window into the scene reflected in the mirror's plane. It reflects the visuals in its view box,
// a box in its local space: for each it keeps a copy, a visual of the same type with VISF_PROXY whose user_data is the
// visual, placed by mirror_matrix. PS2's has only the box, the color and the faces (in an index buffer, with a GetFaces
// the PC lacks), and of these functions LoadImage and, not virtual, SetColor and SetViewBox. Member names are guesses.
// VTABLE: LS3DF 0x1009c0a0 I3D_frame
// VTABLE: LS3DF 0x1009c090 I3D_void_interface
class I3D_mirror : public I3D_visual {
public:
    I3D_mirror();
    ~I3D_mirror();

    virtual void __stdcall Tick(int time);
    virtual LS3D_RESULT __stdcall LinkTo(I3D_frame *frame, uint flags);
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();
    // PS2 names. SetViewBox scales m's axes by half the box's size and moves it to the box's center, so that the view
    // box is the cube (-1, -1, -1)-(1, 1, 1) in view_box's space.
    virtual void __stdcall SetViewBox(const I3D_bbox &bbox, const S_matrix &m);
    virtual void __stdcall SetColor(const S_vector &c);

    // The rendering's (PC only, the names are guesses): whether the camera sees the mirror and whether it reflects, the
    // copies of the visuals it reflects (their number), and the faces in a color, flags as I3D_driver::DrawTriangles'
    void UpdateVisibility();
    int PrepareReflection();
    void Draw(const S_vector &c, uint flags);

    // Adds the visuals in the branch from frm that are in pCurrentMirror's view box to MirroredVisualList
    static void CollectVisuals(I3D_frame *frm);

    uint unknown_1dc;
    I3D_scene *scene;                   // 0x1e0, the scene's AddMirror
    C_list<I3D_visual *> copies;        // 0x1e4, of the visuals it reflects
    I3D_bsphere bsphere;                // 0x1f4, the view box's, in world space
    float radius_2;                     // 0x204, bsphere.radius squared
    S_matrix inv_view_box;              // 0x208, world space to the view box's
    S_matrix mirror_matrix;             // 0x248, a world matrix to its reflection in the mirror's plane
    uint mirror_flags;                  // 0x288, MIRF_*
    S_matrix view_box;                  // 0x28c, in local space; LoadImage reads the members from here to num_faces
    S_vector color;                     // 0x2cc
    float range;                        // 0x2d8, 100: the reflection is rendered nearer than this (FOVDistCorrection
                                        // scales it)
    int num_verts;                      // 0x2dc
    int num_faces;                      // 0x2e0
    S_vector *verts;                    // 0x2e4, in local space
    I3D_triface *faces;                 // 0x2e8
};

// SYNTHETIC: LS3DF 0x100318f0
// I3D_mirror::`scalar deleting destructor'

// The destructor the mirror file's thunk registers for MirroredVisualList
// SYNTHETIC: LS3DF 0x100324e0
// $E6

// I3D_material::flags (names guessed). The loader reads them from the file, and LoadImage makes the texture's
// ITexture::Create flags from them (MATF_LOD0 to MATF_IMAGE_ALPHA).
#define MATF_ENVMAP_SCALE 0xff          // with MATF_ENVMAP, divides its level's env_scale (SetEnvScale; 1 by default)
#define MATF_ENVMAP_BLEND 0x700         // how the environment map is blended (1 to 5, I3D_object::m_RenderFGroup)
#define MATF_LOD0 0x800                 // the objects render the face group from their first lod; its texture is the
                                        // texture manager's (ITC_MANAGED), cleared where there is none
#define MATF_ENVMAP_MODE 0x7000         // with MATF_ENVMAP, a level's MESHF_ENVMAP_MODE value
#define MATF_KEEP_PIXELS 0x8000         // ITC_KEEP_PIXELS
#define MATF_ADDRESS_U_CLAMP 0x10000    // the texture is clamped, not wrapped
#define MATF_ADDRESS_V_CLAMP 0x20000
#define MATF_TEXTURE 0x40000            // texture is used; gives its level MESHF_TEXTURE
#define MATF_ENVMAP 0x80000             // env_texture is used; gives its level MESHF_ENVMAP
#define MATF_TRUECOLOR 0x100000         // ITC_TRUECOLOR (also with the driver's render state 17)
#define MATF_NO_DXT 0x200000            // no ITC_DXT where render state 25 asks for it
#define MATF_FULL_SIZE 0x400000         // no ITC_HALF where RS_LOD_COARSER asks for it
#define MATF_MIPMAP 0x800000            // ITC_MIPMAP, with the driver's render state 13
#define MATF_IMAGE_ALPHA 0x1000000      // the alpha is the image's own (ITC_ALPHA), not a second image's
#define MATF_ANIM_ALPHA 0x2000000       // an animated material's alpha images are numbered like its images
#define MATF_ANIMATED 0x4000000         // UpdateTexture picks the frame
#define MATF_KEEP_DIFFUSE 0x8000000     // textured, the lights still take the diffuse color: gives its level
                                        // MESHF_KEEP_DIFFUSE (after Insanity3D's loader's keep_diffuse)
#define MATF_TWO_SIDED 0x10000000       // not culled
// The last three are Insanity3D's names for the same roles (its values differ)
#define MATF_CKEY_ALPHA 0x20000000      // alpha tested against m_AlphaRef
#define MATF_TXT_ALPHA 0x40000000       // the texture has alpha: blended (I3D_driver::GetIcon sets it)
#define MATF_ADD_MODE 0x80000000        // blended additively

// A material (PS2 I3D_material, 0x40 bytes on PC), reference counted. I3d_material.cpp; the animated one is in
// I3D/I3d_material.h. Every function is virtual on PC, where PS2's set the colors directly. Names are PS2's unless
// noted.
// VTABLE: LS3DF 0x1009bea0
class I3D_material {
public:
    I3D_material();

    virtual int __stdcall Release();
    // Each returns I3D_OK
    virtual LS3D_RESULT __stdcall SetAmbient(const S_vector &color);
    virtual LS3D_RESULT __stdcall SetDiffuse(const S_vector &color);
    virtual LS3D_RESULT __stdcall SetEmissive(const S_vector &color);
    virtual LS3D_RESULT __stdcall SetAlpha(float a);
    virtual LS3D_RESULT __stdcall SetTexture(ITexture *texture);
    // Makes this material a copy of mat
    virtual LS3D_RESULT __stdcall Duplicate(I3D_material *mat);
    // Sets MATF_TWO_SIDED (Insanity3D's name, whose version takes a bool)
    virtual LS3D_RESULT __stdcall Set2Sided();
    // An animated material's frame. Empty here: the copy the original kept is the scene file's (0x1004e970).
    virtual void __stdcall UpdateTexture()
    {
    }
    // The same for the environment map
    virtual void __stdcall UpdateEnvTexture()
    {
    }
    // Overloads share the slot of the first one, in reverse order: the second is slot 10, this one slot 11
    virtual LS3D_RESULT __stdcall SetEnvTexture(ITexture *texture);
    // The environment map, with its MATF_ENVMAP_BLEND and MATF_ENVMAP_MODE values and env_alpha; a NULL texture
    // clears MATF_ENVMAP
    virtual LS3D_RESULT __stdcall SetEnvTexture(ITexture *texture, uint blend, uint mode, float a);
    // PC only (the names are guesses, after Insanity3D's IsCkey)
    // FUNCTION: LS3DF 0x1002cd80
    virtual uint __stdcall IsCkey()
    {
        return flags & MATF_CKEY_ALPHA;
    }
    // FUNCTION: LS3DF 0x1002cd90
    virtual uint __stdcall IsAnimated()
    {
        return flags & MATF_ANIMATED;
    }
    virtual LS3D_RESULT __stdcall SetEnvType(uint type);
    virtual LS3D_RESULT __stdcall SetEnvMode(uint mode);
    // PC only (the name is a guess): MATF_ENVMAP_SCALE
    virtual LS3D_RESULT __stdcall SetEnvScale(uint scale);
    // Without PS2's clamp. The linker folds it with ISND_3Dlistener::SetDistanceFactor, whose copy the original kept.
    virtual LS3D_RESULT __stdcall SetEnvAlpha(float a);
    virtual ~I3D_material();

    // The material's part of the 4ds file: the colors, the textures' names. The loader calls it (load4ds.cpp).
    void LoadImage();

    int ref;                            // 0x04
    uint flags;                         // 0x08, MATF_*
    ITexture *texture;                  // 0x0c
    ITexture *env_texture;              // 0x10
    S_vector ambient;                   // 0x14
    S_vector diffuse;                   // 0x20
    S_vector emissive;                  // 0x2c
    float alpha;                        // 0x38, 1.0 opaque
    float env_alpha;                    // 0x3c, the environment map's blend factor (MATF_ENVMAP_BLEND 1)

    void AddRef()
    {
        ref++;
    }

    // The name is a guess
    const float &GetAlpha() const
    {
        return alpha;
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

// SYNTHETIC: LS3DF 0x1002ae80
// I3D_material::`scalar deleting destructor'

// The animated material (PS2 I3D_anim_material) holds its frames in std::vectors: I3D/mesh/I3d_material.h
class I3D_anim_material;

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

    // The members by reference (the names are guesses)
    I3D_material *const &GetMaterial() const
    {
        return material;
    }

    const int &NumFaces() const
    {
        return num_faces;
    }

    I3D_face_group *const &GetNext() const
    {
        return next;
    }
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
    // Returns a reference (the billboard agent's finding, 2026-10-22): by value, VC6 loads the flags at the start of the
    // inlined lod::Create call in the visuals' m_AddFGroups; by reference it reads them where the argument is pushed
    // (mov dl, byte ptr [eax + 0x3c]; mov ecx, [eax + 8]; test dl, 8), and the first Lock's push_back stays out of
    // line, as in the original (the object's, morph's, particles', single mesh's and billboard's m_AddFGroups).
    const uint &GetFlags() const
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

// A base class function whose folded copy our map lists under an engine object is left out of library_strmbase.h by
// crtannotate and gets a marker here. Which object our map lists a folded copy under changes with the link
// (CBaseRenderer::OnStopStreaming's (0x10088960), folded with CBaseFilter::GetSetupData, was IShow.cpp's until
// I3d_interpolator.cpp, again from I3d_area.cpp to I3d_animation.cpp, from I3d_material.cpp to I3d_sound.cpp, from
// I3d_morph.cpp to I3d_singlemorph.cpp and from i3d_collision.cpp to I3d_shadow.cpp; CBaseRenderer::OnRenderEnd's
// (0x10087ec0), folded with I3D_driver::m_RecomputeUsedVRAM, the driver's until I3d_model.cpp, from I3d_target.cpp to
// I3d_area.cpp, from I3d_material.cpp to I3d_mirror.cpp, from I3d_singlemorph.cpp to i3d_collision.cpp and in
// I3d_shadow.cpp, and with I3D_landpatch::m_RenderFGroup I3d_landscape.cpp's in that file, from I3d_morph.cpp to
// I3d_singlemesh.cpp and from I3d_projector.cpp to render.cpp; CBasePin::EndOfStream's I3d_visual.cpp's until then;
// CBasePin::CompleteConnect's (0x10087520), folded with I3D_void_interface::NumLevels and NumRegions, I3d_visual.cpp's
// from I3d_sector.cpp to I3d_area.cpp, again from clipping.cpp's CheckClipFaces, a function the linker drops, to
// I3d_pose.cpp, from I3d_morph.cpp to I3d_shadow.cpp and since I3d_particle.cpp; OnRenderEnd's again from
// I3d_portal.cpp to I3d_particle.cpp, OnStopStreaming's in I3d_portal.cpp only, and both again from lightmap.cpp to
// Load5ds.cpp, where CompleteConnect's went back to the base classes; with I3d_lit_object.cpp all three are the base
// classes' again), so after a new file or function rerun crtannotate and move such markers here or back. None is needed
// now; the names are the ones our PDB keeps (tools/foldfix.py).

// The function-local statics of I3d_driver.cpp: VC6 puts a function's static into the PDB as a module-level symbol
// with its plain name, where reccmp doesn't look for one, so by-name markers match them. FrameType2StringFunc's
// buffer comes first, VisualType2String's second.

// RenderShadowTexture's list of the points in texture coordinates, its guard and the destructor its first call
// registers with atexit. VC6 numbers the last two per file (after the zero vector's three numbers and one for
// <vector>'s template static, through I3D/mesh/I3d_material.h), so the names hold only while no other file has them.
// The guard's place in the original's .bss, between IconMaterialList and driverTickTime, is the hash bucket of $S27 or
// $S33 (tools/bsshash.py, with the decorated name), ours of $S24 comes after driverTickTime: the original's file
// numbered 3 (or 9) more before it.
// GLOBAL: LS3DF 0x100bfc88
// points

// GLOBAL: LS3DF 0x100bffc0
// $S24

// The guard of I3D_scene's constructor's static bigBox (I3d_scene.cpp), the only $S25 among our function statics
// GLOBAL: LS3DF 0x101b4e5e
// $S25

// SYNTHETIC: LS3DF 0x100165e0
// $E25

// The destructor the driver's thunk registers for IconMaterialList: the first $E10 in link order, before the model
// file's and render.cpp's
// SYNTHETIC: LS3DF 0x10015560
// $E10

// And those for TLVertList and LVertList: the second $E15 and $E20 in link order, after the collision file's
// SYNTHETIC: LS3DF 0x100155b0
// $E15

// SYNTHETIC: LS3DF 0x10015600
// $E20

// DrawSphere's matrix
// GLOBAL: LS3DF 0x100afc28
// m

// GLOBAL: LS3DF 0x100af8e8
// str

// GLOBAL: LS3DF 0x100bffa8
// si

// GLOBAL: LS3DF 0x100bffc8
// str

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
// ?push_back@?$C_list@PAVC_IBuffer@@@@QAEXABQAVC_IBuffer@@@Z

// The frame's empty Tick, folded with the other empty functions of its shape (29 vtables' slots; our PDB names the
// copy after the scene's SetOn, from I3d_singlemesh.cpp to I3d_singlemorph.cpp after Tick itself, in I3d_shadow.cpp
// and render.cpp after the shadow visual's SetOn, in I3d_projector.cpp after vector<ITexture *>::_Destroy, in
// I3d_occluder.cpp and I3d_particle.cpp after Tick itself, in I3d_portal.cpp and from Load.cpp to lightmap.cpp after
// the shadow visual's SetOn); the original keeps the area file's copy
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
    LS3D_RESULT initRender();
    void closeRender();
    void m_DrawAllSortedFG();

    // The fast find tree's rebuild (PS2 name)
    void OFFD_RebuildBranchOfFFTree_(I3D_frame **branch, int first, int num);

public:
    uint init_flags;                    // 0x04, Init's
    C_list<ITexture *> textures;        // 0x08, the loader's
    C_list<I3D_landscape *> landscapes; // 0x18, the landscapes' constructor and destructor keep it
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
// D3DRS_ZFUNC: 1 D3DCMP_LESSEQUAL, 0 D3DCMP_ALWAYS
extern uint m_CurrZFunc;
extern uint m_CurrFogColor;
extern uint m_CullMode;
// The color op that modulates by the texture: D3DTOP_MODULATE2X, or D3DTOP_MODULATE without it
extern uint m_Modulate2X;
// What Init found the hardware lacks (bit 0x10, 0x20, 0x40: the environment blends 1, 2 and 5 in one pass)
extern uint m_ProblemFlags;
// What the device can do (HWCAPS_*, ls3df.h)
extern uint m_HardwareCaps;

extern uint m_AlphaRef;
extern D3DVIEWPORT8 m_D3DViewPort8;

// Functions that set a render state where it changed (names guessed). PS2 inlines them all. On PC they are inline, in
// I3D/render/render.h with SetBlendState and SetVertexFormat: VC6 inlines them where the inline budget allows it, and the
// copies the linker kept are in the billboard file's code.

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

// The draws' vertices (the mirrors' too)
extern C_list<LVERTEX> LVertList;

// I3D_driver::DrawTriangles' flags, which the mirrors' Draw takes too: the transparency is the low byte (Insanity3D's
// name for its mask, spelling included; the other names are guesses)
#define I3DDRAW_TRANPARENCY_MASK 0xff
#define I3DDRAW_TRANSFORMED 0x100       // the vertices are transformed (TLVERTEX)
#define I3DDRAW_NOCULL 0x200
#define I3DDRAW_CULL_CW 0x400

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

// T&l.cpp's $E5, TempLightList's constructor loop: its only reference points inside the array, so reccmp can pair it only
// by name. The eighth $E5 in link order, after clipper.cpp's and the frame, landscape, model, portal, target and lightmap
// files'.
// SYNTHETIC: LS3DF 0x10065a10
// $E5

// And the destructor PrepareLights registers for its static color (S_color's, empty): the second $E11 in link order,
// after IBManager.cpp's
// SYNTHETIC: LS3DF 0x10066b40
// $E11

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
    int first;                          // 0x0c, the particles' renderer's: its group's first entry in the sorted order
    I3D_face_group *fg;                 // 0x10
    I3D_sector *sector;                 // 0x14, lit by
    ITexture *texture;                  // 0x18
    union {
        ITexture *env_texture;          // 0x1c
        uint num;                       // 0x1c, the particles' renderer's: its group's number of particles
    };
};

// A list of data to sort by their keys (PS2 C_sort_list<int>, with these three members; the names and the functions
// are H&D Deluxe's C_sort_list's, an earlier relative): it doubles when it is full, and Reserve makes room for n
// entries, dropping the ones it has. RadixSort32 sorts it.
template<class T>
class C_sort_list {
public:
    sort_entry_32 *base;
    uint res_num;                       // 0x04, the entries base has room for
    uint count;                         // 0x08

    C_sort_list() : base(NULL), res_num(0), count(0) {}

    ~C_sort_list()
    {
        delete[] base;
        base = NULL;
        res_num = 0;
        count = 0;
    }

    bool ReAllocate(uint new_res)
    {
        sort_entry_32 *new_base = new sort_entry_32[new_res];
        if (!new_base)
            return false;
        memcpy(new_base, base, count * sizeof(sort_entry_32));
        res_num = new_res;
        delete[] base;
        base = new_base;
        return true;
    }

    void Reserve(uint n)
    {
        if (n > res_num) {
            if (base)
                delete[] base;
            base = new sort_entry_32[n];
            if (base)
                res_num = n;
        }
    }

    bool Add(const T &t, uint sort_value)
    {
        if (count == res_num && !ReAllocate(res_num * 2))
            return false;
        base[count].sort_value = sort_value;
        base[count].data = (uint)t;
        count++;
        return true;
    }

    void Clear()
    {
        count = 0;
    }

    // The first num entries, all with 0
    void Sort(uint num = 0)
    {
        RadixSort32(base, num ? Min(count, num) : count);
    }

    T &operator[](int i)
    {
        return (T &)base[i].data;
    }
};

// The out-of-line copy of ReAllocate the linker kept, the particles' file's in the original
// TEMPLATE: LS3DF 0x10041770
// C_sort_list<int>::ReAllocate

// What the rendering gives the visuals' m_AddFGroups (PS2 name; the PC's members differ). Its constructor is out of line
// in render.cpp. Aligned to 16 bytes, as PS2's is (its cam_pos is a VU vector): the functions that inline Clear on it,
// render.cpp's Render and BuildAllVBuffers, get the Processor Pack's aligned frames, an ebp frame without ebx (there is
// nothing on the stack to align, so without the and esp). Aligned to 8 they compile the same.
struct __declspec(align(16)) ADD_FG_HELP_STRUCT {
    S_vector cam_pos;                   // 0x00, the camera's position
    uint unknown_0c;
    I3D_camera *camera;                 // 0x10, pCurrentCamera
    I3D_sector *sector;                 // 0x14, the visual's
    I3D_scene *scene;                   // 0x18
    C_list<RENDER_FG_HELP_STRUCT> fgroups; // 0x1c, the face groups to render
    C_sort_list<int> sort;              // 0x2c, the face groups' indices, which the rendering sorts and draws in order
    int num_opaque;                     // 0x38, counts the face groups queued: these come first in the sorted order
    int num_transparent;                // 0x3c, and these after them, drawn from the last

    // FUNCTION: LS3DF 0x100659a0
    ADD_FG_HELP_STRUCT() : fgroups(0x400)
    {
        unknown_0c = 0;
    }

    // Empties the queue (the name is a guess)
    void Clear()
    {
        num_opaque = 0;
        num_transparent = 0;
        fgroups.reset();
        sort.Clear();
    }
};

// SYNTHETIC: LS3DF 0x10061da0
// ADD_FG_HELP_STRUCT::~ADD_FG_HELP_STRUCT

// A particle visual queued for the particles' renderer, with its key (PC only, the names are guesses)
struct S_particle_item {
    I3D_visual *vis;
    uint sort_value;                    // 0x04, the particle's sort_group, and with 3 in its part_flags its sort_dist
};

// The particle visuals the rendering queues (VisualProc) for the particles' renderer, which sorts and renders them
// (PC only, the names are guesses). Its constructor is out of line in render.cpp.
struct PARTICLE_HLP_STRUCT {
    C_list<S_particle_item> visuals;    // 0x00
    C_sort_list<int> sort;              // 0x10, visuals' indices
    uint unknown_1c;                    // nothing references it

    // FUNCTION: LS3DF 0x100659d0
    PARTICLE_HLP_STRUCT() : visuals(0x2000) {}

    void Clear()
    {
        visuals.reset();
        sort.Clear();
    }
};

// SYNTHETIC: LS3DF 0x10061e00
// PARTICLE_HLP_STRUCT::~PARTICLE_HLP_STRUCT

// A vertex of a shadow's or a projector's receivers (0x18 bytes, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1; the name is
// a guess)
struct S_shadow_vertex {
    S_vector pos;                       // in world space
    uint color;                         // the shadow's or projector's color, faded with the depth
    float u, v;                         // in the texture
};

// What a shadow renders (PC only, 0x26c bytes; visual type SHD, the class name is a guess): the faces of the visuals
// the shadow falls on (its receivers), clipped to its volume, which it is placed and scaled to, with the shadow's
// texture. The game never sees it; it overrides the frame functions that would change it with empty ones.
// VTABLE: LS3DF 0x1009cbd8 I3D_frame
// VTABLE: LS3DF 0x1009cbc4 I3D_void_interface
class I3D_shadow_visual : public I3D_visual {
public:
    I3D_shadow_visual();
    ~I3D_shadow_visual();

    virtual void __stdcall SetCallback(I3D_CALLBACK *cb, uint flags) {}
    virtual void __stdcall SetOn(bool on) {}
    virtual LS3D_RESULT __stdcall SetName(const char *new_name)
    {
        return I3D_OK;
    }
    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame)
    {
        return I3D_OK;
    }
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);

    // PC only, the names are guesses but ProcessShadow's, which its messages name
    void AddFGroups();
    void ProcessShadow();

    uint unknown_1dc;                   // 0x1dc, nothing in LS3DF uses it (the objects' sort_dist is here)
    I3D_shadow *shadow;                 // 0x1e0
    S_shadow_vertex *verts;             // 0x1e4, in system memory, without I3DLOCK_RECEIVERS
    int verts_alloc;                    // 0x1e8
    int num_verts;                      // 0x1ec
    ushort *faces;                      // 0x1f0, three indices each, in system memory
    int faces_alloc;                    // 0x1f4
    int num_faces;                      // 0x1f8, the opaque materials'
    int num_faces_t;                    // 0x1fc, the transparent ones', after them
    COMPLETE_VB_STRUCT vb;              // 0x200, the vertices with I3DLOCK_RECEIVERS
    COMPLETE_IB_STRUCT ib;              // 0x20c, the faces
    RENDER_FG_HELP_STRUCT rfg;          // 0x218, what m_AddFGroups queues for the opaque faces
    RENDER_FG_HELP_STRUCT rfg_t;        // 0x238, for the transparent ones
    S_vector center_t;                  // 0x258, of the transparent faces, in world space
    int num_receivers;                  // 0x264
    int receivers_count;                // 0x268, the sum of the receivers' update counts and current lods
};

// SYNTHETIC: LS3DF 0x10052e70
// I3D_shadow_visual::`scalar deleting destructor'

// What SetMode takes (PS2 type; the values' names are guesses). The low two bits choose the blending, 0 adding the
// texture (D3DBLEND_ONE, ONE) and the others blending it by its alpha; the next ones how the projection fades with the
// depth in the projector's volume: not at all (0), from the projector out (4) or from the middle out (8). Fading, an
// added projection darkens its color, a blended one its alpha. 2, 3, 6 and 7 blend without a fade.
enum PROJECTOR_MODE {
    PROJECTOR_MODE_ADD,
    PROJECTOR_MODE_BLEND,
    PROJECTOR_MODE_ADD_FADE = 4,
    PROJECTOR_MODE_BLEND_FADE,
    PROJECTOR_MODE_ADD_FADE_MID = 8,
    PROJECTOR_MODE_BLEND_FADE_MID,
};

// MakeProjection's flags (the names are guesses)
#define PROJF_OMNI_SECTOR 0x2           // without a frame, the scene's omni sector too
#define PROJF_NO_MODELS 0x4             // not into models
#define PROJF_OWN_SECTOR 0x8            // without a frame, only the projector's sector and its own children
#define PROJF_NO_CHILDREN 0x10          // only the frame itself

// I3D_projector::proj_flags (the name is a guess)
#define PROJF_RECEIVERS_VALID 0x1       // the receivers' faces are kept (a lock, MakeProjection)

// A projector (PS2 I3D_projector, 0x264 bytes on PC; I3d_projector.cpp): a visual that projects its texture, or its
// material's, onto the visuals around it that take projectors (SHADOW_MODE 1 or 2 by SetProjectorMode), its receivers,
// within its volume, the unit box (orthogonal) or the frustum along its z axis. Their faces are clipped to the volume
// (clipper.h) and rendered with the texture, as a shadow's visual does (that code is a copy of this). Its virtual
// functions are PS2's in PS2's order. Names are PS2's where it has the member (orthogonal, texture, material, alpha,
// color, mode, the lock, the reverse factor), the others are guesses.
// VTABLE: LS3DF 0x1009c818 I3D_frame
// VTABLE: LS3DF 0x1009c808 I3D_void_interface
class I3D_projector : public I3D_visual {
public:
    I3D_projector();
    ~I3D_projector();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_frame *frame);
    virtual void UpdateV();
    virtual void LoadImage();
    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);
    // PS2 names
    virtual void __stdcall SetTexture(ITexture *tex);
    virtual void __stdcall SetMaterial(I3D_material *mat);
    virtual void __stdcall SetOrthogonal(bool b);
    virtual void __stdcall SetAlpha(float a);
    virtual void __stdcall SetReverseFactor(float f);
    virtual void __stdcall SetColor(const S_vector &c);
    virtual void __stdcall SetMode(PROJECTOR_MODE m);
    virtual void __stdcall SetProjectorLock(I3D_LOCK lock);
    // Collects the receivers under frame (or in the projector's sector or scene, PROJF_*) and clips their faces once,
    // for good: it locks the projector (I3DLOCK_RECEIVERS). PS2's is empty, defined in the class.
    virtual void __stdcall MakeProjection(uint flags, I3D_frame *frame);

    // The name is a guess: Duplicate copies the range through it (fld, fstp), as the value goes through a parameter
    void SetVisibleRange(float r)
    {
        visible_range = r;
    }

    // PC only, the names are guesses but ProcessRecList's, which its messages name. ProcessShadow is the shadows'
    // visual's name for its copy of the code.
    void AddFGroups();
    void ProcessRecList();
    void ProcessShadow();
    static void CollectReceivers(I3D_frame *frm);

    uint unknown_1dc;                   // 0x1dc, nothing in LS3DF uses it (the objects' sort_dist is here)
    bool orthogonal;                    // 0x1e0
    float visible_range;                // 0x1e4, scaled by FOVDistCorrection: farther from the camera it isn't
                                        // rendered (100 by default)
    float alpha;                        // 0x1e8, 1.0 opaque
    S_vector color;                     // 0x1ec, white by default
    I3D_LOCK projector_lock;            // 0x1f8, any but I3DLOCK_NONE keeps the receivers' faces in the buffers
    ITexture *texture;                  // 0x1fc, else the material's
    I3D_material *material;             // 0x200
    PROJECTOR_MODE mode;                // 0x204
    S_shadow_vertex *verts;             // 0x208, in system memory, without a lock
    int verts_alloc;                    // 0x20c
    int num_verts;                      // 0x210
    ushort *faces;                      // 0x214, three indices each, in system memory
    COMPLETE_VB_STRUCT vb;              // 0x218, the vertices with a lock
    COMPLETE_IB_STRUCT ib;              // 0x224, the faces
    int faces_alloc;                    // 0x230
    int num_faces;                      // 0x234, the opaque materials'
    int num_faces_t;                    // 0x238, the transparent ones', after them
    RENDER_FG_HELP_STRUCT *rfg;         // 0x23c, two, what m_AddFGroups queues for the opaque and the transparent faces
    S_vector center_t;                  // 0x240, of the transparent faces, in world space
    int receivers_update;               // 0x24c, the update count the receivers were collected at, -1
    int num_receivers;                  // 0x250
    int receivers_count;                // 0x254, the sum of the receivers' update counts and current lods
    I3D_sector *sector;                 // 0x258, the one it is in, until it moves
    uint proj_flags;                    // 0x25c, PROJF_RECEIVERS_VALID
    float reverse_factor;               // 0x260
};

// SYNTHETIC: LS3DF 0x10045fe0
// I3D_projector::`scalar deleting destructor'

// The destructor the projector file's thunk registers for ProjectorsToRender, the second $E13 in link order (see the
// scene's)
// SYNTHETIC: LS3DF 0x10046760
// $E13

// The shadows' visuals whose receivers the rendering has to clip this frame and what it calls for them (I3d_shadow.cpp),
// the same for the projectors, and the visuals a projector or shadow falls on (I3d_projector.cpp; the names are guesses)
extern C_list<I3D_shadow_visual *> ShadowVisualQueue;
void ProcessShadows();
void ProcessProjectors();
extern I3D_visual *ShadowReceivers[512];

// render.cpp
extern I3D_scene *pCurrentScene;
// The distances' scale for the current camera's field of view and its square (PS2 names). Declared before
// AddFGHlpStruct, which hashes to the same .bss bucket: the global declared last comes first.
extern float FOVDistCorrection;
extern float FOVDistCorrection2;
extern ADD_FG_HELP_STRUCT AddFGHlpStruct;
extern PARTICLE_HLP_STRUCT ParticleElementsList;
extern bool renderCameraChanged;
extern float RSLodQuality;
int GetObjectLOD(I3D_object *obj, const float &dist_2);

// What the rendering calls in other files (PS2 names but the particles' visual's, a guess; the particles' functions
// aren't PS2's): the scene's portals and occluders, which decide the sectors' visuals to render, and the particles'
// renderer, a visual of the particles' file that renders ParticleElementsList
void ProcessPortals(I3D_scene *scene);
void ProcessOccluders(I3D_scene *scene);
LS3D_RESULT InitParticle();
void CloseParticle();
extern I3D_visual *pParticleVisual;

// The particles' renderer's (I3d_particle.cpp, which declares the class)
// SYNTHETIC: LS3DF 0x1003cc80
// I3D_part_renderer::`scalar deleting destructor'

// I3d_occluder.cpp (PS2 names), for the portals' processing: the sectors' lists of occluders, made again where the
// scene's ring changed, and the volumes of a sector's occluders, which hide the portals behind them
void OCCL_UpdateOcclLists(I3D_scene *scene);
void OCCL_PrepareSector2(I3D_sector *sector);

// I3d_driver.cpp: counts the frames rendered (a frame's render_number is its value when the frame was last rendered),
// and its previous value
extern uint RenderNumber;
extern uint RenderNumberMinus1;

// Load.cpp (PS2 names): the loaders of the Open functions, a hierarchy's (.4ds) into a model or a scene's primary
// sector, an animation set's (.5ds) and a shadow's (.6ds); the progress the 4ds loader reports; and the frames and
// materials loaded so far, by their 1-based index in the file, and the number of animations the file says it has
LS3D_RESULT LoadHierarchy(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context, I3D_frame *root);
LS3D_RESULT LoadAnimation(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context,
    I3D_animation_set *set);
LS3D_RESULT LoadShadow(const char *file_name, uint flags, I3D_LOAD_CB_PROC *cb, void *context, ShadowDef *def);
void LoadProgress(float p);
extern I3D_frame *LoadFramy[10000];
extern int LoadFramu;
extern I3D_material *LoadMaterialy[5000];
extern int LoadMaterialu;
extern int LoadAnimCount;

// load4ds.cpp (PS2 names but LoadShadowImage, its message's): the file being loaded, its error, and the targets loaded,
// whose lists hold their frames' indices until the frames are loaded
extern LS3D_RESULT ImageErrFlg;
extern int LoadImageHandle;
extern const char *ImageFileName;
extern C_list<I3D_target *> LoadTargetList;
// The destructor load4ds.cpp's thunk registers for it: the second $E6 in link order, after the mirror file's (the file
// includes I3D/mesh/I3d_material.h, whose <vector> takes a number)
// SYNTHETIC: LS3DF 0x10060c90
// $E6
LS3D_RESULT LoadImage(const char *file_name, const FILETIME *time);
I3D_mesh_object *LoadMeshImage();
LS3D_RESULT LoadShadowImage(ShadowDef *def, const char *file_name, const FILETIME *time);
// Out of line for LoadShadowImage's vertices, the file's last function; the faces' and parts' are inlined
// TEMPLATE: LS3DF 0x10061790
// ?reserve@?$C_list@US_vector@@@@QAEXH@Z

// Load5ds.cpp (PS2 name): an animation file's data
LS3D_RESULT LoadAnimation5ds(const char *file_name, void **data, const FILETIME *time);
