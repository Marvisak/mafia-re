#pragma once

// The clipper (PC only): the projectors and the shadows clip the faces of the meshes they fall on to their volume
// (clipper.cpp, clipping.cpp). A mesh's vertices are transformed into the volume's clip space, where it is the unit box
// (an orthogonal projector) or the frustum |x| <= z, |y| <= z, 0 <= z <= 1, and every face that isn't outside is clipped
// to it. The faces collect in two lists, the opaque materials' and the transparent ones', and the vertices they use
// in the result vertices, which the projector or shadow then renders. Names are guesses, but ReserveClipArrays and
// ClipFacegroups, which the messages name.
// clipper.cpp includes only the math header (I3d_math.h), the others I3D.h too: the clipper's object is the first one
// the linker reads, and its default library requests start the original's list (libcpmt, LIBCMT, OLDNAMES), which
// I3D.h's d3d8.h would put uuid ahead of.
#include "I3D/I3d_math.h"

// A vertex of the clipper (0x14 bytes): a mesh vertex in clip space, or one the clipping made
struct S_clip_vertex {
    S_vector pos;                       // 0x00
    int index;                          // 0x0c, its result vertex, -1 until a face uses it
    uint clip;                          // 0x10, CLIP_*: the planes it lies beyond
};

// S_clip_vertex::clip
#define CLIP_LEFT 0x1                   // x < -1, in the frustum x < -z
#define CLIP_RIGHT 0x2                  // x > 1, in the frustum x > z
#define CLIP_BOTTOM 0x4                 // y < -1, in the frustum y < -z
#define CLIP_TOP 0x8                    // y > 1, in the frustum y > z
#define CLIP_NEAR 0x10                  // z < 0
#define CLIP_FAR 0x20                   // z > 1

// The clipper's arrays (ReserveClipArrays, ClipFacegroups; the names are guesses): 512 to 65000 vertices, and 128 to
// 32000 faces in each of the two lists
#define CLIP_MIN_VERTICES 512
#define CLIP_MAX_VERTICES 65000
#define CLIP_MIN_FACES 128
#define CLIP_MAX_FACES 32000

// A single mesh's or single morph's vertex (I3D.h)
struct S_clip_sm_vertex;

// The driver's (I3d_driver.cpp): the frustum's edges are split where they span more depth than this
extern float RsNonOrthoTessellation;

// clipper.cpp, in the order of the original's .bss. No name is known: these are chosen so that VC6's hash (see
// tools/bsshash.py) gives that order around the header's zero vector (bucket 275), and the last nine are
// zero-initialized, which puts them after the hashed ones in this order.
extern bool clip_backfaces;             // clip_rev_factor is about 1: no face is culled
extern int num_clip_faces;              // the opaque materials' faces
extern int clip_verts_max;              // no more faces are clipped once this many vertices are used
extern bool clip_is_ortho;              // the volume is the unit box, not the frustum
extern int clip_added_transp;           // the transparent faces ClipFacegroups adds
extern S_clip_vertex *clip_vert_end;    // the next free vertex
extern int clip_faces_alloc;            // of each face list
extern int clip_fg_blend;               // the face group's material is transparent
extern float clip_spread;               // how far the clipping's new vertices move away from their polygon's center,
                                        // if above 0
extern I3D_triface *clip_next_face;     // the next free opaque face
extern S_vector clip_center_t;          // the sum of the transparent faces' new result vertices
extern S_vector *clip_results;          // the result vertices, in clip space
extern uint clip_with_alpha;            // the transparent face groups are clipped too
extern float clip_inv_reverse;          // 1 / clip_rev_factor
extern float clip_rev_factor;           // the projector's reverse factor (PS2's SetReverseFactor): which faces turned
                                        // away still get the projection
extern const S_matrix *clip_world;      // the projector's matrix, whose scale gives the ratios
extern ushort clip_poly_buf2[512];      // the polygon, every other plane
extern I3D_triface *clip_faces;         // the opaque materials' faces, then the transparent ones' in the same block
extern int clip_result_alloc;
extern int clip_poly_num;               // the polygon's vertices while it is clipped
extern float clip_lift;                 // how far the clipping's new vertices move along their face's normal
extern int clip_vertices_size;
extern float clip_ratio_y;              // the scale of the projector's matrix, z / y
extern float clip_ratio_x;              // z / x
extern const S_matrix *clip_matrix;     // from world space into clip space
extern S_clip_vertex *clip_verts;
extern I3D_triface *clip_faces_t;       // the transparent materials' faces
extern I3D_triface *clip_next_face_t;
extern int num_clip_faces_t;
extern int clip_added_faces;            // the opaque faces ClipFacegroups adds
extern int num_clip_results;
extern int clip_two_sided;              // the face group's faces aren't culled
extern int num_clip_results_t;          // what the projector divides clip_center_t by; nothing counts it

// The initializer of clip_center_t, empty (S_vector's constructor is): the first $E5 in link order. reccmp pairs the
// annotations of one name with our functions of that name in address order, so it is annotated, before I3D.h's.
// SYNTHETIC: LS3DF 0x10001000
// SYNTHETIC: LS3DF11 0x10001000
// $E5

void SetClipParams(const S_matrix &m, const S_matrix &world, bool ortho, float offset, float expand, float reverse);
void ResetClipArrays();
bool ReserveClipArrays(int vertNum, int faceNum);
void FreeClipArrays();

// clipper2.cpp
extern ushort clip_poly_buf1[512];      // the polygon, every other plane
extern int clip_vert_count;             // the mesh's vertices and the clipping's

// The faces into clip_next_face or clip_next_face_t (clip_fg_blend), their new vertices from clip_vert_end on
void ClipFacesOrtho(const I3D_triface *faces, int num_faces);
bool ClipFacesPersp(const I3D_triface *faces, int num_faces);

// clipping.cpp
class I3D_face_group;
struct I3D_vertex;

bool ClipMesh(const I3D_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces, uint transp,
    const S_matrix &m);
bool ClipMeshW(const I3D_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces, uint transp);
bool ClipSingleMesh(const S_clip_sm_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces, uint transp,
    const S_matrix &m);
bool ClipSingleMeshW(const S_clip_sm_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces,
    uint transp);
