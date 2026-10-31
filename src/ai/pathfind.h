#ifndef PATHFIND_H
#define PATHFIND_H
#include "../../interfaces/services.h"
struct HeapEntry;

struct HeapEntry;

void __stdcall CopyDwordPair(int* dest, int* src);
void __stdcall CopyOneByte(char* dest, char* src);
void __stdcall AdjustHeap(HeapEntry* first, int hole, int len, HeapEntry val);
void __stdcall PushHeapSiftUp(HeapEntry* first, int hole, int top, HeapEntry val);
int __stdcall DirectionFromStep(int dx, int dy);


#endif
