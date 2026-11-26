#include <stdio.h>
#include <string.h>

#include <stdarg.h>

#include "display_hook.h"
#include "display_log.h"
#include "../graphics/rect.h"
#include "../graphics/surface.h"
#include "../graphics/gaf_frame.h"
#include "../util/point.h"

struct Display;
Display* GetDisplay();
int __stdcall RectInsideRect(Rect* a, Rect* b);
int __stdcall PointInRect(void* region, int x, int y);
const char* HarnessMapName();
void HarnessDisplayFrame(const unsigned char* palette, const DlPicture* cursor, int cursorX, int cursorY);
void HarnessCursorPosition(int* x, int* y);

extern char* g_game;
extern float g_lightX;
extern float g_lightY;
extern float g_lightZ;

static const char* s_featureType = 0;
static int s_featureSpot = -1;
static int s_noAngles = 0;

static int RdI(const void* p, int off)
{
    return *(const int*)((const char*)p + off);
}

static int RdS(const void* p, int off)
{
    return *(const short*)((const char*)p + off);
}

static int RdU16(const void* p, int off)
{
    return *(const unsigned short*)((const char*)p + off);
}

static int RdU8(const void* p, int off)
{
    return *(const unsigned char*)((const char*)p + off);
}

static void* RdP(const void* p, int off)
{
    return *(void* const*)((const char*)p + off);
}

enum {
    DISP_WINDOWED = 68,
    DISP_CACHED = 80,
    DISP_VEC = 160,
    DISP_OVERRIDE = 188,
    DISP_WIDTH = 212,
    DISP_HEIGHT = 216,
    DISP_USE_OVERRIDE = 220,
    DISP_CURSOR_HIDE = 430,
    DISP_CURSOR_SPRITE = 434,
    DISP_CURSOR_THREAD = 462,
    DISP_FONT = 516,
    DISP_FORE = 520,
    DISP_BACK = 524,
    DISP_KEY = 528,
    DISP_ENTRIES = 532
};

enum {
    GAME_DEBUG_MODE = 82560,
    GAME_SCROLL_X = 82719,
    GAME_SCROLL_Y = 82723,
    GAME_PLAYERS = 7011,
    GAME_PLAYER_INDEX = 10819,
    GAME_RECT = 228903,
    GAME_VIEW_WIDTH = 228919,
    GAME_VIEW_HEIGHT = 228923,
    PLAYER_SIZE = 331,
    PLAYER_INFO = 39,
    PLAYER_INFO_COLOR = 150
};

static void Surfaceof(void* surface, DlSurface* out)
{
    Surface* s = (Surface*)surface;
    char* d = (char*)GetDisplay();
    if (s == 0) {
        if (RdI(d, DISP_USE_OVERRIDE) != 0)
            s = (Surface*)RdP(d, DISP_OVERRIDE);
        else if (RdI(d, DISP_WINDOWED) != 0)
            s = (Surface*)(d + DISP_CACHED);
    }
    if (s == 0) {
        out->pixels = 0;
        out->width = RdI(d, DISP_WIDTH);
        out->height = RdI(d, DISP_HEIGHT);
        out->pitch = RdI(d, DISP_WIDTH);
        out->clipLeft = RdI(d, DISP_VEC);
        out->clipTop = RdI(d, DISP_VEC + 4);
        out->clipRight = RdI(d, DISP_VEC + 8);
        out->clipBottom = RdI(d, DISP_VEC + 12);
        return;
    }
    out->pixels = s->pixels;
    out->width = s->width;
    out->height = s->height;
    out->pitch = s->pitch;
    out->clipLeft = s->clip.left;
    out->clipTop = s->clip.top;
    out->clipRight = s->clip.right;
    out->clipBottom = s->clip.bottom;
}

static void PictureOf(const GafFrame* f, DlPicture* out)
{
    out->key = f;
    out->pixels = f->pixelsOrLayers;
    out->width = f->width;
    out->height = f->height;
    out->compressed = f->compressed;
    out->layered = f->layers;
    out->xOffset = f->xOffset;
    out->yOffset = f->yOffset;
    out->transparency = f->transparency;
}

static void Union(const GafFrame* f, int x, int y, int depth, int* x0, int* y0, int* x1, int* y1)
{
    int i;
    int l;
    int t;
    if (f == 0 || depth > 4)
        return;
    if (f->layers > 0) {
        for (i = 0; i < (int)f->layers; i++)
            Union(((GafFrame**)f->pixelsOrLayers)[i], x, y, depth + 1, x0, y0, x1, y1);
        return;
    }
    l = x - f->xOffset;
    t = y - f->yOffset;
    if (l < *x0)
        *x0 = l;
    if (t < *y0)
        *y0 = t;
    if (l + f->width > *x1)
        *x1 = l + f->width;
    if (t + f->height > *y1)
        *y1 = t + f->height;
}

static void PutXY(int x, int y)
{
    DlPutRaw(" ");
    DlPutInt(x + DlOffsetX());
    DlPutRaw(" ");
    DlPutInt(y + DlOffsetY());
}

static void PutInt(int v)
{
    DlPutRaw(" ");
    DlPutInt(v);
}

static void PutText(const char* s)
{
    DlPutRaw(" ");
    DlPutName(s, (int)strlen(s));
}

enum { DISPLAY_SPRITE_LIT = 5, DISPLAY_SPRITE_ERASE = 6 };

static void DisplayLogSprite(int kind, void* dst, void* frame, int x, int y, int extra)
{
    static const char* const names[] = { "sprite", "sprite_opaque", "sprite_blend", "sprite_gray", "sprite_shadow", "sprite_lit",
                                         "sprite_erase", "lens" };
    GafFrame* bmp = (GafFrame*)frame;
    DlSurface d;
    DlPicture p;
    const unsigned char* reads;
    int x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30);
    if (bmp == 0)
        return;
    Surfaceof(dst, &d);
    Union(bmp, x, y, 0, &x0, &y0, &x1, &y1);
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    PictureOf(bmp, &p);
    DlPictureReads(&p, &reads);
    DlBegin(names[kind], &d, x0, y0, x1, y1, reads);
    DlPutRaw(" ");
    DlPutPicture(&p);
    PutXY(x, y);
    if (kind == DISPLAY_SPRITE_LIT || kind == DISPLAY_SPRITE_ERASE)
        PutInt(extra);
    DlEnd();
}

static void PutCorner(const Point* p, int offX, int offY)
{
    DlPutInt(p->x + offX);
    DlPutRaw(",");
    DlPutInt(p->y + offY);
}

static void DisplayLogQuad(void* dst, void* frame, void* cornerList, void* sourceList)
{
    GafFrame* bmp = (GafFrame*)frame;
    Point* corners = (Point*)cornerList;
    Point* source = (Point*)sourceList;
    DlSurface d;
    DlPicture p;
    const unsigned char* reads;
    int i;
    int x0;
    int y0;
    int x1;
    int y1;
    if (bmp == 0 || corners == 0)
        return;
    Surfaceof(dst, &d);
    x0 = corners[0].x;
    y0 = corners[0].y;
    x1 = corners[0].x;
    y1 = corners[0].y;
    for (i = 1; i < 4; i++) {
        if (corners[i].x < x0)
            x0 = corners[i].x;
        if (corners[i].x > x1)
            x1 = corners[i].x;
        if (corners[i].y < y0)
            y0 = corners[i].y;
        if (corners[i].y > y1)
            y1 = corners[i].y;
    }
    x1++;
    y1++;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    PictureOf(bmp, &p);
    DlPictureReads(&p, &reads);
    DlBegin("quad", &d, x0, y0, x1, y1, reads);
    DlPutRaw(" ");
    DlPutPicture(&p);
    for (i = 0; i < 4; i++) {
        DlPutRaw(" ");
        PutCorner(&corners[i], DlOffsetX(), DlOffsetY());
    }
    DlPutRaw(" ");
    if (source == 0) {
        DlPutRaw("-");
    } else {
        for (i = 0; i < 4; i++) {
            if (i != 0)
                DlPutRaw(",");
            PutCorner(&source[i], 0, 0);
        }
    }
    DlEnd();
}

static void DisplayLogBlit(void* dstSurface, void* srcSurface, int x, int y)
{
    Surface* src = (Surface*)srcSurface;
    DlSurface d;
    DlSurface s;
    int bx;
    int by;
    int x0;
    int y0;
    int x1;
    int y1;
    int sx;
    int sy;
    int whole;
    const unsigned char* base;
    if (dstSurface == 0 && src == 0)
        return;
    Surfaceof(dstSurface, &d);
    Surfaceof(src, &s);
    bx = x;
    by = y;
    if (src != 0) {
        bx = x - src->x;
        by = y - src->y;
    }
    x0 = bx;
    y0 = by;
    x1 = bx + s.width;
    y1 = by + s.height;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlSourceOffset(&s, &sx, &sy);
    base = DlSourceBase(&s);
    whole = sx == 0 && sy == 0;
    DlBegin(whole ? "blit" : "copy", &d, x0, y0, x1, y1, base);
    DlPutRaw(" ");
    DlPutTarget(&s);
    if (!whole) {
        PutInt(sx);
        PutInt(sy);
        PutInt(sx + s.width - 1);
        PutInt(sy + s.height - 1);
    }
    PutXY(bx, by);
    DlEnd();
}

static void DisplayLogCopy(void* dst, void* src, void* rectArg, void* posArg)
{
    Rect* rect = (Rect*)rectArg;
    Point* pos = (Point*)posArg;
    DlSurface d;
    DlSurface s;
    int w;
    int h;
    int x0;
    int y0;
    int x1;
    int y1;
    int sx;
    int sy;
    if (src == 0 || rect == 0 || pos == 0)
        return;
    Surfaceof(dst, &d);
    Surfaceof(src, &s);
    w = rect->right - rect->left + 1;
    h = rect->bottom - rect->top + 1;
    x0 = pos->x;
    y0 = pos->y;
    x1 = pos->x + w;
    y1 = pos->y + h;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlSourceOffset(&s, &sx, &sy);
    DlBegin("copy", &d, x0, y0, x1, y1, DlSourceBase(&s));
    DlPutRaw(" ");
    DlPutTarget(&s);
    PutInt(rect->left + sx);
    PutInt(rect->top + sy);
    PutInt(rect->right + sx);
    PutInt(rect->bottom + sy);
    PutXY(pos->x, pos->y);
    DlEnd();
}

static void DisplayLogClear(void* dst, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    DlBegin("clear", &d, 0, 0, d.width, d.height, 0);
    PutInt(colour);
    DlEnd();
}

static void DisplayLogClearFrame(void* frameArg, int colour)
{
    GafFrame* frame = (GafFrame*)frameArg;
    DlSurface d;
    d.pixels = frame->pixelsOrLayers;
    d.width = frame->width;
    d.height = frame->height;
    d.pitch = frame->width;
    d.clipLeft = 0;
    d.clipTop = 0;
    d.clipRight = frame->width - 1;
    d.clipBottom = frame->height - 1;
    DlBegin("clear", &d, 0, 0, d.width, d.height, 0);
    PutInt(colour);
    DlEnd();
}

static void DisplayLogLine(void* dst, int ax, int ay, int bx, int by, int colour)
{
    DlSurface d;
    int x0;
    int x1;
    int y0;
    int y1;
    Surfaceof(dst, &d);
    x0 = ax < bx ? ax : bx;
    x1 = (ax < bx ? bx : ax) + 1;
    y0 = ay < by ? ay : by;
    y1 = (ay < by ? by : ay) + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("line", &d, x0, y0, x1, y1, 0);
    PutXY(ax, ay);
    PutXY(bx, by);
    PutInt(colour);
    DlEnd();
}

static void DisplayLogPixel(void* dst, int x, int y, int colour)
{
    DlSurface d;
    int x0 = x;
    int y0 = y;
    int x1 = x + 1;
    int y1 = y + 1;
    Surfaceof(dst, &d);
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("pixel", &d, x0, y0, x1, y1, 0);
    PutXY(x, y);
    PutInt(colour);
    DlEnd();
}

static void RectArea(const DlSurface* d, const Rect* r, int* x0, int* y0, int* x1, int* y1)
{
    if (r == 0) {
        char* display = (char*)GetDisplay();
        *x0 = 0;
        *y0 = 0;
        *x1 = RdI(display, DISP_WIDTH) + 1;
        *y1 = RdI(display, DISP_HEIGHT) + 1;
    } else {
        *x0 = r->left;
        *y0 = r->top;
        *x1 = r->right + 1;
        *y1 = r->bottom + 1;
    }
    DlCutRect(d, x0, y0, x1, y1);
}

static void DisplayLogFade(void* dst, void* rect, int level)
{
    DlSurface d;
    int x0, y0, x1, y1;
    Surfaceof(dst, &d);
    RectArea(&d, (const Rect*)rect, &x0, &y0, &x1, &y1);
    DlBegin("fade", &d, x0, y0, x1, y1, 0);
    PutInt(level);
    DlEnd();
}

static void DisplayLogFill(void* dst, void* rect, int colour)
{
    DlSurface d;
    int x0, y0, x1, y1;
    Surfaceof(dst, &d);
    RectArea(&d, (const Rect*)rect, &x0, &y0, &x1, &y1);
    DlBegin("fill", &d, x0, y0, x1, y1, 0);
    PutInt(colour);
    DlEnd();
}

static void DisplayLogOutline(void* dst, const int* rect, int colour, int lit)
{
    DlSurface d;
    Rect r;
    int x0, y0, x1, y1;
    if (rect == 0)
        return;
    Surfaceof(dst, &d);
    r.left = rect[0];
    r.top = rect[1];
    r.right = rect[2];
    r.bottom = rect[3];
    RectArea(&d, &r, &x0, &y0, &x1, &y1);
    DlBegin(lit ? "outline_lit" : "outline", &d, x0, y0, x1, y1, 0);
    PutXY(rect[0], rect[1]);
    PutXY(rect[2], rect[3]);
    PutInt(colour);
    DlEnd();
}

static void DisplayLogGray(void* dst, void* rect)
{
    DlSurface d;
    int x0, y0, x1, y1;
    Surfaceof(dst, &d);
    RectArea(&d, (const Rect*)rect, &x0, &y0, &x1, &y1);
    DlBegin("gray", &d, x0, y0, x1, y1, 0);
    DlEnd();
}

static void DisplayLogDither(void* dst, void* rect, int phase)
{
    DlSurface d;
    int x0, y0, x1, y1;
    Surfaceof(dst, &d);
    RectArea(&d, (const Rect*)rect, &x0, &y0, &x1, &y1);
    DlBegin("dither", &d, x0, y0, x1, y1, 0);
    PutInt(phase);
    DlEnd();
}

static void DisplayLogCircle(void* dst, int x, int y, int radius, int colour)
{
    DlSurface d;
    int r = radius < 0 ? -radius : radius;
    int x0 = x - r, y0 = y - r, x1 = x + r + 1, y1 = y + r + 1;
    Surfaceof(dst, &d);
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("circle", &d, x0, y0, x1, y1, 0);
    PutXY(x, y);
    PutInt(radius);
    PutInt(colour);
    DlEnd();
}

static void DisplayLogDashed(void* dst, int x, int y, int radius, int colour, int dashes, int start)
{
    DlSurface d;
    int r = radius < 0 ? -radius : radius;
    int x0 = x - r, y0 = y - r, x1 = x + r + 1, y1 = y + r + 1;
    Surfaceof(dst, &d);
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("circle_dashed", &d, x0, y0, x1, y1, 0);
    PutXY(x, y);
    PutInt(radius);
    PutInt(colour);
    PutInt(dashes);
    PutInt(start);
    DlEnd();
}

static void DisplayLogPolygon(void* dst, void* pointList, int count, int colour)
{
    Point* points = (Point*)pointList;
    DlSurface d;
    int i;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    Surfaceof(dst, &d);
    if (count > 0) {
        x0 = x1 = points[0].x;
        y0 = y1 = points[0].y;
        for (i = 1; i < count; i++) {
            if (points[i].x < x0)
                x0 = points[i].x;
            if (points[i].x > x1)
                x1 = points[i].x;
            if (points[i].y < y0)
                y0 = points[i].y;
            if (points[i].y > y1)
                y1 = points[i].y;
        }
        x1++;
        y1++;
    }
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("polygon", &d, x0, y0, x1, y1, 0);
    PutInt(colour);
    for (i = 0; i < count; i++) {
        DlPutRaw(" ");
        PutCorner(&points[i], DlOffsetX(), DlOffsetY());
    }
    DlEnd();
}

static void DisplayLogText(void* dst, const unsigned char* text, int x, int y, int width)
{
    char* display = (char*)GetDisplay();
    void* fontData = RdP(display, DISP_FONT);
    DlSurface d;
    Rect r;
    Rect clip;
    char font[64];
    int height = *(unsigned char*)fontData;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    Surfaceof(dst, &d);
    r.left = x;
    r.top = y;
    r.right = x + width;
    r.bottom = y + height;
    clip.left = d.clipLeft;
    clip.top = d.clipTop;
    clip.right = d.clipRight;
    clip.bottom = d.clipBottom;
    if (RectInsideRect(&r, &clip) != 0) {
        x0 = x;
        y0 = y;
        x1 = x + width;
        y1 = y + height;
        DlCutRect(&d, &x0, &y0, &x1, &y1);
    }
    DlBegin("text", &d, x0, y0, x1, y1, 0);
    DlFontName(fontData, font, sizeof font);
    DlPutRaw(" ");
    DlPutName(font, (int)strlen(font));
    PutInt(RdI(display, DISP_FORE));
    PutInt(RdI(display, DISP_BACK));
    PutInt(RdI(display, DISP_KEY));
    PutXY(x, y);
    DlPutRaw(" ");
    if (text != 0)
        DlPutName((const char*)text, (int)strlen((const char*)text));
    DlEnd();
}

static void DisplayLogTerrain(void* dst)
{
    DlSurface d;
    int x0 = RdI(g_game, GAME_RECT);
    int y0 = RdI(g_game, GAME_RECT + 4);
    int x1 = x0 + RdI(g_game, GAME_VIEW_WIDTH);
    int y1 = y0 + RdI(g_game, GAME_VIEW_HEIGHT);
    Surfaceof(dst, &d);
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("terrain", &d, x0, y0, x1, y1, 0);
    PutText(HarnessMapName());
    PutInt(RdI(g_game, GAME_SCROLL_X));
    PutInt(RdI(g_game, GAME_SCROLL_Y));
    DlEnd();
}

static void PutHexFloat(float f)
{
    unsigned bits;
    memcpy(&bits, &f, 4);
    DlPutHex8(bits);
}

static void PutPieces(const char* state)
{
    int count = RdI(state, 0);
    const char* pieces = (const char*)RdP(state, 34);
    int i;
    for (i = 0; i < count && pieces != 0; i++) {
        const char* p = pieces + i * 54;
        const char* object = (const char*)RdP(p, 0);
        const char* name = object != 0 && RdP(object, 28) != 0 ? (const char*)RdP(object, 28) : "";
        DlPutRaw(" ");
        DlPutName(name, (int)strlen(name));
        DlPutRaw(",");
        DlPutInt(RdU16(p, 40) & 7);
        DlPutRaw(",");
        DlPutInt(RdS(p, 16));
        DlPutRaw(",");
        DlPutInt(RdS(p, 18));
        DlPutRaw(",");
        DlPutInt(RdS(p, 20));
        DlPutRaw(",");
        DlPutInt(RdI(p, 4));
        DlPutRaw(",");
        DlPutInt(RdI(p, 8));
        DlPutRaw(",");
        DlPutInt(RdI(p, 12));
    }
}

static void PutDrawing(const char* owner, const char* state)
{
    char key[16];
    char attrs[16];
    const char* picture = (const char*)RdP(state, 16);
    const char* def = (const char*)RdP(owner, 146);
    unsigned flags = (unsigned)RdI(owner, 272);
    unsigned typeFlags = def != 0 ? (unsigned)RdI(def, 577) : 0;
    int rebuild = RdI(state, 4) == 0;
    int still;
    int n = 0;
    if (s_featureType != 0)
        sprintf(key, "f%d", s_featureSpot);
    else
        sprintf(key, "%u", (unsigned)RdU16(owner, 168));
    DlPutRaw(" ");
    DlPutRaw(key);
    if ((flags & 0x20000000) != 0) {
        if (picture == 0)
            rebuild = 1;
        else if (*(const float*)(owner + 260) != 0.0f && (flags & 0x2000) != 0 && RdP(picture, 20) == 0)
            rebuild = 1;
    }
    if ((RdU8(owner, 276) & 1) != 0 && picture == 0)
        rebuild = 1;
    DlPutRaw(rebuild ? " n" : picture != 0 ? " k" : " -");
    if ((flags & 0x20000000) != 0)
        still = (RdU8(owner, 270) & 1) == 0;
    else
        still = RdI(RdP(owner, 0), 0x20) == 0;
    if ((flags & 0x20000000) != 0)
        attrs[n++] = 's';
    if (!still)
        attrs[n++] = 'm';
    if ((flags & 0x200) != 0 || RdU8(owner, 255) == RdU8(g_game, GAME_PLAYER_INDEX))
        attrs[n++] = 't';
    if ((RdU8(owner, 276) & 1) != 0)
        attrs[n++] = 'z';
    if (RdS(owner, 166) != 0)
        attrs[n++] = 'u';
    if ((typeFlags & 0x2000000) != 0)
        attrs[n++] = 'n';
    if ((typeFlags & 0x40000000) != 0)
        attrs[n++] = 'h';
    if ((typeFlags & 0x81000) != 0)
        attrs[n++] = 'v';
    if (n == 0)
        attrs[n++] = '-';
    attrs[n] = '\0';
    DlPutRaw(" ");
    DlPutRaw(attrs);
}

static void ModelItem(const DlSurface* d, const char* kind, const char* owner, const char* state)
{
    int x0 = d->clipLeft, y0 = d->clipTop, x1 = d->clipRight + 1, y1 = d->clipBottom + 1;
    char type[64];
    const char* source = s_featureType;
    const char* def;
    const char* root;
    const char* model;
    int known = 0;
    int scrollX;
    int scrollY;
    int vx;
    int vy;
    int vz;
    int sx;
    int sy;
    int colour = 0;
    const char* info;
    DlCutRect(d, &x0, &y0, &x1, &y1);
    DlBegin(kind, d, x0, y0, x1, y1, 0);
    type[0] = '\0';
    def = (const char*)RdP(owner, 146);
    if (source == 0 && def != 0)
        source = def + 32;
    if (source != 0) {
        int n = 0;
        while (source[n] != '\0' && n < (int)sizeof type - 1) {
            char c = source[n];
            type[n] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
            n++;
        }
        type[n] = '\0';
    }
    DlPutRaw(" ");
    DlPutName(type, (int)strlen(type));
    root = (const char*)RdP(state, 30);
    model = root != 0 ? DlModelName(RdP(root, 0), &known) : "-";
    DlPutRaw(" ");
    if (known) {
        DlPutName(model, (int)strlen(model));
    } else {
        DlPutName("?", 1);
    }
    scrollX = RdI(g_game, GAME_SCROLL_X);
    scrollY = RdI(g_game, GAME_SCROLL_Y);
    vx = RdI(owner, 106) - (scrollX << 16);
    vy = RdI(owner, 110);
    vz = RdI(owner, 114) - (scrollY << 16);
    sx = (short)((unsigned)vx >> 16) + 0x80;
    sy = (short)((unsigned)vz >> 16) - ((short)((unsigned)vy >> 16) >> 1) + 0x20;
    PutXY(sx, sy);
    PutInt(RdI(owner, 106));
    PutInt(RdI(owner, 110));
    PutInt(RdI(owner, 114));
    PutInt(RdS(state, 24));
    PutInt(RdS(state, 26));
    PutInt(RdS(state, 28));
    info = (const char*)RdP(g_game + GAME_PLAYERS + RdU8(owner, 255) * PLAYER_SIZE, PLAYER_INFO);
    if (info != 0)
        colour = RdU8(info, PLAYER_INFO_COLOR);
    PutInt(colour);
    DlPutRaw(" ");
    PutHexFloat(*(const float*)(owner + 260));
    PutInt((RdU8(owner, 270) & 4) != 0 || RdU8(g_game, GAME_DEBUG_MODE) != 0 ? 1 : 0);
    DlPutRaw(" ");
    PutHexFloat(g_lightX);
    DlPutRaw(",");
    PutHexFloat(g_lightY);
    DlPutRaw(",");
    PutHexFloat(g_lightZ);
    PutDrawing(owner, state);
    PutPieces(state);
    DlEnd();
}

static void DisplayLogModel(void* dst, void* stateArg)
{
    const char* state = (const char*)stateArg;
    const char* owner;
    const char* feature;
    const char* u;
    DlSurface d;
    if (state == 0)
        return;
    owner = (const char*)RdP(state, 12);
    if (owner == 0)
        return;
    Surfaceof(dst, &d);
    ModelItem(&d, "model", owner, state);
    feature = s_featureType;
    s_featureType = 0;
    for (u = (const char*)RdP(owner, 138); u != 0; u = (const char*)RdP(u, 142)) {
        if ((RdI(u, 272) & 0x20000) == 0 && RdP(u, 158) != 0)
            ModelItem(&d, "cargo", u, (const char*)RdP(u, 158));
    }
    s_featureType = feature;
}

static void DisplayLogNoAngles()
{
    s_noAngles = 1;
}

static void PutPiece(const void* piece)
{
    int known = 0;
    const char* model = DlModelName(piece, &known);
    DlPutRaw(" ");
    if (known) {
        DlPutRaw(model);
        DlPutRaw(":");
        DlPutRaw(DlPieceName(piece));
    } else {
        DlPutName("?", 1);
    }
}

static void BeginClipItem(const char* kind, void* dst)
{
    DlSurface d;
    int x0;
    int y0;
    int x1;
    int y1;
    Surfaceof(dst, &d);
    x0 = d.clipLeft;
    y0 = d.clipTop;
    x1 = d.clipRight + 1;
    y1 = d.clipBottom + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin(kind, &d, x0, y0, x1, y1, 0);
}

static void DisplayLogProjectile(void* dst, void* offsetArg, const void* piece, const short* angles)
{
    const int* offset = (const int*)offsetArg;
    BeginClipItem("projectile", dst);
    PutPiece(piece);
    PutInt(offset[0]);
    PutInt(offset[1]);
    PutInt(offset[2]);
    if (s_noAngles != 0) {
        DlPutRaw(" - - -");
        s_noAngles = 0;
    } else {
        PutInt(angles[0]);
        PutInt(angles[1]);
        PutInt(angles[2]);
    }
    DlEnd();
}

static void DisplayLogDebris(void* dst, const void* piece, int colour, int ox, int oy, int oz, const short* angles)
{
    BeginClipItem("debris", dst);
    PutPiece(piece);
    PutInt(colour);
    PutInt(ox);
    PutInt(oy);
    PutInt(oz);
    PutInt(angles[0]);
    PutInt(angles[1]);
    PutInt(angles[2]);
    DlEnd();
}

static void DisplayLogShard(void* dst, void* offsetArg, void* pieceArg, const short* angles)
{
    const int* offset = (const int*)offsetArg;
    const char* piece = (const char*)pieceArg;
    const char* face = (const char*)RdP(piece, 40);
    const char* v = (const char*)RdP(piece, 36);
    int faceCount = RdI(piece, 8);
    int index = RdI(piece, 12) != -1 ? 1 : 0;
    int named = 0;
    int i;
    BeginClipItem("shard", dst);
    DlPutRaw(" ");
    for (; face != 0 && index < faceCount; index++) {
        const char* f = face + index * 32;
        unsigned flags = (unsigned)RdI(f, 28);
        if ((flags & 1) != 0) {
            DlPutRaw("#");
            DlPutInt(RdI(f, 0));
            named = 1;
            break;
        }
        if (RdI(f, 4) == 4) {
            const GafFrame* pic = 0;
            if ((flags & 2) != 0) {
                const char* table = (const char*)RdP(f, 24);
                if (table != 0)
                    pic = *(const GafFrame* const*)(table + 0x28 + RdU16(f, 16) * 8);
            } else {
                pic = (const GafFrame*)RdP(f, 16);
            }
            if (pic != 0) {
                DlPicture p;
                PictureOf(pic, &p);
                DlPutPicture(&p);
                named = 1;
            }
            break;
        }
    }
    if (named == 0)
        DlPutName("", 0);
    PutInt(offset[0]);
    PutInt(offset[1]);
    PutInt(offset[2]);
    PutInt(angles[0]);
    PutInt(angles[1]);
    PutInt(angles[2]);
    for (i = 0; i < 8; i++) {
        DlPutRaw(" ");
        DlPutInt(v != 0 ? RdI(v, i * 12) : 0);
        DlPutRaw(",");
        DlPutInt(v != 0 ? RdI(v, i * 12 + 4) : 0);
        DlPutRaw(",");
        DlPutInt(v != 0 ? RdI(v, i * 12 + 8) : 0);
    }
    DlEnd();
}

static void DisplayLogAllocSurface(void* surface, const char* name)
{
    Surface* s = (Surface*)surface;
    if (DlIsOn() == 0 || s == 0)
        return;
    DlRegisterSurface(s, s->pixels, s->width, s->height, s->pitch, name);
}

static void DisplayLogAllocFrame(void* frame, const char* name)
{
    GafFrame* f = (GafFrame*)frame;
    if (DlIsOn() == 0 || f == 0)
        return;
    DlRegisterFrame(f, f->pixelsOrLayers, f->width, f->height, name);
}

static void DisplayLogGaf(const char* path, void* base)
{
    int* archive = (int*)base;
    int i;
    int j;
    if (DlIsOn() == 0 || base == 0)
        return;
    DlLockRegistries();
    DlGafBegin(path, base);
    for (i = 0; i < (short)archive[1]; i++) {
        unsigned char* record = (unsigned char*)archive[3 + i];
        unsigned short count = *(unsigned short*)record;
        const char* entry = (const char*)(record + 8);
        for (j = 0; j < (int)count; j++) {
            const GafFrame* frame = *(const GafFrame* const*)(record + 0x28 + j * 8);
            DlGafFrame(entry, j, frame, frame != 0 ? frame->xOffset : 0, frame != 0 ? frame->yOffset : 0);
        }
    }
    DlUnlockRegistries();
}

static void RegisterPieces(const char* stem, const char* piece, int depth)
{
    for (; piece != 0 && depth < 64; piece = (const char*)RdP(piece, 44)) {
        const char* name = (const char*)RdP(piece, 28);
        DlRegisterModelPiece(piece, stem, name != 0 ? name : "");
        RegisterPieces(stem, (const char*)RdP(piece, 48), depth + 1);
    }
}

static void DisplayLog3do(const char* path, void* root)
{
    const char* base = path;
    const char* p;
    char stem[64];
    int n = 0;
    if (DlIsOn() == 0 || root == 0)
        return;
    for (p = path; *p != '\0'; p++) {
        if (*p == '\\' || *p == '/')
            base = p + 1;
    }
    while (base[n] != '\0' && base[n] != '.' && n < (int)sizeof stem - 1) {
        stem[n] = base[n];
        n++;
    }
    stem[n] = '\0';
    DlLockRegistries();
    RegisterPieces(stem, (const char*)root, 0);
    DlUnlockRegistries();
}

static void DisplayLogFont(void* font, const char* name)
{
    if (DlIsOn() == 0 || font == 0)
        return;
    DlRegisterFont(font, name);
}

static void DisplayLogFeatureType(const char* name, int spot)
{
    s_featureType = name;
    s_featureSpot = spot;
}

static void DisplayLogFrameEnd(void* displayArg)
{
    const char* display = (const char*)displayArg;
    unsigned char palette[256 * 4];
    DlPicture cursor;
    const GafFrame* sprite;
    int mouseX;
    int mouseY;
    int i;
    if (DlIsOn() == 0)
        return;
    for (i = 0; i < 256; i++) {
        palette[i * 4] = (unsigned char)RdU8(display, DISP_ENTRIES + i * 4);
        palette[i * 4 + 1] = (unsigned char)RdU8(display, DISP_ENTRIES + i * 4 + 1);
        palette[i * 4 + 2] = (unsigned char)RdU8(display, DISP_ENTRIES + i * 4 + 2);
        palette[i * 4 + 3] = 0;
    }
    sprite = (const GafFrame*)RdP(display, DISP_CURSOR_SPRITE);
    if ((RdI(display, DISP_CURSOR_THREAD) != 1 && RdI(display, DISP_CURSOR_HIDE) > 0) || sprite == 0) {
        HarnessDisplayFrame(palette, 0, 0, 0);
        return;
    }
    PictureOf(sprite, &cursor);
    HarnessCursorPosition(&mouseX, &mouseY);
    HarnessDisplayFrame(palette, &cursor, mouseX, mouseY);
}

static void LogDebris(void* surface, const char* inner, const char* obj, const short* angles)
{
    int ox = RdI(inner, 0x16) - (RdI(g_game, GAME_SCROLL_X) << 16);
    int oy = RdI(inner, 0x1a);
    int oz = RdI(inner, 0x1e) - (RdI(g_game, GAME_SCROLL_Y) << 16);
    int sx = (short)((unsigned)ox >> 16) + 0x80;
    int sy = (short)((unsigned)oz >> 16) - ((short)((unsigned)oy >> 16) >> 1) + 0x20;
    const char* unit;
    const char* player = 0;
    const char* info = 0;
    if (!PointInRect(g_game + GAME_RECT, sx, sy))
        return;
    unit = (const char*)RdP(obj, 0);
    if (unit != 0)
        player = (const char*)RdP(unit, 0x96);
    if (player != 0)
        info = (const char*)RdP(player, 0x27);
    DisplayLogDebris(surface, RdP(inner, 0), info != 0 ? RdU8(info, 0x96) : 0, ox, oy, oz, angles);
}

static void Dispatch(int kind, va_list ap)
{
    switch (kind) {
    case DL_SPRITE:
    case DL_SPRITE_OPAQUE:
    case DL_SPRITE_BLEND:
    case DL_SPRITE_GRAY:
    case DL_SPRITE_SHADOW:
    case DL_SPRITE_LIT:
    case DL_SPRITE_ERASE:
    case DL_LENS: {
        void* dst = va_arg(ap, void*);
        void* frame = va_arg(ap, void*);
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        int extra = 0;
        if (kind == DL_SPRITE_LIT || kind == DL_SPRITE_ERASE)
            extra = va_arg(ap, int);
        DisplayLogSprite(kind - DL_SPRITE, dst, frame, x, y, extra);
        break;
    }
    case DL_QUAD: {
        void* dst = va_arg(ap, void*);
        void* frame = va_arg(ap, void*);
        void* corners = va_arg(ap, void*);
        void* source = va_arg(ap, void*);
        DisplayLogQuad(dst, frame, corners, source);
        break;
    }
    case DL_BLIT: {
        void* dst = va_arg(ap, void*);
        void* src = va_arg(ap, void*);
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        DisplayLogBlit(dst, src, x, y);
        break;
    }
    case DL_COPY: {
        void* dst = va_arg(ap, void*);
        void* src = va_arg(ap, void*);
        void* rect = va_arg(ap, void*);
        void* pos = va_arg(ap, void*);
        DisplayLogCopy(dst, src, rect, pos);
        break;
    }
    case DL_CLEAR: {
        void* dst = va_arg(ap, void*);
        int colour = va_arg(ap, int);
        DisplayLogClear(dst, colour);
        break;
    }
    case DL_CLEAR_FRAME: {
        void* frame = va_arg(ap, void*);
        int colour = va_arg(ap, int);
        DisplayLogClearFrame(frame, colour);
        break;
    }
    case DL_LINE: {
        void* dst = va_arg(ap, void*);
        int x0 = va_arg(ap, int);
        int y0 = va_arg(ap, int);
        int x1 = va_arg(ap, int);
        int y1 = va_arg(ap, int);
        int colour = va_arg(ap, int);
        DisplayLogLine(dst, x0, y0, x1, y1, colour);
        break;
    }
    case DL_PIXEL: {
        void* dst = va_arg(ap, void*);
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        int colour = va_arg(ap, int);
        DisplayLogPixel(dst, x, y, colour);
        break;
    }
    case DL_FADE: {
        void* dst = va_arg(ap, void*);
        void* rect = va_arg(ap, void*);
        int level = va_arg(ap, int);
        DisplayLogFade(dst, rect, level);
        break;
    }
    case DL_FILL: {
        void* dst = va_arg(ap, void*);
        void* rect = va_arg(ap, void*);
        int colour = va_arg(ap, int);
        DisplayLogFill(dst, rect, colour);
        break;
    }
    case DL_OUTLINE:
    case DL_OUTLINE_LIT: {
        void* dst = va_arg(ap, void*);
        const int* rect = va_arg(ap, const int*);
        int colour = va_arg(ap, int);
        DisplayLogOutline(dst, rect, colour, kind == DL_OUTLINE_LIT);
        break;
    }
    case DL_GRAY: {
        void* dst = va_arg(ap, void*);
        void* rect = va_arg(ap, void*);
        DisplayLogGray(dst, rect);
        break;
    }
    case DL_DITHER: {
        void* dst = va_arg(ap, void*);
        void* rect = va_arg(ap, void*);
        int phase = va_arg(ap, int);
        DisplayLogDither(dst, rect, phase);
        break;
    }
    case DL_CIRCLE: {
        void* dst = va_arg(ap, void*);
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        int radius = va_arg(ap, int);
        int colour = va_arg(ap, int);
        DisplayLogCircle(dst, x, y, radius, colour);
        break;
    }
    case DL_DASHED: {
        void* dst = va_arg(ap, void*);
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        int radius = va_arg(ap, int);
        int colour = va_arg(ap, int);
        int dashes = va_arg(ap, int);
        int start = va_arg(ap, int);
        DisplayLogDashed(dst, x, y, radius, colour, dashes, start);
        break;
    }
    case DL_POLYGON: {
        void* dst = va_arg(ap, void*);
        void* points = va_arg(ap, void*);
        int count = va_arg(ap, int);
        int colour = va_arg(ap, int);
        DisplayLogPolygon(dst, points, count, colour);
        break;
    }
    case DL_TEXT: {
        void* dst = va_arg(ap, void*);
        const unsigned char* text = va_arg(ap, const unsigned char*);
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        int width = va_arg(ap, int);
        DisplayLogText(dst, text, x, y, width);
        break;
    }
    case DL_TERRAIN: {
        void* dst = va_arg(ap, void*);
        DisplayLogTerrain(dst);
        break;
    }
    case DL_MODEL: {
        void* dst = va_arg(ap, void*);
        void* state = va_arg(ap, void*);
        DisplayLogModel(dst, state);
        break;
    }
    case DL_PROJECTILE: {
        void* dst = va_arg(ap, void*);
        void* offset = va_arg(ap, void*);
        const void* piece = va_arg(ap, const void*);
        const short* angles = va_arg(ap, const short*);
        DisplayLogProjectile(dst, offset, piece, angles);
        break;
    }
    case DL_DEBRIS: {
        void* dst = va_arg(ap, void*);
        const char* inner = va_arg(ap, const char*);
        const char* obj = va_arg(ap, const char*);
        const short* angles = va_arg(ap, const short*);
        LogDebris(dst, inner, obj, angles);
        break;
    }
    case DL_SHARD: {
        void* dst = va_arg(ap, void*);
        void* offset = va_arg(ap, void*);
        void* piece = va_arg(ap, void*);
        const short* angles = va_arg(ap, const short*);
        DisplayLogShard(dst, offset, piece, angles);
        break;
    }
    }
}

DlScope::DlScope(int kind, ...)
{
    va_list ap;
    va_start(ap, kind);
    if (kind != DL_NONE && DlWants())
        Dispatch(kind, ap);
    va_end(ap);
    DlEnter();
}

DlScope::~DlScope()
{
    DlLeave();
}

void __cdecl DlNote(int kind, ...)
{
    va_list ap;
    va_start(ap, kind);
    switch (kind) {
    case DL_NO_ANGLES:
        DisplayLogNoAngles();
        break;
    case DL_ALLOC_SURFACE: {
        void* surface = va_arg(ap, void*);
        const char* name = va_arg(ap, const char*);
        DisplayLogAllocSurface(surface, name);
        break;
    }
    case DL_ALLOC_FRAME: {
        void* frame = va_arg(ap, void*);
        const char* name = va_arg(ap, const char*);
        DisplayLogAllocFrame(frame, name);
        break;
    }
    case DL_GAF: {
        const char* path = va_arg(ap, const char*);
        void* base = va_arg(ap, void*);
        DisplayLogGaf(path, base);
        break;
    }
    case DL_3DO: {
        const char* path = va_arg(ap, const char*);
        void* root = va_arg(ap, void*);
        DisplayLog3do(path, root);
        break;
    }
    case DL_FONT: {
        void* font = va_arg(ap, void*);
        const char* name = va_arg(ap, const char*);
        DisplayLogFont(font, name);
        break;
    }
    case DL_FEATURE_TYPE: {
        const char* name = va_arg(ap, const char*);
        int spot = va_arg(ap, int);
        DisplayLogFeatureType(name, spot);
        break;
    }
    case DL_FRAME_END: {
        void* display = va_arg(ap, void*);
        DisplayLogFrameEnd(display);
        break;
    }
    case DL_FREE: {
        void* block = va_arg(ap, void*);
        DlFree(block);
        break;
    }
    case DL_WRITTEN: {
        void* block = va_arg(ap, void*);
        if (block != 0)
            DlWritten(block);
        break;
    }
    }
    va_end(ap);
}
