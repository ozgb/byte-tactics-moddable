#ifndef DISPLAY_GAME_H
#define DISPLAY_GAME_H

#include "display_log.h"

struct Surface;
struct GafFrame;
struct Rect;
struct Point;
struct Font;
struct Vec3;
struct ModelState;
struct Object3do;
struct DebrisPiece;
struct Display;
struct tagPALETTEENTRY;

enum DisplaySpriteKind {
    DISPLAY_SPRITE,
    DISPLAY_SPRITE_OPAQUE,
    DISPLAY_SPRITE_BLEND,
    DISPLAY_SPRITE_GRAY,
    DISPLAY_SPRITE_SHADOW,
    DISPLAY_SPRITE_LIT,
    DISPLAY_SPRITE_ERASE,
    DISPLAY_LENS
};

void DisplayLogSprite(int kind, Surface* dst, GafFrame* bmp, int x, int y, int extra);
void DisplayLogQuad(void* dst, GafFrame* bmp, Point* corners, Point* source);
void DisplayLogBlit(Surface* dst, Surface* src, int x, int y);
void DisplayLogCopy(Surface* dst, Surface* src, Rect* rect, Point* pos);
void DisplayLogClear(Surface* dst, int colour);
void DisplayLogClearFrame(GafFrame* frame, int colour);
void DisplayLogLine(Surface* dst, int x0, int y0, int x1, int y1, int colour);
void DisplayLogPixel(Surface* dst, int x, int y, int colour);
void DisplayLogFade(Surface* dst, Rect* rect, int level);
void DisplayLogFill(Surface* dst, Rect* rect, int colour);
void DisplayLogOutline(Surface* dst, const int* rect, int colour, int lit);
void DisplayLogGray(Surface* dst, Rect* rect);
void DisplayLogDither(Surface* dst, Rect* rect, int phase);
void DisplayLogCircle(Surface* dst, int x, int y, int radius, int colour);
void DisplayLogDashed(Surface* dst, int x, int y, int radius, int colour, int dashes, int start);
void DisplayLogPolygon(Surface* dst, Point* points, int count, int colour);
void DisplayLogText(Surface* dst, const unsigned char* text, int x, int y, int width);
void DisplayLogTerrain(void* dst);
void DisplayLogModel(void* dst, ModelState* state);
void DisplayLogProjectile(void* dst, Vec3* offset, const void* piece, const short* angles);
void DisplayLogDebris(void* dst, const void* piece, int colour, int ox, int oy, int oz, const short* angles);
void DisplayLogShard(void* dst, Vec3* offset, DebrisPiece* piece, const short* angles);
void DisplayLogNoAngles();

void DisplayLogAllocSurface(Surface* surface, const char* name);
void DisplayLogAllocFrame(GafFrame* frame, const char* name);
void DisplayLogGaf(const char* path, void* base);
void DisplayLog3do(const char* path, Object3do* root);
void DisplayLogFont(Font* font, const char* name);
void DisplayLogFeatureType(const char* name);
void DisplayLogFrameEnd(Display* display);

#endif
