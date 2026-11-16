// The walking paths' web (PS2 pathweb.cpp; the PC's file is the one at 0x549f90-0x54d31c, which starts with the node's
// destructor, docs/game-files.md), written whole: PS2's functions in PS2's order, as far as the PC has them. Not on
// the PC: PS2's help memory (AllocHelpMem, FreeHelpMem, GetNextHlpMemBlock), the node's ClearConn and the editor's
// SaveNodes; the path's ClearNode is inline here. The free functions _STDVECTCOPY and nodeproc serve
// FindPathCompFuncJIPO.

#include "ai/pathweb.h"

#include "C_chunk.h"
#include "mission/Mission.h"
#include "ai/road_web.h"

// FUNCTION: GAME 0x549f90
C_web_node::~C_web_node()
{
    delete[] conns;
}

// Grows the connections' array to num (PS2's takes the path too, for its help memory)
// FUNCTION: GAME 0x549fc0
void C_web_node::Resize(ushort num)
{
    if (max_conns < num) {
        max_conns = (uchar)num;
        C_web_connection *c = new C_web_connection[max_conns];
        for (uint i = 0; i < num_conns; i++)
            c[i] = conns[i];
        delete[] conns;
        conns = c;
    }
}

// FUNCTION: GAME 0x54a0a0
void C_web_node::AddConn(ushort node, S_vector from, S_vector to, ushort flags)
{
    Resize(num_conns + 1);
    float distance = (to - from).Magnitude();
    C_web_connection &c = conns[num_conns];
    c.node = node;
    c.distance = distance;
    c.flags = flags;
    num_conns++;
}

// Moves the last connection into the deleted one's place
// FUNCTION: GAME 0x54a160
void C_web_node::DeleteConn(ushort i)
{
    if (i < num_conns) {
        conns[i] = conns[num_conns - 1];
        num_conns--;
    }
}

// FUNCTION: GAME 0x54a1a0
ushort C_web_node::FindConnIdx(ushort node)
{
    for (ushort i = 0; i < num_conns; i++)
        if (conns[i].node == node)
            return i;
    return 0xffff;
}

// PS2's calls ClearNode, which frees what the path's help memory holds; the PC's has none
// FUNCTION: GAME 0x54a1e0
C_web_path::~C_web_path()
{
    delete[] nodes;
    delete[] pathfind;
}

// Grows the nodes' array by grow_size until num fit (PS2's grows it to num)
// FUNCTION: GAME 0x54a220
void C_web_path::Resize(uint num)
{
    if (max_nodes < num) {
        do
            max_nodes += grow_size;
        while (max_nodes < num);
        C_web_node *n = new C_web_node[max_nodes];
        for (uint i = 0; i < num_nodes; i++) {
            n[i].Resize(nodes[i].num_conns);
            n[i].pos.x = nodes[i].pos.x;
            n[i].pos.y = nodes[i].pos.y;
            n[i].pos.z = nodes[i].pos.z;
            n[i].id = nodes[i].id;
            n[i].flags = nodes[i].flags;
            n[i].num_conns = nodes[i].num_conns;
            n[i].width = nodes[i].width;
            n[i].unknown_14 = nodes[i].unknown_14;
            memcpy(n[i].conns, nodes[i].conns, nodes[i].num_conns * sizeof(C_web_connection));
        }
        delete[] nodes;
        nodes = n;
    }
}

// FUNCTION: GAME 0x54a3b0
void C_web_path::AddNode(float x, float y, float z, ushort flags)
{
    Resize(num_nodes + 1);
    C_web_node &n = nodes[num_nodes];
    n.pos.x = x;
    n.pos.y = y;
    n.pos.z = z;
    n.flags = flags;
    n.id = 0;
    nodes[num_nodes].num_conns = 0;
    num_nodes++;
}

// Deletes a node: the connections to it go, the last node takes its place (and the connections to the last node are
// redirected to the new index)
// FUNCTION: GAME 0x54a410
void C_web_path::DeleteNode(uint node)
{
    if (node < num_nodes) {
        uint i;
        for (i = 0; i < num_nodes; i++)
            if (i != node) {
                uint c = nodes[i].FindConnIdx(node);
                if (c != 0xffff)
                    nodes[i].DeleteConn(c);
            }
        for (i = 0; i < num_nodes; i++)
            if (i != node) {
                uint c = nodes[i].FindConnIdx(num_nodes - 1);
                if (c != 0xffff)
                    nodes[i].conns[c].node = node;
            }
        delete[] nodes[node].conns;
        nodes[node] = nodes[num_nodes - 1];
        nodes[num_nodes - 1].max_conns = 0;
        nodes[num_nodes - 1].num_conns = 0;
        nodes[num_nodes - 1].conns = NULL;
        num_nodes--;
    }
}

// Empty, as PS2's: the vtable's entry is the linker's folded empty function (0x4aabe0, input/C_ff_manager.h)
void C_web_path::DrawConnections()
{
}

// Connects two nodes both ways, or disconnects them if they are connected already
// FUNCTION: GAME 0x54a550
void C_web_path::CreateConnect(uint node1, uint node2)
{
    if (node1 != node2) {
        ushort conn;
        uint i;
        bool found = false;
        for (i = 0; i < nodes[node1].num_conns; i++)
            if (nodes[node1].conns[i].node == node2) {
                found = true;
                break;
            }
        if (!found) {
            if (LegalConnect(node1, node2, conn)) {
                S_vector p1 = nodes[node1].GetPos();
                S_vector p2 = nodes[node2].GetPos();
                nodes[node1].AddConn(node2, p1, p2, conn);
                nodes[node2].AddConn(node1, p1, p2, conn);
            }
        } else {
            nodes[node1].DeleteConn(i);
            for (uint j = 0; j < nodes[node2].num_conns; j++)
                if (nodes[node2].conns[j].node == node1) {
                    nodes[node2].DeleteConn(j);
                    break;
                }
        }
    }
}

// Whether the two nodes may be connected, and the connection's flags in conn
// FUNCTION: GAME 0x54a6d0
bool C_web_path::LegalConnect(uint node1, uint node2, ushort &conn)
{
    conn = 0;
    ushort f1 = nodes[node1].flags;
    ushort f2 = nodes[node2].flags;
    uint both = f1 | f2;
    bool a = (both & WEBF_UNKNOWN_4) || (both & WEBF_UNKNOWN_8);
    bool b = (both & (WEBF_UNKNOWN_1 | WEBF_UNKNOWN_2 | WEBF_UNKNOWN_10)) != 0;
    if ((a && b) || (!a && !b))
        return false;
    if ((f1 & (WEBF_UNKNOWN_4 | WEBF_UNKNOWN_8)) || (f2 & (WEBF_UNKNOWN_4 | WEBF_UNKNOWN_8))) {
        conn = WEBF_UNKNOWN_4;
        return true;
    }
    if ((f1 & WEBF_UNKNOWN_10) || (f2 & WEBF_UNKNOWN_10)) {
        conn = WEBF_UNKNOWN_10;
        return true;
    }
    if ((f1 & WEBF_UNKNOWN_2) || (f2 & WEBF_UNKNOWN_2)) {
        conn = WEBF_UNKNOWN_2;
        return true;
    }
    conn = WEBF_UNKNOWN_1;
    return true;
}

// Sets each connection of the node to what LegalConnect says now (the one-way flag stays), deletes the illegal ones
// FUNCTION: GAME 0x54a790
void C_web_path::ControlNodeConn(uint node)
{
    for (uint j = 0; j < nodes[node].num_conns; j++) {
        C_web_connection *c = &nodes[node].conns[j];
        uint other = c->node;
        uint oneway = c->flags & WEBF_ONEWAY;
        ushort back = nodes[other].FindConnIdx(node);
        uint backoneway = nodes[other].conns[back].flags & WEBF_ONEWAY;
        ushort conn;
        if (LegalConnect(node, other, conn)) {
            if (conn != c->flags) {
                c->flags = oneway | conn;
                nodes[other].conns[nodes[other].FindConnIdx(node)].flags = backoneway | conn;
            }
        } else {
            nodes[node].DeleteConn(j);
            nodes[other].DeleteConn(nodes[other].FindConnIdx(node));
            j--;
        }
    }
}

// Returns 0, as PS2's: the vtable's entry is a folded copy
// FUNCTION: GAME 0x5e3fd0 FOLDED
int C_web_path::SelectLine(int, int)
{
    return 0;
}

// Frees the nodes and reads the mission's nodes and their connections from the chunk's file (check.bin)
// FUNCTION: GAME 0x54a8c0
void C_web_path::LoadNodes(C_chunk *chunk)
{
    delete[] nodes;
    max_nodes = 0;
    num_nodes = 0;
    nodes = NULL;
    uint count;
    uint magic;
    chunk->Read(&magic, sizeof(magic));
    if (magic == WEB_NODES_MAGIC) {
        count = 0;
        chunk->Read(&count, sizeof(count));
        Resize(count);
        num_nodes = count;
        chunk->Read(nodes, num_nodes * sizeof(C_web_node));
        for (uint i = 0; i < num_nodes; i++) {
            count = nodes[i].num_conns;
            nodes[i].max_conns = 0;
            nodes[i].num_conns = 0;
            nodes[i].conns = NULL;
            nodes[i].Resize(count);
            nodes[i].num_conns = (uchar)count;
            chunk->Read(nodes[i].conns, nodes[i].num_conns * sizeof(C_web_connection));
        }
    }
}

// Appends the nodes of a chunk's file to the web, shifting their connections by the nodes it had (PS2's SaveNodes has
// no PC copy: the editor's)
// FUNCTION: GAME 0x54ab40
void C_web_path::MergeNodes(C_chunk *chunk)
{
    uint count;
    uint magic;
    chunk->Read(&magic, sizeof(magic));
    if (magic == WEB_NODES_MAGIC) {
        count = 0;
        chunk->Read(&count, sizeof(count));
        Resize(num_nodes + count);
        chunk->Read(&nodes[num_nodes], count * sizeof(C_web_node));
        uint first = num_nodes;
        num_nodes += count;
        for (uint i = first; i < num_nodes; i++) {
            count = nodes[i].num_conns;
            nodes[i].max_conns = 0;
            nodes[i].num_conns = 0;
            nodes[i].conns = NULL;
            nodes[i].Resize(count);
            nodes[i].num_conns = (uchar)count;
            chunk->Read(nodes[i].conns, nodes[i].num_conns * sizeof(C_web_connection));
            for (uint j = 0; j < count; j++)
                nodes[i].conns[j].node += (ushort)first;
        }
    }
}

// Sets the distance of each of the node's connections and of the connection back
// FUNCTION: GAME 0x54ade0
void C_web_path::RecompileDistance(uint node)
{
    for (uint i = 0; i < nodes[node].num_conns; i++) {
        uint other = nodes[node].conns[i].node;
        S_vector from = nodes[node].GetPos();
        S_vector to = nodes[other].GetPos();
        float distance = (to - from).Magnitude();
        nodes[node].conns[i].distance = distance;
        ushort back = nodes[other].FindConnIdx(node);
        if (back != -1)
            nodes[other].conns[back].distance = distance;
    }
}

// FUNCTION: GAME 0x54af10
void C_web_path::ReleaseSelect()
{
    for (uint i = 0; i < num_nodes; i++)
        for (uint j = 0; j < nodes[i].num_conns; j++)
            nodes[i].conns[j].flags &= ~WEBF_SELECTED;
}

// FUNCTION: GAME 0x54af60
int C_web_path::FindNodeID(ushort id)
{
    int result = -1;
    for (uint i = 0; i < num_nodes; i++)
        if (nodes[i].id == id) {
            result = i;
            break;
        }
    return result;
}

// FindPath over the cars' roads, for two far nodes: the path goes from the node's nearest road node (or the crossing
// beyond it) along the roads' crossings, and the walking web joins the road's positions
// FUNCTION: GAME 0x54afa0
bool C_web_path::FindPath_FastByRoads(uint from, uint to, std::vector<uint> *path, ushort flags)
{
    path->clear();
    ushort near_from = act_mission->GetRoads()->GetNearNode(nodes[from].GetPos());
    if (near_from == NEARNODE_NONE)
        return false;
    near_from |= NEARNODE_FLAG;
    ushort near_to = act_mission->GetRoads()->GetNearNode(nodes[to].GetPos());
    if (near_to == NEARNODE_NONE)
        return false;
    near_to |= NEARNODE_FLAG;
    ushort cross_from = near_from;
    int dir;
    if (near_from & NEARNODE_FLAG) {
        cross_from = act_mission->GetRoads()->FindNearCrossingFromNode(near_from, dir, false);
        if (cross_from == NEARNODE_NONE)
            return false;
    }
    if (near_from & NEARNODE_FLAG) {
        near_to = act_mission->roads->FindNearCrossingFromNode(near_to, dir, false);
        if (near_to == NEARNODE_NONE)
            return false;
    }
    std::vector<ushort> rpath;
    if (cross_from == near_to)
        rpath.push_back(cross_from);
    else if (!act_mission->roads->FindPathBeweenCross(cross_from, near_to, rpath))
        return false;
    if (rpath.size() < 1)
        return false;
    std::vector<uint> tmp;
    int next = 0;
    path->push_back(from);
    int cur = GetNearNode(act_mission->roads->GetPos(rpath[0]), -1.0f, flags);
    if (!FindPath(from, cur, &tmp, flags))
        return false;
    for (uint i = 0; i < tmp.size(); i++)
        path->push_back(tmp[i]);
    if (rpath.size() > 1) {
        for (uint i = 0; i < rpath.size() - 1; i++) {
            next = GetNearNode(act_mission->roads->GetPos(rpath[i + 1]), -1.0f, flags);
            if (next == -1)
                return false;
            if (!FindPath(cur, next, &tmp, flags))
                return false;
            for (uint j = 1; j < tmp.size(); j++) {
                if (path->back() != tmp[j])
                    path->push_back(tmp[j]);
            }
            cur = next;
        }
    } else
        next = cur;
    if (!FindPath(next, to, &tmp, flags))
        return false;
    for (uint j = 1; j < tmp.size(); j++) {
        if (path->back() != tmp[j])
            path->push_back(tmp[j]);
    }
    return true;
}

// Breadth-first search for the shortest path from one node to another, the nodes' count rounds at most; long ways
// between two nodes go through the roads' FindPath_FastByRoads
// FUNCTION: GAME 0x54b540
bool C_web_path::FindPath(uint from, uint to, std::vector<uint> *path, ushort flags)
{
    if (!num_nodes)
        return false;
    if (from >= num_nodes || to >= num_nodes)
        return false;
    if (from == to) {
        path->clear();
        path->push_back(from);
        return true;
    }
    if (!finding_by_roads) {
        S_vector a = nodes[from].GetPos(), b = nodes[to].GetPos();
        S_vector d = a - b;
        if (d.x * d.x + d.y * d.y + d.z * d.z > 250000.0f && act_mission->GetRoads()->crossings.size()) {
            finding_by_roads = true;
            bool result = FindPath_FastByRoads(from, to, path, flags);
            finding_by_roads = false;
            return result;
        }
    }
    if (num_pathfind != num_nodes) {
        delete[] pathfind;
        pathfind = new S_pathfind[num_nodes];
        num_pathfind = num_nodes;
    } else {
        for (uint i = num_nodes; i--;) {
            pathfind[i].from = 0xffff;
            pathfind[i].distance = 1e10f;
        }
    }
    std::vector<uint> list1;
    std::vector<uint> list2;
    std::vector<uint> *cur = &list1;
    std::vector<uint> *next = &list2;
    cur->push_back(from);
    float best = 1e10f;
    uint rounds = 0;
    pathfind[from].distance = 0.0f;

    while (cur->size()) {
        for (uint i = 0; i < cur->size(); i++) {
            for (int j = 0; j < nodes[(*cur)[i]].num_conns; j++) {
                C_web_connection *conn = &nodes[(*cur)[i]].conns[j];
                if (!(conn->flags & flags))
                    continue;
                if (conn->flags & (WEBF_ONEWAY | WEBF_UNKNOWN_4000))
                    continue;
                float distance = pathfind[(*cur)[i]].distance + conn->distance;
                if (!(distance >= best) && !(distance >= pathfind[conn->node].distance)) {
                    bool is_new = true;
                    for (uint k = 0; k < next->size(); k++) {
                        if ((*next)[k] == nodes[(*cur)[i]].conns[j].node) {
                            is_new = false;
                            break;
                        }
                    }
                    if (is_new)
                        next->push_back(nodes[(*cur)[i]].conns[j].node);
                    pathfind[nodes[(*cur)[i]].conns[j].node].distance = distance;
                    pathfind[nodes[(*cur)[i]].conns[j].node].from = (*cur)[i];
                    if (to == nodes[(*cur)[i]].conns[j].node) {
                        best = distance;
                        break;
                    }
                }
            }
        }
        if (++rounds > 100)
            return false;
        std::vector<uint> *t = cur;
        cur = next;
        next = t;
        t->erase(t->begin(), t->end());
    }

    if (pathfind[to].from == 0xffff)
        return false;

    cur->clear();
    cur->push_back(to);
    uint n = to;
    while (n != from) {
        cur->push_back(pathfind[n].from);
        n = pathfind[n].from;
    }
    path->clear();
    for (int i = cur->size() - 1; i > -1; i--)
        path->push_back((*cur)[i]);
    return true;
}

// FindPath with the cost of a connection from the callback instead of its distance, to the node to
// FUNCTION: GAME 0x54bb00
bool C_web_path::FindPathCompFunc(uint from, uint to, std::vector<uint> *path, t_web_cost_func cost, ushort flags)
{
    if (!cost)
        return false;
    if (!num_nodes)
        return false;
    if (from >= num_nodes || to >= num_nodes)
        return false;
    if (from == to) {
        path->clear();
        path->push_back(from);
        return true;
    }

    if (num_pathfind != num_nodes) {
        delete[] pathfind;
        pathfind = new S_pathfind[num_nodes];
        num_pathfind = num_nodes;
    } else {
        for (uint n = num_nodes; n--;) {
            pathfind[n].from = 0xffff;
            pathfind[n].distance = 1e10f;
        }
    }

    std::vector<uint> list1;
    std::vector<uint> list2;
    std::vector<uint> *cur = &list1;
    std::vector<uint> *next = &list2;
    cur->push_back(from);
    float best = 1e10f;
    uint rounds = 0;
    pathfind[from].distance = 0.0f;

    while (cur->size()) {
        for (uint i = 0; i < cur->size(); i++) {
            for (int j = 0; j < nodes[(*cur)[i]].num_conns; j++) {
                if (!(nodes[(*cur)[i]].conns[j].flags & flags))
                    continue;
                if (nodes[(*cur)[i]].conns[j].flags & (WEBF_ONEWAY | WEBF_UNKNOWN_4000))
                    continue;
                float distance = cost((ushort)(*cur)[i], nodes[(*cur)[i]].conns[j].node, j) + pathfind[(*cur)[i]].distance;
                if (!(distance >= best) && !(distance >= pathfind[nodes[(*cur)[i]].conns[j].node].distance)) {
                    bool is_new = true;
                    for (uint k = 0; k < next->size(); k++) {
                        if ((*next)[k] == nodes[(*cur)[i]].conns[j].node) {
                            is_new = false;
                            break;
                        }
                    }
                    if (is_new)
                        next->push_back(nodes[(*cur)[i]].conns[j].node);
                    pathfind[nodes[(*cur)[i]].conns[j].node].distance = distance;
                    pathfind[nodes[(*cur)[i]].conns[j].node].from = (*cur)[i];
                    if (to == nodes[(*cur)[i]].conns[j].node)
                        best = distance;
                }
            }
        }
        if (++rounds > 100)
            return false;
        std::vector<uint> *t = cur;
        cur = next;
        next = t;
        next->clear();
    }

    if (pathfind[to].from == 0xffff)
        return false;

    cur->clear();
    cur->push_back(to);
    if (to != from) {
        uint n = to;
        do {
            cur->push_back(pathfind[n].from);
            n = pathfind[n].from;
        } while (n != from);
    }
    path->clear();
    for (int i = cur->size() - 1; i > -1; i--)
        path->push_back((*cur)[i]);
    return true;
}

// FindPath with the end of the path where the callback accepts a node: it returns 1 for a node that ends the path, 2
// for one that ends it without being in it, 3 for a node whose connections aren't followed (the values are ours)
// FUNCTION: GAME 0x54c050
bool C_web_path::FindPathCompFunc(uint from, t_web_end_func end, std::vector<uint> *path, t_web_cost_func cost,
    ushort flags)
{
    if (!cost)
        return false;
    if (!end)
        return false;
    if (!num_nodes)
        return false;
    if (from >= num_nodes)
        return false;

    if (num_pathfind != num_nodes) {
        delete[] pathfind;
        pathfind = new S_pathfind[num_nodes];
        num_pathfind = num_nodes;
    } else {
        for (uint n = num_nodes; n--;) {
            pathfind[n].from = 0xffff;
            pathfind[n].distance = 1e10f;
        }
    }

    std::vector<uint> list1;
    std::vector<uint> list2;
    std::vector<uint> *cur = &list1;
    std::vector<uint> *next = &list2;
    cur->push_back(from);
    ushort best_node = 0xffff;
    float best = 1e10f;
    bool add_end = true;
    uint rounds = 0;
    pathfind[from].distance = 0.0f;

    while (cur->size()) {
        for (uint i = 0; i < cur->size(); i++) {
            for (int j = 0; j < nodes[(*cur)[i]].num_conns; j++) {
                if (!(nodes[(*cur)[i]].conns[j].flags & flags))
                    continue;
                if (nodes[(*cur)[i]].conns[j].flags & (WEBF_ONEWAY | WEBF_UNKNOWN_4000))
                    continue;
                float distance = cost((ushort)(*cur)[i], nodes[(*cur)[i]].conns[j].node, j) + pathfind[(*cur)[i]].distance;
                if (!(distance >= best) && !(distance >= pathfind[nodes[(*cur)[i]].conns[j].node].distance)) {
                    bool is_new = true;
                    for (uint k = 0; k < next->size(); k++) {
                        if ((*next)[k] == nodes[(*cur)[i]].conns[j].node) {
                            is_new = false;
                            break;
                        }
                    }
                    pathfind[nodes[(*cur)[i]].conns[j].node].distance = distance;
                    pathfind[nodes[(*cur)[i]].conns[j].node].from = (*cur)[i];
                    switch (end(nodes[(*cur)[i]].conns[j].node)) {
                    case 2:
                        add_end = false;
                    case 1:
                        best_node = nodes[(*cur)[i]].conns[j].node;
                        best = distance;
                        break;
                    case 3:
                        continue;
                    }
                    if (is_new)
                        next->push_back(nodes[(*cur)[i]].conns[j].node);
                }
            }
        }
        if (++rounds > 100)
            return false;
        std::vector<uint> *t = cur;
        cur = next;
        next = t;
        next->clear();
    }

    if (best_node == 0xffff)
        return false;

    cur->clear();
    if (add_end)
        cur->push_back(best_node);
    uint n = best_node;
    while (n != from) {
        cur->push_back(pathfind[n].from);
        n = pathfind[n].from;
    }
    path->clear();
    for (int i = cur->size() - 1; i > -1; i--)
        path->push_back((*cur)[i]);
    return true;
}

// FUNCTION: GAME 0x54c5a0
void C_web_path::FindSelectedLine(uint &node1, uint &node2)
{
    for (uint i = 0; i < num_nodes; i++)
        for (uint j = 0; j < nodes[i].num_conns; j++)
            if (nodes[i].conns[j].flags & WEBF_SELECTED) {
                node1 = i;
                node2 = nodes[i].conns[j].node;
                return;
            }
    node2 = (uint)-1;
    node1 = (uint)-1;
}

// Inserts a new node in the middle of the connection between the two, which the connection's flags go to both halves of
// FUNCTION: GAME 0x54c620
void C_web_path::IncludeNode(uint node1, uint node2)
{
    ushort conn = 0;
    S_vector half = (nodes[node1].pos - nodes[node2].pos) * 0.5f;
    S_vector mid = half + nodes[node2].GetPos();
    if (LegalConnect(node1, node2, conn)) {
        AddNode(mid.x, mid.y, mid.z, conn);
        uint idx1 = nodes[node1].FindConnIdx(node2);
        uint idx2 = nodes[node2].FindConnIdx(node1);
        uint flags1 = nodes[node1].conns[idx1].flags;
        uint flags2 = nodes[node2].conns[idx2].flags;
        nodes[node1].DeleteConn(idx1);
        nodes[node2].DeleteConn(idx2);
        S_vector pos1 = nodes[node1].GetPos();
        S_vector pos2 = nodes[node2].GetPos();
        S_vector pos3 = nodes[num_nodes - 1].GetPos();
        nodes[node1].AddConn(num_nodes - 1, pos1, pos3, flags1);
        nodes[num_nodes - 1].AddConn(node1, pos1, pos3, flags2);
        nodes[node2].AddConn(num_nodes - 1, pos2, pos3, flags2);
        nodes[num_nodes - 1].AddConn(node2, pos2, pos3, flags1);
    }
}

// Makes the connection between the two nodes two-way, or else turns its one way around
// FUNCTION: GAME 0x54c8d0
void C_web_path::FlipOneWay(uint node1, uint node2, bool both)
{
    uint c1 = nodes[node1].FindConnIdx(node2);
    uint c2 = nodes[node2].FindConnIdx(node1);
    if (c1 == 0xffff || c2 == 0xffff)
        return;
    if (both) {
        nodes[node1].conns[c1].flags &= ~WEBF_ONEWAY;
        nodes[node2].conns[c2].flags &= ~WEBF_ONEWAY;
    } else if (nodes[node1].conns[c1].flags & WEBF_ONEWAY) {
        nodes[node1].conns[c1].flags &= ~WEBF_ONEWAY;
        nodes[node2].conns[c2].flags |= WEBF_ONEWAY;
    } else {
        nodes[node1].conns[c1].flags |= WEBF_ONEWAY;
        nodes[node2].conns[c2].flags &= ~WEBF_ONEWAY;
    }
}

// The node nearest pos that has one of the flags and isn't closed (WEBF_UNKNOWN_4000), -1 for none: the search
// starts at the node nearest by x (the nodes are sorted by it) and goes both ways until the distance by x alone
// exceeds the best. A radius above 0 is a minimum: nodes closer than it are skipped.
// FUNCTION: GAME 0x54c9a0
int C_web_path::GetNearNode(const S_vector &pos, float radius, ushort flags)
{
    if (radius > 0.0f)
        radius = radius * radius + 0.001f;
    if (num_nodes) {
        uint lo = 0;
        uint hi = num_nodes - 1;
        int idx;
        if (pos.x < nodes[0].pos.x)
            idx = 0;
        else if (pos.x > nodes[hi].pos.x)
            idx = hi;
        else {
            do {
                idx = (hi + lo) >> 1;
                if (pos.x < nodes[idx].pos.x)
                    hi = idx;
                else
                    lo = idx;
            } while (hi - lo > 1);
        }
        float best = 3.4e38f;
        int found = -1;
        int start = idx;
        for (; idx >= 0; idx--) {
            C_web_node &n = nodes[idx];
            if (!(n.flags & flags))
                continue;
            if (n.flags & WEBF_UNKNOWN_4000)
                continue;
            float dy = pos.y - n.pos.y;
            float dx = pos.x - n.pos.x;
            dx = dx * dx;
            if (dx > best)
                break;
            float dz = pos.z - n.pos.z;
            float d = dx + dy * dy + dz * dz;
            if (d < radius)
                continue;
            if (d < best) {
                best = d;
                found = idx;
            }
        }
        for (uint j = start + 1; j < num_nodes; j++) {
            C_web_node &n = nodes[j];
            if (!(n.flags & flags))
                continue;
            if (n.flags & WEBF_UNKNOWN_4000)
                continue;
            float dy = pos.y - n.pos.y;
            float dx = n.pos.x - pos.x;
            dx = dx * dx;
            if (dx > best)
                break;
            float dz = n.pos.z - pos.z;
            float d = dx + dy * dy + dz * dz;
            if (d < radius)
                continue;
            if (d < best) {
                best = d;
                found = j;
            }
        }
        return found;
    }
    return -1;
}

// Selection sort of the nodes by x, then the connections' node numbers follow the swap
// FUNCTION: GAME 0x54cba0
void C_web_path::SortNodesByX()
{
    if (IsSorted())
        return;
    for (uint i = 0; i < num_nodes; i++) {
        uint min = i;
        for (uint j = i + 1; j < num_nodes; j++)
            if (nodes[j].pos.x < nodes[min].pos.x)
                min = j;
        if (min == i)
            continue;
        for (uint k = 0; k < num_nodes; k++)
            for (uint c = 0; c < nodes[k].num_conns; c++) {
                C_web_connection &conn = nodes[k].conns[c];
                if (conn.node == min)
                    conn.node = i;
                else if (conn.node == i)
                    conn.node = min;
            }
        uchar *tmp = new uchar[sizeof(C_web_node)];
        memcpy(tmp, &nodes[i], sizeof(C_web_node));
        memcpy(&nodes[i], &nodes[min], sizeof(C_web_node));
        memcpy(&nodes[min], tmp, sizeof(C_web_node));
        delete[] tmp;
    }
}

// True where the nodes are in order of x, which GetNearNode's search needs
// FUNCTION: GAME 0x54ccf0
bool C_web_path::IsSorted()
{
    for (uint i = 1; i < num_nodes; i++)
        if (nodes[i].pos.x < nodes[i - 1].pos.x)
            return false;
    return true;
}

// Clears two flags of the nodes and their connections, from the last node, as PS2's
// FUNCTION: GAME 0x54cd20
void C_web_path::DeRiciConns()
{
    for (int i = num_nodes; i--;)
    {
        nodes[i].flags &= ~(WEBF_UNKNOWN_1000 | WEBF_UNKNOWN_4000);
        for (int j = nodes[i].num_conns; j--;)
            nodes[i].conns[j].flags &= ~(WEBF_UNKNOWN_1000 | WEBF_UNKNOWN_4000);
    }
}

// Empty, as PS2's: the vtable's entry is a folded copy (0x46ca70, annotated by name in mission/C_program.h)
void C_web_path::DebugFindPath(ushort, ushort)
{
}

// Appends the elements of src to dst
// FUNCTION: GAME 0x54cd70
void _STDVECTCOPY(std::vector<uint> *dst, std::vector<uint> *src)
{
    for (uint i = 0; i < src->size(); i++)
        dst->push_back((*src)[i]);
}

// The cheapest path from a node to the one the end function accepts, found by recursion over the connections (PS2
// name). node is the node it came from, distance the cheapest found so far and add the length up to from. Returns the
// new vector of the path's nodes, last node first, or NULL for none; end returns 0 for a node to go on from, 1 for
// the path's end (which the path takes along with the node before) and 2 for an end that the path doesn't take, 3 for
// a node to skip.
// FUNCTION: GAME 0x54cf30
std::vector<uint> *nodeproc(uint from, uint node, t_web_end_func end, t_web_cost_func cost, float *distance, float add)
{
    std::vector<uint> *result = NULL;
    C_web_node *n = &act_mission->web_path->nodes[from];
    float len = 0;
    for (uint i = 0; i < n->num_conns; i++) {
        if (n->GetConn(i)->flags & (WEBF_ONEWAY | WEBF_UNKNOWN_4000))
            continue;
        uint to = n->GetConn(i)->node;
        if (to == node)
            continue;
        if (act_mission->web_path->nodes[to].flags & WEBF_UNKNOWN_4000)
            continue;
        len = cost(from, to, i);
        switch (end(to)) {
        case 0: {
            len += add;
            if (!(len > *distance)) {
                std::vector<uint> *sub = nodeproc(to, from, end, cost, distance, len);
                if (sub) {
                    if (result) {
                        result->clear();
                        delete result;
                    }
                    result = sub;
                    result->insert(result->begin(), from);
                }
            }
            continue;
        }
        case 1: {
            len += add;
            if (len < *distance) {
                std::vector<uint> *v = new std::vector<uint>;
                v->push_back(from);
                v->push_back(to);
                if (result) {
                    result->clear();
                    delete result;
                }
                result = v;
                *distance = len;
            }
            continue;
        }
        case 2: {
            len += add;
            if (len < *distance) {
                std::vector<uint> *v = new std::vector<uint>;
                v->push_back(from);
                if (result) {
                    result->clear();
                    delete result;
                }
                result = v;
                *distance = len;
            }
            continue;
        }
        case 3:
            continue;
        }
    }
    return result;
}

// Finds the shortest path from a node to one the end function accepts, into path (the flags are unused, as in PS2)
// FUNCTION: GAME 0x54d240
bool C_web_path::FindPathCompFuncJIPO(uint from, t_web_end_func end, std::vector<uint> *path, t_web_cost_func cost,
    ushort flags)
{
    float distance = 666666.0f;
    std::vector<uint> *found = nodeproc(from, -1, end, cost, &distance, 0.0f);
    if (found) {
        _STDVECTCOPY(path, found);
        delete found;
        return true;
    }
    return false;
}
