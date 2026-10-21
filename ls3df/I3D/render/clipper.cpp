// The clipper's arrays (PC only; clipper.cpp is the name the messages give), at 0x10001000-0x100013e0 in the original:
// the parameters of a projector's or shadow's clipping, the arrays and their memory. clipper.h describes the clipper.
// The header's zero vector has its $E at the end (0x100013c0), clip_center_t's its own at the start, and
// ReserveClipArrays' new[] of the clip vertices emits the vector constructor iterator after it, the copy the linker
// keeps. The file creates the precompiled header clipper2.cpp uses (/Yc, see ls3df/CMakeLists.txt), which gives its
// object what /YX does.
// The file includes a C++ standard header: the original's first object asks for libcpmt before LIBCMT (see clipper.h).
// Shapes the original needs: SetClipParams copies its parameters before it computes the ratios.
#include <new>

#include "I3D/render/clipper.h"

#include <string.h>

// Constructed, so its $E comes first; the order of the file's .bss is clipper.h's
// GLOBAL: LS3DF 0x100aef58
S_vector clip_center_t;

// GLOBAL: LS3DF 0x100aef30
bool clip_backfaces;

// GLOBAL: LS3DF 0x100aef34
int num_clip_faces;

// GLOBAL: LS3DF 0x100aef38
int clip_verts_max;

// GLOBAL: LS3DF 0x100aef3c
bool clip_is_ortho;

// GLOBAL: LS3DF 0x100aef40
int clip_added_transp;

// GLOBAL: LS3DF 0x100aef44
S_clip_vertex *clip_vert_end;

// GLOBAL: LS3DF 0x100aef48
int clip_faces_alloc;

// GLOBAL: LS3DF 0x100aef4c
int clip_fg_blend;

// GLOBAL: LS3DF 0x100aef50
float clip_spread;

// GLOBAL: LS3DF 0x100aef54
I3D_triface *clip_next_face;

// GLOBAL: LS3DF 0x100aef64
S_vector *clip_results;

// GLOBAL: LS3DF 0x100aef68
uint clip_with_alpha;

// GLOBAL: LS3DF 0x100aef78
float clip_inv_reverse;

// GLOBAL: LS3DF 0x100aef7c
float clip_rev_factor;

// GLOBAL: LS3DF 0x100aef80
const S_matrix *clip_world;

// GLOBAL: LS3DF 0x100aef88
ushort clip_poly_buf2[512];

// GLOBAL: LS3DF 0x100af388
I3D_triface *clip_faces;

// GLOBAL: LS3DF 0x100af38c
int clip_result_alloc;

// GLOBAL: LS3DF 0x100af390
int clip_poly_num;

// GLOBAL: LS3DF 0x100af394
float clip_lift;

// GLOBAL: LS3DF 0x100af398
int clip_vertices_size;

// GLOBAL: LS3DF 0x100af39c
float clip_ratio_y;

// GLOBAL: LS3DF 0x100af3a0
float clip_ratio_x;

// GLOBAL: LS3DF 0x100af3a4
const S_matrix *clip_matrix = NULL;

// GLOBAL: LS3DF 0x100af3a8
S_clip_vertex *clip_verts = NULL;

// GLOBAL: LS3DF 0x100af3ac
I3D_triface *clip_faces_t = NULL;

// GLOBAL: LS3DF 0x100af3b0
I3D_triface *clip_next_face_t = NULL;

// GLOBAL: LS3DF 0x100af3b4
int num_clip_faces_t = 0;

// GLOBAL: LS3DF 0x100af3b8
int clip_added_faces = 0;

// GLOBAL: LS3DF 0x100af3bc
int num_clip_results = 0;

// GLOBAL: LS3DF 0x100af3c0
int clip_two_sided = 0;

// GLOBAL: LS3DF 0x100af3c4
int num_clip_results_t = 0;

// The parameters of the next clipping: m takes world space into clip space, world is the projector's matrix
// FUNCTION: LS3DF 0x10001010
void SetClipParams(const S_matrix &m, const S_matrix &world, bool ortho, float offset, float expand, float reverse)
{
    clip_matrix = &m;
    clip_world = &world;
    clip_is_ortho = ortho;
    S_vector scale = world.GetScale();
    clip_lift = offset;
    clip_spread = expand;
    clip_rev_factor = reverse;
    clip_ratio_x = scale.z / scale.x;
    clip_ratio_y = scale.z / scale.y;
    if (reverse < MRG_ZERO)
        clip_inv_reverse = 1e8f;
    else
        clip_inv_reverse = 1.0f / reverse;
    clip_backfaces = reverse > 0.999f;
}

// Empties the result before the meshes are clipped
// FUNCTION: LS3DF 0x100010c0
void ResetClipArrays()
{
    num_clip_results = 0;
    num_clip_faces = 0;
    num_clip_faces_t = 0;
    clip_center_t.Zero();
    num_clip_results_t = 0;
}

// Room for a mesh of vertNum vertices and faceNum faces: the vertices aren't kept, the faces are
// FUNCTION: LS3DF 0x100010f0
bool ReserveClipArrays(int vertNum, int faceNum)
{
    if (clip_vertices_size < 65000) {
        int num = vertNum + faceNum * 12;
        if (!clip_is_ortho && RsNonOrthoTessellation < 1.0f)
            num = (int)((1.0f / RsNonOrthoTessellation + 1.0f) * num);
        if (num > 65000)
            num = 65000;
        if (clip_vertices_size < num) {
            if (num < 512)
                num = 512;
            clip_vertices_size = num;
            delete[] clip_verts;
            clip_verts = new S_clip_vertex[num];
            if (!clip_verts) {
                clip_vertices_size = 0;
                dbgPrintf("!! Out of memory  ReserveClipArrays()  vertNum:%d  faceNum:%d  clipper.cpp", vertNum,
                    faceNum);
                return false;
            }
            if (!clip_is_ortho && RsNonOrthoTessellation < 1.0f)
                clip_verts_max = (int)(clip_vertices_size - (1.0f / RsNonOrthoTessellation + 1.0f) * 15.0f);
            else
                clip_verts_max = clip_vertices_size - 15;
        }
    }
    if (clip_faces_alloc < 32000) {
        int num = faceNum * 8;
        if (!clip_is_ortho && RsNonOrthoTessellation < 1.0f)
            num = (int)((1.0f / RsNonOrthoTessellation + 1.0f) * num);
        num += num_clip_faces;
        if (num > 32000)
            num = 32000;
        if (clip_faces_alloc < num) {
            if (num < 128)
                num = 128;
            I3D_triface *faces = new I3D_triface[num * 2];
            if (!faces) {
                dbgPrintf("!! Out of memory  ReserveClipArrays()  vertNum:%d  faceNum:%d  clipper.cpp", vertNum,
                    faceNum);
                return false;
            }
            if (num_clip_faces)
                memcpy(faces, clip_faces, num_clip_faces * sizeof(I3D_triface));
            if (num_clip_faces_t)
                memcpy(faces + num, clip_faces_t, num_clip_faces_t * sizeof(I3D_triface));
            delete[] clip_faces;
            clip_faces_alloc = num;
            clip_faces = faces;
            clip_faces_t = faces + num;
        }
    }
    return true;
}

// The face block isn't freed, in the original too
// FUNCTION: LS3DF 0x10001370
void FreeClipArrays()
{
    delete[] clip_verts;
    clip_verts = NULL;
    clip_vertices_size = 0;
    clip_faces = NULL;
    num_clip_faces = 0;
    delete[] clip_results;
    clip_results = NULL;
    num_clip_results = 0;
    clip_result_alloc = 0;
    clip_faces_alloc = 0;
}
