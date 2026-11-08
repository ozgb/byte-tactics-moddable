#include <stdlib.h>
#include <malloc.h>
#include "../state/block.h"
#include <string.h>
#include "win_types.h"
#include <vector>

void __cdecl operator delete(void* p);
#include "../graphics/object_pool.h"

static inline bool operator<(const HeapEntry& a, const HeapEntry& b)
{
    return a.key < b.key;
}

#include "pathfind.h"
#include "../../interfaces/pathfinding.h"
#include "../../interfaces/state_writer.h"

static int StepsFor(const PathWorldView* world)
{
    unsigned int limit = world->SearchStepLimit();
    return limit != 0 ? (int)limit : 0x535;
}

// FUNCTION: 0x40d5e0
void __stdcall CopyDwordPair(int* dest, int* src) {
    if (dest != 0) {
        dest[0] = src[0];
        dest[1] = src[1];
    }
}

// FUNCTION: 0x40d600
void __stdcall CopyOneByte(char* dest, char* src)
{
    if (dest != 0) {
        *dest = *src;
    }
}

// FUNCTION: 0x40d620
void __stdcall MakeHeap(HeapEntry* first, HeapEntry* last, int*, HeapEntry*)
{
    int n = last - first;
    for (int h = n / 2; 0 < h; ) {
        --h;
        AdjustHeap(first, h, n, HeapEntry(first[h]));
    }
}

// FUNCTION: 0x40d670
void __stdcall AdjustHeap(HeapEntry* first, int hole, int len, HeapEntry val)
{
    int top = hole;
    int k = 2 * hole + 2;

    while (k < len) {
        if (first[k] < first[k - 1])
            k--;
        first[hole] = first[k];
        hole = k;
        k = 2 * k + 2;
    }
    if (k == len) {
        first[hole] = first[k - 1];
        hole = k - 1;
    }
    PushHeapSiftUp(first, hole, top, val);
}

// FUNCTION: 0x40d700
void __stdcall PopHeapFirst(HeapEntry* first, HeapEntry* last, HeapEntry* dest,
                             HeapEntry val, int*)
{
    *dest = *first;
    AdjustHeap(first, 0, last - first, val);
}

// FUNCTION: 0x40d740
void __stdcall PushHeapSiftUp(HeapEntry* first, int hole, int top, HeapEntry val)
{
    for (int idx = (hole - 1) / 2; top < hole && first[idx] < val; idx = (hole - 1) / 2) {
        first[hole] = first[idx];
        hole = idx;
    }
    first[hole] = val;
}

#include <new>
#include "pathfinder.h"

extern signed char g_dirDeltaX[];
extern signed char g_dirDeltaZ[];

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

extern Game* g_game;

inline int Pathfinder::Push(const NodeData& d)
{
    if (topPopped) {
        Node* n = items[0];
        n->data = d;
        SiftDown(0);
        topPopped = 0;
        return n - pool;
    }
    if (count == capacity)
        GrowNodes(-1);
    if (count == capacity)
        return 0;
    int i = count++;
    int k = freeHead;
    if (k == -1)
        k = used++;
    else
        freeHead = pool[k].index;
    pool[k].data = d;
    pool[k].index = i;
    items[i] = &pool[k];
    SiftUp(i);
    return k;
}

#pragma auto_inline(off)

// FUNCTION: 0x40d7b0
int Pathfinder::GetCellState(int x, int y)
{
    PathWorldView* world = g_seams.pathworld;
    if (!world->InBounds(owner, x, y))
        return 0;
    PathExtra* extra = Extra();
    int cx = (x >> 1) + (extra->footprintX >> 2);
    int cy = (y >> 1) + (extra->footprintZ >> 2);
    if (cx >= extra->seenWidth || cy >= extra->seenHeight)
        return 0;
    if (!world->Seen(player, cx, cy))
        return 2;
    return world->Pass(owner, x, y);
}

// FUNCTION: 0x40d880
void Pathfinder::InitCostTables()
{
    table = g_turnCosts;
    pairs[0].a = pairs[1].a = pairs[2].a = pairs[3].a = 0x10;
    pairs[0].b = pairs[1].b = pairs[2].b = pairs[3].b = 0x16;
}

// FUNCTION: 0x40d8b0
void Pathfinder::MarkGoalCell(unsigned int x, unsigned int y)
{
    if (grid.InBounds(x, y))
        grid.Set(x, y, 4);
}

// FUNCTION: 0x40d900
void Pathfinder::ClearDirtyCells()
{
    int n = ((grid.count + 0xff) >> 8) - 1;
    int i;
    for (i = 0; i < n; i++)
        grid.ClearBlock(i, 0);
    grid.ClearLast(i);
}

// FUNCTION: 0x40d9c0
int __stdcall DirectionFromStep(int dx, int dy)
{
    if (dx > 0) {
        if (dy == dx)
            return 5;
        if (dy == -dx)
            return 7;
        if (dy == 0)
            return 6;
    } else if (dx < 0) {
        if (dy == dx)
            return 1;
        if (dy == -dx)
            return 3;
        if (dy == 0)
            return 2;
    } else {
        if (dy > 0)
            return 4;
        if (dy < 0)
            return 0;
    }
    return -1;
}

// FUNCTION: 0x40da40
__int64 Pathfinder::Estimate(int param1, int param2)
{
    int a = costScale;
    PathGoalArea goal = Extra()->goal;
    return ((__int64)PathGoalDistance(&goal, param1, param2) * (__int64)a) >> 0x10;
}

// FUNCTION: 0x40da70
void Pathfinder::ExpandNeighbour(NodeData* from, PathCell* fromCell, int turn)
{
    int dir = (fromCell->dir + turn) & 7;
    unsigned int x = from->pos.x + g_dirDeltaX[dir];
    unsigned int y = from->pos.y + g_dirDeltaZ[dir];
    if (!grid.InBounds(x, y))
        return;
    unsigned int i = grid.width * y + x;
    PathCell* cell = &grid.cells[i];
    switch (cell->flags & 3) {
    case 0: {
        grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);
        unsigned int r = GetCellState(x, y);
        if (r < 1 && !(cell->flags & 8)) {
            cell->flags |= 3;
            return;
        }
        int h = EstimateCost(x, y);
        if (h <= probe)
            cell->flags |= 5;
        else
            cell->flags |= 1;
        cell->dir = dir;
        NodeData d;
        d.pos.x = x;
        d.pos.y = y;
        if (turn)
            d.steps = 1;
        else
            d.steps = from->steps + 1;
        d.penalty = r > 1 ? 0 : 30;
        d.g = turnCost[turn] + stepCost[dir] + from->g + d.penalty;
        if (turn && from->steps < 5)
            d.g += 75;
        d.f = d.g + h;
        cell->node = Push(d);
        break;
    }
    case 1: {
        NodeData* n = &pool[cell->node].data;
        int g = turnCost[turn] + stepCost[dir] + from->g + n->penalty;
        if (turn && from->steps < 5)
            g += 75;
        if (g < n->g) {
            cell->dir = dir;
            n->f += g - n->g;
            n->g = g;
            n->steps = Steps(from, turn);
            Update(cell->node);
        }
        break;
    }
    }
}

// FUNCTION: 0x40df00
int Pathfinder::ExpandBestNode()
{
    if (topPopped != 0) {
        topPopped = 0;
        Pop();
    }
    Node* top = items[0];
    NodeData local = top->data;
    if (topPopped == 0)
        topPopped = 1;
    else
        Pop();
    int y = local.pos.y;
    int i = grid.width * y + local.pos.x;
    PathCell* c = &grid.cells[i];
    if (c->flags & 4) {
        found = local.pos;
        return 1;
    }
    c->flags = 2;
    for (int d = -range; d <= range; d++)
        ExpandNeighbour(&local, c, d & 7);
    return 0;
}

// FUNCTION: 0x40e050
void Pathfinder::TracePath()
{
    Point16 cur = found;
    int dir = (char)grid.At(found.x, found.y)->dir;
    Point16 pts[64];
    pts[0] = cur;
    int n = 1;

    while (cur != start) {
        PathCell* c = grid.At(cur.x, cur.y);
        if ((char)c->dir != dir) {
            dir = (char)c->dir;
            pts[n & 0x3f] = cur;
            n++;
        }
        cur.x -= g_dirDeltaX[dir];
        cur.y -= g_dirDeltaZ[dir];
    }
    pts[n & 0x3f] = start;
    n++;
    int count = n;
    if (count >= 0x40)
        count = 0x40;
    PathExtra* extra = Extra();
    PathEventRecord* event = AddEvent(PATH_EVENT_FINISHED, n > 0x40 ? PATH_PARTIAL : PATH_FOUND);
    if (event == 0)
        return;
    event->event.count = count;
    for (int i = 0; i < count; i++) {
        Point16 pt = pts[(n - 1 - i) & 0x3f];
        event->points[i].x = (pt.x * 2 + extra->footprintX) * 8;
        event->points[i].y = (pt.y * 2 + extra->footprintZ) * 8;
    }
}

// FUNCTION: 0x40e160
int Pathfinder::ProbeStraightPath()
{
    int x = start.x;
    int y = start.y;
    int best = CostFix(x, y);
    if (Passable(start.x, start.y) < 1)
        return best;
    char dir;
    int nx;
    int ny;
greedy:
        for (;;) {
            steps++;
            if (best == 0)
                return 0;
            dir = Dir(x, y);
            nx = g_dirDeltaX[dir] + x;
            ny = g_dirDeltaZ[dir] + y;
            if (Passable(nx, ny) < 1)
                break;
            x = nx;
            y = ny;
            if (Visit(nx, ny, dir) & 4)
                return 0;
            int c = CostFix(nx, ny);
            if (c < best)
                best = c;
        }

        int ax = x;
        int ay = y;
        int bx = x;
        int by = y;
        char dirA = (dir + 2) & 7;
        char dirB = dirA;
        int started = 0;
        for (;;) {
            char stop = (dirA - 3) & 7;
            dirA = (dirA - 2) & 7;
            steps++;
            nx = g_dirDeltaX[dirA] + ax;
            ny = g_dirDeltaZ[dirA] + ay;
            while (Passable(nx, ny) < 1) {
                if (dirA == stop)
                    return best;
                dirA = (dirA + 1) & 7;
                nx = g_dirDeltaX[dirA] + ax;
                ny = g_dirDeltaZ[dirA] + ay;
            }
            if (ax == bx && ay == by && dirA == dirB && started)
                return best;
            ax = nx;
            ay = ny;
            started = 1;
            if (Visit(ax, ay, dirA) & 4)
                return 0;
            if (OnLine(x, y, nx, ny)) {
                x = nx;
                y = ny;
                goto greedy;
            }
            int c = Cost(nx, ny);
            if (c < best)
                best = c;

            stop = (dirB + 3) & 7;
            dirB = (dirB + 2) & 7;
            nx = bx - g_dirDeltaX[dirB];
            ny = by - g_dirDeltaZ[dirB];
            while (Passable(nx, ny) < 1) {
                if (dirB == stop)
                    return best;
                dirB = (dirB - 1) & 7;
                nx = bx - g_dirDeltaX[dirB];
                ny = by - g_dirDeltaZ[dirB];
            }
            if (ax == bx && ay == by && dirA == dirB)
                return best;
            bx = nx;
            by = ny;
            if (Visit(bx, by, dirB) & 4)
                return 0;
            if (OnLine(x, y, nx, ny)) {
                x = nx;
                y = ny;
                goto greedy;
            }
            c = Cost(nx, ny);
            if (c < best)
                best = c;
        }
}

// FUNCTION: 0x40e630
void Pathfinder::StartSearch(const PathPending* request, const PathUnitState* state)
{
    PathWorldView* world = g_seams.pathworld;
    PathExtra* extra = Extra();
    owner = state->moveClass;
    target = request->target;
    extra->goal = request->goal;
    extra->footprintX = state->footprintX;
    extra->footprintZ = state->footprintY;
    int seenWidth;
    int seenHeight;
    world->SeenSize(&seenWidth, &seenHeight);
    extra->seenWidth = seenWidth;
    extra->seenHeight = seenHeight;
    start.x = state->cell.x;
    start.y = state->cell.y;
    world->BeginSearch(object);
    ResetTable();
    ClearDirtyCells();

    PathGoalArea goal = request->goal;
    PathPoint* goals = (PathPoint*)_alloca(extra->goalCapacity * sizeof(PathPoint));
    int goalCount = PathGoalCells(&goal, goals, extra->goalCapacity);
    int bestDist = 0x7fffffff;
    for (PathPoint* p = goals; p != goals + goalCount; p++) {
        MarkGoal(p->x, p->y);
        int dy = start.y - p->y;
        int dx = start.x - p->x;
        int d = dx * dx + dy * dy;
        if (d < bestDist) {
            goalX = p->x;
            bestDist = d;
            goalY = p->y;
        }
    }

    if (PathGoalContains(&goal, start.x, start.y)) {
        AddEvent(PATH_EVENT_STARTED, PATH_START_DIRECT);
        Finish();
        return;
    }
    int cost = Cost(start.x, start.y);
    if (!grid.InBounds(start.x, start.y)) {
        AddEvent(PATH_EVENT_STARTED, PATH_START_INDIRECT);
        Finish();
        return;
    }
    probe = ProbeStraightPath();
    if (probe == 0) {
        AddEvent(PATH_EVENT_STARTED, PATH_START_DIRECT);
    } else {
        AddEvent(PATH_EVENT_STARTED, PATH_START_INDIRECT);
        if (probe >= cost) {
            Finish();
            return;
        }
    }
    Clear();
    NodeData d(start.x, start.y, 0, cost, 100);
    unsigned int i = grid.Index(start.x, start.y);
    grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);
    grid.cells[i].flags |= 1;
    grid.cells[i].dir = ((((unsigned short)state->heading) + 0x1000) >> 13) & 7;
    grid.cells[i].node = Push(d);
    range = 4;
}

void Pathfinder::Finish()
{
    AddEvent(PATH_EVENT_FINISHED, PATH_FAILED);
    Release();
}

// FUNCTION: 0x40e9a0
void Pathfinder::ReleaseHeldPathLock()
{
    g_seams.pathworld->EndSearch(object);
    object = 0;
    owner = 0;
}

// FUNCTION: 0x40e9c0
void Pathfinder::AbortIfGoalMatch(PathHandle goal)
{
    if (goal == path) {
        object = 0;
        path = 0;
        target = 0;
        owner = 0;
    }
}

// FUNCTION: 0x40e9e0
Pathfinder::Pathfinder()
{
    PathExtra* extra = Extra();
    pool = Ref<Node>((Node*)0);
    items = Ref<Ref<Node>>((Ref<Node>*)0);
    capacity = 0;
    count = 0;
    freeHead = -1;
    used = 0;
    topPopped = 0;

    grid.width = g_game->width;
    grid.height = g_game->height;
    grid.count = (grid.height * grid.width + 7) & ~7;
    grid.cells = Ref<PathCell>((PathCell*)((char*)this + extra->cellsOffset));

    unsigned int m = (grid.count + 0xff) >> 8;
    unsigned int n = m * 4;
    grid.dirty = Ref<unsigned int>((unsigned int*)((char*)this + extra->dirtyOffset));
    memset(grid.dirty, 0xff, n - 1);
    *(int*)((char*)grid.dirty.get() + n - 4) = 0;

    for (unsigned int i = grid.count - 0x100; i < (unsigned int)grid.count; i++)
        grid.dirty[i >> 8] |= 1 << ((i >> 3) & 0x1f);

    ClearDirtyCells();

    object = 0;
    stepsPerTick = StepsFor(g_seams.pathworld);
    steps = 0;
    player = 0;
    costScale = 0;
    baseScale = 0x18000;

    for (unsigned int k = 0; k < 10; k++) {
        extra->budgetCap[k] = baseScale;
        budget[k] = 0;
        cursor[k] = g_seams.pathworld->FirstUnit(k);
    }
}

int* Pathfinder::RequesterBucket(PathHandle requester)
{
    PathExtra* extra = Extra();
    return (int*)((char*)this + extra->requesterBuckets) + (((requester * 0x9e3779b1u) >> 16) & extra->bucketMask);
}

int* Pathfinder::UnitBucket(PathUnit unit)
{
    PathExtra* extra = Extra();
    return (int*)((char*)this + extra->unitBuckets) + (((unit * 0x9e3779b1u) >> 16) & extra->bucketMask);
}

int Pathfinder::FindPending(PathHandle requester)
{
    for (int i = *RequesterBucket(requester); i != 0; i = Entry(i)->nextRequester) {
        if (Entry(i)->requester == requester)
            return i;
    }
    return 0;
}

int Pathfinder::FindPendingOfUnit(PathUnit unit)
{
    for (int i = *UnitBucket(unit); i != 0; i = Entry(i)->nextUnit) {
        if (Entry(i)->unit == unit)
            return i;
    }
    return 0;
}

static void UnlinkRequester(Pathfinder* pf, int index)
{
    PathPending* e = pf->Entry(index);
    int* link = pf->RequesterBucket(e->requester);
    while (*link != index)
        link = &pf->Entry(*link)->nextRequester;
    *link = e->nextRequester;
}

static void UnlinkUnit(Pathfinder* pf, int index)
{
    PathPending* e = pf->Entry(index);
    int* link = pf->UnitBucket(e->unit);
    while (*link != index)
        link = &pf->Entry(*link)->nextUnit;
    *link = e->nextUnit;
}

void Pathfinder::RemovePending(int index)
{
    PathExtra* extra = Extra();
    UnlinkRequester(this, index);
    UnlinkUnit(this, index);
    PathPending* e = Entry(index);
    e->requester = 0;
    e->nextRequester = extra->pendingFree;
    extra->pendingFree = index;
    extra->pendingCount--;
}

void Pathfinder::Submit(const PathRequest* request)
{
    if (object != 0 && path == request->requester)
        return;
    PathExtra* extra = Extra();
    int index = FindPending(request->requester);
    if (index != 0) {
        PathPending* e = Entry(index);
        if (e->unit != request->unit) {
            UnlinkUnit(this, index);
            e->unit = request->unit;
            int* bucket = UnitBucket(e->unit);
            e->nextUnit = *bucket;
            *bucket = index;
        }
    } else {
        index = extra->pendingFree;
        if (index == 0)
            return;
        PathPending* e = Entry(index);
        extra->pendingFree = e->nextRequester;
        extra->pendingCount++;
        e->requester = request->requester;
        e->unit = request->unit;
        int* rb = RequesterBucket(e->requester);
        e->nextRequester = *rb;
        *rb = index;
        int* ub = UnitBucket(e->unit);
        e->nextUnit = *ub;
        *ub = index;
    }
    PathPending* e = Entry(index);
    e->target = request->target;
    e->notBefore = request->notBefore;
    e->goal = request->goal;
}

void Pathfinder::Cancel(PathHandle requester)
{
    AbortIfGoalMatch(requester);
    int index = FindPending(requester);
    if (index != 0)
        RemovePending(index);
    PathExtra* extra = Extra();
    PathEventRecord* events = (PathEventRecord*)((char*)this + extra->eventsOffset);
    int kept = extra->eventNext;
    for (int i = extra->eventNext; i < extra->eventCount; i++) {
        if (events[i].event.requester != requester)
            events[kept++] = events[i];
    }
    extra->eventCount = kept;
}

PathEventRecord* Pathfinder::AddEvent(int kind, int status)
{
    PathExtra* extra = Extra();
    if (extra->eventCount >= extra->eventCapacity)
        return 0;
    PathEventRecord* record = (PathEventRecord*)((char*)this + extra->eventsOffset) + extra->eventCount++;
    PathEvent* event = &record->event;
    event->kind = kind;
    event->status = status;
    event->unit = object;
    event->requester = path;
    event->target = target;
    event->tick = g_seams.pathworld->Tick();
    event->count = 0;
    return record;
}

int Pathfinder::Poll(PathEvent* out, PathPoint* points, int capacity)
{
    PathExtra* extra = Extra();
    if (extra->eventNext >= extra->eventCount)
        return 0;
    PathEventRecord* record = (PathEventRecord*)((char*)this + extra->eventsOffset) + extra->eventNext++;
    *out = record->event;
    for (int i = 0; i < record->event.count && i < capacity; i++)
        points[i] = record->points[i];
    return 1;
}

// FUNCTION: 0x40eb70
void Pathfinder::RunSearches()
{
    PathWorldView* world = g_seams.pathworld;
    PathExtra* extra = Extra();
    extra->eventCount = 0;
    extra->eventNext = 0;
    int players = world->PlayerCount();
    if (players == 0)
        return;
    int limit = StepsFor(world);
    if (stepsPerTick != limit)
        stepsPerTick = limit;
    int share = stepsPerTick / players;
    int total = 0;
    if (++extra->refreshCounter >= 150) {
        extra->refreshCounter = 0;
        for (int i = 0; i < 10; i++) {
            int r = extra->tickLoad[i] / world->UnitsPerPlayer();
            if (r < 1)
                extra->budgetCap[i] = baseScale * 6;
            else if (r < 2)
                extra->budgetCap[i] = baseScale * 3;
            else if (r < 3)
                extra->budgetCap[i] = baseScale;
            else
                extra->budgetCap[i] = baseScale;
            extra->tickLoad[i] = 0;
        }
    }
    for (int i = 0; i < 10; i++) {
        if (world->IsPlaying(i)) {
            budget[i] += share;
            total += budget[i];
        }
    }
    while (total > 0 && extra->eventCount + 3 <= extra->eventCapacity) {
        steps = 0;
        if (object == 0) {
            steps = 1;
            while (budget[player] <= 0) {
                if (++player >= 10)
                    player = 0;
            }
            extra->tickLoad[player]++;
            PathUnit* c = &cursor[player];
            if (*c == world->LastUnit(player))
                *c = world->FirstUnit(player);
            else
                *c = world->NextUnit(*c);
            PathUnit u = *c;
            int index = FindPendingOfUnit(u);
            PathUnitState state;
            if (index != 0 && (unsigned int)world->Tick() >= Entry(index)->notBefore && world->UnitState(u, &state)) {
                PathPending request = *Entry(index);
                RemovePending(index);
                path = request.requester;
                object = u;
                steps += 100;
                costScale = extra->budgetCap[player];
                StartSearch(&request, &state);
            }
        } else if (Size() != 0) {
            while (1) {
                if (Size() == 0)
                    break;
                steps++;
                if (topPopped) {
                    topPopped = 0;
                    int k = items[0] - pool;
                    int idx = pool[k].index;
                    ((Pathfinder*)this)->FreeNode(k);
                    count--;
                    if (idx < count) {
                        items[idx] = items[count];
                        items[idx]->index = idx;
                        ((OpenHeap*)this)->SiftDown(idx);
                    }
                }
                NodeData d = items[0]->data;
                if (!topPopped)
                    topPopped = 1;
                else
                    ((Pathfinder*)this)->RemoveNode(items[0] - pool);
                PathCell* cell = &grid.cells[grid.width * d.pos.y + d.pos.x];
                if (cell->flags & 4) {
                    found = d.pos;
                    ((Pathfinder*)this)->TracePath();
                    Release();
                    break;
                }
                cell->flags = 2;
                for (int k = -range; k <= range; k++)
                    ((Pathfinder*)this)->ExpandNeighbour(&d, cell, k & 7);
                range = 2;
                if (steps >= 100)
                    break;
            }
        } else {
            Finish();
        }
        total -= steps;
        budget[player] -= steps;
    }
}

// FUNCTION: 0x40ef20
void Pathfinder::RemoveNode(int k)
{
    int idx = pool[k].index;
    Free(k);
    count--;
    if (idx < count) {
        items[idx] = items[count];
        items[idx]->index = idx;
        SiftDownInline(idx);
    }
}

// FUNCTION: 0x40f060
void OpenHeap::SiftDown(int i)
{
    Node* node = items[i];
    while (true) {
        int left = i * 2 + 1;
        int right = i * 2 + 2;
        if (right < count) {
            Node* l = items[left];
            Node* r = items[right];
            if (r->data.f < l->data.f) {
                if (r->data.f >= node->data.f)
                    break;
                items[i] = r;
                r->index = i;
                i = right;
            } else {
                if (l->data.f >= node->data.f)
                    break;
                items[i] = l;
                l->index = i;
                i = left;
            }
        } else {
            if (left >= count)
                break;
            Node* l = items[left];
            if (l->data.f >= node->data.f)
                break;
            items[i] = l;
            items[left]->index = i;
            i = left;
        }
    }
    items[i] = node;
    node->index = i;
}

// FUNCTION: 0x40f1e0
void Pathfinder::FreeNode(int index)
{
    pool[index].index = freeHead;
    freeHead = index;
}

#pragma auto_inline(on)

Pathfinder* GetPathfinder()
{
    return g_game->pathfinder.get();
}

namespace {
struct RegionLayout {
    int cells, nodeLimit, dirtyBytes, goalCapacity, pendingCapacity, buckets, eventCapacity;
    unsigned int poolOffset, itemsOffset, cellsOffset, dirtyOffset, pendingOffset, requesterBuckets, unitBuckets, eventsOffset, size;
};

RegionLayout LayoutFor(const PathMapInfo* map)
{
    RegionLayout l;
    l.cells = (map->width * map->height + 7) & ~7;
    l.nodeLimit = l.cells + 1;
    if (l.nodeLimit > 0x10000)
        l.nodeLimit = 0x10000;
    l.dirtyBytes = ((l.cells + 0xff) >> 8) * 4;
    l.goalCapacity = 2 * (map->width + map->height) + 256;
    l.pendingCapacity = map->unitSlots + 16;
    l.buckets = 16;
    while (l.buckets < l.pendingCapacity)
        l.buckets <<= 1;
    l.eventCapacity = 512;

    unsigned int size = Pathfinder::kExtraOffset + sizeof(PathExtra);
    l.poolOffset = size;
    size += l.nodeLimit * sizeof(Node);
    l.itemsOffset = size;
    size += l.nodeLimit * sizeof(Ref<Node>);
    l.cellsOffset = size;
    size += l.cells * sizeof(PathCell);
    l.dirtyOffset = size;
    size += l.dirtyBytes;
    l.pendingOffset = size;
    size += l.pendingCapacity * sizeof(PathPending);
    l.requesterBuckets = size;
    size += l.buckets * sizeof(int);
    l.unitBuckets = size;
    size += l.buckets * sizeof(int);
    l.eventsOffset = size;
    size += l.eventCapacity * sizeof(PathEventRecord);
    l.size = size;
    return l;
}
}

unsigned int __stdcall PathfindingRegionSize(const PathMapInfo* map)
{
    return LayoutFor(map).size;
}

void __stdcall PathfindingInit(const PathMapInfo* map, void* regionBase, unsigned int)
{
    RegionLayout l = LayoutFor(map);
    char* region = (char*)regionBase;
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)region);
    PathExtra* extra = (PathExtra*)(region + Pathfinder::kExtraOffset);
    extra->nodeLimit = l.nodeLimit;
    extra->goalCapacity = l.goalCapacity;
    extra->pendingCapacity = l.pendingCapacity;
    extra->pendingFree = 1;
    extra->bucketMask = l.buckets - 1;
    extra->eventCapacity = l.eventCapacity;
    extra->poolOffset = l.poolOffset;
    extra->itemsOffset = l.itemsOffset;
    extra->cellsOffset = l.cellsOffset;
    extra->dirtyOffset = l.dirtyOffset;
    extra->pendingOffset = l.pendingOffset;
    extra->requesterBuckets = l.requesterBuckets;
    extra->unitBuckets = l.unitBuckets;
    extra->eventsOffset = l.eventsOffset;
    PathPending* pending = (PathPending*)(region + l.pendingOffset);
    for (int i = 0; i < l.pendingCapacity; i++)
        pending[i].nextRequester = i + 2 <= l.pendingCapacity ? i + 2 : 0;
    new (region) Pathfinder;
}

void* __stdcall PathfindingShutdown()
{
    void* region = g_game->pathfinder.get();
    g_game->pathfinder = Ref<Pathfinder>((Pathfinder*)0);
    return region;
}

void __stdcall PathfindingSubmit(const PathRequest* request)
{
    Pathfinder* pf = GetPathfinder();
    if (pf != nullptr)
        pf->Submit(request);
}

void __stdcall PathfindingCancel(PathHandle requester)
{
    Pathfinder* pf = GetPathfinder();
    if (pf != nullptr)
        pf->Cancel(requester);
}

void __stdcall PathfindingCellsChanged(int, int, int, int, unsigned int, int)
{
}

void __stdcall PathfindingStep()
{
    Pathfinder* pf = GetPathfinder();
    if (pf != nullptr)
        pf->RunSearches();
}

int __stdcall PathfindingPoll(PathEvent* out, PathPoint* points, int capacity)
{
    Pathfinder* pf = GetPathfinder();
    if (pf == nullptr)
        return 0;
    return pf->Poll(out, points, capacity);
}

namespace {
enum { CELL_LIMIT = 1 << 22 };

unsigned int OwnNodeIndex(const Pathfinder* pf, unsigned int itemOffset)
{
    unsigned int poolOffset = pf->pool.offset;
    if (itemOffset == 0 || poolOffset == 0 || itemOffset < poolOffset)
        return STATE_NO_INDEX;
    unsigned int delta = itemOffset - poolOffset;
    if (delta % sizeof(Node) != 0 || delta / sizeof(Node) >= (unsigned int)pf->used)
        return STATE_NO_INDEX;
    return delta / sizeof(Node);
}

bool NodeIsLive(const Pathfinder* pf, int node, int count)
{
    int slot = pf->pool[node].index;
    return slot >= 0 && slot < count && pf->items[slot].offset == pf->pool.offset + (unsigned int)node * sizeof(Node);
}

void WriteNodes(const StateWriter* w, Pathfinder* pf)
{
    int used = pf->used;
    int count = pf->count;
    if (pf->pool.offset == 0 || pf->items.offset == 0 || used < 0 || used > pf->Extra()->nodeLimit || count < 0 || count > used) {
        used = 0;
        count = 0;
    }
    for (int node = 0; node < used; node++) {
        if (!NodeIsLive(pf, node, count))
            continue;
        Node* n = &pf->pool[node];
        w->Int(nullptr, 0, 4, node);
        w->Run(5);
        w->Int("pool[%u].index", node, 4, n->index);
        w->Int("pool[%u].pos_x", node, 2, (unsigned short)n->data.pos.x);
        w->Int("pool[%u].pos_y", node, 2, (unsigned short)n->data.pos.y);
        w->Int("pool[%u].g", node, 4, n->data.g);
        w->Int("pool[%u].f", node, 4, n->data.f);
        w->Int("pool[%u].steps", node, 2, (unsigned short)n->data.steps);
        if (n->data.g != 0)
            w->Int("pool[%u].penalty", node, 2, (unsigned short)n->data.penalty);
    }
    w->Int("heap.count", 0, 4, count);
    for (int slot = 0; slot < count; slot++)
        w->Handle("heap[%u]", slot, STATE_HANDLE_OWN, OwnNodeIndex(pf, pf->items[slot].offset));
}

void WriteCells(const StateWriter* w, Pathfinder* pf)
{
    unsigned int count = pf->grid.count;
    if (pf->grid.cells.offset == 0 || pf->grid.dirty.offset == 0 || count > CELL_LIMIT)
        count = 0;
    unsigned int words = (count + 255) >> 8;
    for (unsigned int word = 0; word < words; word++) {
        unsigned int bits = pf->grid.dirty[word];
        if (bits == 0)
            continue;
        w->Int("dirty[%u]", word, 4, bits);
        for (unsigned int bit = 0; bit < 32; bit++) {
            if (!((bits >> bit) & 1))
                continue;
            for (unsigned int k = 0; k < 8; k++) {
                unsigned int index = (word << 8) + (bit << 3) + k;
                if (index >= count)
                    break;
                const PathCell& c = pf->grid.cells[index];
                unsigned int state = c.flags & 3;
                w->Int("cells[%u].flags", index, 1, c.flags);
                if ((c.flags & 8) || state == 1 || state == 2)
                    w->Int("cells[%u].dir", index, 1, c.dir);
                if (state == 1 || state == 2)
                    w->Int("cells[%u].node", index, 2, c.node);
            }
        }
    }
}
}

void __stdcall PathfindingWriteState(const StateWriter* w)
{
    Pathfinder* pf = GetPathfinder();
    w->Begin(16, "pathfinder");
    w->Int(nullptr, 0, 1, pf != nullptr);
    if (pf != nullptr) {
        w->Handle("pool", 0, STATE_HANDLE_HEAP, pf->pool.offset);
        w->Handle("items", 0, STATE_HANDLE_HEAP, pf->items.offset);
        w->Run(4);
        w->Int("freeHead", 0, 4, pf->freeHead);
        w->Int("used", 0, 4, pf->used);
        w->Int("count", 0, 4, pf->count);
        w->Int("topPopped", 0, 4, pf->topPopped);
        w->Handle("cells", 0, STATE_HANDLE_HEAP, pf->grid.cells.offset);
        w->Run(3);
        w->Int("width", 0, 4, pf->grid.width);
        w->Int("height", 0, 4, pf->grid.height);
        w->Int("cellCount", 0, 4, pf->grid.count);
        w->Handle("dirty", 0, STATE_HANDLE_HEAP, pf->grid.dirty.offset);
        w->Run(4);
        w->Int("stepsPerTick", 0, 4, pf->stepsPerTick);
        w->Int("steps", 0, 4, pf->steps);
        w->Int("costScale", 0, 4, pf->costScale);
        w->Int("baseScale", 0, 4, pf->baseScale);
        w->Handle("object", 0, STATE_HANDLE_UNIT, pf->object);
        w->Int("player", 0, 1, pf->player);
        for (int k = 0; k < 10; k++)
            w->Handle("cursor[%u]", k, STATE_HANDLE_UNIT, pf->cursor[k]);
        w->Bytes("budget", 0, pf->budget, sizeof(pf->budget));
        if (pf->object != 0) {
            w->Run(2);
            w->Int("start_x", 0, 2, (unsigned short)pf->start.x);
            w->Int("start_y", 0, 2, (unsigned short)pf->start.y);
            w->Run(4);
            w->Int("goalX", 0, 4, pf->goalX);
            w->Int("goalY", 0, 4, pf->goalY);
            w->Int("probe", 0, 4, pf->probe);
            w->Int("range", 0, 4, pf->range);
            w->Handle("path", 0, STATE_HANDLE_HEAP, pf->path);
            w->Handle("target", 0, STATE_HANDLE_HEAP, pf->target);
            w->Handle("owner", 0, STATE_HANDLE_MOVE_CLASS, pf->owner);
            w->Run(2);
            w->Bytes("turnCost", 0, pf->turnCost, 8);
            w->Bytes("stepCost", 0, pf->stepCost, 8);
        }
        WriteNodes(w, pf);
        WriteCells(w, pf);
    }
    w->Begin(17, "pathfinder");
    if (pf != nullptr) {
        PathExtra* extra = pf->Extra();
        w->Bytes("playerTickLoad", 0, extra->tickLoad, sizeof(extra->tickLoad));
        w->Bytes("playerBudgetCap", 0, extra->budgetCap, sizeof(extra->budgetCap));
        w->Int("budgetRefreshCounter", 0, 4, extra->refreshCounter);
    } else {
        for (int k = 0; k < 2; k++) {
            w->Run(10);
            for (int i = 0; i < 10; i++)
                w->Int(nullptr, 0, 4, 0);
        }
        w->Int(nullptr, 0, 4, 0);
    }
}

void PathfindingSetBaseScale(int scale)
{
    Pathfinder* pf = GetPathfinder();
    if (pf != nullptr)
        pf->baseScale = scale;
}

void PathfindingDebugCell(int x, int y, int* flags, int* dir)
{
    PathCell* cell = GetPathfinder()->grid.At(x, y);
    *flags = cell->flags;
    *dir = cell->dir;
}
