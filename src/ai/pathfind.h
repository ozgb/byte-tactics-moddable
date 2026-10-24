#ifndef PATHFIND_H
#define PATHFIND_H
struct Entry_40d670;

struct Entry_40d670;

void __stdcall CopyDwordPair(int* dest, int* src);
void __stdcall CopyOneByte(char* dest, char* src);
void __stdcall AdjustHeap(Entry_40d670* first, int hole, int len, Entry_40d670 val);
void __stdcall PushHeapSiftUp(Entry_40d670* first, int hole, int top, Entry_40d670 val);
int __stdcall DirectionFromStep(int dx, int dy);

void __stdcall MakeHeap(Entry_40d670* first, Entry_40d670* last, int*, Entry_40d670*);

#endif
