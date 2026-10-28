#ifndef GAME_LOAD_H
#define GAME_LOAD_H
struct MinimapWorldPos;
struct Point;
struct WorldViewOrigin;

void __cdecl LoadMatch(void*);
void __stdcall DrawSyncStatus(void* surface);
void __stdcall LoadingScreenFrame(void);

void __stdcall OffsetWorldPosFromView(Point* p, WorldViewOrigin* view, int);
void __stdcall MinimapCursorToWorldPos(MinimapWorldPos* out);

#endif
