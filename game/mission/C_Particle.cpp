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
    subpart = 0;
    type = NULL;
    time_left = 0;
    speed_change = 0.0f;
    flags = 0;
    transparency_change = 0.0f;
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
        time_left = 0x7fffffff;
        return false;
    }
    visual->Duplicate(part->frame);
    // STRING: GAME 0x632588
    visual->SetName("MaraJe");
    visual->part_flags = 1 << part->blend | PARTF_UNKNOWN_100;
    SetSectorPos(visual);
    visual->SetScale(part->scale + part->scale_spread * (rnd() * 2.0f - 1.0f));
    visual->SetPartTransparency((rnd() * 2.0f - 1.0f) * part->transparency_spread + part->transparency);
    if (!part->dir.IsNull())
    {
        if (part->parent && part->flags & PARTF_LOCAL_DIR)
            velocity = part->dir.RotateByMatrix(part->parent->GetMatrix());
        else
            velocity = part->dir;
    }
    else
        velocity = dir;
    type = part;
    SetPhase(0);
    S_vector spread;
    if (part->parent && part->flags & PARTF_LOCAL_DIR)
        spread = part->dir_spread.RotateByMatrix(part->parent->GetMatrix());
    else
        spread = part->dir_spread;
    for (int i = 0; i < 3; i++)
        (&velocity.x)[i] += (rnd() * 2.0f - 1.0f) * (&spread.x)[i];
    velocity *= (rnd() * 2.0f - 1.0f) * part->speed_spread + part->speed;
    S_vector at = part->offset;
    S_vector offset;
    offset.x = (rnd() * 2.0f - 1.0f) * part->offset_spread.x;
    offset.y = (rnd() * 2.0f - 1.0f) * part->offset_spread.y;
    offset.z = (rnd() * 2.0f - 1.0f) * part->offset_spread.z;
    at += offset;
    if (part->parent && part->flags & PARTF_LOCAL_POS)
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
    visual->SetBillboard(part->flags & PARTF_BILLBOARD);
    flags = part->flags;
    if (flags & PARTF_TURN)
        visual->SetDir(velocity, 0.0f);
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
    subpart = phase;
    S_particle_subpart *sp = type->subparts[phase];
    duration = time_left = (int)((rnd() * 2.0f - 1.0f) * sp->duration_range + sp->duration);
    if (!sp->dir.IsNull() || !sp->dir_spread.IsNull())
    {
        S_vector dir = sp->dir;
        if (!sp->dir_spread.IsNull())
            SpreadDir(dir, sp->dir_spread);
        dir.Normalize();
        if (type->parent && type->flags & PARTF_LOCAL_DIR)
            dir = dir.RotateByMatrix(type->parent->GetMatrix());
        S_vector cur = velocity;
        cur.Normalize();
        if (cur.Dot(dir) < .95f)
        {
            start_rot.SetDir(cur, 0.0f);
            end_rot.SetDir(dir, 0.0f);
            turning = true;
        }
        else
        {
            float speed = velocity.Magnitude();
            velocity = dir;
            velocity *= speed;
            turning = false;
        }
    }
    else
        turning = false;
    if (sp->flags & SUBPARTF_OFF)
        visual->SetOn(false);
    else
        visual->SetOn(true);
    float r = rnd() * 2.0f - 1.0f;
    for (int i = 0; i < 3; i++)
    {
        float s = r * (&sp->scale_range.x)[i] + (&sp->scale.x)[i];
        if (s < 0.0f)
            s = 0.0f;
        if (time_left < .001f)
            (&scale_change.x)[i] = .001f;
        else
            (&scale_change.x)[i] = (s - visual->GetScale().x) / (time_left * .001f);
    }
    float a = (rnd() * 2.0f - 1.0f) * sp->transparency_range + sp->transparency;
    if (a > 1.0f)
        a = 1.0f;
    else if (a < 0.0f)
        a = 0.0f;
    transparency_change = (a - visual->GetTransparency()) / (time_left * .001f);
    speed_change = (rnd() * 2.0f - 1.0f) * sp->speed_change_range + sp->speed_change;
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
    if ((time_left -= time) < 0)
        time_left = 0;
    S_vector move(0.0f, 0.0f, 0.0f);
    S_particle_subpart *sp = type->subparts[subpart];
    if (turning)
    {
        float speed = velocity.Magnitude();
        S_quat q = start_rot.Slerp(end_rot, (float)(duration - time_left) / duration, false);
        velocity = q.GetDir();
        velocity *= speed;
    }
    if (!(sp->flags & SUBPARTF_STILL))
        move = velocity * (float)time * .001f;
    if (flags & PARTF_TURN)
    {
        S_vector dir = move;
        dir.Normalize();
        visual->SetDir(dir, 0.0f);
    }
    if (sp->flags & SUBPARTF_ROTATE)
    {
        if (sp->rot_speed.x != 0.0f)
        {
            S_quat q;
            q.Make(S_vector(1.0f, 0.0f, 0.0f), time * sp->rot_speed.x * .001f);
            visual->SetRot(visual->GetRot() * q);
        }
        if (sp->rot_speed.y != 0.0f)
        {
            S_quat q;
            q.Make(S_vector(0.0f, 1.0f, 0.0f), time * sp->rot_speed.y * .001f);
            visual->SetRot(visual->GetRot() * q);
        }
        if (sp->rot_speed.z != 0.0f)
        {
            S_quat q;
            q.Make(S_vector(0.0f, 0.0f, 1.0f), time * sp->rot_speed.z * .001f);
            visual->SetRot(visual->GetRot() * q);
        }
    }
    if (!scale_change.IsNull())
        visual->SetScale(scale_change * (float)time * .001f + visual->GetScale());
    if (transparency_change != 0.0f)
    {
        float a = time * transparency_change * .001f + visual->GetTransparency();
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
    velocity += velocity * speed_change * (float)time * .001f;
    if (sp->flags & SUBPARTF_GRAVITY)
        velocity.y -= time * sp->gravity * .001f;
}
