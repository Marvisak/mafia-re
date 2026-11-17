// The bridges (PS2 C_bridge.cpp), file 80 of docs/game-files.md: 0x567cf0-0x56ad9c, the functions in PS2's order
// (PS2's NetDirect, SaveData, SaveGameGetSize, SaveGameLoad and DuplicateFrom are the linker's folded copies on the PC;
// SetTitanic is PC only). The file starts with an empty initializer (0x567cf0, uzly's) and ends with the volumes'
// vectors' templates (C_bridge.h).
// Open: GameInit's sound positions (the first sum's x loads its operands the other way round, the second's registers
// are swapped) and a load the original hoists in its inlined model cache Open; the .bss order (below), which also
// names the end of parnik in Init's and SetTitanic's loops after another global.

#include "actors/C_bridge.h"

#include "actors/C_human.h"
#include "collision/g_c_glob.h"
#include "I3D_cache.h"
#include "mission/Mission.h"
#include "rw_data.h"
#include "stub.h"
#include "tools.h"
#include "vehicles/C_trf_c.h"

#include <string.h>

// The sound of a steamship's horn, which it blows calling for a bridge (C_sound_table::ConstructSound's id; the name is
// a guess)
#define SOUND_PARNIK_HORN 0x146

// The steamships' routes (PS2 name, Czech "nodes"), tables\parnik.def's, and the length of each stretch plus one
// (PS2 name), which GameInit computes. PS2 defines the globals in this order: the original's .bss has them in the
// exact reverse, the order of another file's precompiled header (/Yu), where ours follows the names' buckets.
// GLOBAL: GAME 0x647060
S_parnik_node uzly[PARNIK_ROUTES][PARNIK_NODES];
// GLOBAL: GAME 0x646ff0
float uzlyDist[PARNIK_ROUTES][PARNIK_NODES];
// GLOBAL: GAME 0x646fa8
S_parnik parnik[PARNIKS];
// The bridges' tables, by C_bridge::index (PS2 names, Czech: "the bridge occupied"): the second bridge's steamships
// need a bridge up, a bridge is up, and the steamships' places taken
// GLOBAL: GAME 0x646f9c
int bridgeNeedUp[2];
// GLOBAL: GAME 0x646f94
int bridgeIsUp[2];
// GLOBAL: GAME 0x646f8c
int bridgeObsazen[2];
// GLOBAL: GAME 0x646f80
int mostObsazen[3];
// Set by the destructor and GameDone, read nowhere (PS2 name, a static)
// GLOBAL: GAME 0x636770
static bool globalFirstBridge = true;

// The defaults: the leaves open to 50 degrees, in half a minute, and close in 20 s. PS2's order: VC6 groups the
// constant stores by value, so the zeroes come first, after the vectors'.
// FUNCTION: GAME 0x567d10
C_bridge::C_bridge()
{
    type = ACTOR_BRIDGE;
    reverse = false;
    max_angle = 50.0f * PI / 180.0f;
    down_time = 120000;
    signal_time = 4000;
    raise_time = 30000;
    up_time = 10000;
    lower_time = 20000;
    sound_raise = 0x71;
    sound_up = 0x72;
    sound_lower = 0x73;
    sound_down = 0x74;
    sound_start = 0xb6;
    sound_signal = 0xb5;
    other_frame = NULL;
    sound = NULL;
    phase = 7;
    shut = false;
}

// FUNCTION: GAME 0x567e20
C_bridge::~C_bridge()
{
    globalFirstBridge = true;
}

// The base's, then the other leaf by its name: the frame's with its last character one up
// FUNCTION: GAME 0x567e70
bool C_bridge::Init(I3D_frame *frm)
{
    if (!C_actor::Init(frm))
        return false;
    timer = 0;
    blink_timer = 0;
    char name[128];
    strcpy(name, frm->GetName());
    if (name[0])
        name[strlen(name) - 1]++;
    other_frame = frm->GetParent()->FindChildFrame(name, ENUMF_ALL);
    if (!other_frame)
        return false;
    if (*frame->GetName() == 'L')
        index = 0;
    else
        index = 1;
    if (index)
    {
        for (int i = 0; i < PARNIKS; i++)
            parnik[i].route = 0xffff;
        // the first two only
        memset(mostObsazen, 0, 2 * sizeof(int));
        memset(bridgeIsUp, 0, sizeof(bridgeIsUp));
        memset(bridgeObsazen, 0, sizeof(bridgeObsazen));
        memset(bridgeNeedUp, 0, sizeof(bridgeNeedUp));
    }
    return true;
}

// The cycle: a steamship's call (bridgeNeedUp) starts the signal, the leaves go up once the traffic's cars have left
// the bridge and the start's sound has ended, stay up while a steamship needs them and come down; shut hurries each
// state to its end. Then the leaves turn to the angle, the frame's and the other's the opposite ways.
// FUNCTION: GAME 0x567fb0
void C_bridge::AI(uint time)
{
    if (time >= wait_time)
        wait_time = 0;
    else
        wait_time -= time;
    if (wait_time)
        return;
    switch (state)
    {
    case BRIDGE_SIGNAL:
        bridgeIsUp[index] = 0;
        timer -= time;
        if (timer < 0 || shut)
        {
            if (C_traffic_car::GetBridgeCarCount(this) <= 0 && !shut)
            {
                state = BRIDGE_START;
                sound->SetOn(false, true);
                sound_table.ConstructSound(sound, sound_start);
                sound->SetLoop(false);
                sound->SetOn(true, true);
            }
            else
                timer = 1000;
        }
        break;
    case BRIDGE_START:
        bridgeIsUp[index] = 0;
        timer = 0;
        if (!sound->IsPlaying(false) || shut)
        {
            state = BRIDGE_RAISING;
            timer = raise_time;
            angle = (1.0f - (float)timer / raise_time) * max_angle;
            sound->SetOn(false, true);
            sound_table.ConstructSound(sound, sound_raise);
            sound->SetLoop(true);
            sound->SetOn(true);
            if (nadparnik_coll)
                gifCollision.DynInsert(nadparnik_coll);
        }
        break;
    case BRIDGE_RAISING:
        bridgeIsUp[index] = 0;
        timer -= time;
        if (timer > 0 && !shut)
            angle = (1.0f - (float)timer / raise_time) * max_angle;
        else
        {
            timer += up_time;
            state = BRIDGE_UP;
            angle = max_angle;
            sound->SetOn(false, true);
            sound_table.ConstructSound(sound, sound_up);
            sound->SetLoop(false);
            sound->SetOn(true);
        }
        break;
    case BRIDGE_UP:
        bridgeIsUp[index] = 1;
        timer -= time;
        if (timer < 0 || shut)
        {
            if (!bridgeNeedUp[index])
            {
                timer += lower_time;
                state = BRIDGE_LOWERING;
                angle = (float)timer * max_angle / lower_time;
                sound->SetOn(false, true);
                sound_table.ConstructSound(sound, sound_lower);
                sound->SetLoop(true);
                sound->SetOn(true);
            }
            else
                timer = 0;
        }
        break;
    case BRIDGE_LOWERING:
        bridgeIsUp[index] = 0;
        timer -= time;
        if (timer > 0 && !shut)
            angle = (float)timer * max_angle / lower_time;
        else
        {
            timer += down_time;
            state = BRIDGE_DOWN;
            angle = 0.0f;
            sound->SetOn(false, true);
            sound_table.ConstructSound(sound, sound_down);
            sound->SetLoop(false);
            sound->SetOn(true);
            signal_sound->SetOn(false);
            phase = 7;
            if (nadparnik_coll)
                gifCollision.DynErase(nadparnik_coll);
        }
        break;
    default:
        bridgeIsUp[index] = 0;
        if (!shut)
        {
            timer -= time;
            if (bridgeNeedUp[index])
            {
                timer = signal_time;
                blink_timer = 600;
                state = BRIDGE_SIGNAL;
                signal_sound->SetOn(false, true);
                sound_table.ConstructSound(signal_sound, sound_signal);
                signal_sound->SetLoop(true);
                signal_sound->SetOn(true);
                phase = 1;
            }
        }
        break;
    }
    if (last_angle != angle)
    {
        if (reverse)
        {
            frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), angle);
            other_frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), -angle);
        }
        else
        {
            frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), -angle);
            other_frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), angle);
        }
        frame->Update();
        other_frame->Update();
        last_angle = angle;
    }
}

// The leaves' volumes follow them: each volume's matrices are its S_BridgeCol's moved by the leaf's world matrix; then
// the semaphore's lights blink, and the second bridge sails the two steamships along their routes (uzly)
// FUNCTION: GAME 0x568540
void C_bridge::Update(uint time)
{
    S_matrix wm = frame->GetMatrix();
    S_matrix inv = wm;
    inv.Inverse(S_matrix(wm));
    for (S_BridgeCol *c = colls.begin(); c < colls.end(); c++)
    {
        switch (c->vol->flags & 0xff)
        {
        case VOLTYPE_XTOBB:
            ((g_volume_xtobb *)c->vol)->inv_matrix = inv * c->inv_matrix;
            ((g_volume_xtobb *)c->vol)->matrix = c->matrix * wm;
            break;
        case VOLTYPE_OBB:
            ((g_volume_obb *)c->vol)->inv_matrix = inv * c->inv_matrix;
            ((g_volume_obb *)c->vol)->matrix = c->matrix * wm;
            break;
        }
    }
    wm = other_frame->GetMatrix();
    inv = wm;
    inv.Inverse(S_matrix(wm));
    for (S_BridgeCol *oc = other_colls.begin(); oc < other_colls.end(); oc++)
    {
        switch (oc->vol->flags & 0xff)
        {
        case VOLTYPE_XTOBB:
            ((g_volume_xtobb *)oc->vol)->inv_matrix = inv * oc->inv_matrix;
            ((g_volume_xtobb *)oc->vol)->matrix = oc->matrix * wm;
            break;
        case VOLTYPE_OBB:
            ((g_volume_obb *)oc->vol)->inv_matrix = inv * oc->inv_matrix;
            ((g_volume_obb *)oc->vol)->matrix = oc->matrix * wm;
            break;
        }
    }
    if (blink_timer)
    {
        blink_timer -= time;
        if (blink_timer <= 0)
        {
            if (state == BRIDGE_DOWN)
            {
                blink_timer = 0;
                if (lights[0])
                    lights[0]->SetOn(false);
                if (lights[1])
                    lights[1]->SetOn(false);
                if (lights[2])
                    lights[2]->SetOn(false);
                if (lights[3])
                    lights[3]->SetOn(false);
                if (lights[4])
                    lights[4]->SetOn(false);
                if (lights[5])
                    lights[5]->SetOn(false);
            }
            else
            {
                blink_timer += 600;
                if (blink_timer == 0)
                    blink_timer = 1;
                blink = !blink;
                if (lights[0])
                    lights[0]->SetOn(!blink);
                if (lights[1])
                    lights[1]->SetOn(!blink);
                if (lights[2])
                    lights[2]->SetOn(!blink);
                if (lights[3])
                    lights[3]->SetOn(blink);
                if (lights[4])
                    lights[4]->SetOn(blink);
                if (lights[5])
                    lights[5]->SetOn(blink);
            }
        }
    }
    if (!index)
        return;
    float dt = time * .001f;
    int need_up[2];
    need_up[0] = bridgeNeedUp[0];
    need_up[1] = bridgeNeedUp[1];
    bridgeNeedUp[0] = 0;
    bridgeNeedUp[1] = 0;
    for (uint i = 0; i < PARNIKS; i++)
    {
        uint node = parnik[i].node;
        parnik[i].last_node = node;
        int route = parnik[i].route;
        float speed = parnik[i].speed;
        float step = speed / uzlyDist[route][node];
        float pos = parnik[i].pos + dt * step;
        if (pos > 1.0f)
        {
            pos = (pos - 1.0f) / step;
            node++;
            if (node >= PARNIK_NODES - 1)
            {
                node = 0;
                pos = 0;
                if (i)
                    parnik[i].route = rnd(2) + 2;
                else
                    parnik[0].route = rnd(2);
                parnik[i].speed = 5.0f;
                speed = 5.0f;
            }
            pos = speed / uzlyDist[route][node] * pos;
        }
        parnik[i].node = node;
        parnik[i].pos = pos;
        // Where the ship is on its route, in nodes: on each route's stretches near a bridge it calls for it
        // (bridgeNeedUp, need its index) and stops short of it while it isn't up, and it takes the places it passes
        // (bridgeObsazen, mostObsazen: the ship's index plus one), stopping where the other ship holds one
        float where = node + pos;
        bool stop = false;
        if (wait_time)
            stop = true;
        if (bridgeObsazen[0] == i + 1)
            bridgeObsazen[0] = 0;
        if (bridgeObsazen[1] == i + 1)
            bridgeObsazen[1] = 0;
        if (mostObsazen[0] == i + 1)
            mostObsazen[0] = 0;
        if (mostObsazen[1] == i + 1)
            mostObsazen[1] = 0;
        if (mostObsazen[2] == i + 1)
            mostObsazen[2] = 0;
        int need = 0xffff;
        switch (route)
        {
        case 0:
            if (where > .72f && where < 1.3f)
            {
                bridgeNeedUp[0] |= 1 << i;
                need = 0;
            }
            if (where > .92f && where < 1.0f && !bridgeIsUp[0])
                stop = true;
            if (where > .6f)
            {
                if (where < 1.2f && bridgeObsazen[0])
                    stop = true;
                if (where < 2.0f && !bridgeObsazen[0])
                    bridgeObsazen[0] = i + 1;
            }
            if (where > 4.0f)
            {
                if (where < 4.5f && mostObsazen[0])
                    stop = true;
                if (where < 5.0f && !mostObsazen[0])
                    mostObsazen[0] = i + 1;
            }
            break;
        case 1:
            if (where > 4.0f && where < 5.2f)
            {
                bridgeNeedUp[0] |= 1 << i;
                need = 0;
            }
            if (where > 4.79f && where < 5.0f && !bridgeIsUp[0])
                stop = true;
            if (where > 4.0f)
            {
                if (where < 5.0f && bridgeObsazen[0])
                    stop = true;
                if (where < 5.3f && !bridgeObsazen[0])
                    bridgeObsazen[0] = i + 1;
            }
            break;
        case 2:
            if (where > 4.4f && where < 5.2f)
            {
                bridgeNeedUp[1] |= 1 << i;
                need = 1;
            }
            if (where > 4.85f && where < 5.0f && !bridgeIsUp[1])
                stop = true;
            if (where > 4.2f)
            {
                if (where < 5.2f && bridgeObsazen[1])
                    stop = true;
                if (where < 5.4f && !bridgeObsazen[1])
                    bridgeObsazen[1] = i + 1;
            }
            if (where > .5f)
            {
                if (where < 1.0f && mostObsazen[0])
                    stop = true;
                if (where < 1.5f && !mostObsazen[0])
                    mostObsazen[0] = i + 1;
            }
            break;
        case 3:
            if (where > .3f && where < 1.3f)
            {
                bridgeNeedUp[1] |= 1 << i;
                need = 1;
            }
            if (where > .75f && where < 1.0f && !bridgeIsUp[1])
                stop = true;
            if (where > .3f)
            {
                if (where < 1.2f && bridgeObsazen[1])
                    stop = true;
                if (where < 2.0f && !bridgeObsazen[1])
                    bridgeObsazen[1] = i + 1;
            }
            if (where > 4.0f)
            {
                if (where < 5.0f && mostObsazen[0])
                    stop = true;
                if (where < 5.5f && !mostObsazen[0])
                    mostObsazen[0] = i + 1;
            }
            break;
        }
        if ((uint)need < 2 && !need_up[need])
        {
            parnik[i].sound->SetOn(false, true);
            sound_table.ConstructSound(parnik[i].sound, SOUND_PARNIK_HORN);
            parnik[i].sound->SetLoop(false);
            parnik[i].sound->SetOn(true, true);
        }
        if (stop)
        {
            speed -= dt;
            if (speed < 0.0f)
                speed = 0.0f;
        }
        else
        {
            speed += dt;
            if (speed > 5.0f)
                speed = 5.0f;
        }
        parnik[i].speed = speed;
        // The cubic Hermite curve of the stretch from node to the next (the nodes' directions scaled by its length),
        // at pos and a hundredth on; the ship's direction is the difference, its middle is 30 behind the point
        int a = route * PARNIK_NODES + node;
        float len = uzlyDist[0][a];
        float t = pos;
        float t2 = t * t;
        float t3 = t2 * t;
        float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        float h2 = -2.0f * t3 + 3.0f * t2;
        float h3 = t3 - 2.0f * t2 + t;
        float h4 = t3 - t2;
        S_vector pt = uzly[0][a].pos * h1 + uzly[0][a + 1].pos * h2 +
            uzly[0][a].dir * len * h3 + uzly[0][a + 1].dir * len * h4;
        t += .01f;
        t2 = t * t;
        t3 = t2 * t;
        h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        h2 = -2.0f * t3 + 3.0f * t2;
        h3 = t3 - 2.0f * t2 + t;
        h4 = t3 - t2;
        S_vector pt2 = uzly[0][a].pos * h1 + uzly[0][a + 1].pos * h2 +
            uzly[0][a].dir * len * h3 + uzly[0][a + 1].dir * len * h4;
        S_vector dir;
        dir = pt2 - pt;
        dir.Normalize();
        parnik[i].model->SetPos(pt - dir * 30.0f);
        parnik[i].model->SetDir(dir, 0.0f);
        parnik[i].model->Update();
    }
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_bridge::NetDirect(uint time)
{
}

// The bridge down, its leaves' volumes taken in their space (S_BridgeCol), the two sounds between the leaves, the
// lights and the collision over the steamships' way by the frame's names; the second bridge loads the steamships'
// routes, puts the two ships on them and runs a minute of ticks, so the game starts with them under way
// FUNCTION: GAME 0x569510
void C_bridge::GameInit()
{
    C_actor::GameInit();
    blink_timer = 1;
    blink = false;
    timer = down_time;
    state = BRIDGE_DOWN;
    angle = 0.0f;
    last_angle = -1.0f;
    SetActState(ACTSTATE_ACTIVE);
    wait_time = 0;
    shut = false;
    if (!down_time)
        down_time = 1;
    if (!up_time)
        up_time = 1;
    if (!lower_time)
        lower_time = 1;
    if (!raise_time)
        raise_time = 1;
    frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), angle);
    other_frame->SetDir(S_vector(0.0f, 0.0f, 1.0f), -angle);
    frame->Update();
    other_frame->Update();
    S_matrix leaf_matrix = frame->GetMatrix();
    S_matrix leaf_inv = leaf_matrix;
    leaf_inv.Invert();
    colls.clear();
    other_colls.clear();
    std::vector<g_collision_header *> found;
    std::vector<g_collision_header *>::iterator it;
    S_BridgeCol col;
    uint i, j;
    gifCollision.FindCollsByFrame(frame, found);
    for (it = found.begin(); it < found.end(); it++)
    {
        col.vol = *it;
        switch ((*it)->flags & 0xff)
        {
        case VOLTYPE_XTOBB:
            col.inv_matrix = leaf_matrix * ((g_volume_xtobb *)*it)->inv_matrix;
            col.matrix = ((g_volume_xtobb *)*it)->matrix * leaf_inv;
            break;
        case VOLTYPE_OBB:
            col.inv_matrix = leaf_matrix * ((g_volume_obb *)*it)->inv_matrix;
            col.matrix = ((g_volume_obb *)*it)->matrix * leaf_inv;
            break;
        }
        col.vol->flags |= COLLHF_UNKNOWN_10000;
        colls.push_back(col);
    }
    leaf_matrix = other_frame->GetMatrix();
    leaf_inv = leaf_matrix;
    leaf_inv.Invert();
    gifCollision.FindCollsByFrame(other_frame, found);
    for (it = found.begin(); it < found.end(); it++)
    {
        col.vol = *it;
        switch ((*it)->flags & 0xff)
        {
        case VOLTYPE_XTOBB:
            col.inv_matrix = leaf_matrix * ((g_volume_xtobb *)*it)->inv_matrix;
            col.matrix = ((g_volume_xtobb *)*it)->matrix * leaf_inv;
            break;
        case VOLTYPE_OBB:
            col.inv_matrix = leaf_matrix * ((g_volume_obb *)*it)->inv_matrix;
            col.matrix = ((g_volume_obb *)*it)->matrix * leaf_inv;
            break;
        }
        col.vol->flags |= COLLHF_UNKNOWN_10000;
        other_colls.push_back(col);
    }
    sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    sound->SetSoundType(I3DSOUND_AMBIENT);
    sound->LinkTo(frame->GetParent(), 0);
    sound->SetPos((frame->GetPos() + other_frame->GetPos()) * 0.5f);
    sound->SetOutVol(0.0f);
    sound->SetVolume(1.0f);
    sound->SetRange(20.0f, 50.0f, 0.0f, 0.0f);
    sound->Update();
    sound->SetOn(false);
    signal_sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
    signal_sound->SetSoundType(I3DSOUND_AMBIENT);
    signal_sound->LinkTo(frame->GetParent(), 0);
    signal_sound->SetPos((other_frame->GetPos() + frame->GetPos()) * 0.5f);
    signal_sound->SetOutVol(0.0f);
    signal_sound->SetVolume(1.0f);
    signal_sound->SetRange(20.0f, 50.0f, 0.0f, 0.0f);
    signal_sound->Update();
    signal_sound->SetOn(false);
    I3D_dummy *over;
    if (*frame->GetName() == 'L')
    {
        // STRING: GAME 0x636884
        lights[0] = act_mission->GetScene()->FindFrame("watercity.RED11", ENUMF_VISUAL);
        // STRING: GAME 0x636874
        lights[1] = act_mission->GetScene()->FindFrame("watercity.RED10", ENUMF_VISUAL);
        // STRING: GAME 0x636864
        lights[2] = act_mission->GetScene()->FindFrame("watercity.RED8", ENUMF_VISUAL);
        // STRING: GAME 0x636854
        lights[3] = act_mission->GetScene()->FindFrame("watercity.RED9", ENUMF_VISUAL);
        // STRING: GAME 0x636844
        lights[4] = act_mission->GetScene()->FindFrame("watercity.RED7", ENUMF_VISUAL);
        // STRING: GAME 0x636834
        lights[5] = act_mission->GetScene()->FindFrame("watercity.RED12", ENUMF_VISUAL);
        // STRING: GAME 0x63681c
        over = (I3D_dummy *)act_mission->GetScene()->FindFrame("watercity.nadparnik1", ENUMF_DUMMY);
    }
    else
    {
        // STRING: GAME 0x63680c
        lights[0] = act_mission->GetScene()->FindFrame("watercity.RED5", ENUMF_VISUAL);
        // STRING: GAME 0x6367fc
        lights[1] = act_mission->GetScene()->FindFrame("watercity.RED4", ENUMF_VISUAL);
        // STRING: GAME 0x6367ec
        lights[2] = act_mission->GetScene()->FindFrame("watercity.RED2", ENUMF_VISUAL);
        // STRING: GAME 0x6367dc
        lights[3] = act_mission->GetScene()->FindFrame("watercity.RED3", ENUMF_VISUAL);
        // STRING: GAME 0x6367cc
        lights[4] = act_mission->GetScene()->FindFrame("watercity.RED1", ENUMF_VISUAL);
        // STRING: GAME 0x6367bc
        lights[5] = act_mission->GetScene()->FindFrame("watercity.RED6", ENUMF_VISUAL);
        // STRING: GAME 0x6367a4
        over = (I3D_dummy *)act_mission->GetScene()->FindFrame("watercity.nadparnik2", ENUMF_DUMMY);
    }
    phase = 7;
    if (over)
    {
        nadparnik_coll = new tDynamicCollObject;
        nadparnik_coll->flags = DYNCOLLF_UNKNOWN_200000;
        nadparnik_coll->owner = this;
        nadparnik_coll->frame = over;
        nadparnik_coll->matrix = &nadparnik_matrix;
        nadparnik_coll->inv_matrix = &nadparnik_inv_matrix;
        nadparnik_coll->type = DYNCOLL_UNKNOWN_40;
        (S_vector &)nadparnik_coll->bsphere = over->GetPos();
        (I3D_bbox &)nadparnik_coll->bbox = over->bbox;
        ((I3D_bsphere &)nadparnik_coll->bsphere).radius =
            (((I3D_bbox &)nadparnik_coll->bbox).max - ((I3D_bbox &)nadparnik_coll->bbox).min).Magnitude();
        nadparnik_matrix = over->GetMatrix();
        nadparnik_inv_matrix = over->GetMatrix();
        nadparnik_inv_matrix.Invert();
    }
    else
        nadparnik_coll = NULL;
    if (index)
    {
        // STRING: GAME 0x636790
        int h = dtaOpen("tables\\parnik.def", false);
        if (h >= 0)
        {
            // The two counts go into the loops' counters, which keeps those in memory; the default as LoadData's
            dtaRead(h, &i, sizeof(i));
            dtaRead(h, &j, sizeof(j));
            dtaRead(h, uzly, sizeof(uzly));
            dtaClose(h);
        }
        else
        {
            for (uint i = 0; i < PARNIK_ROUTES; i++)
            {
                for (uint j = 0; j < PARNIK_NODES; j++)
                {
                    uzly[i][j].pos = S_vector(0.0f, 0.0f, 0.0f);
                    uzly[i][j].dir = S_vector(1.0f, 0.0f, 0.0f);
                }
            }
        }
        for (i = 0; i < PARNIK_ROUTES; i++)
        {
            for (j = 0; j < PARNIK_NODES - 1; j++)
                uzlyDist[i][j] = (uzly[i][j + 1].pos - uzly[i][j].pos).Magnitude() + 1.0f;
        }
        for (j = 0; j < PARNIKS; j++)
        {
            parnik[j].node = rnd(6);
            parnik[j].last_node = 0;
            parnik[j].pos = 0.0f;
            if (j)
                parnik[j].route = 2;
            else
                parnik[0].route = 0;
            parnik[j].speed = 5.0f;
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            parnik[j].model = model;
            // STRING: GAME 0x636780
            model_cache.Open(model, "lodriver.i3d", 0, NULL, NULL, NULL);
            // STRING: GAME 0x636774
            parnik[j].model->SetName("most_parnik");
            parnik[j].model->SetOn(true);
            parnik[j].model->LinkTo(act_mission->GetScene()->GetPrimarySector(), 0);
            parnik[j].model->Update();
            parnik[j].leonardo = NULL;
            parnik[j].kate = NULL;
            parnik[j].sound = (I3D_sound *)driver->CreateFrame(FRAME_SOUND);
            parnik[j].sound->SetSoundType(I3DSOUND_POINT);
            parnik[j].sound->LinkTo(parnik[j].model, 0);
            parnik[j].sound->SetOutVol(0.0f);
            parnik[j].sound->SetVolume(1.0f);
            parnik[j].sound->SetRange(20.0f, 50.0f, 0.0f, 0.0f);
            parnik[j].sound->Update();
            parnik[j].sound->SetOn(false);
        }
        memset(mostObsazen, 0, 2 * sizeof(int));
        memset(bridgeIsUp, 0, sizeof(bridgeIsUp));
        memset(bridgeObsazen, 0, sizeof(bridgeObsazen));
        memset(bridgeNeedUp, 0, sizeof(bridgeNeedUp));
        for (int k = 0; k < 60; k++)
        {
            AI(1000);
            Update(1000);
        }
    }
}

// FUNCTION: GAME 0x56a600
void C_bridge::GameDone()
{
    if (nadparnik_coll)
    {
        gifCollision.DynErase(nadparnik_coll);
        delete nadparnik_coll;
    }
    C_actor::GameDone();
    if (sound)
    {
        sound->Release();
        sound = NULL;
    }
    if (signal_sound)
    {
        signal_sound->Release();
        signal_sound = NULL;
    }
    if (index)
    {
        for (uint i = 0; i < PARNIKS; i++)
        {
            parnik[i].model->Release();
            if (parnik[i].leonardo)
                parnik[i].leonardo->Release();
            if (parnik[i].kate)
                parnik[i].kate->Release();
        }
    }
    globalFirstBridge = true;
}

// The two counts before the routes aren't used; without the file each node is at the origin, facing x
// FUNCTION: GAME 0x56a6b0
bool C_bridge::LoadData(C_chunk *chunk)
{
    int h = dtaOpen("tables\\parnik.def", false);
    if (h >= 0)
    {
        int routes, nodes;
        dtaRead(h, &routes, sizeof(routes));
        dtaRead(h, &nodes, sizeof(nodes));
        dtaRead(h, uzly, sizeof(uzly));
        dtaClose(h);
        return true;
    }
    for (uint i = 0; i < PARNIK_ROUTES; i++)
    {
        for (uint j = 0; j < PARNIK_NODES; j++)
        {
            uzly[i][j].pos = S_vector(0.0f, 0.0f, 0.0f);
            uzly[i][j].dir = S_vector(1.0f, 0.0f, 0.0f);
        }
    }
    return true;
}

// The linker's folded copy of the out-of-line kind (0x4aa430): none of what SaveGameSave writes
int C_bridge::SaveGameGetSize()
{
    return 0;
}

// The state and its timer, into the buffer without moving it on
// FUNCTION: GAME 0x56a790
bool C_bridge::SaveGameSave(void **buf)
{
    int *p = (int *)*buf;
    p[0] = state;
    p[1] = timer;
    return true;
}

// FUNCTION: GAME 0x56a7b0
void C_bridge::ShutDown(bool shut)
{
    this->shut = shut;
    if (nadparnik_coll)
        gifCollision.DynErase(nadparnik_coll);
}

// C_human::MovementIntern passes the human's new position, his direction turned back, his direction and his mass
// FUNCTION: GAME 0x56a7e0
bool C_bridge::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    if (act && act->type == ACTOR_PLAYER)
    {
        ((C_human *)act)->properties[HUMAN_PROPERTY_ENERGY] = 0.0f;
        act->Hit(HIT_LANDING, v2, v1, -v2, 1.0f, NULL, 5, NULL);
    }
    return false;
}

// "I'm the king of the world": Leonardo and Kate on each steamship's bow
// FUNCTION: GAME 0x56a840
void C_bridge::SetTitanic(bool on)
{
    if (!index)
        return;
    for (int i = 0; i < PARNIKS; i++)
    {
        S_parnik *p = &parnik[i];
        if (on)
        {
            if (!p->leonardo)
            {
                p->leonardo = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                model_cache.Open(p->leonardo, "Tommy.i3d", 0, NULL, NULL, NULL);
                // STRING: GAME 0x63689c
                p->leonardo->SetName("Leonardo");
                p->leonardo->SetOn(true);
                p->leonardo->LinkTo(p->model, 0);
                p->leonardo->SetPos(0.0f, 4.7f, 37.0f);
                p->leonardo->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
                p->leonardo->Update();
            }
            if (!p->kate)
            {
                p->kate = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
                // STRING: GAME 0x634a38
                model_cache.Open(p->kate, "czena06.i3d", 0, NULL, NULL, NULL);
                // STRING: GAME 0x636894
                p->kate->SetName("Kate");
                p->kate->SetOn(true);
                p->kate->LinkTo(p->model, 0);
                p->kate->SetPos(0.0f, 4.7f, 37.4f);
                p->kate->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
                p->kate->Update();
            }
        }
        else
        {
            if (p->leonardo)
                p->leonardo->Release();
            if (p->kate)
                p->kate->Release();
            p->leonardo = NULL;
            p->kate = NULL;
        }
    }
}
