#ifndef I3D_RENDER_H
#define I3D_RENDER_H

// Render state helpers of the rendering code (the file's name is a guess). I3d_driver.cpp doesn't include it: its
// draws call the copies the linker kept (in the billboard file's code), where the visuals' rendering inlines them as far
// as the inline budget allows. I3D.h declares the two with copies for the driver.
// They are inline functions because the original's I3D_object::m_RenderFGroup inlines SetBlendState and
// SetVertexFormat in its first calls and calls them in later ones. Written out instead, our build inlined the
// multipass helpers the original calls.
#include "I3D/I3D.h"

// FUNCTION: LS3DF 0x1000ab40
inline void SetBlendState(uint state)
{
    if (m_CurrBlendState != state) {
        m_CurrBlendState = state;
        switch (state) {
        case 0:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            break;
        case 1:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            break;
        case 2:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            break;
        case 3:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
            break;
        case 4:
            m_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
            m_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
            break;
        }
    }
}

// FUNCTION: LS3DF 0x1000ae50
inline void SetVertexFormat(uint format)
{
    if (m_CurrVertexFormat != format) {
        m_CurrVertexFormat = format;
        switch (format) {
        case 0:
            m_pD3DDevice8->SetVertexShader(m_LVertexVS);
            break;
        case 1:
            m_pD3DDevice8->SetVertexShader(m_LVertex2VS);
            break;
        case 2:
            m_pD3DDevice8->SetVertexShader(D3DFVF_DIFFUSE);
            break;
        case 3:
            m_pD3DDevice8->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_TEX1);
            break;
        case 4:
            m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
            break;
        case 5:
            m_pD3DDevice8->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2);
            break;
        case 6:
            m_pD3DDevice8->SetVertexShader(m_VertexVS);
            break;
        }
    }
}

// The second texture stage's color op (always inlined, so the name is a guess)
inline void SetColorOp1(uint op)
{
    if (m_CurrColorOp1 != op) {
        m_CurrColorOp1 = op;
        m_pD3DDevice8->SetTextureStageState(1, D3DTSS_COLOROP, op);
    }
}

// D3DRS_CULLMODE, D3DCULL_CCW being the driver's m_CullMode (the billboards' rendering; the name is a guess). The
// weather renderer calls it out of line; the copy the linker keeps is in an I3D file not written yet.
// FUNCTION: LS3DF 0x10029ef0
inline void SetCullState(uint cull)
{
    if (m_CurrCullState != cull) {
        m_CurrCullState = cull;
        if (cull == D3DCULL_CCW)
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, m_CullMode);
        else
            m_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, cull);
    }
}

// D3DRS_CLIPPING (Insanity3D has the same helper, commented out, under this name). Two calls, as IGraph::SetState's
// DXS_CLIPPING: the copy the linker kept is the mirrors' file's.
// FUNCTION: LS3DF 0x100333d0
inline void SetClipping(uint clip)
{
    if (m_CurrClipState != clip) {
        m_CurrClipState = clip;
        if (clip == 1)
            m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, TRUE);
        else
            m_pD3DDevice8->SetRenderState(D3DRS_CLIPPING, FALSE);
    }
}

#endif
