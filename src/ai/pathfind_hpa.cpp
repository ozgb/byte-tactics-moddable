#include <stddef.h>

#include "../../interfaces/seams.h"
#include "../../interfaces/ref.h"
#include "../state/block.h"
#include "../network/player.h"
#include "../game/game.h"

#include "pathfind_hpa.h"

#include "../../link/globals.h"

namespace {

enum {
    kMagic = 0x31415048,
    kStepBudget = 0x535,
    kMaxNodes = 24,
    kMaxPerSide = 6,
    kTri = kMaxNodes * (kMaxNodes - 1) / 2,
    kMaxGoalCells = 64,
    kMaxGoalClusters = 8,
    kMaxGoalNodes = kMaxGoalClusters * kMaxNodes,
    kEventCap = 128,
    kPathCap = 8192,
    kMaxLocal = 32 * 32,
    kStraight = 10,
    kDiagonal = 14,
    kPassOneExtra = 30,
    kInf = 0x3fffffff,
    kNoEdge = 0xffff,
    kClassBudgetBytes = 12 << 20
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
    int cell[kMaxNodes];
    unsigned char side[kMaxNodes];
    unsigned short cost[kTri];
};

struct Local {
    int ox, oy, w, h;
    int mode;
    int tx, ty;
    int n;
    int found;
    int goalAt;
    int g[kMaxLocal];
    int key[kMaxLocal];
    int heap[kMaxLocal];
    int hpos[kMaxLocal];
    signed char par[kMaxLocal];
    unsigned char st[kMaxLocal];
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
    PathClass classes[32];
    int slotOf[32];
    int slotsUsed;
    unsigned seqCounter;
    unsigned searchId;
    int debt;
    int evHead, evCount;
    unsigned lastTick;

    int offVer, offRec, offReq, offEv;
    int offAG, offAPar, offAStamp, offAKey, offAPos, offAHeap;
    int offRoute, offPath;

    int active;
    PathRequest req;
    unsigned aseq;
    int phase;
    int slot;
    int sx, sy, fx, fy;
    int startCluster;
    PathPoint goalCells[kMaxGoalCells];
    int nGoal;
    int nearGoal;
    int goalClusters[kMaxGoalClusters];
    int nGoalClusters;
    int gcIdx;
    int localReady;
    int directCost;
    int startCost[kMaxNodes];
    GoalNode goalNodes[kMaxGoalNodes];
    int nGoalNodes;
    int aHeapN;
    int aInit;
    int nRoute;
    int segIdx;
    int nPath;
    int truncated;
    int pullAnchor, pullK;
    int nWp;
    int wp[PATH_MAX_WAYPOINTS + 2];
    int wpOverflow;
    int pullInit;
    int retried;

    Local scratch[2];
};

struct HpaView {
    Hdr* h;
    int* ver;
    ClusterRec* rec;
    Req* req;
    PathEvent* ev;
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
    if (h == 0 || h->magic != kMagic)
        return false;
    char* b = (char*)h;
    v->h = h;
    v->ver = (int*)(b + h->offVer);
    v->rec = (ClusterRec*)(b + h->offRec);
    v->req = (Req*)(b + h->offReq);
    v->ev = (PathEvent*)(b + h->offEv);
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
    return hi * kStraight + lo * (kDiagonal - kStraight);
}

int CellExtra(const HpaView* v, PathClass cls, int x, int y)
{
    const Hdr* h = v->h;
    if (x < 0 || y < 0 || x >= h->W || y >= h->H)
        return -1;
    if (!v->w->InBounds(cls, x, y))
        return -1;
    int p = v->w->Pass(cls, x, y);
    if (p <= 0)
        return -1;
    return p == 1 ? kPassOneExtra : 0;
}

void LInit(Local* L, const Hdr* h, int cluster, int mode)
{
    L->ox = (cluster % h->CW) << h->shift;
    L->oy = (cluster / h->CW) << h->shift;
    L->w = h->W - L->ox < h->cs ? h->W - L->ox : h->cs;
    L->h = h->H - L->oy < h->cs ? h->H - L->oy : h->cs;
    L->mode = mode;
    L->tx = L->ty = 0;
    L->n = 0;
    L->found = -1;
    L->goalAt = -1;
    int cells = h->cs * h->cs;
    for (int i = 0; i < cells; i++) {
        L->st[i] = 0;
        L->hpos[i] = -1;
    }
}

inline int LIndex(const Hdr* h, const Local* L, int x, int y)
{
    return ((y - L->oy) << h->shift) + (x - L->ox);
}

void LPush(Local* L, const Hdr* h, int x, int y, int g)
{
    int i = LIndex(h, L, x, y);
    L->st[i] = 1;
    L->g[i] = g;
    L->par[i] = -1;
    L->key[i] = g;
    if (L->mode == 1)
        L->key[i] += Octile(x - L->tx, y - L->ty);
    HeapPush(L->heap, &L->n, L->hpos, L->key, i);
}

int LStep(Local* L, const HpaView* v, PathClass cls, const PathGoalArea* goal)
{
    const Hdr* h = v->h;
    if (L->n == 0)
        return 2;
    int u = HeapPop(L->heap, &L->n, L->hpos, L->key);
    L->st[u] = 2;
    int ux = L->ox + (u & (h->cs - 1));
    int uy = L->oy + (u >> h->shift);
    if (L->mode == 1 && ux == L->tx && uy == L->ty) {
        L->found = u;
        return 1;
    }
    if (L->mode == 2 && PathGoalContains(goal, ux, uy)) {
        L->found = u;
        return 1;
    }
    if (L->mode == 0 && goal != 0 && L->goalAt < 0 && PathGoalContains(goal, ux, uy))
        L->goalAt = u;
    for (int d = 0; d < 8; d++) {
        int nx = ux + kDx[d];
        int ny = uy + kDy[d];
        if (nx < L->ox || ny < L->oy || nx >= L->ox + L->w || ny >= L->oy + L->h)
            continue;
        int vi = LIndex(h, L, nx, ny);
        if (L->st[vi] == 2)
            continue;
        int extra = CellExtra(v, cls, nx, ny);
        if (extra < 0)
            continue;
        int step = kStraight;
        if (d & 1) {
            if (CellExtra(v, cls, nx, uy) < 0 || CellExtra(v, cls, ux, ny) < 0)
                continue;
            step = kDiagonal;
        }
        int ng = L->g[u] + step + extra;
        if (L->st[vi] == 0 || ng < L->g[vi]) {
            L->g[vi] = ng;
            L->par[vi] = (signed char)d;
            L->st[vi] = 1;
            L->key[vi] = ng;
            if (L->mode == 1)
                L->key[vi] += Octile(nx - L->tx, ny - L->ty);
            HeapPush(L->heap, &L->n, L->hpos, L->key, vi);
        }
    }
    return 0;
}

inline int TriIndex(int i, int j)
{
    return i * (2 * kMaxNodes - i - 1) / 2 + (j - i - 1);
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
    Local* L = &h->scratch[1];
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
                open = CellExtra(v, cls, ax, ay) >= 0 && CellExtra(v, cls, bx, by) >= 0;
            }
            if (open) {
                if (runStart < 0)
                    runStart = i;
            } else if (runStart >= 0) {
                if (segments < kMaxPerSide && r->count < kMaxNodes) {
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
    for (int i = 0; i < kTri; i++)
        r->cost[i] = kNoEdge;
    for (int i = 0; i + 1 < r->count; i++) {
        int sx = r->cell[i] % h->W;
        int sy = r->cell[i] / h->W;
        LInit(L, h, c, 0);
        LPush(L, h, sx, sy, 0);
        for (;;) {
            int res = LStep(L, v, cls, 0);
            work++;
            if (res != 0)
                break;
            bool all = true;
            for (int j = i + 1; j < r->count; j++) {
                int lj = LIndex(h, L, r->cell[j] % h->W, r->cell[j] / h->W);
                if (L->st[lj] != 2) {
                    all = false;
                    break;
                }
            }
            if (all)
                break;
        }
        for (int j = i + 1; j < r->count; j++) {
            int lj = LIndex(h, L, r->cell[j] % h->W, r->cell[j] / h->W);
            if (L->st[lj] == 2) {
                int g = L->g[lj];
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
    p.x = (short)((cell % h->W) * 16 + h->fx * 8 + 8);
    p.y = (short)((cell / h->W) * 16 + h->fy * 8 + 8);
    return p;
}

PathEvent* NewEvent(HpaView* v, int kind, int status)
{
    Hdr* h = v->h;
    if (h->evCount >= kEventCap)
        return 0;
    PathEvent* e = &v->ev[h->evCount++];
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
    if (e == 0)
        return;
    if (n > PATH_MAX_WAYPOINTS)
        n = PATH_MAX_WAYPOINTS;
    for (int i = 0; i < n; i++)
        e->points[i] = WorldPoint(v->h, cells[i]);
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
    int cells[PATH_MAX_WAYPOINTS];
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
        for (int i = 0; i < h->nRoute && n < PATH_MAX_WAYPOINTS - 1; i++) {
            int u = v->route[i];
            const ClusterRec* r = &v->rec[h->slot * h->nClu + (u >> 5)];
            cells[n++] = r->cell[u & 31];
        }
        if (n < PATH_MAX_WAYPOINTS)
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
        v->ag[n] = kInf;
        v->apar[n] = -1;
    }
}

int HeuristicOf(const HpaView* v, int n)
{
    if (n == v->h->NN - 1)
        return 0;
    int cell = NodeCell(v, n);
    PathGoalArea goal = v->h->req.goal;
    return PathGoalDistance(&goal, cell % v->h->W, cell / v->h->W) * kStraight;
}

void Relax(HpaView* v, int from, int to, int cost)
{
    Hdr* h = v->h;
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
}

void StartSearch(HpaView* v, const Req* q, int* budget)
{
    Hdr* h = v->h;
    const PathWorldView* w = v->w;
    h->active = 1;
    h->req = q->r;
    h->phase = PH_INIT;
    h->retried = 0;
    h->aseq = q->seq;
    h->lastTick = w->Tick();
    ResetSearchState(h);
    w->BeginSearch(q->r.unit);
    (*budget)--;

    PathUnitState us;
    if (!w->UnitState(q->r.unit, &us)) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_INDIRECT);
        FinishSearch(v, PATH_FAILED, 0, 0);
        return;
    }
    h->sx = us.cell.x;
    h->sy = us.cell.y;
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
        FinishSearch(v, PATH_FAILED, 0, 0);
        return;
    }
    h->startCluster = ClusterOf(h, h->sx, h->sy);
    PathGoalArea goal = h->req.goal;
    if (PathGoalContains(&goal, h->sx, h->sy)) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_DIRECT);
        FinishSearch(v, PATH_FOUND, &start, 1);
        return;
    }

    PathPoint raw[kMaxGoalCells];
    int n = PathGoalCells(&goal, raw, kMaxGoalCells);
    h->nGoal = 0;
    int best = kInf;
    h->nearGoal = start;
    for (int i = 0; i < n; i++) {
        if (CellExtra(v, us.moveClass, raw[i].x, raw[i].y) < 0)
            continue;
        h->goalCells[h->nGoal++] = raw[i];
        int d = Octile(raw[i].x - h->sx, raw[i].y - h->sy);
        if (d < best) {
            best = d;
            h->nearGoal = raw[i].y * h->W + raw[i].x;
        }
    }
    if (h->nGoal == 0) {
        NewEvent(v, PATH_EVENT_STARTED, PATH_START_INDIRECT);
        FinishSearch(v, PATH_FAILED, 0, 0);
        return;
    }
    (*budget)--;
    int gx = h->nearGoal % h->W;
    int gy = h->nearGoal / h->W;
    if (w->LineOfSight(us.moveClass, h->sx, h->sy, gx, gy)) {
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
        if (!seen && h->nGoalClusters < kMaxGoalClusters)
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
    Local* L = &h->scratch[0];
    PathClass cls = ClassOfActive(h);
    if (!h->localReady) {
        LInit(L, h, h->startCluster, 0);
        LPush(L, h, h->sx, h->sy, 0);
        h->localReady = 1;
    }
    PathGoalArea goal = h->req.goal;
    int res = LStep(L, v, cls, &goal);
    (*budget)--;
    if (res == 0)
        return;
    const ClusterRec* r = &v->rec[h->slot * h->nClu + h->startCluster];
    for (int i = 0; i < kMaxNodes; i++) {
        h->startCost[i] = -1;
        if (i < r->count) {
            int li = LIndex(h, L, r->cell[i] % h->W, r->cell[i] / h->W);
            if (L->st[li] == 2)
                h->startCost[i] = L->g[li];
        }
    }
    h->directCost = L->goalAt >= 0 ? L->g[L->goalAt] : -1;
    h->phase = PH_GOAL_FLOOD;
    h->localReady = 0;
    h->gcIdx = 0;
}

void RunGoalFlood(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    Local* L = &h->scratch[0];
    PathClass cls = ClassOfActive(h);
    if (h->gcIdx >= h->nGoalClusters) {
        h->phase = PH_ABSTRACT;
        h->aInit = 0;
        return;
    }
    int c = h->goalClusters[h->gcIdx];
    if (!h->localReady) {
        LInit(L, h, c, 0);
        for (int i = 0; i < h->nGoal; i++)
            if (ClusterOf(h, h->goalCells[i].x, h->goalCells[i].y) == c)
                LPush(L, h, h->goalCells[i].x, h->goalCells[i].y, 0);
        h->localReady = 1;
    }
    int res = LStep(L, v, cls, 0);
    (*budget)--;
    if (res == 0)
        return;
    const ClusterRec* r = &v->rec[h->slot * h->nClu + c];
    for (int i = 0; i < r->count; i++) {
        int li = LIndex(h, L, r->cell[i] % h->W, r->cell[i] / h->W);
        if (L->st[li] == 2 && h->nGoalNodes < kMaxGoalNodes) {
            GoalNode* g = &h->goalNodes[h->nGoalNodes++];
            g->cluster = c;
            g->local = i;
            g->cost = L->g[li];
        }
    }
    h->gcIdx++;
    h->localReady = 0;
}

void BuildRoute(HpaView* v)
{
    Hdr* h = v->h;
    int goal = h->NN - 1;
    h->nRoute = 0;
    int u = v->apar[goal];
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
        Invalidate(v, h->startCluster);
        for (int i = 0; i < h->nGoalClusters; i++)
            Invalidate(v, h->goalClusters[i]);
        FinishSearch(v, PATH_FAILED, 0, 0);
        return;
    }
    (*budget)--;
    int u = HeapPop(v->aheap, &h->aHeapN, v->apos, v->akey);
    if (u == goal) {
        BuildRoute(v);
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
        if (cost != kNoEdge)
            Relax(v, u, (c << 5) | j, cost);
    }
    int s = r->side[i];
    int ax = r->cell[i] % h->W;
    int ay = r->cell[i] / h->W;
    int bx = ax + kSideDx[s];
    int by = ay + kSideDy[s];
    int extra = CellExtra(v, ClassOfActive(h), bx, by);
    if (extra >= 0) {
        int nc = ClusterOf(h, bx, by);
        Ensure(v, h->slot, nc);
        Settle(h, budget);
        const ClusterRec* nr = &v->rec[h->slot * h->nClu + nc];
        int bcell = by * h->W + bx;
        for (int j = 0; j < nr->count; j++)
            if (nr->cell[j] == bcell)
                Relax(v, u, (nc << 5) | j, kStraight + extra);
    }
}

void AppendLocalPath(HpaView* v, Local* L)
{
    Hdr* h = v->h;
    int tmp[kMaxLocal];
    int n = 0;
    int u = L->found;
    while (L->par[u] >= 0) {
        tmp[n++] = u;
        int d = L->par[u];
        u -= kDy[d] * h->cs + kDx[d];
    }
    for (int i = n - 1; i >= 0; i--) {
        if (h->nPath >= kPathCap) {
            h->truncated = 1;
            return;
        }
        int x = L->ox + (tmp[i] & (h->cs - 1));
        int y = L->oy + (tmp[i] >> h->shift);
        v->path[h->nPath++] = y * h->W + x;
    }
}

void RunRefine(HpaView* v, int* budget)
{
    Hdr* h = v->h;
    Local* L = &h->scratch[0];
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
                if (h->nPath >= kPathCap)
                    h->truncated = 1;
                else
                    v->path[h->nPath++] = tcell;
                h->segIdx++;
                return;
            }
            LInit(L, h, ClusterOf(h, lx, ly), 1);
            L->tx = tx;
            L->ty = ty;
        } else {
            LInit(L, h, ClusterOf(h, lx, ly), 2);
        }
        LPush(L, h, lx, ly, 0);
        h->localReady = 1;
    }
    PathGoalArea goal = h->req.goal;
    int res = LStep(L, v, cls, &goal);
    (*budget)--;
    if (res == 0)
        return;
    if (res == 1) {
        AppendLocalPath(v, L);
        h->segIdx++;
        h->localReady = 0;
        return;
    }
    Invalidate(v, ClusterOf(h, lx, ly));
    if (h->segIdx < h->nRoute) {
        int tcell = NodeCell(v, v->route[h->segIdx]);
        Invalidate(v, ClusterOf(h, tcell % h->W, tcell / h->W));
    }
    if (!h->retried) {
        h->retried = 1;
        ResetSearchState(h);
        h->phase = PH_BUILD;
        return;
    }
    if (h->nPath <= 1) {
        FinishSearch(v, PATH_FAILED, 0, 0);
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
            if (h->nWp < PATH_MAX_WAYPOINTS + 1)
                h->wp[h->nWp++] = v->path[na];
            h->pullAnchor = na;
            h->pullK = na + 1;
            if (h->nWp > PATH_MAX_WAYPOINTS) {
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
            if (h->nWp < PATH_MAX_WAYPOINTS + 1)
                h->wp[h->nWp++] = lastCell;
            else
                h->wpOverflow = 1;
        }
    }
    int status = PATH_FOUND;
    if (h->truncated || h->nWp > PATH_MAX_WAYPOINTS || h->wpOverflow)
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
    Req* best = 0;
    for (int i = 0; i < h->reqCap; i++) {
        Req* q = &v->req[i];
        if (!q->used || q->r.notBefore > tick)
            continue;
        if (best == 0 || q->seq < best->seq)
            best = q;
    }
    return best;
}

void PurgeEvents(HpaView* v, PathHandle requester)
{
    Hdr* h = v->h;
    int out = h->evHead;
    for (int i = h->evHead; i < h->evCount; i++) {
        if (v->ev[i].requester == requester)
            continue;
        if (out != i)
            v->ev[out] = v->ev[i];
        out++;
    }
    h->evCount = out;
}

void __stdcall HpaInit(const PathMapInfo* map)
{
    const PathWorldView* w = World();
    Hdr* old = (Hdr*)g_game->pathfinder.get();
    if (old != 0 && old->magic == kMagic)
        SimFree(old);
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)0);

    int W = map->width;
    int H = map->height;
    int cs = 16;
    int shift = 4;
    int nClu = ((W + cs - 1) >> shift) * ((H + cs - 1) >> shift);
    int nClasses = 0;
    for (int i = 0; i < 32; i++)
        if (w->MoveClass(i) != 0)
            nClasses++;
    if (nClu * (int)sizeof(ClusterRec) * (nClasses < 6 ? nClasses : 6) > kClassBudgetBytes) {
        cs = 32;
        shift = 5;
        nClu = ((W + cs - 1) >> shift) * ((H + cs - 1) >> shift);
    }
    int perSlot = nClu * (int)sizeof(ClusterRec);
    int nSlots = kClassBudgetBytes / perSlot;
    if (nSlots > nClasses)
        nSlots = nClasses;
    if (nSlots < 1)
        nSlots = 1;
    int reqCap = map->unitSlots > 16 ? map->unitSlots : 16;
    int NN = nClu * 32 + 1;

    int off = Align8((int)sizeof(Hdr));
    int offVer = off;
    off = Align8(off + nClu * 4);
    int offRec = off;
    off = Align8(off + nSlots * perSlot);
    int offReq = off;
    off = Align8(off + reqCap * (int)sizeof(Req));
    int offEv = off;
    off = Align8(off + kEventCap * (int)sizeof(PathEvent));
    int offAG = off;
    off += NN * 4;
    int offAPar = off;
    off += NN * 4;
    int offAStamp = off;
    off += NN * 4;
    int offAKey = off;
    off += NN * 4;
    int offAPos = off;
    off += NN * 4;
    int offAHeap = off;
    off += NN * 4;
    int offRoute = off;
    off += NN * 4;
    int offPath = off;
    off += kPathCap * 4;

    char* region = (char*)SimAllocTagged("AISearch region", (unsigned)off);
    Hdr* h = (Hdr*)region;
    h->magic = kMagic;
    h->W = W;
    h->H = H;
    h->cs = cs;
    h->shift = shift;
    h->CW = (W + cs - 1) >> shift;
    h->CH = (H + cs - 1) >> shift;
    h->nClu = nClu;
    h->NN = NN;
    h->nSlots = nSlots;
    h->reqCap = reqCap;
    h->offVer = offVer;
    h->offRec = offRec;
    h->offReq = offReq;
    h->offEv = offEv;
    h->offAG = offAG;
    h->offAPar = offAPar;
    h->offAStamp = offAStamp;
    h->offAKey = offAKey;
    h->offAPos = offAPos;
    h->offAHeap = offAHeap;
    h->offRoute = offRoute;
    h->offPath = offPath;
    for (int i = 0; i < 32; i++) {
        h->classes[i] = w->MoveClass(i);
        h->slotOf[i] = -1;
    }
    int* ver = (int*)(region + offVer);
    for (int i = 0; i < nClu; i++)
        ver[i] = 1;
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)region);
}

void __stdcall HpaShutdown()
{
    Hdr* h = (Hdr*)g_game->pathfinder.get();
    if (h != 0 && h->magic == kMagic) {
        HpaView v;
        if (Open(&v) && h->active)
            v.w->EndSearch(h->req.unit);
        SimFree(h);
    }
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)0);
}

void __stdcall HpaSubmit(const PathRequest* request)
{
    HpaView v;
    if (!Open(&v))
        return;
    Hdr* h = v.h;
    if (h->active && h->req.requester == request->requester)
        return;
    Req* free = 0;
    for (int i = 0; i < h->reqCap; i++) {
        Req* q = &v.req[i];
        if (q->used && q->r.requester == request->requester) {
            q->r = *request;
            return;
        }
        if (!q->used && free == 0)
            free = q;
    }
    if (free == 0)
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

void __stdcall HpaCellsChanged(int left, int top, int right, int bottom)
{
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
    int budget = kStepBudget;
    Settle(h, &budget);
    while (budget > 0 && h->evCount < kEventCap - 4) {
        if (!h->active) {
            Req* q = PickNext(&v, tick);
            if (q == 0)
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

int __stdcall HpaPoll(PathEvent* out)
{
    HpaView v;
    if (!Open(&v))
        return 0;
    Hdr* h = v.h;
    if (h->evHead >= h->evCount)
        return 0;
    *out = v.ev[h->evHead++];
    return 1;
}

void __stdcall HpaProject(PathTraceView* out)
{
    out->record = 0;
    out->budgetCap = 0;
    out->tickLoad = 0;
    out->refreshCounter = 0;
}

PathfindingSeam s_hpa = {
    HpaInit, HpaShutdown, HpaSubmit, HpaCancel, HpaCellsChanged, HpaStep, HpaPoll, HpaProject
};

}

PathfindingSeam* PathfindingHpaSeam()
{
    return &s_hpa;
}
