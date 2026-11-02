// The effects' particles (named after PS2's C_Particle.cpp, which sorts here: the PC's file is most likely the one at
// 0x446f20-0x44856c, docs/game-files.md), written whole: SetPhase's direction helper, the particle's destructor, which
// C_effects::Mission_Done runs, Reset, which C_ParticleType's destructor and Reset run, Init, which the emitter's
// CreateParticle runs, and SetPhase and Tick, which Init and the emitter's Tick run.

#include "mission/C_Particle.h"
#include "mission/C_ParticleType.h"
#include "mission/Mission.h"
#include "actors/C_actor.h"

#include "I3D/I3D.h"
#include "I3D/mesh/I3d_material.h"

// Puts the frame into the sector of its position (the name is ours). The scene is read before the position, which a
// call written as act_mission->GetScene()->SetFrameSectorPos(...) reads after it.
static inline void SetSectorPos(I3D_frame *frm)
{
    I3D_scene *scene = act_mission->scene;
    scene->SetFrameSectorPos(frm, frm->GetWorldPos());
}

// Turns dir at random about spread and the axis across both, by up to spread's length each way, and normalizes it (the
// name is ours; SetPhase's)
// FUNCTION: GAME 0x446f20
void SpreadDir(S_vector &dir, const S_vector &spread)
{
    if (spread.Square() == 0.0f)
        return;
    S_vector across = dir.Cross(spread);
    S_vector delta = spread * (rnd() * 2.0f - 1.0f);
    delta += across * (rnd() * 2.0f - 1.0f);
    dir += delta;
    dir.Normalize();
}

// FUNCTION: GAME 0x447150
C_Particle::~C_Particle()
{
    if (visual)
        visual->Release();
}

// FUNCTION: GAME 0x447160
void C_Particle::Reset()
{
    if (visual)
        visual->LinkTo(NULL, 0);
    unknown_8 = 0;
    type = NULL;
    unknown_3c = 0;
    unknown_60 = 0.0f;
    unknown_44 = 0;
    unknown_5c = 0.0f;
}

// Each of the part's values spread at random by its range, either way: the direction (the effect's where the part has
// none), the speed, the visual's scale and transparency, and the position, the effect's plus the part's offset. The
// visual is a copy of the part's frame; an animated material gets a copy of its own, from its first frame.
// FUNCTION: GAME 0x447190
bool C_Particle::Init(ushort id, const S_vector &pos, const S_vector &dir, C_ParticleType *part, int time)
{
    if (!visual)
        visual = (I3D_part_base *)I3DGetDriver()->CreateVisual(I3D_VISUAL_PART_BASE);
    visual->sort_group = id;
    visual->vis_flags &= ~VISF_PROJECTOR;
    if (!visual || !part || !part->subparts.size())
    {
        unknown_3c = 0x7fffffff;
        return false;
    }
    visual->Duplicate(part->frame);
    // STRING: GAME 0x632588
    visual->SetName("MaraJe");
    visual->part_flags = 1 << part->unknown_2c | PARTF_UNKNOWN_100;
    SetSectorPos(visual);
    visual->SetScale(part->unknown_68 + part->unknown_74 * (rnd() * 2.0f - 1.0f));
    visual->SetPartTransparency((rnd() * 2.0f - 1.0f) * part->unknown_84 + part->unknown_80);
    if (!part->unknown_48.IsNull())
    {
        if (part->parent && part->unknown_bc & PARTF_LOCAL_DIR)
            unknown_c = part->unknown_48.RotateByMatrix(part->parent->GetMatrix());
        else
            unknown_c = part->unknown_48;
    }
    else
        unknown_c = dir;
    type = part;
    SetPhase(0);
    S_vector spread;
    if (part->parent && part->unknown_bc & PARTF_LOCAL_DIR)
        spread = part->unknown_54.RotateByMatrix(part->parent->GetMatrix());
    else
        spread = part->unknown_54;
    for (int i = 0; i < 3; i++)
        (&unknown_c.x)[i] += (rnd() * 2.0f - 1.0f) * (&spread.x)[i];
    unknown_c *= (rnd() * 2.0f - 1.0f) * part->unknown_64 + part->unknown_60;
    S_vector at = part->unknown_30;
    S_vector offset;
    offset.x = (rnd() * 2.0f - 1.0f) * part->unknown_3c.x;
    offset.y = (rnd() * 2.0f - 1.0f) * part->unknown_3c.y;
    offset.z = (rnd() * 2.0f - 1.0f) * part->unknown_3c.z;
    at += offset;
    if (part->parent && part->unknown_bc & PARTF_LOCAL_POS)
        at = at.RotateByMatrix(part->parent->GetMatrix());
    visual->SetPos(at + pos);
    if (type->subparts[0]->IsOn())
        visual->SetOn(true);
    else
        visual->SetOn(false);
    if (visual->GetMesh()->GetLOD(0)->GetFGroup(0)->material->flags & MATF_ANIMATED)
    {
        visual->DetachMesh();
        visual->GetMesh()->GetLOD(0)->GetFGroup(0)->DetachMaterial();
        ((I3D_anim_material *)visual->GetMesh()->GetLOD(0)->GetFGroup(0)->material)->SetCurrentFrame(0);
    }
    visual->SetBillboard(part->unknown_bc & PARTF_BILLBOARD);
    unknown_44 = part->unknown_bc;
    if (unknown_44 & PARTF_TURN)
        visual->SetDir(unknown_c, 0.0f);
    SetSectorPos(visual);
    visual->Update();
    if (time)
        Tick(time);
    return true;
}

// The subpart's time, and its direction: the particle turns to it over the time where it heads elsewhere, else it keeps
// its speed in the new direction; then how its scale and transparency change to reach the subpart's by its end
// FUNCTION: GAME 0x4476c0
void C_Particle::SetPhase(int phase)
{
    unknown_8 = phase;
    S_particle_subpart *sp = type->subparts[phase];
    unknown_4c = unknown_3c = (int)((rnd() * 2.0f - 1.0f) * sp->unknown_4 + sp->unknown_0);
    if (!sp->unknown_c.IsNull() || !sp->unknown_18.IsNull())
    {
        S_vector dir = sp->unknown_c;
        if (!sp->unknown_18.IsNull())
            SpreadDir(dir, sp->unknown_18);
        dir.Normalize();
        if (type->parent && type->unknown_bc & PARTF_LOCAL_DIR)
            dir = dir.RotateByMatrix(type->parent->GetMatrix());
        S_vector cur = unknown_c;
        cur.Normalize();
        if (cur.Dot(dir) < .95f)
        {
            unknown_18.SetDir(cur, 0.0f);
            unknown_28.SetDir(dir, 0.0f);
            unknown_38 = true;
        }
        else
        {
            float speed = unknown_c.Magnitude();
            unknown_c = dir;
            unknown_c *= speed;
            unknown_38 = false;
        }
    }
    else
        unknown_38 = false;
    if (sp->unknown_8 & SUBPARTF_OFF)
        visual->SetOn(false);
    else
        visual->SetOn(true);
    float r = rnd() * 2.0f - 1.0f;
    for (int i = 0; i < 3; i++)
    {
        float s = r * (&sp->unknown_44.x)[i] + (&sp->unknown_38.x)[i];
        if (s < 0.0f)
            s = 0.0f;
        if (unknown_3c < .001f)
            (&unknown_50.x)[i] = .001f;
        else
            (&unknown_50.x)[i] = (s - visual->GetScale().x) / (unknown_3c * .001f);
    }
    float a = (rnd() * 2.0f - 1.0f) * sp->unknown_54 + sp->unknown_50;
    if (a > 1.0f)
        a = 1.0f;
    else if (a < 0.0f)
        a = 0.0f;
    unknown_5c = (a - visual->GetTransparency()) / (unknown_3c * .001f);
    unknown_60 = (rnd() * 2.0f - 1.0f) * sp->unknown_5c + sp->unknown_58;
}

// The time left, the direction turned on to the subpart's, the move by the speed, the visual turned to it or about its
// axes, scaled and faded, then the speed's change and the fall. move is a product (the original calls the constructor
// of the second's result out of line), and the visual's transparency is read through GetTransparency, whose pending
// call leaves that constructor out of line as the original. Open: the original calls the three operator* of the
// speed's change out of line, ours inlines them: it had less inline budget left there (about 14 statements' worth of
// dead code before that statement gives the original's code), and fewer statements elsewhere (one S_quat for the
// three turns) bring the constructor back in.
// FUNCTION: GAME 0x447c20
void C_Particle::Tick(int time)
{
    if ((unknown_3c -= time) < 0)
        unknown_3c = 0;
    S_vector move(0.0f, 0.0f, 0.0f);
    S_particle_subpart *sp = type->subparts[unknown_8];
    if (unknown_38)
    {
        float speed = unknown_c.Magnitude();
        S_quat q = unknown_18.Slerp(unknown_28, (float)(unknown_4c - unknown_3c) / unknown_4c, false);
        unknown_c = q.GetDir();
        unknown_c *= speed;
    }
    if (!(sp->unknown_8 & SUBPARTF_STILL))
        move = unknown_c * (float)time * .001f;
    if (unknown_44 & PARTF_TURN)
    {
        S_vector dir = move;
        dir.Normalize();
        visual->SetDir(dir, 0.0f);
    }
    if (sp->unknown_8 & SUBPARTF_ROTATE)
    {
        if (sp->unknown_24.x != 0.0f)
        {
            S_quat q;
            q.Make(S_vector(1.0f, 0.0f, 0.0f), time * sp->unknown_24.x * .001f);
            visual->SetRot(visual->GetRot() * q);
        }
        if (sp->unknown_24.y != 0.0f)
        {
            S_quat q;
            q.Make(S_vector(0.0f, 1.0f, 0.0f), time * sp->unknown_24.y * .001f);
            visual->SetRot(visual->GetRot() * q);
        }
        if (sp->unknown_24.z != 0.0f)
        {
            S_quat q;
            q.Make(S_vector(0.0f, 0.0f, 1.0f), time * sp->unknown_24.z * .001f);
            visual->SetRot(visual->GetRot() * q);
        }
    }
    if (!unknown_50.IsNull())
        visual->SetScale(unknown_50 * (float)time * .001f + visual->GetScale());
    if (unknown_5c != 0.0f)
    {
        float a = time * unknown_5c * .001f + visual->GetTransparency();
        if (a > 1.0f)
            a = 1.0f;
        else if (a < 0.0f)
            a = 0.0f;
        visual->SetPartTransparency(a);
    }
    S_vector pos = visual->GetPos();
    pos += move;
    visual->SetPos(pos);
    visual->Update();
    unknown_c += unknown_c * unknown_60 * (float)time * .001f;
    if (sp->unknown_8 & SUBPARTF_GRAVITY)
        unknown_c.y -= time * sp->unknown_60 * .001f;
}
