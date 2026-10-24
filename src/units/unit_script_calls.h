#ifndef UNIT_SCRIPT_CALLS_H
#define UNIT_SCRIPT_CALLS_H

struct Point16;

char __stdcall OctantBetween(Point16 from, Point16 to);
int __stdcall StepTowards(int value, int target, int step);
Point16 __stdcall StepPointTowards(Point16 p, Point16 target, short step);
int __stdcall ChebyshevDistance(Point16 a, Point16 b);
void __stdcall ClampPointToSegment(Point16* p, Point16 b, Point16 c);

#endif
