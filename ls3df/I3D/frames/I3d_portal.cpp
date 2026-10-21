// The portals (PS2 I3d_portal.cpp), at 0x10041800-0x10045930 in the original: I3D_portal in PS2's order from its
// constructor, with the PC's SetOn and IsOn after AddToSceneRing, then the walk through the portals: CTP_VFnormals'
// empty $E, CTP_EnumProcFirst and CTP_EnumProc, the header's zero vector's $E (0x100437d0, where ours comes too),
// CullThruPortal, ParsePortal, ParsePortalsInSector, UpdatePortalInterconnection, ProcessPortals and LoadImage. Names
// are PS2's unless noted.
// - PS2's Duplicate isn't on PC. CullThruPortal_First, ParsePortalsInSector_Begin and ReportBadPortals have no copy of
//   their own: VC6 inlines them everywhere.
// - The enum procs' tests of a frame's sphere against the side planes, and of its box against those planes it crosses,
//   are inline assembly: the bits pass through memory at each block's ends (stored, then loaded again into the register
//   the next statement tests), the blocks pop the x87 stack with fcompp, both functions have an ebp frame, and the Xbox
//   build has the blocks byte for byte (with a push and pop of ebx around the first).
//
// Not matching yet:
// - UpdatePortal and the occluders' part of CTP_EnumProcFirst: S_vector::Normalize, which the original inlines inside
//   S_plane's constructor (I3d_math.h's open question; with Normalize __forceinline UpdatePortal is exact).
// - CTP_EnumProcFirst's frame (0x114 bytes, ours smaller): the original's slots are known, see the function.
// - CullThruPortal: the corners of the near plane, see the function.
// - ParsePortal: the vertex count and the search loops' counter have swapped registers.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <math.h>

// FUNCTION: LS3DF 0x10041800
I3D_portal::I3D_portal()
{
    num_verts = 0;
    sector = NULL;
    connect_sector = NULL;
    parse_flags = 0;
    scene = NULL;
    prev_in_scene = NULL;
    next_in_scene = NULL;
    twin = NULL;
    portal_flags = PORTALF_ON;
    range_2 = 0.0f;
    portalVersion++;
}

// It leaves the scene's ring through AddToSceneRing with its own scene, which does nothing: PS2's does the same
// FUNCTION: LS3DF 0x10041850
I3D_portal::~I3D_portal()
{
    portalVersion++;
    AddToSceneRing(scene);
    if (twin) {
        twin->twin = NULL;
        twin = NULL;
    }
}

// Moves the portal from its scene's ring of portals into the other scene's
// FUNCTION: LS3DF 0x10041890
void I3D_portal::AddToSceneRing(I3D_scene *new_scene)
{
    if (scene == new_scene)
        return;
    if (scene) {
        scene->num_portals--;
        if (scene->portals == this)
            scene->portals = next_in_scene;
        if (prev_in_scene == this)
            scene->portals = NULL;
        else {
            prev_in_scene->next_in_scene = next_in_scene;
            next_in_scene->prev_in_scene = prev_in_scene;
        }
    }
    scene = new_scene;
    if (!scene)
        return;
    if (!scene->portals) {
        prev_in_scene = this;
        next_in_scene = this;
        scene->portals = this;
    } else {
        scene->portals->prev_in_scene->next_in_scene = this;
        prev_in_scene = scene->portals->prev_in_scene;
        next_in_scene = scene->portals;
        scene->portals->prev_in_scene = this;
    }
    scene->num_portals++;
}

// FUNCTION: LS3DF 0x10041980
void I3D_portal::SetOn(bool on)
{
    if (on)
        portal_flags |= PORTALF_ON;
    else
        portal_flags &= ~PORTALF_ON;
    if (twin) {
        if (on)
            twin->portal_flags |= PORTALF_ON;
        else
            twin->portal_flags &= ~PORTALF_ON;
    }
}

// FUNCTION: LS3DF 0x100419d0
bool I3D_portal::IsOn() const
{
    return portal_flags & PORTALF_ON ? true : false;
}

// The vertices in world space, the box's center and the sphere around it, and the plane (its normal out of the sector),
// through S_plane's three-point constructor: the original's normal and d are one temporary, copied into the member.
// Open: the original inlines Normalize there, ours calls it (see the file's comment).
// FUNCTION: LS3DF 0x100419e0
void I3D_portal::UpdatePortal()
{
    S_vector bmin(1e6f, 1e6f, 1e6f);
    S_vector bmax(-1e6f, -1e6f, -1e6f);
    I3D_sector *sct = sector;
    int i = num_verts;
    const S_vector *v = (const S_vector *)verts;
    S_vector *vw = (S_vector *)verts_w;
    const S_matrix &m = sct->GetMatrix();
    while (i--) {
        *vw = *v++ * m;
        if (bmin.x > vw->x)
            bmin.x = vw->x;
        if (bmin.y > vw->y)
            bmin.y = vw->y;
        if (bmin.z > vw->z)
            bmin.z = vw->z;
        if (bmax.x < vw->x)
            bmax.x = vw->x;
        if (bmax.y < vw->y)
            bmax.y = vw->y;
        if (bmax.z < vw->z)
            bmax.z = vw->z;
        vw++;
    }
    center = (bmax + bmin) * .5f;
    radius = (bmax - bmin).Magnitude() / 2.0f;
    plane = S_plane((const S_vector &)verts_w[num_verts - 1], (const S_vector &)verts_w[0],
        (const S_vector &)verts_w[1]);
    if (sector)
        AddToSceneRing(sector->scene);
}

// The walk's state (PS2 names): the side planes of what the camera sees through the portals so far (the first, the
// view's, is the camera's own) and the near and far distances, for the frames in the sector CTP_pSector the walk is in;
// the portal it came through (NULL in the camera's own sector and the scene's other three), the flags of the scene's
// sector it is in (SCTF_BACKDROP, SCTF_NEAR, else 0), and whether the portal is small on the screen or farther than its
// range: the walk then goes on only into the children of the sectors' own hierarchies (CTP_EnumProc). Their .bss order
// is VC6's hash order of these names (tools/bsshash.py); the zero-initialized ones (CTP_pSector, pTopSct,
// SctLookStckStop) follow all the others.
// GLOBAL: LS3DF 0x101b45a4
static S_vector CTP_VFnormals[5];

// GLOBAL: LS3DF 0x101b43f0
static float CTP_VFds[5];

// GLOBAL: LS3DF 0x101b43ec
static float CTP_Near;

// GLOBAL: LS3DF 0x101b440c
static float CTP_Far;

// GLOBAL: LS3DF 0x101b4408
static I3D_portal *CTP_pPortalW;

// GLOBAL: LS3DF 0x101b45a0
static uint CTP_SectFlg;

// GLOBAL: LS3DF 0x101b4404
static bool CTP_PartlyOffPortal;

// Zero-initialized, so after the others in .bss
// GLOBAL: LS3DF 0x101b45e0
static I3D_sector *CTP_pSector = NULL;

// The frames of the sector's hierarchy against the view: their spheres against the near and far distances and the
// side planes, then the boxes of those that cross a side plane against it. A visual in the view goes onto the sector's
// list of visuals to render, clipped where its sphere crosses a plane. Through a portal the frames are only marked here
// (FRMFLAGS_UNKNOWN_10000), and CTP_EnumProc cuts them down to the portal's planes. Then the sector's occluders in the
// camera's own view are marked.
// - The sums are written out (through Dot VC6 loads the vector's products the other way round), the side tests are
//   > (their jne), and the radius is read from the frame again.
// - The occluders' view plane goes through an S_frustum local: the original's frame has its 0x50 bytes at
//   [ebp - 0x114], of which only normal[0] and d[0] are used.
// Open: the frame. The original's slots: r at -4; d, -r, bits and clip at -8; pos at -0x14, the box at -0x7c and -0x70,
// d1 to d4 at -0x64, -0xb4, -0xac and -0xb0, the blocks' t at -0xbc, -0xc0, -0xb8 and -0x60, f at -0xc4, root at -0xa8;
// in the occluders' part cpos at -0x14, the corners at -0x7c, -0x70, -0x98 and -0xa4, the planes at -0x5c, -0x4c,
// -0x3c and -0x8c.
// FUNCTION: LS3DF 0x10041df0
void CTP_EnumProcFirst(I3D_frame *root)
{
    for (I3D_frame *frm = root->child; frm; frm = frm->next) {
        frm->frm_flags &= ~(FRMFLAGS_CLIP | FRMFLAGS_UNKNOWN_10000);
        if ((frm->frm_flags & FRMFLAGS_UNKNOWN_8000000) || !(frm->frm_flags & FRMFLAGS_ON))
            continue;
        S_vector pos = frm->GetHRBoundVolumeW().bsphere.pos;
        float r = frm->bound_w.bsphere.radius;
        float d = CTP_VFnormals[0].x * pos.x + CTP_VFnormals[0].y * pos.y + CTP_VFnormals[0].z * pos.z + CTP_VFds[0];
        if (frm->type == FRAME_VISUAL) {
            float f = d + 10000.0f;
            if (f < 0.0f)
                ((I3D_object *)frm)->sort_dist = 0.0f;
            else
                ((I3D_object *)frm)->sort_dist = f * 1000.0f;
        }
        if (d + r < CTP_Near)
            continue;
        if (d - CTP_Near <= r)
            frm->frm_flags |= FRMFLAGS_CLIP;
        if (d - r > CTP_Far)
            continue;
        if (CTP_Far - d <= r)
            frm->frm_flags |= FRMFLAGS_CLIP;
        if (!CTP_pPortalW) {
            float d1 = CTP_VFnormals[1].x * pos.x + CTP_VFnormals[1].y * pos.y + CTP_VFnormals[1].z * pos.z +
                CTP_VFds[1];
            if (!(d1 > -r))
                continue;
            float d2 = CTP_VFnormals[2].x * pos.x + CTP_VFnormals[2].y * pos.y + CTP_VFnormals[2].z * pos.z +
                CTP_VFds[2];
            if (!(d2 > -r))
                continue;
            float d3 = CTP_VFnormals[3].x * pos.x + CTP_VFnormals[3].y * pos.y + CTP_VFnormals[3].z * pos.z +
                CTP_VFds[3];
            if (!(d3 > -r))
                continue;
            float d4 = CTP_VFnormals[4].x * pos.x + CTP_VFnormals[4].y * pos.y + CTP_VFnormals[4].z * pos.z +
                CTP_VFds[4];
            if (!(d4 > -r))
                continue;
            bool visible = false;
            {
                // The planes the sphere crosses
                uint bits = 0;
                __asm {
                    fld r
                    fcom d1
                    mov ebx, bits
                    fnstsw ax
                    test ah, 0x41
                    jnz f_b1
                    or ebx, 1
                f_b1:
                    fcom d2
                    fnstsw ax
                    test ah, 0x41
                    jnz f_b2
                    or ebx, 2
                f_b2:
                    fcom d3
                    fnstsw ax
                    test ah, 0x41
                    jnz f_b3
                    or ebx, 4
                f_b3:
                    fcom d4
                    fnstsw ax
                    test ah, 0x41
                    jnz f_b4
                    or ebx, 8
                f_b4:
                    fstp st(0)
                    mov bits, ebx
                }
                if (bits) {
                    // A plane the box's corners are all behind hides the frame; one the box reaches in front of is
                    // cleared
                    S_vector bmin = frm->GetHRBoundVolumeW().bbox.min;
                    S_vector bmax = frm->GetHRBoundVolumeW().bbox.max;
                    if (bits & 1) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[12]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[16]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[20]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[4]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_3
                            fld dword ptr CTP_VFnormals[20]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_2
                            fld dword ptr CTP_VFnormals[16]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_1
                            fld dword ptr CTP_VFnormals[12]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p1_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp f_p1_done
                        f_p1_0:
                            fstp st(0)
                        f_p1_1:
                            fstp st(0)
                        f_p1_2:
                            fstp st(0)
                        f_p1_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 1
                            mov bits, edx
                        f_p1_done:
                        }
                    }
                    if (bits & 2) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[24]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[28]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[32]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[8]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_3
                            fld dword ptr CTP_VFnormals[32]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_2
                            fld dword ptr CTP_VFnormals[28]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_1
                            fld dword ptr CTP_VFnormals[24]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p2_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp f_p2_done
                        f_p2_0:
                            fstp st(0)
                        f_p2_1:
                            fstp st(0)
                        f_p2_2:
                            fstp st(0)
                        f_p2_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 2
                            mov bits, edx
                        f_p2_done:
                        }
                    }
                    if (bits & 4) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[36]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[40]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[44]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[12]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_3
                            fld dword ptr CTP_VFnormals[44]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_2
                            fld dword ptr CTP_VFnormals[40]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_1
                            fld dword ptr CTP_VFnormals[36]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p3_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp f_p3_done
                        f_p3_0:
                            fstp st(0)
                        f_p3_1:
                            fstp st(0)
                        f_p3_2:
                            fstp st(0)
                        f_p3_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 4
                            mov bits, edx
                        f_p3_done:
                        }
                    }
                    if (bits & 8) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[48]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[52]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[56]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[16]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_3
                            fld dword ptr CTP_VFnormals[56]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_2
                            fld dword ptr CTP_VFnormals[52]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_1
                            fld dword ptr CTP_VFnormals[48]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe f_p4_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp f_p4_done
                        f_p4_0:
                            fstp st(0)
                        f_p4_1:
                            fstp st(0)
                        f_p4_2:
                            fstp st(0)
                        f_p4_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 8
                            mov bits, edx
                        f_p4_done:
                        }
                    }
                    if (!bits)
                        visible = true;
                } else
                    visible = true;
            }
            if (!visible)
                continue;
            // Clipped where the sphere crosses a side plane: the portal's, through a portal
            uint clip = 0;
            if (!CTP_pPortalW) {
                float f;
                __asm {
                    fld r
                    fcom d1
                    mov ebx, clip
                    fnstsw ax
                    test ah, 0x41
                    jnz f_c1
                    or ebx, FRMFLAGS_CLIP
                f_c1:
                    fcom d2
                    fnstsw ax
                    test ah, 0x41
                    jnz f_c2
                    or ebx, FRMFLAGS_CLIP
                f_c2:
                    fcom d3
                    fnstsw ax
                    test ah, 0x41
                    jnz f_c3
                    or ebx, FRMFLAGS_CLIP
                f_c3:
                    fcom d4
                    fnstsw ax
                    test ah, 0x41
                    jnz f_c4
                    or ebx, FRMFLAGS_CLIP
                f_c4:
                    fstp f
                    mov clip, ebx
                }
            } else {
                // The camera's planes, read past the frustum's arrays: normal[5], [10], [15] and [20], d[5] to d[20]
                // (PS2 reads the same elements): a pointer to the array where a pointer to its elements was meant. A
                // switch: the compares come in value order, the default case's code first.
                const D3DVECTOR (*vfn)[5];
                const float (*vfd)[5];
                switch (CTP_SectFlg) {
                case SCTF_BACKDROP:
                    vfn = &pCurrentCamera->frustums[1].normal;
                    vfd = &pCurrentCamera->frustums[1].d;
                    break;
                case SCTF_NEAR:
                    vfn = &pCurrentCamera->frustums[2].normal;
                    vfd = &pCurrentCamera->frustums[2].d;
                    break;
                default:
                    vfn = &pCurrentCamera->frustums[0].normal;
                    vfd = &pCurrentCamera->frustums[0].d;
                }
                if (pos.Dot((const S_vector &)*vfn[1]) + *vfd[1] < r)
                    clip = FRMFLAGS_CLIP;
                if (pos.Dot((const S_vector &)*vfn[2]) + *vfd[2] < r)
                    clip |= FRMFLAGS_CLIP;
                if (pos.Dot((const S_vector &)*vfn[3]) + *vfd[3] < r)
                    clip |= FRMFLAGS_CLIP;
                if (pos.Dot((const S_vector &)*vfn[4]) + *vfd[4] < r)
                    clip |= FRMFLAGS_CLIP;
            }
            frm->frm_flags |= clip;
            frm->render_number = RenderNumber;
            if (frm->type == FRAME_VISUAL) {
                CTP_pSector->last_visual->next_visual = (I3D_visual *)frm;
                frm->next_visual = NULL;
                CTP_pSector->last_visual = frm;
            }
        } else
            frm->frm_flags |= FRMFLAGS_UNKNOWN_10000;
        if (frm->child)
            CTP_EnumProcFirst(frm);
    }
    if (root->type != FRAME_SECTOR)
        return;
    // The sector's occluders in front of the camera, between its planes and the corners of the near plane (the
    // camera's own view, through any portal)
    I3D_camera *cam = pCurrentCamera;
    float x = cam->ncp_x * 100.0f;
    float y = cam->ncp_y * 100.0f;
    float z = cam->ncp * 100.0f;
    S_vector p0 = S_vector(-x, y, z) * cam->view_inv;
    S_vector p1 = S_vector(-x, -y, z) * pCurrentCamera->view_inv;
    S_vector p2 = S_vector(x, y, z) * pCurrentCamera->view_inv;
    S_vector p3 = S_vector(x, -y, z) * pCurrentCamera->view_inv;
    S_vector cpos = pCurrentCamera->GetWorldPos();
    S_frustum vf;
    vf.normal[0] = pCurrentCamera->frustums[0].normal[0];
    vf.d[0] = pCurrentCamera->frustums[0].d[0];
    S_plane pl1(cpos, p0, p1);
    S_plane pl2(cpos, p3, p2);
    S_plane pl3(cpos, p2, p0);
    S_plane pl4(cpos, p1, p3);
    for (I3D_occluder *o = ((I3D_sector *)root)->occluders; o; o = o->next_in_sector) {
        S_vector c = o->center;
        o->dist = ((S_vector &)vf.normal[0]).Dot(c) + vf.d[0];
        if (o->dist + o->radius <= pCurrentCamera->ncp)
            continue;
        if (o->dist - o->radius < pCurrentCamera->fcp) {
            float nr = -o->radius;
            if (pl1.normal.Dot(c) + pl1.d > nr && pl2.normal.Dot(c) + pl2.d > nr && pl3.normal.Dot(c) + pl3.d > nr &&
                pl4.normal.Dot(c) + pl4.d > nr)
                o->frm_flags |= FRMFLAGS_UNKNOWN_10000;
        }
    }
}

// The frames CTP_EnumProcFirst marked, against the portal's side planes, the same way. Those already rendered this
// frame are only walked through. Through a portal that is small on the screen or out of its range, only the children
// of the frames the sectors' own hierarchies hold are walked.
// FUNCTION: LS3DF 0x10043000
void CTP_EnumProc(I3D_frame *root)
{
    if (!CTP_pPortalW)
        return;
    for (I3D_frame *frm = root->child; frm; frm = frm->next) {
        if (frm->render_number != RenderNumber) {
            if (!(frm->frm_flags & FRMFLAGS_UNKNOWN_10000))
                continue;
            S_vector pos = frm->GetHRBoundVolumeW().bsphere.pos;
            float r = frm->bound_w.bsphere.radius;
            float d1 = CTP_VFnormals[1].x * pos.x + CTP_VFnormals[1].y * pos.y + CTP_VFnormals[1].z * pos.z +
                CTP_VFds[1];
            if (!(d1 > -r))
                continue;
            float d2 = CTP_VFnormals[2].x * pos.x + CTP_VFnormals[2].y * pos.y + CTP_VFnormals[2].z * pos.z +
                CTP_VFds[2];
            if (!(d2 > -r))
                continue;
            float d3 = CTP_VFnormals[3].x * pos.x + CTP_VFnormals[3].y * pos.y + CTP_VFnormals[3].z * pos.z +
                CTP_VFds[3];
            if (!(d3 > -r))
                continue;
            float d4 = CTP_VFnormals[4].x * pos.x + CTP_VFnormals[4].y * pos.y + CTP_VFnormals[4].z * pos.z +
                CTP_VFds[4];
            if (!(d4 > -r))
                continue;
            float f;
            bool visible = false;
            {
                uint bits = 0;
                __asm {
                    fld r
                    fcom d1
                    mov ebx, bits
                    fnstsw ax
                    test ah, 0x41
                    jnz b1
                    or ebx, 1
                b1:
                    fcom d2
                    fnstsw ax
                    test ah, 0x41
                    jnz b2
                    or ebx, 2
                b2:
                    fcom d3
                    fnstsw ax
                    test ah, 0x41
                    jnz b3
                    or ebx, 4
                b3:
                    fcom d4
                    fnstsw ax
                    test ah, 0x41
                    jnz b4
                    or ebx, 8
                b4:
                    fstp f
                    mov bits, ebx
                }
                if (bits) {
                    S_vector bmin = frm->GetHRBoundVolumeW().bbox.min;
                    S_vector bmax = frm->GetHRBoundVolumeW().bbox.max;
                    if (bits & 1) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[12]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[16]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[20]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[4]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_3
                            fld dword ptr CTP_VFnormals[20]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_2
                            fld dword ptr CTP_VFnormals[16]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_1
                            fld dword ptr CTP_VFnormals[12]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p1_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp p1_done
                        p1_0:
                            fstp st(0)
                        p1_1:
                            fstp st(0)
                        p1_2:
                            fstp st(0)
                        p1_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 1
                            mov bits, edx
                        p1_done:
                        }
                    }
                    if (bits & 2) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[24]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[28]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[32]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[8]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_3
                            fld dword ptr CTP_VFnormals[32]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_2
                            fld dword ptr CTP_VFnormals[28]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_1
                            fld dword ptr CTP_VFnormals[24]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p2_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp p2_done
                        p2_0:
                            fstp st(0)
                        p2_1:
                            fstp st(0)
                        p2_2:
                            fstp st(0)
                        p2_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 2
                            mov bits, edx
                        p2_done:
                        }
                    }
                    if (bits & 4) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[36]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[40]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[44]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[12]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_3
                            fld dword ptr CTP_VFnormals[44]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_2
                            fld dword ptr CTP_VFnormals[40]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_1
                            fld dword ptr CTP_VFnormals[36]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p3_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp p3_done
                        p3_0:
                            fstp st(0)
                        p3_1:
                            fstp st(0)
                        p3_2:
                            fstp st(0)
                        p3_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 4
                            mov bits, edx
                        p3_done:
                        }
                    }
                    if (bits & 8) {
                        float t;
                        __asm {
                            fld dword ptr CTP_VFnormals[48]
                            fmul bmin.x
                            fld dword ptr CTP_VFnormals[52]
                            fmul bmin.y
                            fld dword ptr CTP_VFnormals[56]
                            fmul bmin.z
                            fld dword ptr CTP_VFds[16]
                            fadd st, st(3)
                            fadd st, st(2)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_3
                            fld dword ptr CTP_VFnormals[56]
                            fmul bmax.z
                            fsub st, st(2)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_2
                            fld dword ptr CTP_VFnormals[52]
                            fmul bmax.y
                            fsub st, st(4)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_1
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_1
                            fld dword ptr CTP_VFnormals[48]
                            fmul bmax.x
                            fsub st, st(6)
                            fxch st(1)
                            fadd st, st(1)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_0
                            fadd st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_0
                            fsub st, st(2)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_0
                            fsub st, st(3)
                            fst t
                            mov eax, t
                            cmp eax, 0x80000000
                            jbe p4_0
                            fcompp
                            fcompp
                            fcompp
                            fstp st(0)
                            jmp p4_done
                        p4_0:
                            fstp st(0)
                        p4_1:
                            fstp st(0)
                        p4_2:
                            fstp st(0)
                        p4_3:
                            fcompp
                            fcompp
                            mov edx, bits
                            xor edx, 8
                            mov bits, edx
                        p4_done:
                        }
                    }
                    if (!bits)
                        visible = true;
                } else
                    visible = true;
            }
            if (!visible)
                continue;
            uint clip = 0;
            if (!CTP_pPortalW) {
                __asm {
                    fld r
                    fcom d1
                    mov ebx, clip
                    fnstsw ax
                    test ah, 0x41
                    jnz c1
                    or ebx, FRMFLAGS_CLIP
                c1:
                    fcom d2
                    fnstsw ax
                    test ah, 0x41
                    jnz c2
                    or ebx, FRMFLAGS_CLIP
                c2:
                    fcom d3
                    fnstsw ax
                    test ah, 0x41
                    jnz c3
                    or ebx, FRMFLAGS_CLIP
                c3:
                    fcom d4
                    fnstsw ax
                    test ah, 0x41
                    jnz c4
                    or ebx, FRMFLAGS_CLIP
                c4:
                    fstp f
                    mov clip, ebx
                }
            } else {
                const D3DVECTOR (*vfn)[5];
                const float (*vfd)[5];
                switch (CTP_SectFlg) {
                case SCTF_BACKDROP:
                    vfn = &pCurrentCamera->frustums[1].normal;
                    vfd = &pCurrentCamera->frustums[1].d;
                    break;
                case SCTF_NEAR:
                    vfn = &pCurrentCamera->frustums[2].normal;
                    vfd = &pCurrentCamera->frustums[2].d;
                    break;
                default:
                    vfn = &pCurrentCamera->frustums[0].normal;
                    vfd = &pCurrentCamera->frustums[0].d;
                }
                if (pos.Dot((const S_vector &)*vfn[1]) + *vfd[1] < r)
                    clip = FRMFLAGS_CLIP;
                if (pos.Dot((const S_vector &)*vfn[2]) + *vfd[2] < r)
                    clip |= FRMFLAGS_CLIP;
                if (pos.Dot((const S_vector &)*vfn[3]) + *vfd[3] < r)
                    clip |= FRMFLAGS_CLIP;
                if (pos.Dot((const S_vector &)*vfn[4]) + *vfd[4] < r)
                    clip |= FRMFLAGS_CLIP;
            }
            frm->frm_flags |= clip;
            frm->render_number = RenderNumber;
            if (frm->type == FRAME_VISUAL) {
                CTP_pSector->last_visual->next_visual = (I3D_visual *)frm;
                frm->next_visual = NULL;
                CTP_pSector->last_visual = frm;
            }
        }
        if (frm->child && (!CTP_PartlyOffPortal || (frm->parent->type == FRAME_SECTOR &&
            !(((I3D_sector *)frm->parent)->sct_flags & SCTF_UNKNOWN_100))))
            CTP_EnumProc(frm);
    }
}

// The side planes of what the camera sees, from the camera through the corners of the near plane (scaled out to 100
// times its distance) or, through a portal, of the portal's clip box on it, the near and far distances and the view's
// plane of the sector's projection, then the sector's frames against them. In the backdrop the camera is at the origin.
// The PC computes the corners from the matrix's rows (PS2 multiplies four vectors by the matrix), each branch with its
// own switch over the projection, whose near and default cases VC6 merges across the branches.
// Open: the corners. The original computes the plane's center c into v0's slot (stored at once), keeps the six row
// products on the x87 stack and builds each corner from c reloaded (c - y - x, ...; v0 last, over c). Ours keeps c in
// registers whatever the spelling (floats, a vector built by its constructor or by the operators, products as locals or
// as common subexpressions), and the inline budget differs: the original inlines GetNormal in all four planes with its
// operator- calls partly out of line (the first one's constructor, the second), as nothing written out here gives;
// the operators for the corners starve the planes instead.
// FUNCTION: LS3DF 0x100437f0
void CullThruPortal(I3D_frame *frm, I3D_portal *portal, uint flags)
{
    I3D_camera *cam = pCurrentCamera;
    CTP_pPortalW = portal;
    CTP_SectFlg = flags;
    float x = cam->ncp_x * 100.0f;
    float y = cam->ncp_y * 100.0f;
    float z = cam->ncp * 100.0f;
    S_vector v0, v1, v2, v3;
    S_vector cpos;
    if (!portal) {
        S_matrix m = flags == SCTF_BACKDROP ? cam->m_bd_view_inv : cam->view_inv;
        v0.x = m.m[2][0] * z + m.m[3][0];
        v0.y = m.m[2][1] * z + m.m[3][1];
        v0.z = m.m[2][2] * z + m.m[3][2];
        float ax = m.m[0][0] * x, ay = m.m[0][1] * x, az = m.m[0][2] * x;
        float bx = m.m[1][0] * y, by = m.m[1][1] * y, bz = m.m[1][2] * y;
        v1.x = v0.x - bx - ax;
        v1.y = v0.y - by - ay;
        v1.z = v0.z - bz - az;
        v2.x = v0.x + bx + ax;
        v2.y = v0.y + by + ay;
        v2.z = v0.z + bz + az;
        v3.x = v0.x - bx + ax;
        v3.y = v0.y - by + ay;
        v3.z = v0.z - bz + az;
        v0.x = v0.x + bx - ax;
        v0.y = v0.y + by - ay;
        v0.z = v0.z + bz - az;
        switch (flags) {
        case SCTF_BACKDROP:
            cpos = S_vector(0.0f, 0.0f, 0.0f);
            CTP_VFnormals[0] = (const S_vector &)pCurrentCamera->frustums[1].normal[0];
            CTP_VFds[0] = pCurrentCamera->frustums[1].d[0];
            CTP_Near = ((I3D_sector *)frm)->scene->backdrop_ncp;
            CTP_Far = ((I3D_sector *)frm)->scene->backdrop_fcp;
            break;
        case SCTF_NEAR:
            cpos = pCurrentCamera->GetWorldPos();
            CTP_VFnormals[0] = (const S_vector &)pCurrentCamera->frustums[2].normal[0];
            CTP_VFds[0] = pCurrentCamera->frustums[2].d[0];
            CTP_Near = pCurrentCamera->near_ncp;
            CTP_Far = pCurrentCamera->near_fcp;
            break;
        default:
            cpos = pCurrentCamera->GetWorldPos();
            CTP_VFnormals[0] = (const S_vector &)pCurrentCamera->frustums[0].normal[0];
            CTP_VFds[0] = pCurrentCamera->frustums[0].d[0];
            CTP_Near = pCurrentCamera->ncp;
            CTP_Far = pCurrentCamera->fcp;
        }
    } else {
        // A portal of a few pixels lets the walk go on only into the sectors' own frames
        if (portal->clip_max.x - portal->clip_min.x < 10.0f * (2.0f / pGlobalIGraph->Scrn_sx()) &&
            portal->clip_max.y - portal->clip_min.y < 10.0f * (2.0f / pGlobalIGraph->Scrn_sy()))
            CTP_PartlyOffPortal = true;
        S_matrix m = flags == SCTF_BACKDROP ? pCurrentCamera->m_bd_view_inv : pCurrentCamera->view_inv;
        float x0 = portal->clip_min.x, y0 = portal->clip_min.y;
        float x1 = portal->clip_max.x, y1 = portal->clip_max.y;
        v0.x = m.m[2][0] * z + m.m[3][0];
        v0.y = m.m[2][1] * z + m.m[3][1];
        v0.z = m.m[2][2] * z + m.m[3][2];
        float ax = m.m[0][0] * x, ay = m.m[0][1] * x, az = m.m[0][2] * x;
        float bx = m.m[1][0] * y, by = m.m[1][1] * y, bz = m.m[1][2] * y;
        v1.x = v0.x + y0 * bx + x0 * ax;
        v1.y = v0.y + y0 * by + x0 * ay;
        v1.z = v0.z + y0 * bz + x0 * az;
        v2.x = v0.x + y1 * bx + x1 * ax;
        v2.y = v0.y + y1 * by + x1 * ay;
        v2.z = v0.z + y1 * bz + x1 * az;
        v3.x = v0.x + y0 * bx + x1 * ax;
        v3.y = v0.y + y0 * by + x1 * ay;
        v3.z = v0.z + y0 * bz + x1 * az;
        v0.x = v0.x + y1 * bx + x0 * ax;
        v0.y = v0.y + y1 * by + x0 * ay;
        v0.z = v0.z + y1 * bz + x0 * az;
        switch (flags) {
        case SCTF_BACKDROP:
            cpos = S_vector(0.0f, 0.0f, 0.0f);
            CTP_VFnormals[0] = (const S_vector &)pCurrentCamera->frustums[1].normal[0];
            CTP_VFds[0] = pCurrentCamera->frustums[1].d[0];
            CTP_Near = ((I3D_sector *)frm)->scene->backdrop_ncp;
            CTP_Far = ((I3D_sector *)frm)->scene->backdrop_fcp;
            break;
        case SCTF_NEAR:
            cpos = pCurrentCamera->GetWorldPos();
            CTP_VFnormals[0] = (const S_vector &)pCurrentCamera->frustums[2].normal[0];
            CTP_VFds[0] = pCurrentCamera->frustums[2].d[0];
            CTP_Near = pCurrentCamera->near_ncp;
            CTP_Far = pCurrentCamera->near_fcp;
            break;
        default:
            cpos = pCurrentCamera->GetWorldPos();
            CTP_VFnormals[0] = (const S_vector &)pCurrentCamera->frustums[0].normal[0];
            CTP_VFds[0] = pCurrentCamera->frustums[0].d[0];
            CTP_Near = pCurrentCamera->ncp;
            CTP_Far = pCurrentCamera->fcp;
        }
    }
    S_vector n;
    n.GetNormal(cpos, v0, v1);
    n.Normalize();
    CTP_VFnormals[1] = n;
    CTP_VFds[1] = -n.Dot(cpos);
    n.GetNormal(cpos, v3, v2);
    n.Normalize();
    CTP_VFnormals[2] = n;
    CTP_VFds[2] = -n.Dot(cpos);
    n.GetNormal(cpos, v2, v0);
    n.Normalize();
    CTP_VFnormals[3] = n;
    CTP_VFds[3] = -n.Dot(cpos);
    n.GetNormal(cpos, v1, v3);
    n.Normalize();
    CTP_VFnormals[4] = n;
    CTP_VFds[4] = -n.Dot(cpos);
    I3D_sector *sct = (I3D_sector *)frm;
    if (!(sct->sct_flags & SCTF_CULLED)) {
        CTP_EnumProcFirst(frm);
        sct->sct_flags |= SCTF_CULLED;
    }
    CTP_EnumProc(frm);
}

// The first culling of a sector this frame: its list of visuals starts empty
void CullThruPortal_First(I3D_frame *frm, I3D_portal *portal, uint flags)
{
    CTP_pSector = (I3D_sector *)frm;
    CTP_pSector->last_visual = frm;
    CullThruPortal(frm, portal, flags);
}

void ParsePortalsInSector(I3D_sector *sct, I3D_portal *p);

// The portal's box on the screen, cut down to the parent's clip box, then the walk on through it into the sector it
// leads to (out of, for a portal into the sector the walk is in). A portal partly behind the camera reaches the
// screen's edges on the sides where its edges cross behind it; one all behind is skipped; one through the camera's
// plane (a vertex at w = 0) is the whole screen.
// - The whole screen is one block that the w = 0 exit and ERROR 1 go to (in the original ERROR 1 runs into the
//   projection loop's exit, which VC6 moved after the loop); ERROR 2 has its own copy.
// - The edges are a vertex copied and the other subtracted in place (operator-=; their z is dead), the wrap test reads
//   the count from the portal (PS2 too), and the parent's clip box is applied by if/else statements (windows.h's
//   min and max load both operands onto the x87 stack).
// FUNCTION: LS3DF 0x10044600
void ParsePortal(I3D_portal *parent, I3D_portal *p, bool in)
{
    S_vector pv[16];
    I3D_sector *sct;
    if (in)
        sct = p->sector;
    else
        sct = p->connect_sector;
    const S_vector *v = (const S_vector *)p->verts_w;
    int n = p->num_verts;
    S_vector *pp = pv;
    if (p->parse_flags & PORTALPF_SOME_BEHIND)
        p->parse_flags ^= PORTALPF_SOME_BEHIND;
    p->parse_flags |= PORTALPF_ALL_BEHIND;
    while (n--) {
        const S_matrix &m = *pViewProjMatrix;
        float w = m.m[0][3] * v->x + m.m[1][3] * v->y + m.m[2][3] * v->z + m.m[3][3];
        if (w == 0.0f)
            goto all;
        if (w < 0.0f)
            p->parse_flags |= PORTALPF_SOME_BEHIND;
        else if (p->parse_flags & PORTALPF_ALL_BEHIND)
            p->parse_flags ^= PORTALPF_ALL_BEHIND;
        float rw = 1.0f / w;
        *pp = (*v++ * *pViewProjMatrix) * rw;
        pp->z = w;
        pp++;
    }
    if (p->parse_flags & PORTALPF_ALL_BEHIND)
        return;
    if (p->parse_flags & PORTALPF_SOME_BEHIND) {
        n = p->num_verts;
        // The vertices twice, for the walk around from the first behind the camera
        const S_vector *src = pv;
        S_vector *dst = pv + n;
        int i = n;
        while (i--)
            *dst++ = *src++;
        S_vector *q = pv;
        i = n;
        while (i--) {
            if (!(q->z > 0.0f))
                break;
            q++;
        }
        if (q->z > 0.0f) {
            dbgPrintf("!! ParsePortal ERROR 1 !!");
            goto all;
        }
        i = n;
        while (i--) {
            if (!(q->z <= 0.0f))
                break;
            q++;
        }
        if (q->z <= 0.0f) {
            dbgPrintf("!! ParsePortal ERROR 2 !!");
            p->clip_min = S_vector(-1.01f, -1.01f, 0.0f);
            p->clip_max = S_vector(1.01f, 1.01f, 0.0f);
            ParsePortalsInSector(sct, p);
            return;
        }
        // The vertices in front, from the first after those behind: the edges into and out of them cross behind
        S_vector d1 = *q;
        d1 -= q[-1];
        if (q >= q + p->num_verts)
            q -= p->num_verts;
        p->rect_min = *q;
        p->rect_max = *q;
        while (q->z > 0.0f) {
            if (p->rect_min.x > q->x)
                p->rect_min.x = q->x;
            if (p->rect_min.y > q->y)
                p->rect_min.y = q->y;
            if (p->rect_max.x < q->x)
                p->rect_max.x = q->x;
            if (p->rect_max.y < q->y)
                p->rect_max.y = q->y;
            q++;
        }
        S_vector d2 = q[-1];
        d2 -= *q;
        if (d1.x >= 0.0f || d2.x >= 0.0f)
            p->rect_max.x = 1.01f;
        if (d1.x <= 0.0f || d2.x <= 0.0f)
            p->rect_min.x = -1.01f;
        if (d1.y >= 0.0f || d2.y >= 0.0f)
            p->rect_max.y = 1.01f;
        if (d1.y <= 0.0f || d2.y <= 0.0f)
            p->rect_min.y = -1.01f;
    } else {
        int i = p->num_verts - 1;
        p->rect_min = pv[0];
        p->rect_max = pv[0];
        const S_vector *q = pv + 1;
        for (; i--; q++) {
            if (p->rect_min.x > q->x)
                p->rect_min.x = q->x;
            if (p->rect_min.y > q->y)
                p->rect_min.y = q->y;
            if (p->rect_max.x < q->x)
                p->rect_max.x = q->x;
            if (p->rect_max.y < q->y)
                p->rect_max.y = q->y;
        }
    }
    if (!parent) {
        p->clip_min = p->rect_min;
        p->clip_max = p->rect_max;
        if (p->clip_min.x < -1.01f)
            p->clip_min.x = -1.01f;
        if (p->clip_min.x > 1.01f)
            p->clip_min.x = 1.01f;
        if (p->clip_min.y < -1.01f)
            p->clip_min.y = -1.01f;
        if (p->clip_min.y > 1.01f)
            p->clip_min.y = 1.01f;
        if (p->clip_max.x < -1.01f)
            p->clip_max.x = -1.01f;
        if (p->clip_max.x > 1.01f)
            p->clip_max.x = 1.01f;
        if (p->clip_max.y < -1.01f)
            p->clip_max.y = -1.01f;
        if (p->clip_max.y > 1.01f)
            p->clip_max.y = 1.01f;
        if (p->clip_min.x != p->clip_max.x && p->clip_min.y != p->clip_max.y)
            ParsePortalsInSector(sct, p);
    } else if (parent->clip_min.x <= p->rect_max.x && parent->clip_max.x >= p->rect_min.x &&
        parent->clip_min.y <= p->rect_max.y && parent->clip_max.y >= p->rect_min.y) {
        if (parent->clip_min.x > p->rect_min.x)
            p->clip_min.x = parent->clip_min.x;
        else
            p->clip_min.x = p->rect_min.x;
        if (parent->clip_max.x < p->rect_max.x)
            p->clip_max.x = parent->clip_max.x;
        else
            p->clip_max.x = p->rect_max.x;
        if (parent->clip_min.y > p->rect_min.y)
            p->clip_min.y = parent->clip_min.y;
        else
            p->clip_min.y = p->rect_min.y;
        if (parent->clip_max.y < p->rect_max.y)
            p->clip_max.y = parent->clip_max.y;
        else
            p->clip_max.y = p->rect_max.y;
        ParsePortalsInSector(sct, p);
    }
    return;
all:
    p->clip_min = S_vector(-1.01f, -1.01f, 0.0f);
    p->clip_max = S_vector(1.01f, 1.01f, 0.0f);
    ParsePortalsInSector(sct, p);
}

// The stack of the sectors the walk is in (PS2 names): a sector reached again through a portal without a twin marks
// those it went through since (SCTF_UNKNOWN_80)
// GLOBAL: LS3DF 0x101b4410
static I3D_sector *SctLookStck[100];

// GLOBAL: LS3DF 0x101b45e4
static I3D_sector **pTopSct = NULL;

// The stack ran over (nothing reads it)
// GLOBAL: LS3DF 0x101b45e8
static bool SctLookStckStop = false;

// The walk into a sector through portal p (the camera's own sector without one): its frames culled to what p lets the
// camera see, then on through those of its portals (and of the other sectors' portals into it) the camera looks
// out through, unless the sector is on the walk's stack already. Through a portal the camera is near enough to.
// FUNCTION: LS3DF 0x10044da0
void ParsePortalsInSector(I3D_sector *sct, I3D_portal *p)
{
    if (!(sct->frm_flags & FRMFLAGS_ON) || (p && !(p->portal_flags & PORTALF_ON)))
        return;
    if (p) {
        S_vector d = p->center - pCurrentCamera->GetWorldPos();
        float fcp = pCurrentCamera->fcp;
        if (d.Square() > p->radius * p->radius + fcp * fcp * 1.2)
            return;
    }
    CTP_PartlyOffPortal = false;
    if (p) {
        if (p->range_2 > 0.0f) {
            S_vector d = p->center - pCurrentCamera->GetWorldPos();
            if (d.Square() / FOVDistCorrection2 > p->range_2) {
                if (p->portal_flags & PORTALF_RANGE_STOP)
                    return;
                CTP_PartlyOffPortal = true;
            }
        }
        p->portal_flags |= PORTALF_PASSED;
        if (!p->twin && p->connect_sector == sct) {
            I3D_sector **s = pTopSct;
            if (s < SctLookStck + 100) {
                while (s > SctLookStck) {
                    I3D_sector *t = *--s;
                    if (t == sct)
                        break;
                    t->sct_flags |= SCTF_UNKNOWN_80;
                }
            }
        }
    }
    if (sct->sct_flags & SCTF_IN_WALK)
        return;
    sct->sct_flags |= SCTF_VISIBLE;
    sct->sct_flags |= SCTF_IN_WALK;
    if (pTopSct < SctLookStck + 100)
        *pTopSct++ = sct;
    else
        SctLookStckStop = true;
    if (!(sct->sct_flags & SCTF_CULLED)) {
        sct->next_visual = NULL;
        CullThruPortal_First(sct, p, 0);
        OCCL_PrepareSector2(sct);
    } else {
        CTP_pSector = sct;
        CullThruPortal(sct, p, 0);
    }
    S_vector cpos = pCurrentCamera->GetWorldPos();
    I3D_portal **pp = &sct->portals[0];
    for (int i = sct->portals.size(); i--; pp++) {
        I3D_portal *pt = *pp;
        if (cpos.Dot(pt->plane.normal) + pt->plane.d > 0.0f && !(pt->portal_flags & PORTALF_OCCLUDED))
            ParsePortal(p, pt, false);
    }
    pp = &sct->portals_in[0];
    for (i = sct->portals_in.size(); i--; pp++) {
        I3D_portal *pt = *pp;
        if (!pt->twin) {
            if (cpos.Dot(pt->plane.normal) + pt->plane.d < 0.0f && !(pt->portal_flags & PORTALF_OCCLUDED))
                ParsePortal(p, pt, true);
        }
    }
    pTopSct--;
    sct->sct_flags ^= SCTF_IN_WALK;
}

void ParsePortalsInSector_Begin(I3D_sector *sct)
{
    SctLookStckStop = false;
    pTopSct = SctLookStck;
    ParsePortalsInSector(sct, NULL);
}

// Connects the portals: two of different sectors at the same place (their centers apart less than a tenth of their
// common box) are twins, each leading into the other's sector. The others lead into the sector their first vertex is
// in, else the primary sector.
// - PS2's shapes: the box in six floats, each set from the first vertex, the centers' tests nested, and a count for
//   each portal (one count for both moves the scene's spilled pointer to the other slot).
// - The centers are scaled component by component (through operator*= VC6 doesn't let the last product consume the
//   reciprocal), and the second portal's count and vertices are taken before its center is zeroed.
// FUNCTION: LS3DF 0x100450a0
void UpdatePortalInterconnection(I3D_scene *scene)
{
    I3D_portal *p = scene->portals;
    if (!p)
        return;
    do {
        if (!p->connect_sector) {
            I3D_portal *p1 = p->next_in_scene;
            while (p1 != scene->portals) {
                if (p1->connect_sector) {
                    p1 = p1->next_in_scene;
                    continue;
                }
                const S_vector *v = (const S_vector *)p->verts_w;
                S_vector c1 = *v;
                float min_x = v->x, min_y = v->y, min_z = v->z;
                float max_x = v->x, max_y = v->y, max_z = v->z;
                int n = p->num_verts;
                int i = n - 1;
                v++;
                for (; i--; v++) {
                    if (min_x > v->x)
                        min_x = v->x;
                    if (min_y > v->y)
                        min_y = v->y;
                    if (min_z > v->z)
                        min_z = v->z;
                    if (max_x < v->x)
                        max_x = v->x;
                    if (max_y < v->y)
                        max_y = v->y;
                    if (max_z < v->z)
                        max_z = v->z;
                    c1 += *v;
                }
                c1.x *= 1.0f / n;
                c1.y *= 1.0f / n;
                c1.z *= 1.0f / n;
                int n2 = p1->num_verts;
                v = (const S_vector *)p1->verts_w;
                S_vector c2(0.0f, 0.0f, 0.0f);
                for (i = n2; i--; v++) {
                    S_vector u = *v;
                    if (min_x > u.x)
                        min_x = u.x;
                    if (min_y > u.y)
                        min_y = u.y;
                    if (min_z > u.z)
                        min_z = u.z;
                    if (max_x < u.x)
                        max_x = u.x;
                    if (max_y < u.y)
                        max_y = u.y;
                    if (max_z < u.z)
                        max_z = u.z;
                    c2 += u;
                }
                c2.x *= 1.0f / n2;
                c2.y *= 1.0f / n2;
                c2.z *= 1.0f / n2;
                if (max_x - min_x < .01)
                    max_x = min_x + .1;
                if (max_y - min_y < .01)
                    max_y = min_y + .1;
                if (max_z - min_z < .01)
                    max_z = min_z + .1;
                if (fabs(c2.x - c1.x) <= (max_x - min_x) * .1) {
                    if (fabs(c2.y - c1.y) <= (max_y - min_y) * .1) {
                        if (fabs(c2.z - c1.z) <= (max_z - min_z) * .1) {
                            if (p1->sector != p->sector) {
                                p->twin = p1;
                                p1->twin = p;
                                p->connect_sector = p1->sector;
                                p1->connect_sector = p->sector;
                                p->sector->portals_in.push_back(p1);
                                p1->sector->portals_in.push_back(p);
                                p1 = scene->portals;
                            } else {
                                p->portal_flags |= PORTALF_IN_PORTAL;
                                p1->portal_flags |= PORTALF_IN_PORTAL;
                                p1 = p1->next_in_scene;
                                dbgPrintf("!! Portal in portal (sector \"%s\")", p->sector->GetName());
                            }
                            continue;
                        }
                    }
                }
                p1 = p1->next_in_scene;
            }
        }
        if (!p->connect_sector) {
            p->twin = NULL;
            S_vector pos = (const S_vector &)p->verts_w[0];
            I3D_sector *s = scene->sectors;
            for (int i = scene->num_sectors; i--; s = s->next_in_scene) {
                if (s->CheckPoint(pos) && s != p->sector) {
                    p->connect_sector = s;
                    s->portals_in.push_back(p);
                    break;
                }
            }
            if (!p->connect_sector) {
                p->connect_sector = scene->primary_sector;
                scene->primary_sector->portals_in.push_back(p);
            }
        }
        p = p->next_in_scene;
    } while (p != scene->portals);
}

void ReportBadPortals(I3D_scene *scene)
{
    I3D_portal *p = scene->portals;
    if (!p)
        return;
    do {
        if (p->portal_flags & PORTALF_DIVIDES)
            dbgPrintf("!! Portal divides sector (portal in sector \"%s\" to sector \"%s\" - autorepair CAN'T be used )",
                p->sector->GetName(), p->connect_sector->GetName());
        p = p->next_in_scene;
    } while (p != scene->portals);
}

// The frame's walk through the portals: the sectors made ready and the portals connected where they changed, then from
// the camera's sector out through the portals the camera sees, and the scene's backdrop, omni and near sectors with
// their own views
// FUNCTION: LS3DF 0x10045700
void ProcessPortals(I3D_scene *scene)
{
    if (!scene)
        return;
    if (scene->sector_version != sectorVersion) {
        scene->sector_version = sectorVersion;
        scene->MakeReadySectors();
    }
    if (scene->portal_version != portalVersion) {
        scene->portal_version = portalVersion;
        UpdatePortalInterconnection(scene);
        ReportBadPortals(scene);
    }
    OCCL_UpdateOcclLists(scene);
    I3D_sector *cam_sector = pCurrentCamera->sector;
    if (!cam_sector)
        return;
    if (scene->portals) {
        for (I3D_portal *p = scene->portals->next_in_scene; p != scene->portals; p = p->next_in_scene) {
            p->parse_flags = 0;
            p->portal_flags &= ~PORTALF_PASSED;
        }
    }
    int i = scene->num_sectors;
    I3D_sector *s = scene->sectors;
    while (i--) {
        s->sct_flags &= ~(SCTF_VISIBLE | SCTF_CULLED | SCTF_UNKNOWN_80);
        s = s->next_in_scene;
    }
    scene->primary_sector->next_visual = NULL;
    scene->backdrop_sector->next_visual = NULL;
    scene->omni_sector->next_visual = NULL;
    scene->near_sector->next_visual = NULL;
    scene->primary_sector->sct_flags &= ~(SCTF_VISIBLE | SCTF_CULLED | SCTF_UNKNOWN_80);
    ParsePortalsInSector_Begin(cam_sector);
    scene->backdrop_sector->sct_flags &= ~(SCTF_VISIBLE | SCTF_CULLED | SCTF_UNKNOWN_80);
    CullThruPortal_First(scene->backdrop_sector, NULL, SCTF_BACKDROP);
    scene->omni_sector->sct_flags &= ~(SCTF_VISIBLE | SCTF_CULLED | SCTF_UNKNOWN_80);
    CullThruPortal_First(scene->omni_sector, NULL, 0);
    scene->near_sector->sct_flags &= ~(SCTF_VISIBLE | SCTF_CULLED | SCTF_UNKNOWN_80);
    CullThruPortal_First(scene->near_sector, NULL, SCTF_NEAR);
}

// The vertex count (a byte), then the flags and the rest up to the vertices (0x1c bytes; PS2's own layout has 0x20),
// and the vertices
// FUNCTION: LS3DF 0x100458f0
void I3D_portal::LoadImage()
{
    num_verts = 0;
    dtaRead(LoadImageHandle, &num_verts, 1);
    dtaRead(LoadImageHandle, &portal_flags, num_verts * sizeof(S_vector) + 0x1c);
}
