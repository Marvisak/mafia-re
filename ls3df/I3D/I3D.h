#pragma once

#include "ls3df.h"
#include "I3D/I3d_math.h"

#include <d3d8.h>

// The 3D engine (PS2 I3d_*.cpp). The driver, the frame and the dummy are written (I3d_driver.cpp, I3d_frame.cpp,
// I3d_dummy.cpp); the other frames, the visuals, materials and meshes aren't recovered yet. Their placeholders keep
// the members and virtual functions the written files use at their offsets and vtable slots, the sizes the driver
// allocates, and declare the constructors it calls (stubs, see I3d_driver.cpp). They are __declspec(novtable) so that
// the stubs don't need the virtual functions, none of which is written yet; the real classes will drop it.

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
    RS_HWVERTEXPROC = 19,               // refused where the hardware lacks it (m_ProblemFlags 0x80)
    RS_FLOAT_LOD = 20,                  // a float, 1.0 by default
    RS_SHADOW_SW = 48,                  // shadow textures are filtered in software
    RS_FAST_FIND = 49,                  // named frames go into the driver's fast find tree (the name is a guess)
    RS_LOD_SCALE = 50,                  // in percent
    RS_NONORTHO_TESSELLATION = 53,      // RsNonOrthoTessellation, inverted
    RS_ZBIAS_PROJECTOR = 54,            // RsZBiasProjector
    RS_ZBIAS_SHADOW = 55,               // RsZBiasShadow
    RS_FREE_VRAM_VB = 62,               // 666 frees the vertex buffers in video memory
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
#define FRMFLAGS_TICK 0x400000          // in its scene's tick list
#define FRMFLAGS_FFTREE 0x20000000      // in the driver's fast find tree
#define FRMFLAGS_UPDATE 0x40000000      // to update
#define FRMFLAGS_WEATHER 0x80000000     // a dummy in a scene's weather system (I3D_scene::WeatherSystemAddDummy)

// What a frame's callback is told (PS2 type; the values aren't known yet)
enum LS3D_CALLBACK_MESSAGE {
    LS3D_CALLBACK_MESSAGE_0,
};

// A frame's callback (PS2's parameters; the calling convention isn't confirmed)
typedef uint __stdcall I3D_CALLBACK(I3D_frame *frame, LS3D_CALLBACK_MESSAGE message, uint param1, uint param2);

// A bounding box and the sphere around it (PS2 I3D_bvolume)
struct I3D_bvolume {
    I3D_bbox bbox;
    I3D_bsphere bsphere;
};

// The base of the scene graph's nodes (PS2 I3D_frame, 0x140 bytes on PC, vtable 0x1009b9b8). The game reaches its
// members through the class's inline functions. Member names are Insanity3D's where the roles agree (ref, frm_flags,
// matrix, f_matrix, name, parent, type), the others are guesses.
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
    int unknown_13c;                    // 0x13c
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

    virtual void __stdcall vfunc_20();
    // PS2 name, 0x100597f0
    virtual bool __stdcall IsPlaying(bool unknown) const;

    uint sound_flags;                   // 0x140
    uchar unknown_144[0xa8];
};

class __declspec(novtable) I3D_sector : public I3D_frame {
public:
    I3D_sector();

    // PS2 name
    void AddToRemoveFromSceneRing(I3D_scene *scene);

    uchar unknown_140[0xd4];
    I3D_scene *scene;                   // 0x214
    uchar unknown_218[0x1c];
};

// The scene (PS2 I3D_scene, 0x33c bytes on PC). The virtual functions after I3D_frame's are placeholders up to the
// ones the driver and the frames call.
class __declspec(novtable) I3D_scene : public I3D_frame {
public:
    I3D_scene();

    virtual void __stdcall vfunc_20(); virtual void __stdcall vfunc_21(); virtual void __stdcall vfunc_22();
    virtual void __stdcall vfunc_23();
    // PS2 name
    virtual void __stdcall DeleteFrame(I3D_frame *frame);
    virtual void __stdcall vfunc_25(); virtual void __stdcall vfunc_26();
    virtual void __stdcall vfunc_27(); virtual void __stdcall vfunc_28(); virtual void __stdcall vfunc_29();
    virtual void __stdcall vfunc_30(); virtual void __stdcall vfunc_31(); virtual void __stdcall vfunc_32();
    virtual void __stdcall vfunc_33(); virtual void __stdcall vfunc_34(); virtual void __stdcall vfunc_35();
    virtual void __stdcall vfunc_36(); virtual void __stdcall vfunc_37(); virtual void __stdcall vfunc_38();
    virtual void __stdcall vfunc_39(); virtual void __stdcall vfunc_40(); virtual void __stdcall vfunc_41();
    // PS2 name: removes the frame from the frames the scene ticks, and tells whether to look further
    virtual int __stdcall DeleteTickFrame(I3D_frame *frame);
    virtual void __stdcall vfunc_43(); virtual void __stdcall vfunc_44(); virtual void __stdcall vfunc_45();
    virtual void __stdcall vfunc_46(); virtual void __stdcall vfunc_47(); virtual void __stdcall vfunc_48();
    virtual void __stdcall vfunc_49(); virtual void __stdcall vfunc_50(); virtual void __stdcall vfunc_51();
    virtual void __stdcall vfunc_52(); virtual void __stdcall vfunc_53(); virtual void __stdcall vfunc_54();
    virtual void __stdcall vfunc_55(); virtual void __stdcall vfunc_56(); virtual void __stdcall vfunc_57();
    virtual void __stdcall vfunc_58(); virtual void __stdcall vfunc_59(); virtual void __stdcall vfunc_60();
    virtual void __stdcall vfunc_61();
    // PS2 name: takes the dummy out of the scene's weather system
    virtual void __stdcall WeatherSystemDelDummy(I3D_dummy *dummy);
    virtual void __stdcall vfunc_63(); virtual void __stdcall vfunc_64();
    // PS2 name
    virtual void __stdcall SetViewport(uint x, uint y, uint sx, uint sy);

    uchar unknown_140[0x144];
    ISND_3Dlistener *listener;          // 0x284
    uchar unknown_288[0xb4];
};

// I3D_dummy::dummy_flags (names guessed)
#define DUMMYF_NORMALIZED 0x1           // Normalize made the box (0, 0, 0) to (1, 1, 1)
#define DUMMYF_NORMALIZED_CENTER 0x2    // Normalize made the box (-1, -1, -1) to (1, 1, 1)
#define DUMMYF_UNKNOWN_10 0x10          // PC only: cleared whenever the frame moves (UpdateV) or is normalized

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

    I3D_bbox bbox;                      // 0x140, in local space
    uchar unknown_158[0x40];            // PC only, nothing in LS3DF reads it
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

I3D_PLACEHOLDER_FRAME(I3D_light, 0x1f0)
I3D_PLACEHOLDER_FRAME(I3D_camera, 0x4f4)
I3D_PLACEHOLDER_FRAME(I3D_user, 0x144)
I3D_PLACEHOLDER_FRAME(I3D_joint, 0x20c)
I3D_PLACEHOLDER_FRAME(I3D_volume, 0x174)
I3D_PLACEHOLDER_FRAME(I3D_landscape, 0x62144)
I3D_PLACEHOLDER_FRAME(I3D_emitor, 0x1f8)

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

    uchar unknown_140[0xd4];
};

// The visuals, a kind of frame (PS2 I3D_visual), by the sizes the driver allocates. The particles' and the landscape
// patch's names are guesses.
class __declspec(novtable) I3D_visual : public I3D_frame {
public:
    virtual void __stdcall vfunc_20(); virtual void __stdcall vfunc_21(); virtual void __stdcall vfunc_22();
    virtual void __stdcall vfunc_23(); virtual void __stdcall vfunc_24();
    // PS2 name: computes bbox
    virtual void __stdcall UpdateBoundProc();

    uchar unknown_140[0x6c];
    I3D_bbox bbox;                      // 0x1ac, in local space
    uchar unknown_1c4[0x14];
    uint vis_flags;                     // 0x1d8
};

#define I3D_PLACEHOLDER_VISUAL(name, size) \
    class __declspec(novtable) name : public I3D_visual { \
    public: \
        name(); \
        uchar unknown_1dc[size - 0x1dc]; \
    };

I3D_PLACEHOLDER_VISUAL(I3D_object, 0x21c)
I3D_PLACEHOLDER_VISUAL(I3D_lit_object, 0x370)
I3D_PLACEHOLDER_VISUAL(I3D_billboard, 0x268)
I3D_PLACEHOLDER_VISUAL(I3D_singlemesh, 0x22c)
I3D_PLACEHOLDER_VISUAL(I3D_lensflare, 0x27c)
I3D_PLACEHOLDER_VISUAL(I3D_morph, 0x238)
I3D_PLACEHOLDER_VISUAL(I3D_singlemorph, 0x248)
I3D_PLACEHOLDER_VISUAL(I3D_projector, 0x264)
I3D_PLACEHOLDER_VISUAL(I3D_part_base, 0x284)
I3D_PLACEHOLDER_VISUAL(I3D_part_element, 0x310)
I3D_PLACEHOLDER_VISUAL(I3D_landpatch, 0x620)
I3D_PLACEHOLDER_VISUAL(I3D_mirror, 0x2ec)

// I3D_material::flags
#define MATF_ANIMATED 0x4000000         // UpdateTexture picks the frame
#define MATF_ICON 0x40000000            // made by I3D_driver::GetIcon

// A material (PS2 I3D_material, 0x40 bytes on PC), reference counted
class __declspec(novtable) I3D_material {
public:
    I3D_material();

    virtual int __stdcall Release();
    virtual void __stdcall vfunc_01();
    virtual void __stdcall vfunc_02();
    virtual void __stdcall vfunc_03();
    virtual void __stdcall vfunc_04();
    virtual void __stdcall SetTexture(ITexture *texture);
    virtual void __stdcall vfunc_06();
    virtual void __stdcall vfunc_07();
    // PS2 name: an animated material's frame
    virtual void __stdcall UpdateTexture();

    int ref;                            // 0x04
    uint flags;                         // 0x08, MATF_*
    ITexture *texture;                  // 0x0c
    ITexture *env_texture;              // 0x10
    S_vector ambient;                   // 0x14
    S_vector diffuse;                   // 0x20
    S_vector emissive;                  // 0x2c
    uchar unknown_38[8];

    ITexture *GetTexture()
    {
        if (flags & MATF_ANIMATED)
            UpdateTexture();
        return texture;
    }
};

// An animated material (PS2 I3D_anim_material)
class __declspec(novtable) I3D_anim_material : public I3D_material {
public:
    I3D_anim_material();

    uchar unknown_40[0x48];
};

// The mesh parts the driver makes (PS2 names)
#define I3D_PLACEHOLDER(name, size) \
    class name { \
    public: \
        name(); \
        uchar unknown_00[size]; \
    };

I3D_PLACEHOLDER(I3D_face_group, 0x1c)
I3D_PLACEHOLDER(I3D_mesh_level, 0x44)
I3D_PLACEHOLDER(I3D_mesh_object, 0x6c)
I3D_PLACEHOLDER(I3D_animation_set, 0x14)
I3D_PLACEHOLDER(I3D_pose, 0x38)
I3D_PLACEHOLDER(I3D_pose_set, 0x18)

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

// Stubs in I3d_driver.cpp and I3d_frame.cpp, after their files' functions though their addresses come before them, so
// their markers are here, by name

// STUB: LS3DF 0x10001370
// FreeClipArrays

// STUB: LS3DF 0x10004890
// I3D_animation_set::I3D_animation_set

// STUB: LS3DF 0x100051a0
// I3D_area::I3D_area

// STUB: LS3DF 0x10005440
// I3D_area::AddToRemoveFromSceneRing

// STUB: LS3DF 0x10005db0
// I3D_billboard::I3D_billboard

// STUB: LS3DF 0x10009520
// I3D_lensflare::I3D_lensflare

// STUB: LS3DF 0x1000b0c0
// I3D_camera::I3D_camera

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

// The draws' lists' reserve, out of line where VC6 didn't inline it, after I3d_driver.cpp's functions
// TEMPLATE: LS3DF 0x1001a100
// C_list<TLVERTEX>::reserve
// TEMPLATE: LS3DF 0x1001a170
// C_list<LVERTEX>::reserve

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

    // PC only (names guessed): a material whose texture is one of LS3DF's bitmaps, and the shadow textures'
    // rasterizer, which fills a texture with the shadow's triangles, projected onto the box (x, y, -, x2, y2)
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

extern uint m_AlphaRef;

// Functions that set a render state where it changed (names guessed). PS2 inlines them all. On PC the three below
// are inline: VC6 inlines them where the inline budget allows it, and the copies the linker kept are in the billboard
// file's code. SetBlendState and SetVertexFormat are that file's functions: the draws call them, or have their code
// written out. They are written in I3d_driver.cpp, after the driver's functions, so their markers are here, by name.
// FUNCTION: LS3DF 0x1000ab40
// SetBlendState
void SetBlendState(uint state);

// FUNCTION: LS3DF 0x1000ae50
// SetVertexFormat
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

// A scale the I3D code applies to lighting, 128 or 255 without MODULATE2X (another I3D file's; the name is a guess)
extern float i3d_light_scale;
