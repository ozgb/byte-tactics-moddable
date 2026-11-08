#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/pathfinding.h"
#include "../../interfaces/state_writer.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/services.h"
#include "../network/player.h"
#include "../game/game.h"

#include "pathfind_hpa.h"

#include "../../link/globals.h"

namespace {

enum {
    MAGIC = 0x31415048,
    STEP_BUDGET = 0x535,
    MAX_NODES = 12,
    MAX_PER_SIDE = 3,
    TRI = MAX_NODES * (MAX_NODES - 1) / 2,
    MAX_GOAL_CELLS = 64,
    MAX_GOAL_CLUSTERS = 8,
    MAX_GOAL_NODES = MAX_GOAL_CLUSTERS * MAX_NODES,
    EVENT_CAP = 128,
    PATH_CAP = 8192,
    MAX_LOCAL = 32 * 32,
    STRAIGHT = 10,
    DIAGONAL = 14,
    PASS_ONE_EXTRA = 30,
    INF = 0x3fffffff,
    NO_EDGE = 0xffff,
    CLASS_BUDGET_BYTES = 12 << 20
};

enum {
    MAX_WAYPOINTS = 64,
    MAX_BANNED = 8,
    POINT_CAP = 128 * 64
};

enum Phase {
    PH_INIT = 0,
    PH_BUILD = 1,
    PH_START_FLOOD = 2,
    PH_GOAL_FLOOD = 3,
    PH_ABSTRACT = 4,
    PH_REFINE = 5,
    PH_PULL = 6
};

const int kDx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
const int kDy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
const int kSideDx[4] = { 0, 1, 0, -1 };
const int kSideDy[4] = { -1, 0, 1, 0 };

struct ClusterRec {
    int version;
    int count;
    int cell[MAX_NODES];
    unsigned char side[MAX_NODES];
    unsigned short cost[TRI];
};

struct Local {
    int ox, oy, w, h;
    int mode;
    int terrain;
    int tx, ty;
    int n;
    int found;
    int goalAt;
    int g[MAX_LOCAL];
    int key[MAX_LOCAL];
    int heap[MAX_LOCAL];
    int hpos[MAX_LOCAL];
    signed char par[MAX_LOCAL];
    unsigned char st[MAX_LOCAL];
};

struct Ev {
    PathEvent e;
    int first;
};

struct Req {
    PathRequest r;
    unsigned seq;
    int used;
};

struct GoalNode {
    int cluster;
    int local;
    int cost;
};

struct Hdr {
    unsigned magic;
    int W, H, cs, shift, CW, CH, nClu, NN;
    int nSlots, reqCap;
    int nPts;
    PathClass classes[32];
    int slotOf[32];
    int slotsUsed;
    unsigned seqCounter;
    unsigned searchId;
    int debt;
    int evHead, evCount;
    unsigned lastTick;

    int offVer, offRec, offReq, offEv;
    int offPts;
    int offAG, offAPar, offAStamp, offAKey, offAPos, offAHeap;
    int offRoute, offPath;

    int active;
    PathRequest req;
    unsigned aseq;
    int phase;
    int slot;
    int sx, sy, fx, fy;
    int startCluster;
    PathPoint goalCells[MAX_GOAL_CELLS];
    int nGoal;
    int nearGoal;
    int goalClusters[MAX_GOAL_CLUSTERS];
    int nGoalClusters;
    int gcIdx;
    int localReady;
    int directCost;
    int startCost[MAX_NODES];
    GoalNode goalNodes[MAX_GOAL_NODES];
    int nGoalNodes;
    int aHeapN;
    int aInit;
    int nRoute;
    int segIdx;
    int nPath;
    int truncated;
    int pullAnchor, pullK;
    int nWp;
    int wp[MAX_WAYPOINTS + 2];
    int wpOverflow;
    int pullInit;
    PathPoint rawGoal[MAX_GOAL_CELLS];
    int nRaw;
    int nearest;
    int nearNode;
    int nearDist;
    int nBanned;
    int banned[MAX_BANNED];

    Local scratch[2];
};

struct HpaView {
    Hdr* h;
    int* ver;
    ClusterRec* rec;
    Req* req;
    Ev* ev;
    PathPoint* pts;
    int* ag;
    int* apar;
    int* astamp;
    int* akey;
    int* apos;
    int* aheap;
    int* route;
    int* path;
    const PathWorldView* w;
};

inline int Align8(int v)
{
    return (v + 7) & ~7;
}

inline const PathWorldView* World()
{
    return g_seams.pathworld;
}

bool Open(HpaView* v)
{
    Hdr* h = (Hdr*)g_game->pathfinder.get();
    if (h == nullptr || h->magic != MAGIC)
        return false;
    char* b = (char*)h;
    v->h = h;
    v->ver = (int*)(b + h->offVer);
    v->rec = (ClusterRec*)(b + h->offRec);
    v->req = (Req*)(b + h->offReq);
    v->ev = (Ev*)(b + h->offEv);
    v->pts = (PathPoint*)(b + h->offPts);
    v->ag = (int*)(b + h->offAG);
    v->apar = (int*)(b + h->offAPar);
    v->astamp = (int*)(b + h->offAStamp);
    v->akey = (int*)(b + h->offAKey);
    v->apos = (int*)(b + h->offAPos);
    v->aheap = (int*)(b + h->offAHeap);
    v->route = (int*)(b + h->offRoute);
    v->path = (int*)(b + h->offPath);
    v->w = World();
    return true;
}

inline bool Less(const int* key, int a, int b)
{
    return key[a] < key[b] || (key[a] == key[b] && a < b);
}

void HeapUp(int* heap, int* pos, const int* key, int i)
{
    int v = heap[i];
    while (i > 0) {
        int p = (i - 1) / 2;
        int pv = heap[p];
        if (!Less(key, v, pv))
            break;
        heap[i] = pv;
        pos[pv] = i;
        i = p;
    }
    heap[i] = v;
    pos[v] = i;
}

void HeapDown(int* heap, int* pos, const int* key, int n, int i)
{
    int v = heap[i];
    for (;;) {
        int c = 2 * i + 1;
        if (c >= n)
            break;
        if (c + 1 < n && Less(key, heap[c + 1], heap[c]))
            c++;
        if (!Less(key, heap[c], v))
            break;
        heap[i] = heap[c];
        pos[heap[i]] = i;
        i = c;
    }
    heap[i] = v;
    pos[v] = i;
}

void HeapPush(int* heap, int* n, int* pos, const int* key, int v)
{
    if (pos[v] >= 0) {
        HeapUp(heap, pos, key, pos[v]);
        return;
    }
    heap[*n] = v;
    pos[v] = *n;
    (*n)++;
    HeapUp(heap, pos, key, *n - 1);
}

int HeapPop(int* heap, int* n, int* pos, const int* key)
{
    int top = heap[0];
    pos[top] = -1;
    (*n)--;
    if (*n > 0) {
        heap[0] = heap[*n];
        pos[heap[0]] = 0;
        HeapDown(heap, pos, key, *n, 0);
    }
    return top;
}

inline int ClusterOf(const Hdr* h, int x, int y)
{
    return (y >> h->shift) * h->CW + (x >> h->shift);
}

inline int Octile(int dx, int dy)
{
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    int lo = dx < dy ? dx : dy;
    int hi = dx < dy ? dy : dx;
    return hi * STRAIGHT + lo * (DIAGONAL - STRAIGHT);
}

int CellExtra(const HpaView* v, PathClass cls, int x, int y, int terrain)
{
    const Hdr* h = v->h;
    if (x < 0 || y < 0 || x >= h->W || y >= h->H)
        return -1;
    if (!v->w->InBounds(cls, x, y))
        return -1;
    int p = terrain ? v->w->TerrainPass(cls, x, y) : v->w->Pass(cls, x, y);
    if (p <= 0)
        return -1;
    return p == 1 ? PASS_ONE_EXTRA : 0;
}

void LInit(Local* lc, const Hdr* h, int cluster, int mode, int terrain)
{
    lc->ox = (cluster % h->CW) << h->shift;
    lc->oy = (cluster / h->CW) << h->shift;
    lc->w = h->W - lc->ox < h->cs ? h->W - lc->ox : h->cs;
    lc->h = h->H - lc->oy < h->cs ? h->H - lc->oy : h->cs;
    lc->mode = mode;
    lc->terrain = terrain;
    lc->tx = lc->ty = 0;
    lc->n = 0;
    lc->found = -1;
    lc->goalAt = -1;
    int cells = h->cs * h->cs;
    for (int i = 0; i < cells; i++) {
        lc->st[i] = 0;
        lc->hpos[i] = -1;
    }
}

inline int LIndex(const Hdr* h, const Local* lc, int x, int y)
{
    return ((y - lc->oy) << h->shift) + (x - lc->ox);
}

void LPush(Local* lc, const Hdr* h, int x, int y, int g)
{
    int i = LIndex(h, lc, x, y);
    lc->st[i] = 1;
    lc->g[i] = g;
    lc->par[i] = -1;
    lc->key[i] = g;
    if (lc->mode == 1)
        lc->key[i] += Octile(x - lc->tx, y - lc->ty);
    HeapPush(lc->heap, &lc->n, lc->hpos, lc->key, i);
}

int LStep(Local* lc, const HpaView* v, PathClass cls, const PathGoalArea* goal)
{
    const Hdr* h = v->h;
    if (lc->n == 0)
        return 2;
    int u = HeapPop(lc->heap, &lc->n, lc->hpos, lc->key);
    lc->st[u] = 2;
    int ux = lc->ox + (u & (h->cs - 1));
    int uy = lc->oy + (u >> h->shift);
    if (lc->mode == 1 && ux == lc->tx && uy == lc->ty) {
        lc->found = u;
        return 1;
    }
    if (lc->mode == 2 && PathGoalContains(goal, ux, uy)) {
        lc->found = u;
        return 1;
    }
    if (lc->mode == 0 && goal != nullptr && lc->goalAt < 0 && PathGoalContains(goal, ux, uy))
        lc->goalAt = u;
    for (int d = 0; d < 8; d++) {
        int nx = ux + kDx[d];
        int ny = uy + kDy[d];
        if (nx < lc->ox || ny < lc->oy || nx >= lc->ox + lc->w || ny >= lc->oy + lc->h)
            continue;
        int vi = LIndex(h, lc, nx, ny);
        if (lc->st[vi] == 2)
            continue;
        int extra = CellExtra(v, cls, nx, ny, lc->terrain);
        if (extra < 0)
            continue;
        int step = STRAIGHT;
        if (d & 1) {
            if (CellExtra(v, cls, nx, uy, lc->terrain) < 0 || CellExtra(v, cls, ux, ny, lc->terrain) < 0)
                continue;
            step = DIAGONAL;
        }
        int ng = lc->g[u] + step + extra;
        if (lc->st[vi] == 0 || ng < lc->g[vi]) {
            lc->g[vi] = ng;
            lc->par[vi] = (signed char)d;
            lc->st[vi] = 1;
            lc->key[vi] = ng;
            if (lc->mode == 1)
                lc->key[vi] += Octile(nx - lc->tx, ny - lc->ty);
            HeapPush(lc->heap, &lc->n, lc->hpos, lc->key, vi);
        }
    }
    return 0;
}

inline int TriIndex(int i, int j)
{
    return i * (2 * MAX_NODES - i - 1) / 2 + (j - i - 1);
}

inline int NodeCost(const ClusterRec* r, int i, int j)
{
    if (i == j)
        return 0;
    return i < j ? r->cost[TriIndex(i, j)] : r->cost[TriIndex(j, i)];
}

void BuildCluster(HpaView* v, int slot, int c)
{
    Hdr* h = v->h;
    PathClass cls = h->classes[0];
    for (int i = 0; i < 32; i++)
        if (h->slotOf[i] == slot)
            cls = h->classes[i];
    ClusterRec* r = &v->rec[slot * h->nClu + c];
    Local* lc = &h->scratch[1];
    int ox = (c % h->CW) << h->shift;
    int oy = (c / h->CW) << h->shift;
    int cw = h->W - ox < h->cs ? h->W - ox : h->cs;
    int ch = h->H - oy < h->cs ? h->H - oy : h->cs;
    int work = 1;
    r->count = 0;
    for (int s = 0; s < 4; s++) {
        int len = (s & 1) ? ch : cw;
        int ax0, ay0, adx, ady;
        if (s == 0) {
            if (oy == 0)
                continue;
            ax0 = ox; ay0 = oy; adx = 1; ady = 0;
        } else if (s == 1) {
            if (ox + cw >= h->W)
                continue;
            ax0 = ox + cw - 1; ay0 = oy; adx = 0; ady = 1;
        } else if (s == 2) {
            if (oy + ch >= h->H)
                continue;
            ax0 = ox; ay0 = oy + ch - 1; adx = 1; ady = 0;
        } else {
            if (ox == 0)
                continue;
            ax0 = ox; ay0 = oy; adx = 0; ady = 1;
        }
        int segments = 0;
        int runStart = -1;
        for (int i = 0; i <= len; i++) {
            bool open = false;
            if (i < len) {
                int ax = ax0 + adx * i;
                int ay = ay0 + ady * i;
                int bx = ax + kSideDx[s];
                int by = ay + kSideDy[s];
                open = CellExtra(v, cls, ax, ay, 1) >= 0 && CellExtra(v, cls, bx, by, 1) >= 0;
            }
            if (open) {
                if (runStart < 0)
                    runStart = i;
            } else if (runStart >= 0) {
                if (segments < MAX_PER_SIDE && r->count < MAX_NODES) {
                    int mid = (runStart + i - 1) / 2;
                    r->cell[r->count] = (ay0 + ady * mid) * h->W + ax0 + adx * mid;
                    r->side[r->count] = (unsigned char)s;
                    r->count++;
                }
                segments++;
                runStart = -1;
            }
        }
    }
    for (int i = 0; i < TRI; i++)
        r->cost[i] = NO_EDGE;
    for (int i = 0; i + 1 < r->count; i++) {
        int sx = r->cell[i] % h->W;
        int sy = r->cell[i] / h->W;
        LInit(lc, h, c, 0, 1);
        LPush(lc, h, sx, sy, 0);
        for (;;) {
            int res = LStep(lc, v, cls, nullptr);
            work++;
            if (res != 0)
                break;
            bool all = true;
            for (int j = i + 1; j < r->count; j++) {
                int lj = LIndex(h, lc, r->cell[j] % h->W, r->cell[j] / h->W);
                if (lc->st[lj] != 2) {
                    all = false;
                    break;
                }
            }
            if (all)
                break;
        }
        for (int j = i + 1; j < r->count; j++) {
            int lj = LIndex(h, lc, r->cell[j] % h->W, r->cell[j] / h->W);
            if (lc->st[lj] == 2) {
                int g = lc->g[lj];
                r->cost[TriIndex(i, j)] = (unsigned short)(g > 0xfffe ? 0xfffe : g);
            }
        }
    }
    h->debt += work;
}

void Ensure(HpaView* v, int slot, int c)
{
    ClusterRec* r = &v->rec[slot * v->h->nClu + c];
    if (r->version == v->ver[c])
        return;
    BuildCluster(v, slot, c);
    r->version = v->ver[c];
}

void Settle(Hdr* h, int* budget)
{
    if (h->debt <= 0)
        return;
    int pay = h->debt < *budget ? h->debt : *budget;
    h->debt -= pay;
    *budget -= pay;
}

inline PathPoint WorldPoint(const Hdr* h, int cell)
{
    PathPoint p;
    p.x = (short)((cell % h->W) * 16 + h->fx * 8);
    p.y = (short)((cell / h->W) * 16 + h->fy * 8);
    return p;
}

PathEvent* NewEvent(HpaView* v, int kind, int status)
{
    Hdr* h = v->h;
    if (h->evCount >= EVENT_CAP)
        return nullptr;
    Ev* rec = &v->ev[h->evCount++];
    rec->first = h->nPts;
    PathEvent* e = &rec->e;
    e->kind = kind;
    e->status = status;
    e->unit = h->req.unit;
    e->requester = h->req.requester;
    e->target = h->req.target;
    e->tick = h->lastTick;
    e->count = 0;
    return e;
}

void EmitCells(HpaView* v, int kind, int status, const int* cells, int n)
{
    PathEvent* e = NewEvent(v, kind, status);
    if (e == nullptr)
        return;
    if (n > MAX_WAYPOINTS)
        n = MAX_WAYPOINTS;
    for (int i = 0; i < n; i++)
        v->pts[v->h->nPts + i] = WorldPoint(v->h, cells[i]);
    v->h->nPts += n;
    e->count = n;
}

void FinishSearch(HpaView* v, int status, const int* cells, int n)
{
    Hdr* h = v->h;
    if (status == PATH_FAILED)
        n = 0;
    EmitCells(v, PATH_EVENT_FINISHED, status, cells, n);
    v->w->EndSearch(h->req.unit);
    h->active = 0;
}

void EmitProgress(HpaView* v)
{
    Hdr* h = v->h;
    int cells[MAX_WAYPOINTS];
    int n = 0;
    cells[n++] = h->sy * h->W + h->sx;
    if (h->phase == PH_ABSTRACT && h->aInit) {
        if (h->aHeapN > 0) {
            int u = v->aheap[0];
            if (u < h->NN - 1) {
                const ClusterRec* r = &v->rec[h->slot * h->nClu + (u >> 5)];
                cells[n++] = r->cell[u & 31];
            }
        }
    } else if (h->phase >= PH_REFINE) {
        for (int i = 0; i < h->nRoute && n < MAX_WAYPOINTS - 1; i++) {
            int u = v->route[i];
            const ClusterRec* r = &v->rec[h->slot * h->nClu + (u >> 5)];
            cells[n++] = r->cell[u & 31];
        }
        if (n < MAX_WAYPOINTS)
            cells[n++] = h->nearGoal;
    }
    if (n < 2)
        return;
    EmitCells(v, PATH_EVENT_PROGRESS, 0, cells, n);
}

int NodeCell(const HpaView* v, int u)
{
    const ClusterRec* r = &v->rec[v->h->slot * v->h->nClu + (u >> 5)];
    return r->cell[u & 31];
}

void Touch(HpaView* v, int n)
{
    Hdr* h = v->h;
    if (v->astamp[n] != (int)h->searchId) {
        v->astamp[n] = (int)h->searchId;
        v->apos[n] = -1;
        v->ag[n] = INF;
        v->apar[n] = -1;
    }
}

int HeuristicOf(const HpaView* v, int n)
{
    if (n == v->h->NN - 1)
        return 0;
    int cell = NodeCell(v, n);
    PathGoalArea goal = v->h->req.goal;
    return PathGoalDistance(&goal, cell % v->h->W, cell / v->h->W) * STRAIGHT;
}

void Relax(HpaView* v, int from, int to, int cost)
{
    Hdr* h = v->h;
    for (int i = 0; i < h->nBanned; i++)
        if (h->banned[i] == to)
            return;
    Touch(v, to);
    int ng = v->ag[from] + cost;
    if (ng >= v->ag[to])
        return;
    v->ag[to] = ng;
    v->apar[to] = from;
    v->akey[to] = ng + HeuristicOf(v, to);
    HeapPush(v->aheap, &h->aHeapN, v->apos, v->akey, to);
}

void Invalidate(HpaView* v, int cluster)
{
    v->ver[cluster]++;
}

void ResetSearchState(Hdr* h)
{
    h->localReady = 0;
    h->gcIdx = 0;
    h->nGoalNodes = 0;
    h->directCost = -1;
    h->aInit = 0;
    h->aHeapN = 0;
    h->nRoute = 0;
    h->segIdx = 0;
    h->nPath = 0;
    h->truncated = 0;
    h->pullInit = 0;
    h->nWp = 0;
    h->wpOverflow = 0;
    h->nearest = 0;
    h->nearNode = -1;
    h->nearDist = INF;
}

void StartSearch(HpaView* v, const Req* q, int* budget)
{
    Hdr* h = v->h;
    h->nBanned = 0;
    const PathWorldView* w = v->w;
    h->active = 1;
    h->req = q->r;
    h->phase = PH_INIT;
    h->aseq = q->seq;
    h->lastTick = w->Tick();
    ResetSearchState(h);
    w->BeginSearch(q->r.unit);
    (*budget)--;

    PathUnitState us;
    if (!w->UnitState(q->r.unit, &us)) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_INDIRECT);
        FinishSearch(v, PATH_FAILED, nullptr, 0);
        return;
    }
    h->sx = q->r.useStart ? q->r.start.x : us.cell.x;
    h->sy = q->r.useStart ? q->r.start.y : us.cell.y;
    h->fx = us.footprintX;
    h->fy = us.footprintY;
    h->slot = -1;
    for (int i = 0; i < 32; i++) {
        if (h->classes[i] == us.moveClass) {
            if (h->slotOf[i] < 0 && h->slotsUsed < h->nSlots)
                h->slotOf[i] = h->slotsUsed++;
            h->slot = h->slotOf[i];
        }
    }
    int start = h->sy * h->W + h->sx;
    if (h->slot < 0 || h->sx < 0 || h->sy < 0 || h->sx >= h->W || h->sy >= h->H) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_INDIRECT);
        FinishSearch(v, PATH_FAILED, nullptr, 0);
        return;
    }
    h->startCluster = ClusterOf(h, h->sx, h->sy);
    PathGoalArea goal = h->req.goal;
    if (PathGoalContains(&goal, h->sx, h->sy)) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_DIRECT);
        FinishSearch(v, PATH_FOUND, &start, 1);
        return;
    }

    PathPoint raw[MAX_GOAL_CELLS];
    int n = PathGoalCells(&goal, raw, MAX_GOAL_CELLS);
    h->nGoal = 0;
    int best = INF;
    h->nearGoal = start;
    h->nRaw = 0;
    for (int i = 0; i < n && i < MAX_GOAL_CELLS; i++)
        h->rawGoal[h->nRaw++] = raw[i];
    for (int i = 0; i < n; i++) {
        if (CellExtra(v, us.moveClass, raw[i].x, raw[i].y, 0) < 0)
            continue;
        h->goalCells[h->nGoal++] = raw[i];
        int d = Octile(raw[i].x - h->sx, raw[i].y - h->sy);
        if (d < best) {
            best = d;
            h->nearGoal = raw[i].y * h->W + raw[i].x;
        }
    }
    if (h->nRaw == 0) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_INDIRECT);
        FinishSearch(v, PATH_FAILED, nullptr, 0);
        return;
    }
    (*budget)--;
    int gx = h->nearGoal % h->W;
    int gy = h->nearGoal / h->W;
    if (h->nGoal > 0 && w->LineOfSight(us.moveClass, h->sx, h->sy, gx, gy)) {
        int cells[2] = { start, h->nearGoal };
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_DIRECT);
        FinishSearch(v, PATH_FOUND, cells, 2);
        return;
    }
    NewEvent(v, PATH_EVENT_STARTED, PATH_START_INDIRECT);

    h->nGoalClusters = 0;
    for (int i = 0; i < h->nGoal; i++) {
        int c = ClusterOf(h, h->goalCells[i].x, h->goalCells[i].y);
        bool seen = false;
        for (int k = 0; k < h->nGoalClusters; k++)
            if (h->goalClusters[k] == c)
                seen = true;
        if (!seen && h->nGoalClusters < MAX_GOAL_CLUSTERS)
            h->goalClusters[h->nGoalClusters++] = c;
    }
    h->phase = PH_BUILD;
}

PathClass ClassOfActive(const Hdr* h)
{
    for (int i = 0; i < 32; i++)
        if (h->slotOf[i] == h->slot)
            return h->classes[i];
    return 0;
}

void RunBuild(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    Ensure(v, h->slot, h->startCluster);
    for (int i = 0; i < h->nGoalClusters; i++)
        Ensure(v, h->slot, h->goalClusters[i]);
    (*budget)--;
    Settle(h, budget);
    h->phase = PH_START_FLOOD;
    h->localReady = 0;
}

void RunStartFlood(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    Local* lc = &h->scratch[0];
    PathClass cls = ClassOfActive(h);
    if (!h->localReady) {
        LInit(lc, h, h->startCluster, 0, 0);
        LPush(lc, h, h->sx, h->sy, 0);
        h->localReady = 1;
    }
    PathGoalArea goal = h->req.goal;
    int res = LStep(lc, v, cls, &goal);
    (*budget)--;
    if (res == 0)
        return;
    const ClusterRec* r = &v->rec[h->slot * h->nClu + h->startCluster];
    for (int i = 0; i < MAX_NODES; i++) {
        h->startCost[i] = -1;
        if (i < r->count) {
            int li = LIndex(h, lc, r->cell[i] % h->W, r->cell[i] / h->W);
            if (lc->st[li] == 2)
                h->startCost[i] = lc->g[li];
        }
    }
    h->directCost = lc->goalAt >= 0 ? lc->g[lc->goalAt] : -1;
    h->phase = PH_GOAL_FLOOD;
    h->localReady = 0;
    h->gcIdx = 0;
}

void RunGoalFlood(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    Local* lc = &h->scratch[0];
    PathClass cls = ClassOfActive(h);
    if (h->gcIdx >= h->nGoalClusters) {
        h->phase = PH_ABSTRACT;
        h->aInit = 0;
        return;
    }
    int c = h->goalClusters[h->gcIdx];
    if (!h->localReady) {
        LInit(lc, h, c, 0, 0);
        for (int i = 0; i < h->nGoal; i++)
            if (ClusterOf(h, h->goalCells[i].x, h->goalCells[i].y) == c)
                LPush(lc, h, h->goalCells[i].x, h->goalCells[i].y, 0);
        h->localReady = 1;
    }
    int res = LStep(lc, v, cls, nullptr);
    (*budget)--;
    if (res == 0)
        return;
    const ClusterRec* r = &v->rec[h->slot * h->nClu + c];
    for (int i = 0; i < r->count; i++) {
        int li = LIndex(h, lc, r->cell[i] % h->W, r->cell[i] / h->W);
        if (lc->st[li] == 2 && h->nGoalNodes < MAX_GOAL_NODES) {
            GoalNode* g = &h->goalNodes[h->nGoalNodes++];
            g->cluster = c;
            g->local = i;
            g->cost = lc->g[li];
        }
    }
    h->gcIdx++;
    h->localReady = 0;
}

int GoalDistSq(const Hdr* h, int cell)
{
    int x = cell % h->W;
    int y = cell / h->W;
    int best = INF;
    for (int i = 0; i < h->nRaw; i++) {
        int dx = x - h->rawGoal[i].x;
        int dy = y - h->rawGoal[i].y;
        int d = dx * dx + dy * dy;
        if (d < best)
            best = d;
    }
    return best;
}

void BuildRoute(HpaView* v, int end)
{
    Hdr* h = v->h;
    h->nRoute = 0;
    int u = end == h->NN - 1 ? v->apar[end] : end;
    int count = 0;
    while (u >= 0) {
        v->route[count++] = u;
        u = v->apar[u];
    }
    for (int i = 0; i < count / 2; i++) {
        int t = v->route[i];
        v->route[i] = v->route[count - 1 - i];
        v->route[count - 1 - i] = t;
    }
    h->nRoute = count;
}

void RunAbstract(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    int goal = h->NN - 1;
    if (!h->aInit) {
        h->searchId++;
        h->aHeapN = 0;
        h->aInit = 1;
        const ClusterRec* r = &v->rec[h->slot * h->nClu + h->startCluster];
        for (int i = 0; i < r->count; i++) {
            if (h->startCost[i] < 0)
                continue;
            int n = (h->startCluster << 5) | i;
            Touch(v, n);
            v->ag[n] = h->startCost[i];
            v->apar[n] = -1;
            v->akey[n] = v->ag[n] + HeuristicOf(v, n);
            HeapPush(v->aheap, &h->aHeapN, v->apos, v->akey, n);
        }
        if (h->directCost >= 0) {
            Touch(v, goal);
            v->ag[goal] = h->directCost;
            v->apar[goal] = -2;
            v->akey[goal] = h->directCost;
            HeapPush(v->aheap, &h->aHeapN, v->apos, v->akey, goal);
        }
    }
    if (h->aHeapN == 0) {
        if (h->nearNode < 0) {
            int start = h->sy * h->W + h->sx;
            FinishSearch(v, PATH_NEAREST, &start, 1);
            return;
        }
        BuildRoute(v, h->nearNode);
        h->nearest = 1;
        h->phase = PH_REFINE;
        h->segIdx = 0;
        h->nPath = 1;
        v->path[0] = h->sy * h->W + h->sx;
        h->localReady = 0;
        return;
    }
    (*budget)--;
    int u = HeapPop(v->aheap, &h->aHeapN, v->apos, v->akey);
    if (u != goal) {
        int d = GoalDistSq(h, NodeCell(v, u));
        if (d < h->nearDist && d < GoalDistSq(h, h->sy * h->W + h->sx)) {
            h->nearDist = d;
            h->nearNode = u;
        }
    }
    if (u == goal) {
        BuildRoute(v, goal);
        h->phase = PH_REFINE;
        h->segIdx = 0;
        h->nPath = 1;
        v->path[0] = h->sy * h->W + h->sx;
        h->localReady = 0;
        return;
    }
    int c = u >> 5;
    int i = u & 31;
    const ClusterRec* r = &v->rec[h->slot * h->nClu + c];
    for (int k = 0; k < h->nGoalNodes; k++)
        if (h->goalNodes[k].cluster == c && h->goalNodes[k].local == i)
            Relax(v, u, goal, h->goalNodes[k].cost);
    for (int j = 0; j < r->count; j++) {
        if (j == i)
            continue;
        int cost = NodeCost(r, i, j);
        if (cost != NO_EDGE)
            Relax(v, u, (c << 5) | j, cost);
    }
    int s = r->side[i];
    int ax = r->cell[i] % h->W;
    int ay = r->cell[i] / h->W;
    int bx = ax + kSideDx[s];
    int by = ay + kSideDy[s];
    int extra = CellExtra(v, ClassOfActive(h), bx, by, 1);
    if (extra >= 0) {
        int nc = ClusterOf(h, bx, by);
        Ensure(v, h->slot, nc);
        Settle(h, budget);
        const ClusterRec* nr = &v->rec[h->slot * h->nClu + nc];
        int bcell = by * h->W + bx;
        for (int j = 0; j < nr->count; j++)
            if (nr->cell[j] == bcell)
                Relax(v, u, (nc << 5) | j, STRAIGHT + extra);
    }
}

void AppendLocalPath(HpaView* v, Local* lc)
{
    Hdr* h = v->h;
    int tmp[MAX_LOCAL];
    int n = 0;
    int u = lc->found;
    while (lc->par[u] >= 0) {
        tmp[n++] = u;
        int d = (unsigned char)lc->par[u];
        u -= kDy[d] * h->cs + kDx[d];
    }
    for (int i = n - 1; i >= 0; i--) {
        if (h->nPath >= PATH_CAP) {
            h->truncated = 1;
            return;
        }
        int x = lc->ox + (tmp[i] & (h->cs - 1));
        int y = lc->oy + (tmp[i] >> h->shift);
        v->path[h->nPath++] = y * h->W + x;
    }
}

void RunRefine(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    Local* lc = &h->scratch[0];
    PathClass cls = ClassOfActive(h);
    int last = h->nPath > 0 ? v->path[h->nPath - 1] : 0;
    int lx = last % h->W;
    int ly = last / h->W;
    if (h->segIdx > h->nRoute || h->truncated) {
        h->phase = PH_PULL;
        h->pullInit = 0;
        return;
    }
    if (!h->localReady) {
        if (h->segIdx < h->nRoute) {
            int tcell = NodeCell(v, v->route[h->segIdx]);
            int tx = tcell % h->W;
            int ty = tcell / h->W;
            if (ClusterOf(h, tx, ty) != ClusterOf(h, lx, ly)) {
                if (h->nPath >= PATH_CAP)
                    h->truncated = 1;
                else
                    v->path[h->nPath++] = tcell;
                h->segIdx++;
                return;
            }
            LInit(lc, h, ClusterOf(h, lx, ly), 1, 0);
            lc->tx = tx;
            lc->ty = ty;
        } else {
            if (h->nearest) {
                h->segIdx++;
                return;
            }
            LInit(lc, h, ClusterOf(h, lx, ly), 2, 0);
        }
        LPush(lc, h, lx, ly, 0);
        h->localReady = 1;
    }
    PathGoalArea goal = h->req.goal;
    int res = LStep(lc, v, cls, &goal);
    (*budget)--;
    if (res == 0)
        return;
    if (res == 1) {
        AppendLocalPath(v, lc);
        h->segIdx++;
        h->localReady = 0;
        return;
    }
    if (h->segIdx < h->nRoute && h->nBanned < MAX_BANNED) {
        h->banned[h->nBanned++] = v->route[h->segIdx];
        h->aInit = 0;
        h->aHeapN = 0;
        h->nRoute = 0;
        h->segIdx = 0;
        h->nPath = 0;
        h->truncated = 0;
        h->pullInit = 0;
        h->nWp = 0;
        h->wpOverflow = 0;
        h->localReady = 0;
        h->nearest = 0;
        h->nearNode = -1;
        h->nearDist = INF;
        h->phase = PH_ABSTRACT;
        return;
    }
    if (h->nPath <= 1) {
        FinishSearch(v, PATH_FAILED, nullptr, 0);
        return;
    }
    h->truncated = 1;
    h->phase = PH_PULL;
    h->pullInit = 0;
}

bool ClearLine(const HpaView* v, PathClass cls, int ax, int ay, int bx, int by)
{
    int gx = bx - ax;
    int gy = by - ay;
    if (!(gx == 0 || gy == 0 || gx == gy || gx == -gy))
        return false;
    return v->w->LineOfSight(cls, ax, ay, bx, by) != 0;
}

void RunPull(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    PathClass cls = ClassOfActive(h);
    if (!h->pullInit) {
        h->pullInit = 1;
        h->nWp = 1;
        h->wp[0] = v->path[0];
        h->pullAnchor = 0;
        h->pullK = 1;
        h->wpOverflow = 0;
    }
    bool done = false;
    if (h->pullK >= h->nPath) {
        done = true;
    } else {
        int a = v->path[h->pullAnchor];
        int b = v->path[h->pullK];
        (*budget)--;
        if (ClearLine(v, cls, a % h->W, a / h->W, b % h->W, b / h->W)) {
            h->pullK++;
        } else {
            int na = h->pullK - 1 > h->pullAnchor ? h->pullK - 1 : h->pullK;
            if (h->nWp < MAX_WAYPOINTS + 1)
                h->wp[h->nWp++] = v->path[na];
            h->pullAnchor = na;
            h->pullK = na + 1;
            if (h->nWp > MAX_WAYPOINTS) {
                h->wpOverflow = 1;
                done = true;
            }
        }
    }
    if (!done)
        return;
    if (!h->wpOverflow) {
        int lastCell = v->path[h->nPath - 1];
        if (h->wp[h->nWp - 1] != lastCell) {
            if (h->nWp < MAX_WAYPOINTS + 1)
                h->wp[h->nWp++] = lastCell;
            else
                h->wpOverflow = 1;
        }
    }
    int status = h->nearest ? PATH_NEAREST : PATH_FOUND;
    if (h->truncated || h->nWp > MAX_WAYPOINTS || h->wpOverflow)
        status = PATH_PARTIAL;
    FinishSearch(v, status, h->wp, h->nWp);
}

void RunActive(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    switch (h->phase) {
    case PH_BUILD:
        RunBuild(v, budget);
        break;
    case PH_START_FLOOD:
        RunStartFlood(v, budget);
        break;
    case PH_GOAL_FLOOD:
        RunGoalFlood(v, budget);
        break;
    case PH_ABSTRACT:
        RunAbstract(v, budget);
        break;
    case PH_REFINE:
        RunRefine(v, budget);
        break;
    case PH_PULL:
        RunPull(v, budget);
        break;
    default:
        h->active = 0;
        break;
    }
}

Req* PickNext(HpaView* v, unsigned tick)
{
    Hdr* h = v->h;
    Req* best = nullptr;
    for (int i = 0; i < h->reqCap; i++) {
        Req* q = &v->req[i];
        if (!q->used || q->r.notBefore > tick)
            continue;
        if (best == nullptr || q->seq < best->seq)
            best = q;
    }
    return best;
}

void PurgeEvents(HpaView* v, PathHandle requester)
{
    Hdr* h = v->h;
    int out = h->evHead;
    for (int i = h->evHead; i < h->evCount; i++) {
        if (v->ev[i].e.requester == requester)
            continue;
        if (out != i)
            v->ev[out] = v->ev[i];
        out++;
    }
    h->evCount = out;
}

struct Layout {
    int W, H, cs, shift, nClu, nSlots, reqCap, NN;
    int offVer, offRec, offReq, offEv, offPts;
    int offAG, offAPar, offAStamp, offAKey, offAPos, offAHeap, offRoute, offPath;
    int size;
};

Layout LayoutFor(const PathMapInfo* map)
{
    Layout l;
    l.W = map->width;
    l.H = map->height;
    int nClasses = map->moveClassCount < 1 ? 1 : map->moveClassCount;
    l.cs = 16;
    l.shift = 4;
    l.nClu = ((l.W + l.cs - 1) >> l.shift) * ((l.H + l.cs - 1) >> l.shift);
    if (l.nClu * (int)sizeof(ClusterRec) * nClasses > CLASS_BUDGET_BYTES) {
        l.cs = 32;
        l.shift = 5;
        l.nClu = ((l.W + l.cs - 1) >> l.shift) * ((l.H + l.cs - 1) >> l.shift);
    }
    l.nSlots = nClasses;
    l.reqCap = map->unitSlots > 16 ? map->unitSlots : 16;
    l.NN = l.nClu * 32 + 1;

    int off = Align8((int)sizeof(Hdr));
    l.offVer = off;
    off = Align8(off + l.nClu * 4);
    l.offRec = off;
    off = Align8(off + l.nSlots * l.nClu * (int)sizeof(ClusterRec));
    l.offReq = off;
    off = Align8(off + l.reqCap * (int)sizeof(Req));
    l.offEv = off;
    off = Align8(off + EVENT_CAP * (int)sizeof(Ev));
    l.offPts = off;
    off = Align8(off + POINT_CAP * (int)sizeof(PathPoint));
    l.offAG = off;
    off += l.NN * 4;
    l.offAPar = off;
    off += l.NN * 4;
    l.offAStamp = off;
    off += l.NN * 4;
    l.offAKey = off;
    off += l.NN * 4;
    l.offAPos = off;
    off += l.NN * 4;
    l.offAHeap = off;
    off += l.NN * 4;
    l.offRoute = off;
    off += l.NN * 4;
    l.offPath = off;
    off += PATH_CAP * 4;
    l.size = off;
    return l;
}

unsigned int __stdcall HpaRegionSize(const PathMapInfo* map)
{
    return (unsigned int)LayoutFor(map).size;
}

void __stdcall HpaInit(const PathMapInfo* map, void* regionBase, unsigned int)
{
    const PathWorldView* w = World();
    Layout l = LayoutFor(map);
    char* region = (char*)regionBase;
    Hdr* h = (Hdr*)region;
    h->magic = MAGIC;
    h->W = l.W;
    h->H = l.H;
    h->cs = l.cs;
    h->shift = l.shift;
    h->CW = (l.W + l.cs - 1) >> l.shift;
    h->CH = (l.H + l.cs - 1) >> l.shift;
    h->nClu = l.nClu;
    h->NN = l.NN;
    h->nSlots = l.nSlots;
    h->reqCap = l.reqCap;
    h->offVer = l.offVer;
    h->offRec = l.offRec;
    h->offReq = l.offReq;
    h->offEv = l.offEv;
    h->offPts = l.offPts;
    h->offAG = l.offAG;
    h->offAPar = l.offAPar;
    h->offAStamp = l.offAStamp;
    h->offAKey = l.offAKey;
    h->offAPos = l.offAPos;
    h->offAHeap = l.offAHeap;
    h->offRoute = l.offRoute;
    h->offPath = l.offPath;
    for (int i = 0; i < 32; i++) {
        h->classes[i] = w->MoveClass(i);
        h->slotOf[i] = -1;
    }
    int* ver = (int*)(region + l.offVer);
    for (int i = 0; i < l.nClu; i++)
        ver[i] = 1;
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)region);
}

void* __stdcall HpaShutdown()
{
    Hdr* h = (Hdr*)g_game->pathfinder.get();
    if (h != nullptr && h->magic == MAGIC) {
        HpaView v;
        if (Open(&v) && h->active)
            v.w->EndSearch(h->req.unit);
    }
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)nullptr);
    return h;
}

void __stdcall HpaSubmit(const PathRequest* request)
{
    HpaView v;
    if (!Open(&v))
        return;
    Hdr* h = v.h;
    if (h->active && h->req.requester == request->requester)
        return;
    Req* free = nullptr;
    for (int i = 0; i < h->reqCap; i++) {
        Req* q = &v.req[i];
        if (q->used && q->r.requester == request->requester) {
            q->r = *request;
            return;
        }
        if (!q->used && free == nullptr)
            free = q;
    }
    if (free == nullptr)
        return;
    free->r = *request;
    free->seq = ++h->seqCounter;
    free->used = 1;
}

void __stdcall HpaCancel(PathHandle requester)
{
    HpaView v;
    if (!Open(&v))
        return;
    Hdr* h = v.h;
    for (int i = 0; i < h->reqCap; i++)
        if (v.req[i].used && v.req[i].r.requester == requester)
            v.req[i].used = 0;
    if (h->active && h->req.requester == requester) {
        v.w->EndSearch(h->req.unit);
        h->active = 0;
    }
    PurgeEvents(&v, requester);
}

void __stdcall HpaCellsChanged(int left, int top, int right, int bottom, unsigned int classes, int cause)
{
    if (cause != PATH_CHANGE_TERRAIN || classes == 0)
        return;
    HpaView v;
    if (!Open(&v))
        return;
    Hdr* h = v.h;
    int x0 = (left - 1 < 0 ? 0 : left - 1) >> h->shift;
    int y0 = (top - 1 < 0 ? 0 : top - 1) >> h->shift;
    int x1 = (right + 1 >= h->W ? h->W - 1 : right + 1) >> h->shift;
    int y1 = (bottom + 1 >= h->H ? h->H - 1 : bottom + 1) >> h->shift;
    for (int cy = y0; cy <= y1; cy++)
        for (int cx = x0; cx <= x1; cx++)
            v.ver[cy * h->CW + cx]++;
    if (h->active && h->phase >= PH_BUILD) {
        ResetSearchState(h);
        h->phase = PH_BUILD;
    }
}

void __stdcall HpaStep()
{
    HpaView v;
    if (!Open(&v))
        return;
    Hdr* h = v.h;
    unsigned tick = v.w->Tick();
    h->lastTick = tick;
    h->evHead = 0;
    h->evCount = 0;
    h->nPts = 0;
    unsigned int limit = v.w->SearchStepLimit();
    int budget = limit != 0 ? (int)limit : STEP_BUDGET;
    Settle(h, &budget);
    while (budget > 0 && h->evCount < EVENT_CAP - 4) {
        if (!h->active) {
            Req* q = PickNext(&v, tick);
            if (q == nullptr)
                break;
            Req copy = *q;
            q->used = 0;
            StartSearch(&v, &copy, &budget);
            continue;
        }
        RunActive(&v, &budget);
    }
    if (h->active)
        EmitProgress(&v);
}

int __stdcall HpaPoll(PathEvent* out, PathPoint* points, int capacity)
{
    HpaView v;
    if (!Open(&v))
        return 0;
    Hdr* h = v.h;
    if (h->evHead >= h->evCount)
        return 0;
    const Ev* rec = &v.ev[h->evHead++];
    *out = rec->e;
    for (int i = 0; i < rec->e.count && i < capacity; i++)
        points[i] = v.pts[rec->first + i];
    return 1;
}

void WriteRequest(const StateWriter* w, int pending, int index, const PathRequest* r, unsigned seq)
{
    if (pending) {
        w->Int("request[%u].seq", index, 4, seq);
        w->Handle("request[%u].unit", index, STATE_HANDLE_UNIT, r->unit);
        w->Handle("request[%u].requester", index, STATE_HANDLE_HEAP, r->requester);
        w->Handle("request[%u].target", index, STATE_HANDLE_HEAP, r->target);
        w->Int("request[%u].notBefore", index, 4, r->notBefore);
        w->Bytes("request[%u].goal", index, &r->goal, sizeof(r->goal));
        w->Int("request[%u].useStart", index, 4, r->useStart);
        w->Bytes("request[%u].start", index, &r->start, sizeof(r->start));
    } else {
        w->Int("search.seq", 0, 4, seq);
        w->Handle("search.unit", 0, STATE_HANDLE_UNIT, r->unit);
        w->Handle("search.requester", 0, STATE_HANDLE_HEAP, r->requester);
        w->Handle("search.target", 0, STATE_HANDLE_HEAP, r->target);
        w->Int("search.notBefore", 0, 4, r->notBefore);
        w->Bytes("search.goal", 0, &r->goal, sizeof(r->goal));
        w->Int("search.useStart", 0, 4, r->useStart);
        w->Bytes("search.start", 0, &r->start, sizeof(r->start));
    }
}

void WriteSearch(const StateWriter* w, HpaView* v)
{
    Hdr* h = v->h;
    WriteRequest(w, 0, 0, &h->req, h->aseq);
    w->Int("search.phase", 0, 4, h->phase);
    w->Int("search.slot", 0, 4, h->slot);
    w->Int("search.sx", 0, 4, h->sx);
    w->Int("search.sy", 0, 4, h->sy);
    w->Int("search.fx", 0, 4, h->fx);
    w->Int("search.fy", 0, 4, h->fy);
    w->Int("search.startCluster", 0, 4, h->startCluster);
    w->Int("search.nGoal", 0, 4, h->nGoal);
    w->Bytes("search.goalCells", 0, h->goalCells, h->nGoal * (int)sizeof(PathPoint));
    w->Int("search.nRaw", 0, 4, h->nRaw);
    w->Bytes("search.rawGoal", 0, h->rawGoal, h->nRaw * (int)sizeof(PathPoint));
    w->Int("search.nearGoal", 0, 4, h->nearGoal);
    w->Int("search.nGoalClusters", 0, 4, h->nGoalClusters);
    w->Bytes("search.goalClusters", 0, h->goalClusters, h->nGoalClusters * 4);
    w->Int("search.gcIdx", 0, 4, h->gcIdx);
    w->Int("search.localReady", 0, 4, h->localReady);
    w->Int("search.directCost", 0, 4, h->directCost);
    w->Bytes("search.startCost", 0, h->startCost, sizeof(h->startCost));
    w->Int("search.nGoalNodes", 0, 4, h->nGoalNodes);
    w->Bytes("search.goalNodes", 0, h->goalNodes, h->nGoalNodes * (int)sizeof(GoalNode));
    w->Int("search.aHeapN", 0, 4, h->aHeapN);
    w->Int("search.aInit", 0, 4, h->aInit);
    w->Int("search.nRoute", 0, 4, h->nRoute);
    w->Int("search.segIdx", 0, 4, h->segIdx);
    w->Int("search.nPath", 0, 4, h->nPath);
    w->Int("search.truncated", 0, 4, h->truncated);
    w->Int("search.pullAnchor", 0, 4, h->pullAnchor);
    w->Int("search.pullK", 0, 4, h->pullK);
    w->Int("search.nWp", 0, 4, h->nWp);
    w->Bytes("search.wp", 0, h->wp, h->nWp * 4);
    w->Int("search.wpOverflow", 0, 4, h->wpOverflow);
    w->Int("search.pullInit", 0, 4, h->pullInit);
    w->Int("search.nearest", 0, 4, h->nearest);
    w->Int("search.nearNode", 0, 4, h->nearNode);
    w->Int("search.nearDist", 0, 4, h->nearDist);
    w->Int("search.nBanned", 0, 4, h->nBanned);
    w->Bytes("search.banned", 0, h->banned, h->nBanned * 4);
    if (h->localReady) {
        const Local* lc = &h->scratch[0];
        int cells = h->cs * h->cs;
        w->Int("local.ox", 0, 4, lc->ox);
        w->Int("local.oy", 0, 4, lc->oy);
        w->Int("local.w", 0, 4, lc->w);
        w->Int("local.h", 0, 4, lc->h);
        w->Int("local.mode", 0, 4, lc->mode);
        w->Int("local.terrain", 0, 4, lc->terrain);
        w->Int("local.tx", 0, 4, lc->tx);
        w->Int("local.ty", 0, 4, lc->ty);
        w->Int("local.n", 0, 4, lc->n);
        w->Int("local.found", 0, 4, lc->found);
        w->Int("local.goalAt", 0, 4, lc->goalAt);
        w->Bytes("local.st", 0, lc->st, cells);
        w->Bytes("local.hpos", 0, lc->hpos, cells * 4);
        w->Bytes("local.heap", 0, lc->heap, lc->n * 4);
        for (int i = 0; i < cells; i++) {
            if (lc->st[i] == 0)
                continue;
            w->Int("local.g[%u]", i, 4, lc->g[i]);
            w->Int("local.key[%u]", i, 4, lc->key[i]);
            w->Int("local.par[%u]", i, 1, (unsigned char)lc->par[i]);
        }
    }
    if (h->phase >= PH_ABSTRACT) {
        for (int n = 0; n < h->NN; n++) {
            if (v->astamp[n] != (int)h->searchId)
                continue;
            w->Int("abstract.ag[%u]", n, 4, v->ag[n]);
            w->Int("abstract.apar[%u]", n, 4, v->apar[n]);
            w->Int("abstract.akey[%u]", n, 4, v->akey[n]);
            w->Int("abstract.apos[%u]", n, 4, v->apos[n]);
        }
        w->Bytes("abstract.heap", 0, v->aheap, h->aHeapN * 4);
        w->Bytes("abstract.route", 0, v->route, h->nRoute * 4);
        w->Bytes("abstract.path", 0, v->path, h->nPath * 4);
    }
}

void __stdcall HpaWriteState(const StateWriter* w)
{
    w->Begin(16, "hpa");
    HpaView v;
    if (!Open(&v)) {
        w->Int(nullptr, 0, 1, 0);
        return;
    }
    Hdr* h = v.h;
    w->Int(nullptr, 0, 1, 1);
    w->Int("width", 0, 4, h->W);
    w->Int("height", 0, 4, h->H);
    w->Int("clusterSize", 0, 4, h->cs);
    w->Int("clusters", 0, 4, h->nClu);
    w->Int("slots", 0, 4, h->nSlots);
    w->Int("slotsUsed", 0, 4, h->slotsUsed);
    w->Int("seqCounter", 0, 4, h->seqCounter);
    w->Int("searchId", 0, 4, h->searchId);
    w->Int("debt", 0, 4, h->debt);
    w->Int("lastTick", 0, 4, h->lastTick);
    w->Int("active", 0, 4, h->active);
    for (int i = 0; i < 32; i++)
        w->Handle("class[%u]", i, STATE_HANDLE_MOVE_CLASS, h->classes[i]);
    w->Bytes("slotOf", 0, h->slotOf, sizeof(h->slotOf));
    w->Bytes("version", 0, v.ver, h->nClu * 4);
    for (int slot = 0; slot < h->slotsUsed; slot++) {
        for (int c = 0; c < h->nClu; c++) {
            const ClusterRec* r = &v.rec[slot * h->nClu + c];
            if (r->version != v.ver[c])
                continue;
            int index = slot * h->nClu + c;
            w->Int("cluster[%u].count", index, 4, r->count);
            w->Bytes("cluster[%u].cell", index, r->cell, r->count * 4);
            w->Bytes("cluster[%u].side", index, r->side, r->count);
            for (int i = 0; i + 1 < r->count; i++)
                w->Bytes("cluster[%u].cost", index, &r->cost[TriIndex(i, i + 1)], (r->count - 1 - i) * 2);
        }
    }
    for (int i = 0; i < h->reqCap; i++) {
        if (v.req[i].used)
            WriteRequest(w, 1, i, &v.req[i].r, v.req[i].seq);
    }
    if (h->active)
        WriteSearch(w, &v);
}

PathfindingSeam s_hpa = {
    HpaRegionSize, HpaInit, HpaShutdown, HpaSubmit, HpaCancel, HpaCellsChanged, HpaStep, HpaPoll, HpaWriteState
};

}

PathfindingSeam* PathfindingHpaSeam()
{
    return &s_hpa;
}
