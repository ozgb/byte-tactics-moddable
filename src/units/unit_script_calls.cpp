#include <stdlib.h>

#include "../util/vec3.h"

int __stdcall CheckDirectXVersion(int, int, int, int, int);
void __stdcall EnumPlayersCallback(int, int, int, int, int);
void __stdcall EmitBubbles(int, int, int, short);

struct Diff_00480570 { int dx; int dy; };

// FUNCTION: 0x480720
char __stdcall OctantBetween(Point16 from, Point16 to)
{
    int dx = to.x - from.x;
    int dy = to.y - from.y;
    if (dx > 0) {
        if (dy > 0)
            return 5;
        if (dy < 0)
            return 7;
        return 6;
    }
    if (dx < 0) {
        if (dy > 0)
            return 3;
        if (dy < 0)
            return 1;
        return 2;
    }
    if (dy > 0)
        return 4;
    if (dy < 0)
        return 0;
    return 0;
}

// FUNCTION: 0x4804e0
int __stdcall StepTowards(int value, int target, int step)
{
    if (value < target) {
        value += step;
        if (value >= target) {
            value = target;
        }
    } else if (value > target) {
        value -= step;
        if (value <= target) {
            value = target;
        }
    }
    return value;
}

// FUNCTION: 0x480510
Point16 __stdcall StepPointTowards(Point16 p, Point16 target, short step)
{
    if (target.x < p.x)
        p.x -= step;
    else if (target.x > p.x)
        p.x += step;
    if (target.y < p.y)
        p.y -= step;
    else if (target.y > p.y)
        p.y += step;
    return p;
}

static inline Diff_00480570 Sub(Point16 a, Point16 b)
{
    Diff_00480570 d;
    d.dx = a.x - b.x;
    d.dy = a.y - b.y;
    return d;
}

// FUNCTION: 0x480570
int __stdcall ChebyshevDistance(Point16 a, Point16 b)
{
    Diff_00480570 d = Sub(a, b);
    int dx = abs(d.dx);
    int dy = abs(d.dy);
    return dx > dy ? dx : dy;
}

// FUNCTION: 0x4805b0
void __stdcall ClampPointToSegment(Point16* p, Point16 b, Point16 c)
{
    int d1 = ChebyshevDistance(*p, b);
    int d2 = ChebyshevDistance(*p, c);
    int d3 = ChebyshevDistance(b, c);
    if (d1 > d2) {
        if (d1 > d3) d1 = d3;
        *p = StepPointTowards(b, c, d1);
    } else {
        if (d2 > d3) d2 = d3;
        *p = StepPointTowards(c, b, d2);
    }
}

#include "weapon_aim_cob_cb.h"

// FUNCTION: 0x481490
void WeaponAimCobCb::OnAimCobReturn(int enable)
{
    if (enable) {
        nAimCobDone = 1;
    }
}

// FUNCTION: 0x4814b0
int __stdcall AimCobStub(int, int, int, int)
{
    return 0;
}
