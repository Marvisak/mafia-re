// The cars' road network (PS2 road_web.cpp; the PC's file is the one at 0x559cd0-0x55cf3c, which starts with the
// constructor, docs/game-files.md): the crossings and the nodes between them, read from the mission's road.bin, and
// the paths the traffic and the chases drive along them.

#include "ai/road_web.h"

#include "C_chunk.h"
#include "actors/C_actor.h"
#include "ai/C_driver.h"

// FUNCTION: GAME 0x559cd0
C_roads::C_roads()
{
}

// FUNCTION: GAME 0x559d00
C_roads::~C_roads()
{
}

// Writes road.bin: its version, then the crossings' and the nodes' counts and arrays
// FUNCTION: GAME 0x559d40
bool C_roads::SaveData(C_chunk *chunk)
{
    int version = ROAD_VERSION;
    chunk->Write(&version, sizeof(version));
    int count = crossings.size();
    chunk->Write(&count, sizeof(count));
    if (count > 0)
        chunk->Write(&crossings[0], count * sizeof(S_road_crossing));
    count = nodes.size();
    chunk->Write(&count, sizeof(count));
    if (count > 0)
        chunk->Write(&nodes[0], count * sizeof(S_road_node));
    return true;
}

// Reads road.bin: the crossings and the nodes straight into the vectors, the speeds within 1.0 of 16.666 (60 km/h) made
// 13.3, then drops the roads without lanes and recompiles the distances
// FUNCTION: GAME 0x559e60
bool C_roads::LoadData(C_chunk *chunk)
{
    int version;
    chunk->Read(&version, sizeof(version));
    if (version != ROAD_VERSION)
        return false;
    crossings.clear();
    nodes.clear();
    int count;
    chunk->Read(&count, sizeof(count));
    if (count > 0)
    {
        S_road_crossing zero_crossing;
        memset(&zero_crossing, 0, sizeof(zero_crossing));
        crossings.resize(count, zero_crossing);
        chunk->Read(&crossings[0], count * sizeof(S_road_crossing));
    }
    chunk->Read(&count, sizeof(count));
    if (count > 0)
    {
        S_road_node zero_node;
        memset(&zero_node, 0, sizeof(zero_node));
        nodes.resize(count, zero_node);
        chunk->Read(&nodes[0], count * sizeof(S_road_node));
    }
    count = crossings.size();
    while (count--)
        crossings[count].speed = fabs(crossings[count].speed - 16.666f) < 1.0 ? 13.3f : crossings[count].speed;
    count = nodes.size();
    while (count--)
        nodes[count].speed = fabs(nodes[count].speed - 16.666f) < 1.0 ? 13.3f : nodes[count].speed;
    DestroyAllOffPaths();
    RecompileDistances();
    return true;
}

// Reads a road.bin like LoadData but appends it to the network: the crossings and nodes it holds are kept, the new
// ones' indices shifted past them (the name is ours)
// FUNCTION: GAME 0x55a210
bool C_roads::MergeData(C_chunk *chunk)
{
    int version;
    chunk->Read(&version, sizeof(version));
    if (version != ROAD_VERSION)
        return false;
    int count;
    int old_crossings = crossings.size();
    int old_nodes = nodes.size();
    chunk->Read(&count, sizeof(count));
    if (count > 0)
    {
        S_road_crossing zero_crossing;
        memset(&zero_crossing, 0, sizeof(zero_crossing));
        crossings.resize(old_crossings + count, zero_crossing);
        chunk->Read(&crossings[old_crossings], count * sizeof(S_road_crossing));
    }
    chunk->Read(&count, sizeof(count));
    if (count > 0)
    {
        S_road_node zero_node;
        memset(&zero_node, 0, sizeof(zero_node));
        nodes.resize(old_nodes + count, zero_node);
        chunk->Read(&nodes[old_nodes], count * sizeof(S_road_node));
    }
    // The connections are indices of the file's own network: shift them past what was there
    if (old_crossings)
        for (count = old_crossings; count < crossings.size(); count++)
            for (int j = 0; j < 4; j++)
            {
                if (crossings[count].conns[j] & NEARNODE_FLAG)
                    crossings[count].conns[j] += old_nodes;
                else
                    crossings[count].conns[j] += old_crossings;
            }
    if (old_nodes)
        for (count = old_nodes; count < nodes.size(); count++)
            for (int j = 0; j < 2; j++)
            {
                if (nodes[count].conns[j] & NEARNODE_FLAG)
                    nodes[count].conns[j] += old_nodes;
                else
                    nodes[count].conns[j] += old_crossings;
            }
    return true;
}

// Recomputes the network's distances: each node's end crossings and each crossing's roads (the crossing at the end,
// the length), then orders the roads around each crossing by angle and gives each its angle to the previous one
// FUNCTION: GAME 0x55a620
void C_roads::RecompileDistances()
{
    int i, j;
    for (i = nodes.size(); i--; )
    {
        nodes[i].cross0 = NEARNODE_NONE;
        nodes[i].cross1 = NEARNODE_NONE;
    }
    // Along each road from a crossing: the nodes' crossings at the ends, the road's length
    for (i = crossings.size(); i--; )
    {
        for (j = 4; j--; )
        {
            if (crossings[i].conns[j] == NEARNODE_NONE || crossings[i].lanes[j].cross == NEARNODE_NONE)
                continue;
            std::vector<int> path;
            ushort cur = crossings[i].conns[j];
            ushort prev = i;
            ushort last = NEARNODE_NONE;
            int idx = cur & NEARNODE_INDEX;
            float dist = (crossings[i].pos - nodes[idx].pos).Magnitude();
            while (true)
            {
                if (cur & NEARNODE_FLAG)
                {
                    nodes[idx].cross0 = i;
                    path.push_back(idx);
                }
                ushort next = nodes[idx].conns[0];
                if (next == prev || next == NEARNODE_NONE)
                    next = nodes[idx].conns[1];
                if (next == prev || next == NEARNODE_NONE)
                    break;
                prev = cur;
                if (next & NEARNODE_FLAG)
                {
                    idx = next & NEARNODE_INDEX;
                    dist += (nodes[idx].pos - nodes[cur & NEARNODE_INDEX].pos).Magnitude();
                    cur = next;
                    continue;
                }
                dist += (nodes[cur & NEARNODE_INDEX].pos - crossings[next].pos).Magnitude();
                last = next;
                for (uint k = path.size(); k > 0; k--)
                    nodes[path[k - 1]].cross1 = next;
                break;
            }
            crossings[i].lanes[j].distance = dist;
            crossings[i].lanes[j].cross = last;
        }
    }
    // Each crossing's roads: the used ones first, in the order of their angle round the first
    for (i = crossings.size(); i--; )
    {
        S_road_lanes tl[4];
        ushort tc[4];
        tl[0] = crossings[i].lanes[0];
        tc[0] = crossings[i].conns[0];
        tl[1] = crossings[i].lanes[1];
        tc[1] = crossings[i].conns[1];
        tl[2] = crossings[i].lanes[2];
        tc[2] = crossings[i].conns[2];
        tl[3] = crossings[i].lanes[3];
        tc[3] = crossings[i].conns[3];
        int front = 0, back = 3;
        for (j = 0; j < 4; j++)
        {
            if (tl[j].cross != NEARNODE_NONE)
            {
                crossings[i].lanes[front] = tl[j];
                crossings[i].conns[front] = tc[j];
                front++;
            }
            else
            {
                crossings[i].lanes[back] = tl[j];
                crossings[i].conns[back] = tc[j];
                back--;
            }
        }
        if (crossings[i].conns[0] == NEARNODE_NONE)
            continue;
        S_vector v;
        S_vector2 ref, cur;
        S_road_node &n0 = nodes[crossings[i].conns[0] & NEARNODE_INDEX];
        if (n0.conns[0] != NEARNODE_NONE && n0.conns[1] != NEARNODE_NONE)
        {
            if (n0.conns[0] != i)
                v = n0.pos - nodes[n0.conns[0] & NEARNODE_INDEX].pos;
            else
                v = n0.pos - nodes[n0.conns[1] & NEARNODE_INDEX].pos;
        }
        else
            v = crossings[i].pos - n0.pos;
        ref = S_vector2(v.x, v.z);
        float a[3];
        ushort c;
        for (j = 1; j < 4; j++)
        {
            if (crossings[i].conns[j] == NEARNODE_NONE)
                break;
            S_road_node &n = nodes[crossings[i].conns[j] & NEARNODE_INDEX];
            if (n.conns[0] != NEARNODE_NONE && n.conns[1] != NEARNODE_NONE)
            {
                if (n.conns[0] != i)
                    v = nodes[n.conns[0] & NEARNODE_INDEX].pos - n.pos;
                else
                    v = nodes[n.conns[1] & NEARNODE_INDEX].pos - n.pos;
            }
            else
                v = n.pos - crossings[i].pos;
            cur = S_vector2(v.x, v.z);
            a[j - 1] = AngleTo2(cur, ref);
        }
        if (j > 2)
        {
            if (j > 3)
            {
                if (a[0] > a[1])
                {
                    if (a[0] > a[2])
                    {
                        if (a[1] > a[2])
                        {
                            tl[0] = crossings[i].lanes[3];
                            tc[0] = crossings[i].conns[3];
                            tl[1] = crossings[i].lanes[2];
                            tc[1] = crossings[i].conns[2];
                            tl[2] = crossings[i].lanes[1];
                            c = crossings[i].conns[1];
                        }
                        else
                        {
                            tl[0] = crossings[i].lanes[2];
                            tc[0] = crossings[i].conns[2];
                            tl[1] = crossings[i].lanes[3];
                            tc[1] = crossings[i].conns[3];
                            tl[2] = crossings[i].lanes[1];
                            c = crossings[i].conns[1];
                        }
                    }
                    else
                    {
                        tl[0] = crossings[i].lanes[2];
                        tc[0] = crossings[i].conns[2];
                        tl[1] = crossings[i].lanes[1];
                        tc[1] = crossings[i].conns[1];
                        tl[2] = crossings[i].lanes[3];
                        c = crossings[i].conns[3];
                    }
                    crossings[i].lanes[1] = tl[0];
                    crossings[i].conns[1] = tc[0];
                    crossings[i].lanes[2] = tl[1];
                    crossings[i].conns[2] = tc[1];
                    crossings[i].lanes[3] = tl[2];
                    crossings[i].conns[3] = c;
                }
                else if (a[1] > a[2])
                {
                    if (a[0] > a[2])
                    {
                        tl[0] = crossings[i].lanes[3];
                        tc[0] = crossings[i].conns[3];
                        tl[1] = crossings[i].lanes[1];
                        tc[1] = crossings[i].conns[1];
                        tl[2] = crossings[i].lanes[2];
                        c = crossings[i].conns[2];
                    }
                    else
                    {
                        tl[0] = crossings[i].lanes[3];
                        tc[0] = crossings[i].conns[3];
                        tl[1] = crossings[i].lanes[2];
                        tc[1] = crossings[i].conns[2];
                        tl[2] = crossings[i].lanes[1];
                        c = crossings[i].conns[1];
                    }
                    crossings[i].lanes[1] = tl[0];
                    crossings[i].conns[1] = tc[0];
                    crossings[i].lanes[2] = tl[1];
                    crossings[i].conns[2] = tc[1];
                    crossings[i].lanes[3] = tl[2];
                    crossings[i].conns[3] = c;
                }
            }
            else if (a[0] > a[1])
            {
                tl[0] = crossings[i].lanes[2];
                tc[0] = crossings[i].conns[2];
                crossings[i].lanes[2] = crossings[i].lanes[1];
                crossings[i].conns[2] = crossings[i].conns[1];
                crossings[i].lanes[1] = tl[0];
                crossings[i].conns[1] = tc[0];
            }
        }
        // Each road's angle from the one before it
        crossings[i].lanes[0].angle = 0.0f;
        float angle, prevangle = 0.0f;
        for (j = 1; j < 4; j++)
        {
            if (crossings[i].conns[j] == NEARNODE_NONE)
                break;
            S_road_node &n = nodes[crossings[i].conns[j] & NEARNODE_INDEX];
            if (n.conns[0] != NEARNODE_NONE && n.conns[1] != NEARNODE_NONE)
            {
                if (n.conns[0] != i)
                    v = nodes[n.conns[0] & NEARNODE_INDEX].pos - n.pos;
                else
                    v = nodes[n.conns[1] & NEARNODE_INDEX].pos - n.pos;
            }
            else
                v = n.pos - crossings[i].pos;
            cur = S_vector2(v.x, v.z);
            angle = AngleTo2(cur, ref);
            crossings[i].lanes[j].angle = angle + PI - prevangle;
            prevangle = angle + PI;
        }
        crossings[i].lanes[0].angle = PI * 2.0f - prevangle;
    }
}

// The node nearest to a position: a binary search on the nodes' x, then a scan down and up from there until a node's x
// alone is farther than the best squared distance so far. An index of nodes, without NEARNODE_FLAG; none where there
// are no nodes.
// FUNCTION: GAME 0x55b370
ushort C_roads::GetNearNode(S_vector pos)
{
    if (nodes.size() != 0)
    {
        ushort lo = 0;
        ushort hi = nodes.size() - 1;
        ushort mid;
        if (pos.x < nodes[0].pos.x)
            mid = 0;
        else if (pos.x > nodes[hi].pos.x)
            mid = hi;
        else
        {
            do
            {
                mid = (lo + hi) / 2;
                if (pos.x < nodes[mid].pos.x)
                    hi = mid;
                else
                    lo = mid;
            } while (hi - lo > 1);
        }
        float best = 3.4e38f;
        ushort nearest = NEARNODE_NONE;
        ushort j = mid;
        while (mid != NEARNODE_NONE)
        {
            S_road_node &n = nodes[mid];
            float dy = pos.y - n.pos.y;
            float dx = pos.x - n.pos.x;
            if (dx * dx > best)
                break;
            float dz = pos.z - n.pos.z;
            float dist = dx * dx + dy * dy + dz * dz;
            if (dist < best)
            {
                best = dist;
                nearest = mid;
            }
            mid--;
        }
        while (++j < nodes.size())
        {
            S_road_node &n = nodes[j];
            float dy = pos.y - n.pos.y;
            float dx = n.pos.x - pos.x;
            if (dx * dx > best)
                break;
            float dz = n.pos.z - pos.z;
            float dist = dx * dx + dy * dy + dz * dz;
            if (dist < best)
            {
                best = dist;
                nearest = j;
            }
        }
        return nearest;
    }
    return NEARNODE_NONE;
}

// The crossing a node's road leads to, walking along it away from the index from: a crossing is its own, none
// where the road ends
// FUNCTION: GAME 0x55b570
ushort C_roads::FindCrossing(ushort node, ushort from)
{
    if (!(node & NEARNODE_FLAG))
        return node;
    if (node == NEARNODE_NONE || from == NEARNODE_NONE)
        return NEARNODE_NONE;
    while (true)
    {
        S_road_node &n = nodes[node & NEARNODE_INDEX];
        ushort next = n.conns[0];
        if (next == from)
            next = n.conns[1];
        if (next == NEARNODE_NONE)
            return NEARNODE_NONE;
        if (!(next & NEARNODE_FLAG))
            return next;
        from = node;
        node = next;
    }
}

// Which of a crossing's roads leads to the crossing to, -1 for none. With several, a node other than none picks the
// shortest one; none walks each road's chain (the comparison with node there is the original's, it never holds)
// FUNCTION: GAME 0x55b5d0
int C_roads::NPC_FindCrossIdxToCross(ushort cross, ushort to, ushort node)
{
    int found = -1;
    int count = 0;
    for (int i = 0; i < 4; i++)
    {
        if (crossings[cross].lanes[i].cross == to)
        {
            found = i;
            count++;
        }
    }
    if (count > 1)
    {
        if (node != NEARNODE_NONE)
        {
            float best = FLT_MAX;
            for (int i = 0; i < 4; i++)
            {
                if (crossings[cross].lanes[i].cross == to && crossings[cross].lanes[i].distance < best)
                {
                    best = crossings[cross].lanes[i].distance;
                    found = i;
                }
            }
            return found;
        }
        for (int i = 0; i < 4; i++)
        {
            if (crossings[cross].lanes[i].cross != to)
                continue;
            ushort prev = cross;
            ushort cur = crossings[cross].conns[i];
            while (cur != NEARNODE_NONE)
            {
                if (cur == node)
                    return i;
                if (!(cur & NEARNODE_FLAG))
                    break;
                S_road_node &n = nodes[cur & NEARNODE_INDEX];
                ushort next = n.conns[0];
                if (next == prev)
                    next = n.conns[1];
                prev = cur;
                cur = next;
            }
        }
        return found;
    }
    return found;
}

// A traffic car's path from a node of a road to the crossing at one of its ends, which one the car's heading (a unit
// vector, the traffic's) says: it empties path, then fills it with the node, the road's nodes up to that crossing, and
// the crossing, all of them with the lanes of the road in that direction. Fails (path left empty) without a node, where
// either end of the road leads nowhere (no crossing, FindCrossing) or where the crossings have no road to each other.
// FUNCTION: GAME 0x55b710
bool C_roads::NPC_FindPathFromNodeToCross(ushort node, S_vector heading, std::vector<S_car_node> *path)
{
    path->clear();
    if (node == NEARNODE_NONE)
        return false;
    int node_index = node;
    ushort flagged = node | NEARNODE_FLAG;
    // The crossings at the road's ends: cross[0] is the one reached walking away from conns[0] (towards conns[1]),
    // cross[1] the other. idx[i] is the connection of cross[i] along which the road leads to the other crossing, or -1.
    ushort cross[2];
    cross[0] = FindCrossing(flagged, nodes[node_index].conns[0]);
    cross[1] = FindCrossing(flagged, nodes[node_index].conns[1]);
    if (cross[0] == NEARNODE_NONE || cross[1] == NEARNODE_NONE)
        return false;
    int idx[2];
    idx[0] = NPC_FindCrossIdxToCross(cross[0], cross[1], node);
    idx[1] = NPC_FindCrossIdxToCross(cross[1], cross[0], node);
    // The crossing to drive to: the only one with a road, else the end the heading points to
    int dir = -1;
    if (idx[0] != -1)
    {
        if (idx[1] != -1)
        {
            S_road_node &from = nodes[node_index & NEARNODE_INDEX];
            S_vector d0, d1;
            if (from.conns[0] & NEARNODE_FLAG)
                d0 = nodes[from.conns[0] & NEARNODE_INDEX].pos - from.pos;
            else
                d0 = crossings[from.conns[0]].pos - from.pos;
            if (from.conns[1] & NEARNODE_FLAG)
                d1 = nodes[from.conns[1] & NEARNODE_INDEX].pos - from.pos;
            else
                d1 = crossings[from.conns[1]].pos - from.pos;
            float a0 = heading.AngleTo(d0);
            float a1 = heading.AngleTo(d1);
            if (a1 < a0)
                dir = 0;
            else
                dir = 1;
        }
        else
            dir = 0;
    }
    else if (idx[1] != -1)
        dir = 1;
    int other = dir == 0;
    if (dir != -1)
    {
        // The nodes between that crossing and the node, from the crossing on
        std::vector<ushort> list;
        ushort cur = crossings[cross[dir]].conns[idx[dir]];
        ushort prev = cross[dir];
        list.push_back(prev);
        while ((cur & NEARNODE_INDEX) != node)
        {
            list.push_back(cur);
            ushort next;
            if (nodes[cur & NEARNODE_INDEX].conns[0] == prev || !(nodes[cur & NEARNODE_INDEX].conns[0] & NEARNODE_FLAG))
                next = nodes[cur & NEARNODE_INDEX].conns[1];
            else
                next = nodes[cur & NEARNODE_INDEX].conns[0];
            prev = cur;
            cur = next;
        }
        S_car_node n;
        memset(&n, 0, sizeof(n));
        n.road_node = flagged;
        n.lanes = &crossings[cross[other]].lanes[idx[other]];
        path->push_back(n);
        for (int i = list.size(); i != 0; )
        {
            n.road_node = list[--i];
            path->push_back(n);
        }
        return true;
    }
    return false;
}

// Continues a traffic car's path from a crossing along its connection dir: the last node of path takes the road's
// lanes, and the road's nodes follow up to the crossing at its end, which is the last one. Fails where the connection
// leads nowhere. path mustn't be empty.
// FUNCTION: GAME 0x55bad0
bool C_roads::NPC_FindPathToNextCross1(ushort cross, int dir, std::vector<S_car_node> *path)
{
    ushort cur = crossings[cross].conns[dir];
    ushort prev = cross;
    (*path)[path->size() - 1].lanes = &crossings[cross].lanes[dir];
    while (true)
    {
        S_car_node n;
        memset(&n, 0, sizeof(n));
        if (cur == NEARNODE_NONE)
            return false;
        n.road_node = cur;
        n.lanes = &crossings[cross].lanes[dir];
        path->push_back(n);
        if (!(cur & NEARNODE_FLAG))
            return true;
        S_road_node &node = nodes[cur & NEARNODE_INDEX];
        ushort next = node.conns[0];
        if (next == prev)
            next = node.conns[1];
        prev = cur;
        cur = next;
    }
}

// Appends node and the indices after it along its road to path, walking from node through next away from node, up to
// the crossing it reaches or end; path starts again where the walk meets start
// FUNCTION: GAME 0x55bc10
void C_roads::FillPathFromNodeToCross(ushort node, ushort next, std::vector<ushort> &path, ushort start, ushort end)
{
    ushort prev = node;
    ushort index = next;
    path.push_back(node);
    while (index != NEARNODE_NONE)
    {
        path.push_back(index);
        if (index == start)
            path.clear();
        if (index == end)
            return;
        if (!(index & NEARNODE_FLAG))
            return;
        S_road_node &n = nodes[index & NEARNODE_INDEX];
        ushort onward = n.conns[0];
        if (onward == prev)
            onward = n.conns[1];
        prev = index;
        index = onward;
    }
}

// The crossing the road from node leads to through from, none where it ends, and the squared length of the way there
// (the sum of the squares of each step's x, z and y offsets) in distance
// FUNCTION: GAME 0x55bcd0
ushort C_roads::FindCrossingFromNode(ushort node, ushort from, float &distance)
{
    distance = 0.0f;
    ushort cur = node;
    while (from != NEARNODE_NONE)
    {
        S_vector a = GetPos(cur);
        S_vector b = GetPos(from);
        S_vector d = b - a;
        distance += d.y * d.y + d.x * d.x + d.z * d.z;
        if (!(from & NEARNODE_FLAG))
            return from;
        S_road_node &n = nodes[from & NEARNODE_INDEX];
        ushort next = n.conns[0];
        if (next == cur)
            next = n.conns[1];
        cur = from;
        from = next;
    }
    return NEARNODE_NONE;
}

// The crossing nearest to a node along its road, whichever of its two directions has the shorter way (dir the index
// of its conns). A direction that its crossing has no road back along (NPC_FindCrossIdxToCross: -1) is dropped; back
// swaps which of the two roads is checked for it. None where neither is left.
// FUNCTION: GAME 0x55be10
ushort C_roads::FindNearCrossingFromNode(ushort node, int &dir, bool back)
{
    S_road_node &n = nodes[node & NEARNODE_INDEX];
    float dist1, dist0;
    ushort cross0 = FindCrossingFromNode(node, n.conns[0], dist0);
    ushort cross1 = FindCrossingFromNode(node, n.conns[1], dist1);
    if (cross0 == NEARNODE_NONE)
    {
        if (cross1 == NEARNODE_NONE)
            return NEARNODE_NONE;
        dir = 1;
        return cross1;
    }
    if (cross1 == NEARNODE_NONE)
    {
        dir = 0;
        return cross0;
    }
    int idx0 = NPC_FindCrossIdxToCross(cross1, cross0, node);
    int idx1 = NPC_FindCrossIdxToCross(cross0, cross1, node);
    if (back)
    {
        int t = idx0;
        idx0 = idx1;
        idx1 = t;
    }
    if (idx0 == -1)
    {
        if (idx1 == -1)
            return NEARNODE_NONE;
        dir = 1;
        return cross1;
    }
    if (idx1 == -1)
    {
        dir = 0;
        return cross0;
    }
    if (dist1 < dist0)
    {
        dir = 1;
        return cross1;
    }
    dir = 0;
    return cross0;
}

// The crossings from one crossing to another (PS2 name, typo included): a search outward from from, one ring of
// crossings at a time, that keeps each crossing's shortest distance and the crossing it came from
// FUNCTION: GAME 0x55bf60
bool C_roads::FindPathBeweenCross(ushort from, ushort to, std::vector<ushort> &path)
{
    struct S_pathfind
    {
        ushort from;
        float distance;
        // FUNCTION: GAME 0x55c430
        S_pathfind()
        {
            from = NEARNODE_NONE;
            distance = 1e10f;
        }
    };

    if (from == to || from == NEARNODE_NONE || to == NEARNODE_NONE)
        return false;
    if (!crossings.size())
        return false;
    S_pathfind *pathfind = new S_pathfind[crossings.size()];
    std::vector<uint> list1;
    std::vector<uint> list2;
    std::vector<uint> *cur = &list1;
    std::vector<uint> *next = &list2;
    cur->push_back(from);
    float best = 1e10f;
    pathfind[from].distance = 0.0f;

    while (cur->size())
    {
        for (uint i = 0; i < cur->size(); i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (crossings[(*cur)[i]].lanes[j].cross == NEARNODE_NONE)
                    continue;
                float distance = pathfind[(*cur)[i]].distance + crossings[(*cur)[i]].lanes[j].distance;
                if (!(distance >= best) && !(distance >= pathfind[crossings[(*cur)[i]].lanes[j].cross].distance))
                {
                    bool is_new = true;
                    for (uint k = 0; k < next->size(); k++)
                    {
                        if ((*next)[k] == crossings[(*cur)[i]].lanes[j].cross)
                        {
                            is_new = false;
                            break;
                        }
                    }
                    if (is_new)
                        next->push_back(crossings[(*cur)[i]].lanes[j].cross);
                    pathfind[crossings[(*cur)[i]].lanes[j].cross].distance = distance;
                    pathfind[crossings[(*cur)[i]].lanes[j].cross].from = (*cur)[i];
                    if (crossings[(*cur)[i]].lanes[j].cross == to)
                    {
                        best = distance;
                        break;
                    }
                }
            }
        }
        std::vector<uint> *t = cur;
        cur = next;
        next = t;
        t->clear();
    }

    if (pathfind[to].from == NEARNODE_NONE)
    {
        delete[] pathfind;
        return false;
    }

    cur->clear();
    cur->push_back(to);
    uint n = to;
    while (n != from)
    {
        cur->push_back(pathfind[n].from);
        n = pathfind[n].from;
    }
    path.clear();
    for (int i = cur->size() - 1; i > -1; i--)
        path.push_back((*cur)[i]);
    delete[] pathfind;
    return true;
}

// Appends the indices from index along its road, away from the index prev, up to the crossing it reaches; the path
// starts again at start, and false where it reaches end
// FUNCTION: GAME 0x55c440
bool C_roads::FillPathFromCrossToCross(ushort cross, ushort first, std::vector<ushort> &path, ushort start, ushort end)
{
    ushort prev = cross;
    ushort index = first;
    while (index != NEARNODE_NONE)
    {
        if (index == start)
            path.clear();
        path.push_back(index);
        if (index == end)
            return false;
        if (!(index & NEARNODE_FLAG))
            return true;
        S_road_node &n = nodes[index & NEARNODE_INDEX];
        ushort next = n.conns[0];
        if (next == prev)
            next = n.conns[1];
        prev = index;
        index = next;
    }
    return true;
}

// The nodes from one index to another: each end that is a node (NEARNODE_FLAG) first goes to its nearest crossing,
// the crossings between come from FindPathBeweenCross, and each hop is filled with the nodes along its road
// FUNCTION: GAME 0x55c4e0
bool C_roads::FindPath(ushort from, ushort to, std::vector<ushort> &path)
{
    if (from == NEARNODE_NONE || to == NEARNODE_NONE)
        return false;
    path.clear();
    ushort from_cross = from;
    int from_dir;
    ushort from_node = from & NEARNODE_FLAG;
    if (from_node)
    {
        from_cross = FindNearCrossingFromNode(from, from_dir, false);
        if (from_cross == NEARNODE_NONE)
            return false;
    }
    ushort to_cross = to;
    int to_dir;
    ushort to_node = to & NEARNODE_FLAG;
    if (to_node)
    {
        to_cross = FindNearCrossingFromNode(to, to_dir, true);
        if (to_cross == NEARNODE_NONE)
            return false;
    }
    if (from_node)
        FillPathFromNodeToCross(from, nodes[from & NEARNODE_INDEX].conns[from_dir], path, NEARNODE_NONE, to);
    else
        path.push_back(from);
    if (from_cross != to_cross)
    {
        float distance;
        ushort a = FindCrossingFromNode(from, nodes[from & NEARNODE_INDEX].conns[from_dir == 0], distance);
        int to_other = to_dir == 0;
        ushort b = FindCrossingFromNode(to, nodes[to & NEARNODE_INDEX].conns[to_other], distance);
        if (a == to_cross && b == from_cross)
        {
            to_cross = from_cross;
            to_dir = to_other;
        }
    }
    bool ok = true;
    if (from_cross != to_cross)
    {
        std::vector<ushort> cross_path;
        if (FindPathBeweenCross(from_cross, to_cross, cross_path))
        {
            for (uint i = 1; i < cross_path.size(); i++)
            {
                ushort cross = cross_path[i - 1];
                int idx = NPC_FindCrossIdxToCross(cross, cross_path[i], NEARNODE_NONE);
                if (idx < 0)
                    return false;
                if (!FillPathFromCrossToCross(cross, crossings[cross].conns[idx], path, from, to))
                {
                    ok = false;
                    break;
                }
            }
        }
        else
            return false;
    }
    if (ok && (path.empty() || path.back() != to) && to_node)
    {
        std::vector<ushort> node_path;
        FillPathFromNodeToCross(to, nodes[to & NEARNODE_INDEX].conns[to_dir], node_path, NEARNODE_NONE,
            NEARNODE_NONE);
        for (int i = node_path.size() - 1; i != 0;)
        {
            i--;
            if (node_path[i] == from)
                path.clear();
            path.push_back(node_path[i]);
        }
    }
    return true;
}

// Selection sort of the nodes by x; each swap renumbers the roads' references to the two nodes
// FUNCTION: GAME 0x55c890
void C_roads::SortNodesByX()
{
    if (IsSorted())
        return;
    for (uint i = 0; i < nodes.size(); i++)
    {
        uint min = i;
        for (uint j = i + 1; j < nodes.size(); j++)
        {
            if (nodes[j].pos.x < nodes[min].pos.x)
                min = j;
        }
        if (min == i)
            continue;
        ushort a = i | NEARNODE_FLAG;
        ushort b = min | NEARNODE_FLAG;
        for (int n = nodes.size(); n--;)
        {
            for (int e = 2; e--;)
            {
                ushort &end = nodes[n].conns[e];
                if (end == a)
                    end = b;
                else if (end == b)
                    end = a;
            }
        }
        for (int c = crossings.size(); c--;)
        {
            for (int e = 4; e--;)
            {
                ushort &conn = crossings[c].conns[e];
                if (conn == a)
                    conn = b;
                else if (conn == b)
                    conn = a;
            }
        }
        S_road_node *tmp = new S_road_node;
        *tmp = nodes[i];
        nodes[i] = nodes[min];
        nodes[min] = *tmp;
        delete tmp;
    }
}

// Whether the nodes are sorted by x, as GetNearNode's search needs them
// FUNCTION: GAME 0x55caa0
bool C_roads::IsSorted()
{
    for (uint i = 1; i < nodes.size(); i++)
    {
        if (nodes[i].pos.x < nodes[i - 1].pos.x)
            return false;
    }
    return true;
}

// Drops the roads none of whose lanes can be driven (all of type 0): their crossing's connection leads nowhere
// FUNCTION: GAME 0x55cb00
void C_roads::DestroyAllOffPaths()
{
    int i = crossings.size();
    while (i--)
    {
        int j = 4;
        while (j--)
        {
            if (crossings[i].lanes[j].cross != NEARNODE_NONE)
            {
                bool off = true;
                int k = 4;
                while (k--)
                {
                    if (crossings[i].lanes[j].lane[k].type)
                    {
                        off = false;
                        break;
                    }
                }
                if (off)
                    crossings[i].lanes[j].cross = NEARNODE_NONE;
            }
        }
    }
}
