#include "../../interfaces/pathfinding.h"
#include "../../interfaces/services.h"

static int CellsSquared(int radius)
{
    return (radius / 16) * (radius / 16);
}

int __stdcall PathGoalContains(const PathGoalArea* goal, int x, int y)
{
    switch (goal->kind) {
    case PATH_GOAL_CIRCLE: {
        int dy = y - goal->centre.y;
        int dx = x - goal->centre.x;
        return dx * dx + dy * dy <= CellsSquared(goal->radius[0]);
    }
    case PATH_GOAL_RING: {
        int dy = y - goal->centre.y;
        int dx = x - goal->centre.x;
        int distSq = dy * dy + dx * dx;
        return distSq <= CellsSquared(goal->radius[1]) && distSq >= CellsSquared(goal->radius[0]);
    }
    case PATH_GOAL_RECT:
        if ((x == goal->left || x == goal->right) && y >= goal->top && y <= goal->bottom)
            return 1;
        if ((y == goal->top || y == goal->bottom) && x >= goal->left && x <= goal->right)
            return 1;
        return 0;
    }
    return 0;
}

static int RadialDistance(const PathGoalArea* goal, int px, int py)
{
    int dx = abs(px - goal->centre.x);
    int dy = abs(py - goal->centre.y);
    if (dx > dy)
        return dy * 7 + dx * 18;
    return dx * 7 + dy * 18;
}

static int MinOf(int a, int b)
{
    return a < b ? a : b;
}

int __stdcall PathGoalDistance(const PathGoalArea* goal, int x, int y)
{
    switch (goal->kind) {
    case PATH_GOAL_CIRCLE: {
        int d = RadialDistance(goal, x, y);
        if (d < goal->radius[0])
            return 0;
        return d - goal->radius[0];
    }
    case PATH_GOAL_RING: {
        int d = RadialDistance(goal, x, y);
        if (d > goal->radius[1])
            return d - goal->radius[1];
        if (d < goal->radius[0])
            return goal->radius[0] - d;
        return 0;
    }
    case PATH_GOAL_RECT: {
        int dx;
        if (x < goal->left) {
            dx = goal->left - x;
        } else if (x > goal->right) {
            dx = x - goal->right;
        } else {
            if (y < goal->top)
                return (goal->top - y) * 16;
            if (y > goal->bottom)
                return (y - goal->bottom) * 16;
            int mx = MinOf(x - goal->left, goal->right - x);
            int my = MinOf(y - goal->top, goal->bottom - y);
            return MinOf(mx, my) * 16;
        }
        int dy;
        if (y < goal->top)
            dy = goal->top - y;
        else if (y > goal->bottom)
            dy = y - goal->bottom;
        else
            return dx * 16;
        if (dx > dy)
            return dx * 16 + dy * 6;
        return dy * 16 + dx * 6;
    }
    }
    return 0;
}

namespace {
struct CellSink {
    PathPoint* out;
    int max;
    int total;
    void Add(int x, int y)
    {
        if (out != 0 && total < max) {
            out[total].x = (short)x;
            out[total].y = (short)y;
        }
        total++;
    }
};
}

int __stdcall PathGoalCells(const PathGoalArea* goal, PathPoint* out, int max)
{
    CellSink sink = { out, max, 0 };
    switch (goal->kind) {
    case PATH_GOAL_CIRCLE:
        sink.Add(goal->centre.x, goal->centre.y);
        break;
    case PATH_GOAL_RING:
        sink.Add(goal->centre.x, goal->centre.y + (goal->radius[1] + goal->radius[0]) / 32);
        break;
    case PATH_GOAL_RECT:
        for (int i = goal->left; i <= goal->right; i++) {
            sink.Add(i, goal->top);
            sink.Add(i, goal->bottom);
        }
        for (int j = goal->top + 1; j <= goal->bottom - 1; j++) {
            sink.Add(goal->left, j);
            sink.Add(goal->right, j);
        }
        break;
    }
    if (out != 0 && sink.total > max)
        return max;
    return sink.total;
}
