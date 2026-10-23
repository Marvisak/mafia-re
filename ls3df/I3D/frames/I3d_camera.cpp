// The camera (PS2 I3d_camera.cpp), at 0x1000b0c0-0x1000de70 in the original: I3D_camera in PS2's order. After the
// last function come the copies of S_vector's inline constructor, operator-, Dot and GetNormal the linker kept, then
// the header's zero vector's $E (0x1000de50). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

// FUNCTION: LS3DF 0x1000b0c0
// FUNCTION: LS3DF11 0x1000ac60
I3D_camera::I3D_camera()
{
    type = FRAME_CAMERA;
    fov_a = near_fov_a = PI / 4;
    ncp = 1.0f;
    fcp = 1000.0f;
    near_ncp = .01f;
    near_fcp = 20.0f;
    aspect_ratio = (float)pGlobalIGraph->Scrn_sx() / (float)pGlobalIGraph->Scrn_sy();
    scene = NULL;
    sector = NULL;
    orthogonal = false;
    ortho_scale = 1.0f;
    unknown_3fc = 0;
    unknown_400 = 0;
    ncp_x = ncp;
    ncp_y = ncp;
    near_ncp_x = near_ncp;
    near_ncp_y = near_ncp;
    frm_flags |= FRMFLAGS_UNKNOWN_8000000;
    UpdateProjMatrix();
    m_view.Identity();
    m_view_proj = m_proj;
    view_inv.Identity();
    m_bd_view_proj = m_bd_proj;
    m_bd_view.Identity();
    m_bd_view_inv.Identity();
    m_near_view_proj = m_near_proj;
}

// PC only: the scene it is active in forgets it
// FUNCTION: LS3DF 0x1000b270
// FUNCTION: LS3DF11 0x1000ae10
I3D_camera::~I3D_camera()
{
    if (scene)
        scene->current_camera = NULL;
}

// FUNCTION: LS3DF 0x1000b290
// FUNCTION: LS3DF11 0x1000ae30
LS3D_RESULT I3D_camera::Duplicate(I3D_frame *frame)
{
    LS3D_RESULT ret = I3D_frame::Duplicate(frame);
    if (ret < 0)
        return ret;
    if (frame->GetType() == FRAME_CAMERA) {
        I3D_camera *cam = (I3D_camera *)frame;
        fov_a = cam->fov_a;
        ncp = cam->ncp;
        fcp = cam->fcp;
        near_fov_a = cam->near_fov_a;
        near_ncp = cam->near_ncp;
        near_fcp = cam->near_fcp;
        aspect_ratio = cam->aspect_ratio;
        scene = NULL;
        sector = NULL;
        orthogonal = cam->orthogonal;
        ortho_scale = cam->ortho_scale;
        unknown_3fc = 0;
        unknown_400 = 0;
        ncp_x = cam->ncp_x;
        ncp_y = cam->ncp_y;
        frm_flags |= FRMFLAGS_UNKNOWN_8000000;
        m_view.Identity();
        view_inv.Identity();
        m_proj = cam->m_proj;
        m_view_proj = m_proj;
        m_bd_view.Identity();
        m_bd_view_inv.Identity();
        m_bd_proj = cam->m_bd_proj;
        m_bd_view_proj = m_bd_proj;
        m_near_proj = cam->m_near_proj;
    }
    return I3D_OK;
}

// The projections: the view's from the field of view or the ortho scale and the range, the backdrop's the same with the
// scene's backdrop range (without a scene 10 times the near and 1000 times the far clipping plane), and the near one's
// from the near field of view and range (orthogonal: the view's with the near plane moved). The backdrop's elements are
// written as m[r][c]: through operator()(int, int) VC6 reads scene again after the first store.
// Open: the original hoists the backdrop copy's rep movsd setup above the branch as lea edi, mov ecx, mov esi; ours
// comes out as mov esi, mov ecx, lea edi (where the copy and the declarations go doesn't change it).
// FUNCTION: LS3DF 0x1000b400
// FUNCTION: LS3DF11 0x1000afa0
void I3D_camera::UpdateProjMatrix()
{
    memset(&m_proj, 0, sizeof(m_proj));
    if (orthogonal) {
        float r = 1.0f / (fcp - ncp);
        m_proj(0, 0) = ortho_scale;
        m_proj(1, 1) = aspect_ratio * ortho_scale;
        m_proj(2, 2) = r;
        m_proj(3, 2) = -r * ncp;
        m_proj(3, 3) = 1.0f;
        m_bd_proj = m_proj;
        float r1;
        if (scene) {
            r1 = 1.0f / (scene->backdrop_fcp - scene->backdrop_ncp);
            m_bd_proj.m[2][2] = r1;
            m_bd_proj.m[3][2] = -r1 * scene->backdrop_ncp;
        } else {
            r1 = 1.0f / (fcp * 1000.0f - ncp * 10.0f);
            m_bd_proj.m[2][2] = r1;
            m_bd_proj.m[3][2] = -r1 * ncp * 10.0f;
        }
        m_near_proj = m_proj;
        m_near_proj(3, 2) = -r1 * near_ncp;
    } else {
        float a = fov_a * .5f;
        float w = (float)(cos(a) / sin(a));
        float h = w * aspect_ratio;
        m_proj(0, 0) = w;
        m_proj(1, 1) = h;
        float q = fcp / (fcp - ncp);
        m_proj(2, 2) = q;
        m_proj(2, 3) = 1.0f;
        m_proj(3, 2) = -q * ncp;
        m_bd_proj = m_proj;
        ncp_x = ncp / w;
        ncp_y = ncp / h;
        if (scene) {
            float q1 = scene->backdrop_fcp / (scene->backdrop_fcp - scene->backdrop_ncp);
            m_bd_proj.m[2][2] = q1;
            m_bd_proj.m[3][2] = -q1 * scene->backdrop_ncp;
        } else {
            float f = fcp * 1000.0f;
            float q1 = f / (f - ncp * 10.0f);
            m_bd_proj.m[2][2] = q1;
            m_bd_proj.m[3][2] = -q1 * ncp * 10.0f;
        }
        memset(&m_near_proj, 0, sizeof(m_near_proj));
        a = near_fov_a * .5f;
        w = (float)(cos(a) / sin(a));
        h = w * aspect_ratio;
        m_near_proj(0, 0) = w;
        m_near_proj(1, 1) = h;
        q = near_fcp / (near_fcp - near_ncp);
        m_near_proj(2, 2) = q;
        m_near_proj(2, 3) = 1.0f;
        m_near_proj(3, 2) = -q * near_ncp;
        near_ncp_x = near_ncp / w;
        near_ncp_y = near_ncp / h;
    }
    update_count++;
}

// The setters report a bad value with the value they take instead (a field of view below 0.01 or from PI on is clamped,
// a bad range or aspect is ignored). The perspective's take effect when not orthogonal, the ortho scale's when
// orthogonal, the range's always.
// FUNCTION: LS3DF 0x1000b690
// FUNCTION: LS3DF11 0x1000b230
void I3D_camera::SetFOV(float fov)
{
    if (fov < .01f || fov >= PI) {
        fov = fov < .01f ? .01f : PI;
        dbgPrintf("!! Invalid FOV:%f  I3D_camera::SetFOV()  this:0x%x  %s", fov, this, GetName());
    }
    fov_a = fov;
    if (!orthogonal)
        UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b720
// FUNCTION: LS3DF11 0x1000b2c0
void I3D_camera::SetNearFOV(float fov)
{
    if (fov < .01f || fov >= PI) {
        fov = fov < .01f ? .01f : PI;
        dbgPrintf("!! Invalid FOV:%f  I3D_camera::SetNearFOV()  this:0x%x  %s", fov, this, GetName());
    }
    near_fov_a = fov;
    if (!orthogonal)
        UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b7b0
// FUNCTION: LS3DF11 0x1000b350
void I3D_camera::SetRange(float near1, float far1)
{
    if (fabs(far1 - near1) < .01f || near1 < .001 || far1 < .001) {
        dbgPrintf("!! Invalid range near1:%f  far1:%f I3D_camera::SetRange()  this:0x%x  %s", near1, far1, this,
            GetName());
        return;
    }
    ncp = near1;
    fcp = far1;
    UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b840
// FUNCTION: LS3DF11 0x1000b3e0
void I3D_camera::SetNearRange(float near1, float far1)
{
    if (fabs(far1 - near1) < .01f || near1 < .001 || far1 < .001) {
        dbgPrintf("!! Invalid range near1:%f  far1:%f I3D_camera::SetNearRange()  this:0x%x  %s", near1, far1, this,
            GetName());
        return;
    }
    near_ncp = near1;
    near_fcp = far1;
    UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b8d0
// FUNCTION: LS3DF11 0x1000b470
void I3D_camera::SetOrthoScale(float scale)
{
    if (scale < 1e-6)
        dbgPrintf("!! Invalid ortho scale: %f  I3D_camera::SetOrthoScale()  this:0x%x  %s", scale, this, GetName());
    ortho_scale = scale;
    if (orthogonal)
        UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b930
// FUNCTION: LS3DF11 0x1000b4d0
void I3D_camera::SetOrthogonal(bool b)
{
    orthogonal = b;
    UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b950
// FUNCTION: LS3DF11 0x1000b4f0
void I3D_camera::SetAspectRatio(float aspect)
{
    if (aspect < .01 || aspect > 100.0f) {
        dbgPrintf("!! Invalid aspect:%f   I3D_camera::SetApectRatio()  this:0x%x  %s", aspect, this, GetName());
        return;
    }
    aspect_ratio = aspect;
    if (!orthogonal)
        UpdateProjMatrix();
}

// FUNCTION: LS3DF 0x1000b9d0
// FUNCTION: LS3DF11 0x1000b570
void I3D_camera::UpdateSector()
{
    if (scene) {
        UpdateWMatrix();
        sector = scene->GetSector(matrix(3), sector);
    } else {
        sector = NULL;
        UpdateWMatrix();
    }
}

// The view matrices and the frustums. The frustums' side planes go through the camera's position and the far corners
// of the near clipping plane scaled by 100 (orthogonal: the edges of the view box); the backdrop's has the same sides,
// its first plane through the origin, the near projection's the view's first plane. PS2's statements, with the PC's
// shapes: the transposition through m[r][c] (the original loads ahead of the stores), the four corners shared by both
// branches and assigned (copied from the product's temporary), the others constructed in place, the near projection's
// first plane copied by element and the orthogonal one's whole frustum by array (two rep movsd, as PS2's two memcpy).
// Each side plane is PS2's `pl = S_plane(pos, a, b)`: the original stores a null normal's -1 or 1 as integer constants,
// S_plane's normalization (S_vector::Normalize stores them through the FPU), its plane local's copies vanish (a
// ComputePlane on a named plane keeps a store of d and a larger frame), and the plane constructor's nested GetNormal is
// what VC6 starves early and inlines in the third frustum. The non-orthogonal code is the else block: its aggregates
// share the areas of the orthogonal block's (the near corners nc1 and nc2 over that block's temporaries, the three
// regions' plane temporaries apart), as in the original; after a `return` in the if block they don't.
// Open: the inline budget. The original's calls (GetNormal out of line in the first two frustums with Dot inlined, in
// the third GetNormal inlined with its operator- calls out of line, the first Dot out of line and the last plane's first
// operator- inlined without its constructor) need about 7 more statements of source and two more pending inline calls
// right before the near projection's second plane: dead code (`if (0) { ... }` x7) and two default-constructed
// vectors declared there give the original's calls and 99% (the rest is the sums' operand order). Without them ours
// calls Dot out of line in the orthogonal frustum's last two planes and in all four of the third's. No source for
// either is known.
// FUNCTION: LS3DF 0x1000ba30
// FUNCTION: LS3DF11 0x1000b5d0
void I3D_camera::UpdateV()
{
    UpdateSector();
    m_view.Identity();
    m_view.m[0][0] = matrix.m[0][0];
    m_view.m[0][1] = matrix.m[1][0];
    m_view.m[0][2] = matrix.m[2][0];
    m_view.m[1][0] = matrix.m[0][1];
    m_view.m[1][1] = matrix.m[1][1];
    m_view.m[1][2] = matrix.m[2][1];
    m_view.m[2][0] = matrix.m[0][2];
    m_view.m[2][1] = matrix.m[1][2];
    m_view.m[2][2] = matrix.m[2][2];
    m_view(3) = -matrix(3) * m_view;
    view_inv.Inverse(m_view);
    m_view_proj.Make4X4(m_view, m_proj);
    m_bd_view = m_view;
    m_bd_view(3).Zero();
    m_bd_view_proj.Make4X4(m_bd_view, m_bd_proj);
    m_bd_view_inv = view_inv;
    m_bd_view_inv(3).Zero();
    m_near_view_proj.Make4X4(m_view, m_near_proj);
    frustums[0].normal[0] = (const D3DVECTOR &)matrix(2);
    frustums[0].d[0] = -matrix(2).Dot(matrix(3));
    S_vector c0, c1, c2, c3;
    if (orthogonal) {
        float x = 1.0f / ortho_scale;
        float y = x / aspect_ratio;
        c0 = S_vector(-x, y, 0.0f) * matrix;
        c1 = S_vector(-x, -y, 0.0f) * matrix;
        c2 = S_vector(x, y, 0.0f) * matrix;
        c3 = S_vector(x, -y, 0.0f) * matrix;
        S_vector v4 = S_vector(-x, -y, 1.0f) * matrix;
        S_vector v5 = S_vector(x, y, 1.0f) * matrix;
        S_plane pl;
        pl = S_plane(v4, c0, c1);
        frustums[0].normal[1] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[1] = pl.d;
        pl = S_plane(v5, c3, c2);
        frustums[0].normal[2] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[2] = pl.d;
        pl = S_plane(v5, c2, c0);
        frustums[0].normal[3] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[3] = pl.d;
        pl = S_plane(v4, c1, c3);
        frustums[0].normal[4] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[4] = pl.d;
        frustums[1].normal[0] = (const D3DVECTOR &)matrix(2);
        frustums[1].d[0] = 0.0f;
        pl = S_plane(v4, c0, c1);
        frustums[1].normal[1] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[1] = pl.d;
        pl = S_plane(v5, c3, c2);
        frustums[1].normal[2] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[2] = pl.d;
        pl = S_plane(v5, c2, c0);
        frustums[1].normal[3] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[3] = pl.d;
        pl = S_plane(v4, c1, c3);
        frustums[1].normal[4] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[4] = pl.d;
        memcpy(frustums[2].normal, frustums[0].normal, sizeof(frustums[0].normal));
        memcpy(frustums[2].d, frustums[0].d, sizeof(frustums[0].d));
    } else {
        const S_vector &pos = matrix(3);
        S_plane pl;
        c0 = S_vector(-ncp_x * 100.0f, ncp_y * 100.0f, ncp * 100.0f) * matrix;
        c1 = S_vector(-ncp_x * 100.0f, -ncp_y * 100.0f, ncp * 100.0f) * matrix;
        c2 = S_vector(ncp_x * 100.0f, ncp_y * 100.0f, ncp * 100.0f) * matrix;
        c3 = S_vector(ncp_x * 100.0f, -ncp_y * 100.0f, ncp * 100.0f) * matrix;
        pl = S_plane(pos, c0, c1);
        frustums[0].normal[1] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[1] = pl.d;
        pl = S_plane(pos, c3, c2);
        frustums[0].normal[2] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[2] = pl.d;
        pl = S_plane(pos, c2, c0);
        frustums[0].normal[3] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[3] = pl.d;
        pl = S_plane(pos, c1, c3);
        frustums[0].normal[4] = (const D3DVECTOR &)pl.normal;
        frustums[0].d[4] = pl.d;
        frustums[1].normal[0] = (const D3DVECTOR &)matrix(2);
        frustums[1].d[0] = 0.0f;
        pl = S_plane(pos, c0, c1);
        frustums[1].normal[1] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[1] = pl.d;
        pl = S_plane(pos, c3, c2);
        frustums[1].normal[2] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[2] = pl.d;
        pl = S_plane(pos, c2, c0);
        frustums[1].normal[3] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[3] = pl.d;
        pl = S_plane(pos, c1, c3);
        frustums[1].normal[4] = (const D3DVECTOR &)pl.normal;
        frustums[1].d[4] = pl.d;
        frustums[2].normal[0] = frustums[0].normal[0];
        frustums[2].d[0] = frustums[0].d[0];
        S_vector nc0 = S_vector(-near_ncp_x * 100.0f, near_ncp_y * 100.0f, near_ncp * 100.0f) * matrix;
        S_vector nc1 = S_vector(-near_ncp_x * 100.0f, -near_ncp_y * 100.0f, near_ncp * 100.0f) * matrix;
        S_vector nc2 = S_vector(near_ncp_x * 100.0f, near_ncp_y * 100.0f, near_ncp * 100.0f) * matrix;
        S_vector nc3 = S_vector(near_ncp_x * 100.0f, -near_ncp_y * 100.0f, near_ncp * 100.0f) * matrix;
        pl = S_plane(pos, nc0, nc1);
        frustums[2].normal[1] = (const D3DVECTOR &)pl.normal;
        frustums[2].d[1] = pl.d;
        pl = S_plane(pos, nc3, nc2);
        frustums[2].normal[2] = (const D3DVECTOR &)pl.normal;
        frustums[2].d[2] = pl.d;
        pl = S_plane(pos, nc2, nc0);
        frustums[2].normal[3] = (const D3DVECTOR &)pl.normal;
        frustums[2].d[3] = pl.d;
        pl = S_plane(pos, nc1, nc3);
        frustums[2].normal[4] = (const D3DVECTOR &)pl.normal;
        frustums[2].d[4] = pl.d;
    }
}
