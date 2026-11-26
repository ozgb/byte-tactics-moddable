#include "win_types.h"
#include <string.h>

#include "display_game.h"
#include "display_log.h"
#include "harness.h"
#include "../../interfaces/host.h"
#include "../game/game.h"
#include "../network/player.h"
#include "../network/player_info.h"
#include "../graphics/display.h"
#include "../graphics/gaf_frame.h"
#include "../graphics/model_types.h"
#include "../graphics/font.h"
#include "../graphics/rect.h"
#include "../units/unit_def.h"
#include "../weapons/explosion_types.h"
#include "../weapons/debris.h"
#include "../util/point.h"
#include "../util/registry.h"
#include "../util/vec3.h"
#include "../../link/globals.h"

static const char* s_featureType = nullptr;

static void Surfaceof(Surface* s, DlSurface* out)
{
    if (s == nullptr) {
        Display* d = GetDisplay();
        if (d->useOverrideSurface != 0)
            s = d->overrideSurface;
        else if (d->windowed != 0)
            s = &d->cached;
    }
    if (s == nullptr) {
        Display* d = GetDisplay();
        out->pixels = nullptr;
        out->width = d->width;
        out->height = d->height;
        out->pitch = d->width;
        out->clipLeft = d->vec.left;
        out->clipTop = d->vec.top;
        out->clipRight = d->vec.right;
        out->clipBottom = d->vec.bottom;
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

static void SurfaceOfVoid(void* s, DlSurface* out)
{
    Surfaceof((Surface*)s, out);
}

static void PictureOf(const GafFrame* f, DlPicture* out)
{
    out->key = f;
    out->pixels = f->pixelsOrLayers;
    out->width = f->width;
    out->height = f->height;
    out->compressed = f->compressed;
    out->layered = f->layers;
}

static void Union(const GafFrame* f, int x, int y, int depth, int* x0, int* y0, int* x1, int* y1)
{
    if (f == nullptr || depth > 4)
        return;
    if (f->layers > 0) {
        for (int i = 0; i < (int)f->layers; i++)
            Union(((GafFrame**)f->pixelsOrLayers)[i], x, y, depth + 1, x0, y0, x1, y1);
        return;
    }
    int l = x - f->xOffset;
    int t = y - f->yOffset;
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

void DisplayLogSprite(int kind, Surface* dst, GafFrame* bmp, int x, int y, int extra)
{
    static const char* const names[] = { "sprite", "sprite_opaque", "sprite_blend", "sprite_gray", "sprite_shadow", "sprite_lit",
                                         "sprite_erase", "lens" };
    if (bmp == nullptr)
        return;
    DlSurface d;
    Surfaceof(dst, &d);
    int x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30);
    Union(bmp, x, y, 0, &x0, &y0, &x1, &y1);
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlPicture p;
    PictureOf(bmp, &p);
    const unsigned char* reads;
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

void DisplayLogQuad(void* dst, GafFrame* bmp, Point* corners, Point* source)
{
    if (bmp == nullptr || corners == nullptr)
        return;
    DlSurface d;
    SurfaceOfVoid(dst, &d);
    int x0 = corners[0].x, y0 = corners[0].y, x1 = corners[0].x, y1 = corners[0].y;
    for (int i = 1; i < 4; i++) {
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
    DlPicture p;
    PictureOf(bmp, &p);
    const unsigned char* reads;
    DlPictureReads(&p, &reads);
    DlBegin("quad", &d, x0, y0, x1, y1, reads);
    DlPutRaw(" ");
    DlPutPicture(&p);
    for (int i = 0; i < 4; i++) {
        DlPutRaw(" ");
        PutCorner(&corners[i], DlOffsetX(), DlOffsetY());
    }
    DlPutRaw(" ");
    if (source == nullptr) {
        DlPutRaw("-");
    } else {
        for (int i = 0; i < 4; i++) {
            if (i != 0)
                DlPutRaw(",");
            PutCorner(&source[i], 0, 0);
        }
    }
    DlEnd();
}

static void Describe(const Surface* s, DlSurface* out)
{
    if (s == nullptr) {
        Surfaceof(nullptr, out);
        return;
    }
    Surfaceof((Surface*)s, out);
}

void DisplayLogBlit(Surface* dst, Surface* src, int x, int y)
{
    if (dst == nullptr && src == nullptr)
        return;
    DlSurface d;
    DlSurface s;
    Surfaceof(dst, &d);
    Describe(src, &s);
    int bx = x;
    int by = y;
    if (src != nullptr) {
        bx = x - src->x;
        by = y - src->y;
    }
    int x0 = bx, y0 = by, x1 = bx + s.width, y1 = by + s.height;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    int sx, sy;
    DlSourceOffset(&s, &sx, &sy);
    const unsigned char* base = DlSourceBase(&s);
    int whole = sx == 0 && sy == 0;
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

void DisplayLogCopy(Surface* dst, Surface* src, Rect* rect, Point* pos)
{
    if (src == nullptr || rect == nullptr || pos == nullptr)
        return;
    DlSurface d;
    DlSurface s;
    Surfaceof(dst, &d);
    Describe(src, &s);
    int w = rect->right - rect->left + 1;
    int h = rect->bottom - rect->top + 1;
    int x0 = pos->x, y0 = pos->y, x1 = pos->x + w, y1 = pos->y + h;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    int sx, sy;
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

void DisplayLogClear(Surface* dst, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0 = 0, y0 = 0, x1 = d.width, y1 = d.height;
    DlBegin("clear", &d, x0, y0, x1, y1, nullptr);
    PutInt(colour);
    DlEnd();
}

void DisplayLogClearFrame(GafFrame* frame, int colour)
{
    DlSurface d;
    d.pixels = frame->pixelsOrLayers;
    d.width = frame->width;
    d.height = frame->height;
    d.pitch = frame->width;
    d.clipLeft = 0;
    d.clipTop = 0;
    d.clipRight = frame->width - 1;
    d.clipBottom = frame->height - 1;
    DlBegin("clear", &d, 0, 0, d.width, d.height, nullptr);
    PutInt(colour);
    DlEnd();
}

void DisplayLogLine(Surface* dst, int ax, int ay, int bx, int by, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0 = ax < bx ? ax : bx;
    int x1 = (ax < bx ? bx : ax) + 1;
    int y0 = ay < by ? ay : by;
    int y1 = (ay < by ? by : ay) + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("line", &d, x0, y0, x1, y1, nullptr);
    PutXY(ax, ay);
    PutXY(bx, by);
    PutInt(colour);
    DlEnd();
}

void DisplayLogPixel(Surface* dst, int x, int y, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0 = x, y0 = y, x1 = x + 1, y1 = y + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("pixel", &d, x0, y0, x1, y1, nullptr);
    PutXY(x, y);
    PutInt(colour);
    DlEnd();
}

static void RectArea(const DlSurface* d, const Rect* r, int* x0, int* y0, int* x1, int* y1)
{
    if (r == nullptr) {
        Display* display = GetDisplay();
        *x0 = 0;
        *y0 = 0;
        *x1 = display->width + 1;
        *y1 = display->height + 1;
    } else {
        *x0 = r->left;
        *y0 = r->top;
        *x1 = r->right + 1;
        *y1 = r->bottom + 1;
    }
    DlCutRect(d, x0, y0, x1, y1);
}

void DisplayLogFade(Surface* dst, Rect* rect, int level)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0, y0, x1, y1;
    RectArea(&d, rect, &x0, &y0, &x1, &y1);
    DlBegin("fade", &d, x0, y0, x1, y1, nullptr);
    PutInt(level);
    DlEnd();
}

void DisplayLogFill(Surface* dst, Rect* rect, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0, y0, x1, y1;
    RectArea(&d, rect, &x0, &y0, &x1, &y1);
    DlBegin("fill", &d, x0, y0, x1, y1, nullptr);
    PutInt(colour);
    DlEnd();
}

void DisplayLogOutline(Surface* dst, const int* rect, int colour, int lit)
{
    if (rect == nullptr)
        return;
    DlSurface d;
    Surfaceof(dst, &d);
    Rect r;
    r.left = rect[0];
    r.top = rect[1];
    r.right = rect[2];
    r.bottom = rect[3];
    int x0, y0, x1, y1;
    RectArea(&d, &r, &x0, &y0, &x1, &y1);
    DlBegin(lit ? "outline_lit" : "outline", &d, x0, y0, x1, y1, nullptr);
    PutXY(rect[0], rect[1]);
    PutXY(rect[2], rect[3]);
    PutInt(colour);
    DlEnd();
}

void DisplayLogGray(Surface* dst, Rect* rect)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0, y0, x1, y1;
    RectArea(&d, rect, &x0, &y0, &x1, &y1);
    DlBegin("gray", &d, x0, y0, x1, y1, nullptr);
    DlEnd();
}

void DisplayLogDither(Surface* dst, Rect* rect, int phase)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0, y0, x1, y1;
    RectArea(&d, rect, &x0, &y0, &x1, &y1);
    DlBegin("dither", &d, x0, y0, x1, y1, nullptr);
    PutInt(phase);
    DlEnd();
}

void DisplayLogCircle(Surface* dst, int x, int y, int radius, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int r = radius < 0 ? -radius : radius;
    int x0 = x - r, y0 = y - r, x1 = x + r + 1, y1 = y + r + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("circle", &d, x0, y0, x1, y1, nullptr);
    PutXY(x, y);
    PutInt(radius);
    PutInt(colour);
    DlEnd();
}

void DisplayLogDashed(Surface* dst, int x, int y, int radius, int colour, int dashes, int start)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int r = radius < 0 ? -radius : radius;
    int x0 = x - r, y0 = y - r, x1 = x + r + 1, y1 = y + r + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("circle_dashed", &d, x0, y0, x1, y1, nullptr);
    PutXY(x, y);
    PutInt(radius);
    PutInt(colour);
    PutInt(dashes);
    PutInt(start);
    DlEnd();
}

void DisplayLogPolygon(Surface* dst, Point* points, int count, int colour)
{
    DlSurface d;
    Surfaceof(dst, &d);
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    if (count > 0) {
        x0 = x1 = points[0].x;
        y0 = y1 = points[0].y;
        for (int i = 1; i < count; i++) {
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
    DlBegin("polygon", &d, x0, y0, x1, y1, nullptr);
    PutInt(colour);
    for (int i = 0; i < count; i++) {
        DlPutRaw(" ");
        PutCorner(&points[i], DlOffsetX(), DlOffsetY());
    }
    DlEnd();
}

void DisplayLogText(Surface* dst, const unsigned char* text, int x, int y, int width)
{
    Display* display = GetDisplay();
    DlSurface d;
    Surfaceof(dst, &d);
    int height = display->font->glyphs[0];
    Rect r;
    r.left = x;
    r.top = y;
    r.right = x + width;
    r.bottom = y + height;
    Rect clip;
    clip.left = d.clipLeft;
    clip.top = d.clipTop;
    clip.right = d.clipRight;
    clip.bottom = d.clipBottom;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    if (RectInsideRect(&r, &clip) != 0) {
        x0 = x;
        y0 = y;
        x1 = x + width;
        y1 = y + height;
        DlCutRect(&d, &x0, &y0, &x1, &y1);
    }
    DlBegin("text", &d, x0, y0, x1, y1, nullptr);
    char font[64];
    DlFontName(display->font, font, sizeof font);
    DlPutRaw(" ");
    DlPutName(font, (int)strlen(font));
    PutInt(display->foreColor);
    PutInt(display->backColor);
    PutInt(display->keyColor);
    PutXY(x, y);
    DlPutRaw(" ");
    if (text != nullptr)
        DlPutName((const char*)text, (int)strlen((const char*)text));
    DlEnd();
}

void DisplayLogTerrain(void* dst)
{
    DlSurface d;
    SurfaceOfVoid(dst, &d);
    int x0 = g_game->rect.left;
    int y0 = g_game->rect.top;
    int x1 = x0 + g_game->viewWidth;
    int y1 = y0 + g_game->viewHeight;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin("terrain", &d, x0, y0, x1, y1, nullptr);
    PutText(HarnessMapName());
    PutInt(g_game->scrollX);
    PutInt(g_game->scrollY);
    DlEnd();
}

static void PutHexFloat(float f)
{
    unsigned bits;
    memcpy(&bits, &f, 4);
    DlPutHex8(bits);
}

static void PutPieces(const ModelPiece* p, int depth)
{
    for (; p != nullptr && depth < 64; p = p->sibling.get()) {
        DlPutRaw(" ");
        const char* name = p->object != nullptr && p->object->name != nullptr ? p->object->name : "";
        DlPutName(name, (int)strlen(name));
        DlPutRaw(",");
        DlPutInt(p->flags.word & 3);
        DlPutRaw(",");
        DlPutInt(p->rot_x);
        DlPutRaw(",");
        DlPutInt(p->rot_y);
        DlPutRaw(",");
        DlPutInt(p->rot_z);
        DlPutRaw(",");
        DlPutInt(p->offset.x);
        DlPutRaw(",");
        DlPutInt(p->offset.y);
        DlPutRaw(",");
        DlPutInt(p->offset.z);
        PutPieces(p->child.get(), depth + 1);
    }
}

static void ModelItem(const DlSurface* d, const char* kind, UnitDrawView* owner, ModelState* state)
{
    int x0 = d->clipLeft, y0 = d->clipTop, x1 = d->clipRight + 1, y1 = d->clipBottom + 1;
    DlCutRect(d, &x0, &y0, &x1, &y1);
    DlBegin(kind, d, x0, y0, x1, y1, nullptr);
    char type[64];
    type[0] = '\0';
    const char* source = s_featureType;
    if (source == nullptr && owner->def.get() != nullptr)
        source = owner->def.get()->unitname;
    if (source != nullptr) {
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
    ModelPiece* root = state->root.get();
    int known = 0;
    const char* model = root != nullptr ? DlModelName(root->object, &known) : "-";
    DlPutRaw(" ");
    if (known) {
        DlPutName(model, (int)strlen(model));
    } else {
        DlPutName("?", 1);
    }
    FixedVec3Union v;
    v.v[0] = owner->pos_x - (g_game->scrollX << 16);
    v.v[1] = owner->pos_y;
    v.v[2] = owner->pos_z - (g_game->scrollY << 16);
    int sx = v.p.x.whole + 0x80;
    int sy = v.p.z.whole - (v.p.y.whole >> 1) + 0x20;
    PutXY(sx, sy);
    PutInt(owner->pos_x);
    PutInt(owner->pos_y);
    PutInt(owner->pos_z);
    PutInt(owner->angles.x);
    PutInt(owner->angles.y);
    PutInt(owner->angles.z);
    int colour = 0;
    Player* player = &g_game->players_1b63[owner->playerIndex];
    if (player->info.get() != nullptr)
        colour = player->info.get()->color;
    PutInt(colour);
    DlPutRaw(" ");
    PutHexFloat(owner->buildLeft);
    PutInt((owner->activateFlags & 4) != 0 || g_game->debugMode != 0 ? 1 : 0);
    DlPutRaw(" ");
    PutHexFloat(g_lightX);
    DlPutRaw(",");
    PutHexFloat(g_lightY);
    DlPutRaw(",");
    PutHexFloat(g_lightZ);
    PutPieces(root, 0);
    DlEnd();
}

void DisplayLogModel(void* dst, ModelState* state)
{
    if (state == nullptr)
        return;
    UnitDrawView* owner = state->OwnerView();
    if (owner == nullptr)
        return;
    DlSurface d;
    SurfaceOfVoid(dst, &d);
    ModelItem(&d, "model", owner, state);
    const char* feature = s_featureType;
    s_featureType = nullptr;
    for (UnitDrawView* u = owner->cargo.get(); u != nullptr; u = u->cargoNext.get()) {
        if ((u->flags & 0x20000) == 0 && u->state.get() != nullptr)
            ModelItem(&d, "cargo", u, u->state.get());
    }
    s_featureType = feature;
}

static int s_noAngles = 0;

void DisplayLogNoAngles()
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
    SurfaceOfVoid(dst, &d);
    int x0 = d.clipLeft, y0 = d.clipTop, x1 = d.clipRight + 1, y1 = d.clipBottom + 1;
    DlCutRect(&d, &x0, &y0, &x1, &y1);
    DlBegin(kind, &d, x0, y0, x1, y1, nullptr);
}

void DisplayLogProjectile(void* dst, Vec3* offset, const void* piece, const short* angles)
{
    BeginClipItem("projectile", dst);
    PutPiece(piece);
    PutInt(offset->x);
    PutInt(offset->y);
    PutInt(offset->z);
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

void DisplayLogDebris(void* dst, const void* piece, int colour, int ox, int oy, int oz, const short* angles)
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

void DisplayLogShard(void* dst, Vec3* offset, DebrisPiece* piece, const short* angles)
{
    BeginClipItem("shard", dst);
    DlPutRaw(" ");
    Face* face = piece->faces.get();
    int index = piece->firstFace != -1 ? 1 : 0;
    int named = 0;
    for (; face != nullptr && index < piece->faceCount; index++) {
        const Face* f = &face[index];
        if (f->flags.bits.a != 0) {
            DlPutRaw("#");
            DlPutInt(f->color);
            named = 1;
            break;
        }
        if (f->nverts == 4) {
            const GafFrame* pic = nullptr;
            if (f->flags.bits.b != 0) {
                const char* table = (const char*)f->tex.src;
                if (table != nullptr)
                    pic = *(const GafFrame* const*)(table + 0x28 + (int)f->tex.index * 8);
            } else {
                pic = (const GafFrame*)f->tex.pic;
            }
            if (pic != nullptr) {
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
    PutInt(offset->x);
    PutInt(offset->y);
    PutInt(offset->z);
    PutInt(angles[0]);
    PutInt(angles[1]);
    PutInt(angles[2]);
    const Vec3* v = piece->verts.get();
    for (int i = 0; i < 8; i++) {
        DlPutRaw(" ");
        DlPutInt(v != nullptr ? v[i].x : 0);
        DlPutRaw(",");
        DlPutInt(v != nullptr ? v[i].y : 0);
        DlPutRaw(",");
        DlPutInt(v != nullptr ? v[i].z : 0);
    }
    DlEnd();
}

void DisplayLogAllocSurface(Surface* surface, const char* name)
{
    if (DlIsOn() == 0 || surface == nullptr)
        return;
    DlRegisterSurface(surface, surface->pixels, surface->width, surface->height, surface->pitch, name);
}

void DisplayLogAllocFrame(GafFrame* frame, const char* name)
{
    if (DlIsOn() == 0 || frame == nullptr)
        return;
    DlRegisterFrame(frame, frame->pixelsOrLayers, frame->width, frame->height, name);
}

void DisplayLogGaf(const char* path, void* base)
{
    if (DlIsOn() == 0 || base == nullptr)
        return;
    int* archive = (int*)base;
    DlLockRegistries();
    DlGafBegin(path, base);
    for (int i = 0; i < (short)archive[1]; i++) {
        unsigned char* record = (unsigned char*)archive[3 + i];
        unsigned short count = *(unsigned short*)record;
        const char* entry = (const char*)(record + 8);
        for (int j = 0; j < (int)count; j++) {
            int frame = *(int*)(record + 0x28 + j * 8);
            DlGafFrame(entry, j, (const void*)frame);
        }
    }
    DlUnlockRegistries();
}

static void RegisterPieces(const char* stem, Object3do* piece, int depth)
{
    for (; piece != nullptr && depth < 64; piece = piece->sibling) {
        DlRegisterModelPiece(piece, stem, piece->name != nullptr ? piece->name : "");
        RegisterPieces(stem, piece->child, depth + 1);
    }
}

void DisplayLog3do(const char* path, Object3do* root)
{
    if (DlIsOn() == 0 || root == nullptr)
        return;
    const char* base = path;
    for (const char* p = path; *p != '\0'; p++) {
        if (*p == '\\' || *p == '/')
            base = p + 1;
    }
    char stem[64];
    int n = 0;
    while (base[n] != '\0' && base[n] != '.' && n < (int)sizeof stem - 1) {
        stem[n] = base[n];
        n++;
    }
    stem[n] = '\0';
    DlLockRegistries();
    RegisterPieces(stem, root, 0);
    DlUnlockRegistries();
}

void DisplayLogFont(Font* font, const char* name)
{
    if (DlIsOn() == 0 || font == nullptr)
        return;
    DlRegisterFont(font, name);
}

void DisplayLogFeatureType(const char* name)
{
    s_featureType = name;
}

void DisplayLogFrameEnd(Display* d)
{
    if (DlIsOn() == 0)
        return;
    unsigned char palette[256 * 4];
    for (int i = 0; i < 256; i++) {
        palette[i * 4] = d->entries[i].peRed;
        palette[i * 4 + 1] = d->entries[i].peGreen;
        palette[i * 4 + 2] = d->entries[i].peBlue;
        palette[i * 4 + 3] = 0;
    }
    DlPicture cursor;
    if ((d->cursorThreadEnabled != 1 && d->cursorHideCount > 0) || d->cursorSprite == nullptr) {
        HarnessDisplayFrame(palette, nullptr, 0, 0);
        return;
    }
    PictureOf(d->cursorSprite, &cursor);
    int x = 0;
    int y = 0;
    host_cursor_get(&x, &y);
    HarnessDisplayFrame(palette, &cursor, x, y);
}
