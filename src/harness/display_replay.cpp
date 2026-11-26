#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display_replay.h"
#include "display_log.h"
#include "harness.h"
#include "../../interfaces/host.h"
#include "../game/game.h"
#include "../graphics/display.h"
#include "../graphics/gaf_frame.h"
#include "../graphics/handle.h"
#include "../graphics/surface.h"
#include "../graphics/font.h"
#include "../graphics/draw.h"
#include "../graphics/gaf.h"
#include "../graphics/rect.h"
#include "../files/files_api.h"
#include "../map/line_of_sight.h"
#include "../util/point.h"
#include "../graphics/model_types.h"
#include "../graphics/memory_cache.h"
#include "../scripts/object_state.h"
#include "../units/unit_def.h"
#include "../network/player.h"
#include "../network/player_info.h"
#include "../state/block.h"
#include "../util/engine_services.h"
#include "../../link/globals.h"
#include "../weapons/explosion_types.h"
#include "../weapons/debris.h"
#include "../ingame/info_panel.h"

void* __stdcall LoadGaf(char* name);
class ProjectedModel;
void __stdcall DrawModel3doProjected(void* surface, Vec3* offset, ProjectedModel* obj, short* angles);
void* __stdcall GetGafSequenceFrame(GafRef* ref);
void* __stdcall GetGafFrame(GafSequence* table, int index);
void __stdcall InitGafSequence(void* ref, void* src, int index);
#include "../network/net_stats.h"
void __stdcall DrawFrame(Surface* dst, GafFrame* bmp, int x, int y);
void __stdcall DrawFrameBlended(Surface* surface, GafFrame* frame, int x, int y);
void __stdcall DrawSurface(Surface* dst, Surface* bmp, int x, int y);
int __stdcall DrawLine(Surface* surface, int x0, int y0, int x1, int y1, int color);
int __stdcall DrawRectangle(Surface* surface, Rect* r, int color);
Surface* __stdcall DrawLitRectangle(Surface* surface, int* rect, int color);
void __stdcall FillPolygon(Surface* surface, Point* points, int n, unsigned char color);
void __stdcall DrawString(Surface* dst, unsigned char* text, int x, int y, int maxWidth);
#include "../game/data_files.h"

void __stdcall DrawFrameQuad(void* surf, GafFrame* bmp, FrameQuad* dst, FrameQuad* src);

static const int kMaxTargets = 64;
static const int kMaxWords = 4096;
static const int kMaxGafs = 256;
static const int kMaxFonts = 32;
static const int kMaxKinds = 64;

struct Target {
    char name[128];
    int width;
    int height;
    unsigned char* pixels;
};

struct LoadedGaf {
    char path[260];
    int* base;
};

struct LoadedFont {
    char name[64];
    Font* font;
};

struct Skipped {
    char kind[32];
    int count;
};

static Target s_targets[kMaxTargets];
static int s_targetCount;
static LoadedGaf s_gafs[kMaxGafs];
static int s_gafCount;
static LoadedFont s_fonts[kMaxFonts];
static int s_fontCount;
static Skipped s_skipped[kMaxKinds];
static int s_skippedCount;
static unsigned char s_palette[1024];
static char s_paletteHash[20];

[[noreturn]] static void ReplayFail(const char* what, const char* detail)
{
    char msg[600];
    sprintf(msg, "testreplay: %s%s\n", what, detail != nullptr ? detail : "");
    host_log(msg);
    host_exit(2);
    __builtin_unreachable();
}

static int Hex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

static void Unescape(const char* in, char* out, int cap)
{
    int n = 0;
    if (strcmp(in, "-") == 0) {
        out[0] = '\0';
        return;
    }
    for (const char* p = in; *p != '\0' && n < cap - 1; p++) {
        if (p[0] == '%' && Hex(p[1]) >= 0 && Hex(p[2]) >= 0) {
            out[n++] = (char)(Hex(p[1]) * 16 + Hex(p[2]));
            p += 2;
        } else {
            out[n++] = *p;
        }
    }
    out[n] = '\0';
}

static unsigned long long HashBytes(const unsigned char* p, unsigned n)
{
    unsigned long long h = 0xcbf29ce484222325ull;
    unsigned i = 0;
    for (; i + 8 <= n; i += 8) {
        unsigned long long w;
        memcpy(&w, p + i, 8);
        h = (h ^ w) * 0x100000001b3ull;
    }
    for (; i < n; i++)
        h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

static void Skip(const char* kind)
{
    for (int i = 0; i < s_skippedCount; i++) {
        if (strcmp(s_skipped[i].kind, kind) == 0) {
            s_skipped[i].count++;
            return;
        }
    }
    if (s_skippedCount < kMaxKinds) {
        strncpy(s_skipped[s_skippedCount].kind, kind, sizeof s_skipped[0].kind - 1);
        s_skipped[s_skippedCount].count = 1;
        s_skippedCount++;
    }
}

static Target* FindTarget(const char* name)
{
    for (int i = 0; i < s_targetCount; i++) {
        if (strcmp(s_targets[i].name, name) == 0)
            return &s_targets[i];
    }
    return nullptr;
}

static void Grow(const char* name, int width, int height)
{
    Target* t = FindTarget(name);
    if (t == nullptr) {
        if (s_targetCount >= kMaxTargets)
            ReplayFail("too many targets", nullptr);
        t = &s_targets[s_targetCount++];
        strncpy(t->name, name, sizeof t->name - 1);
        t->width = 0;
        t->height = 0;
    }
    if (width > t->width)
        t->width = width;
    if (height > t->height)
        t->height = height;
}

static int Split(char* line, char** words)
{
    int n = 0;
    char* p = line;
    while (*p != '\0' && n < kMaxWords) {
        words[n++] = p;
        while (*p != '\0' && *p != ' ')
            p++;
        if (*p == ' ')
            *p++ = '\0';
    }
    return n;
}

struct NamedObject {
    char name[64];
    Object3do* object;
};

static const int kMaxObjects = 1024;
static NamedObject s_objects[kMaxObjects];
static int s_objectCount;

static Object3do* ObjectNamed(const char* escaped)
{
    for (int i = 0; i < s_objectCount; i++) {
        if (strcmp(s_objects[i].name, escaped) == 0)
            return s_objects[i].object;
    }
    if (s_objectCount >= kMaxObjects)
        ReplayFail("too many models", nullptr);
    char name[64];
    Unescape(escaped, name, sizeof name);
    Object3do* object = (Object3do*)LoadObject3d(name);
    if (object == nullptr)
        ReplayFail("cannot load the model ", name);
    NamedObject* o = &s_objects[s_objectCount++];
    strncpy(o->name, escaped, sizeof o->name - 1);
    o->object = object;
    return object;
}

static Object3do* FindPiece(Object3do* p, const char* name, int depth)
{
    for (; p != nullptr && depth < 64; p = p->sibling) {
        if (p->name != nullptr && _stricmp(p->name, name) == 0)
            return p;
        Object3do* found = FindPiece(p->child, name, depth + 1);
        if (found != nullptr)
            return found;
    }
    return nullptr;
}

static Object3do* PieceNamed(const char* escaped)
{
    char model[100];
    strncpy(model, escaped, sizeof model - 1);
    model[sizeof model - 1] = '\0';
    char* colon = strchr(model, ':');
    if (colon == nullptr)
        ReplayFail("a piece name without its model: ", escaped);
    *colon = '\0';
    char piece[100];
    Unescape(colon + 1, piece, sizeof piece);
    Object3do* found = FindPiece(ObjectNamed(model), piece, 0);
    if (found == nullptr)
        ReplayFail("no such piece: ", escaped);
    return found;
}

static void Measure(char* text)
{
    Grow("screen", 640, 480);
    char* words[kMaxWords];
    for (char* line = text; line != nullptr && *line != '\0';) {
        char* next = strchr(line, '\n');
        if (next != nullptr)
            *next = '\0';
        char copy[256];
        strncpy(copy, line, sizeof copy - 1);
        copy[sizeof copy - 1] = '\0';
        int n = Split(copy, words);
        if (n >= 8 && (strcmp(words[0], "model") == 0 || strcmp(words[0], "cargo") == 0))
            ObjectNamed(words[7]);
        if (n >= 7 && (strcmp(words[0], "projectile") == 0 || strcmp(words[0], "debris") == 0))
            PieceNamed(words[6]);
        if (n >= 6 && strcmp(words[0], "frame") != 0 && strcmp(words[0], "cursor") != 0 && strcmp(words[0], "bt-display") != 0
            && strcmp(words[0], "picture") != 0) {
            int x0 = atoi(words[2]), y0 = atoi(words[3]), x1 = atoi(words[4]), y1 = atoi(words[5]);
            if (x0 < x1 && y0 < y1) {
                Grow(words[1], x1, y1);
                if (strcmp(words[0], "blit") == 0 && n >= 9)
                    Grow(words[6], x1 - atoi(words[7]), y1 - atoi(words[8]));
                else if (strcmp(words[0], "copy") == 0 && n >= 13)
                    Grow(words[6], atoi(words[9]) + 1, atoi(words[10]) + 1);
            }
        }
        if (next != nullptr)
            *next = '\n';
        line = next != nullptr ? next + 1 : nullptr;
    }
    for (int i = 0; i < s_targetCount; i++) {
        Target* t = &s_targets[i];
        t->pixels = (unsigned char*)calloc((size_t)t->width * t->height + 64, 1);
        if (t->pixels == nullptr)
            ReplayFail("no memory for the target ", t->name);
    }
}

static void SurfaceFor(Target* t, Surface* s, int x0, int y0, int x1, int y1)
{
    memset(s, 0, sizeof *s);
    s->width = t->width;
    s->height = t->height;
    s->pitch = t->width;
    s->pixels = t->pixels;
    s->clip.left = x0;
    s->clip.top = y0;
    s->clip.right = x1 - 1;
    s->clip.bottom = y1 - 1;
    s->flag0 = 1;
}

static int* GafFile(const char* path)
{
    for (int i = 0; i < s_gafCount; i++) {
        if (strcmp(s_gafs[i].path, path) == 0)
            return s_gafs[i].base;
    }
    if (s_gafCount >= kMaxGafs)
        ReplayFail("too many picture files", nullptr);
    char game[260];
    strncpy(game, path, sizeof game - 1);
    game[sizeof game - 1] = '\0';
    for (char* p = game; *p != '\0'; p++) {
        if (*p == '/')
            *p = '\\';
    }
    LoadedGaf* g = &s_gafs[s_gafCount++];
    strcpy(g->path, path);
    g->base = (int*)LoadGaf(game);
    if (g->base == nullptr)
        ReplayFail("cannot load the picture file ", path);
    return g->base;
}

struct NamedPicture {
    char* name;
    GafFrame* frame;
};

static const int kMaxPictures = 1 << 16;
static NamedPicture s_pictures[kMaxPictures];
static int s_pictureCount;

static NamedPicture* FindPicture(const char* name)
{
    for (int i = s_pictureCount - 1; i >= 0; i--) {
        if (strcmp(s_pictures[i].name, name) == 0)
            return &s_pictures[i];
    }
    return nullptr;
}

static GafFrame* KeepPicture(const char* name, GafFrame* frame)
{
    NamedPicture* known = FindPicture(name);
    if (known == nullptr) {
        if (s_pictureCount >= kMaxPictures)
            ReplayFail("too many pictures", nullptr);
        known = &s_pictures[s_pictureCount++];
        known->name = (char*)malloc(strlen(name) + 1);
        strcpy(known->name, name);
    }
    known->frame = frame;
    return frame;
}

static void DefinePicture(char** w, int n, const char* line)
{
    if (n < 6)
        ReplayFail("a picture line without its fields", nullptr);
    const char* size = strchr(w[1] + 1, ':');
    int width = size != nullptr ? atoi(size + 1) : 0;
    const char* by = size != nullptr ? strchr(size, 'x') : nullptr;
    int height = by != nullptr ? atoi(by + 1) : 0;
    const char* hex = w[5];
    if (width <= 0 || height <= 0 || (int)strlen(hex) != width * height * 2)
        ReplayFail("a picture line whose pixels do not fit its size: ", w[1]);
    GafFrame* f = (GafFrame*)calloc(sizeof(GafFrame) + (size_t)width * height, 1);
    if (f == nullptr)
        ReplayFail("no memory for a picture", nullptr);
    f->width = (unsigned short)width;
    f->height = (unsigned short)height;
    f->xOffset = (short)atoi(w[2]);
    f->yOffset = (short)atoi(w[3]);
    f->transparency = (unsigned char)atoi(w[4]);
    f->pixelsOrLayers = (unsigned char*)(f + 1);
    for (int i = 0; i < width * height; i++)
        f->pixelsOrLayers[i] = (unsigned char)(Hex(hex[i * 2]) * 16 + Hex(hex[i * 2 + 1]));
    NamedPicture* old = FindPicture(w[1]);
    if (old != nullptr)
        free(old->frame);
    KeepPicture(w[1], f);
    (void)line;
}

static GafFrame* Picture(const char* name)
{
    NamedPicture* known = FindPicture(name);
    if (known != nullptr)
        return known->frame;
    if (name[0] == '@' || name[0] == '?')
        ReplayFail("a picture with no picture line: ", name);
    if (strncmp(name, "target:", 7) == 0) {
        Target* from = FindTarget(name + 7);
        if (from == nullptr)
            return nullptr;
        GafFrame* f = (GafFrame*)calloc(1, sizeof(GafFrame));
        if (f == nullptr)
            ReplayFail("no memory for a picture", nullptr);
        f->width = (unsigned short)from->width;
        f->height = (unsigned short)from->height;
        f->pixelsOrLayers = from->pixels;
        f->transparency = 0xff;
        return KeepPicture(name, f);
    }
    char text[400];
    strncpy(text, name, sizeof text - 1);
    text[sizeof text - 1] = '\0';
    int moved = 0;
    int xOffset = 0, yOffset = 0;
    char* last = strrchr(text, ':');
    if (last != nullptr && strchr(last, ',') != nullptr) {
        moved = 1;
        xOffset = atoi(last + 1);
        yOffset = atoi(strchr(last, ',') + 1);
        *last = '\0';
        last = strrchr(text, ':');
    }
    if (last == nullptr)
        ReplayFail("a picture name without its frame: ", name);
    *last = '\0';
    char* entry = strrchr(text, ':');
    if (entry == nullptr)
        ReplayFail("a picture name without its entry: ", name);
    *entry++ = '\0';
    int index = atoi(last + 1);
    char file[300];
    char entryName[100];
    Unescape(text, file, sizeof file);
    Unescape(entry, entryName, sizeof entryName);
    int* base = GafFile(file);
    for (int i = 0; i < (short)base[1]; i++) {
        unsigned char* record = (unsigned char*)base[3 + i];
        unsigned short count = *(unsigned short*)record;
        if (strcmp((const char*)(record + 8), entryName) == 0) {
            if (index < 0 || index >= (int)count)
                ReplayFail("no such frame in the picture ", name);
            GafFrame* frame = (GafFrame*)*(int*)(record + 0x28 + index * 8);
            if (moved) {
                GafFrame* copy = (GafFrame*)malloc(sizeof(GafFrame));
                *copy = *frame;
                copy->xOffset = (short)xOffset;
                copy->yOffset = (short)yOffset;
                frame = copy;
            }
            return KeepPicture(name, frame);
        }
    }
    ReplayFail("no such picture ", name);
    return nullptr;
}

static Font* FontNamed(const char* escaped)
{
    char name[64];
    Unescape(escaped, name, sizeof name);
    for (int i = 0; i < s_fontCount; i++) {
        if (strcmp(s_fonts[i].name, name) == 0)
            return s_fonts[i].font;
    }
    if (s_fontCount >= kMaxFonts)
        ReplayFail("too many fonts", nullptr);
    LoadedFont* f = &s_fonts[s_fontCount++];
    strcpy(f->name, name);
    f->font = LoadFontByName(name);
    return f->font;
}

static void Corner(const char* text, Point* p)
{
    p->x = atoi(text);
    const char* comma = strchr(text, ',');
    p->y = comma != nullptr ? atoi(comma + 1) : 0;
}

static void Corners(const char* text, FrameQuad* q)
{
    const char* p = text;
    for (int i = 0; i < 4; i++) {
        q->p[i].x = atoi(p);
        p = strchr(p, ',');
        p = p != nullptr ? p + 1 : "0";
        q->p[i].y = atoi(p);
        p = strchr(p, ',');
        p = p != nullptr ? p + 1 : "0";
    }
}

struct ModelSlot {
    char key[24];
    char model[64];
    UnitDrawView* unit;
    ModelState* state;
    unsigned char* picture;
    int pictureSize;
    unsigned char* shadow;
    int shadowSize;
};

static const int kMaxModels = 4096;
static const int kMaxCargo = 16;
static ModelSlot s_models[kMaxModels];
static int s_modelCount;
static char s_pending[1 + kMaxCargo][8192];
static int s_pendingCount;
static unsigned s_tick;

static unsigned HexWord(const char* text)
{
    return (unsigned)strtoul(text, nullptr, 16);
}

static float HexFloat(const char* text)
{
    unsigned bits = HexWord(text);
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

static int HasAttr(const char* attrs, char c)
{
    return strchr(attrs, c) != nullptr;
}

static void PieceName(const char* field, char* out, int size)
{
    char escaped[100];
    int n = 0;
    while (field[n] != '\0' && field[n] != ',' && n < (int)sizeof escaped - 1) {
        escaped[n] = field[n];
        n++;
    }
    escaped[n] = '\0';
    Unescape(escaped, out, size);
}

static ModelState* StateInOrder(Object3do* object, char** pieces, int count, UnitDrawView* unit)
{
    static char names[512][64];
    static const char* order[512];
    if (count > 512)
        ReplayFail("a model with too many pieces", nullptr);
    for (int i = 0; i < count; i++) {
        PieceName(pieces[i], names[i], sizeof names[i]);
        order[i] = names[i];
    }
    ModelBuildList list;
    memset(&list, 0, sizeof list);
    list.count = count;
    list.names = order;
    return CreatePlayerObjectState(object, &list, (Unit*)unit);
}

static ModelSlot* SlotFor(const char* key, const char* model, char** pieces, int pieceCount)
{
    ModelSlot* slot = nullptr;
    for (int i = 0; i < s_modelCount; i++) {
        if (strcmp(s_models[i].key, key) == 0) {
            slot = &s_models[i];
            break;
        }
    }
    if (slot != nullptr && strcmp(slot->model, model) == 0)
        return slot;
    if (slot == nullptr) {
        if (s_modelCount >= kMaxModels)
            ReplayFail("too many models", nullptr);
        slot = &s_models[s_modelCount++];
        memset(slot, 0, sizeof *slot);
        strncpy(slot->key, key, sizeof slot->key - 1);
    }
    Object3do* object = ObjectNamed(model);
    strncpy(slot->model, model, sizeof slot->model - 1);
    if (slot->unit == nullptr) {
        slot->unit = (UnitDrawView*)SimAllocTagged("Replay Unit", 0x118);
        memset(slot->unit, 0, 0x118);
        char* motion = (char*)SimAllocTagged("Replay Motion", 0x40);
        memset(motion, 0, 0x40);
        slot->unit->motion = motion;
        UnitDef* def = (UnitDef*)SimAllocTagged("Replay Def", sizeof(UnitDef));
        memset(static_cast<void*>(def), 0, sizeof(UnitDef));
        slot->unit->def = def;
    }
    slot->state = StateInOrder(object, pieces, pieceCount, slot->unit);
    slot->unit->state = slot->state;
    slot->picture = nullptr;
    slot->shadow = nullptr;
    return slot;
}

static int s_otherPlayer = -1;

static int OtherPlayer()
{
    if (s_otherPlayer >= 0)
        return s_otherPlayer;
    for (int i = 0; i < 10; i++) {
        if (i != g_game->playerIndex && g_game->players_1b63[i].info.get() != nullptr) {
            s_otherPlayer = i;
            return i;
        }
    }
    ReplayFail("no player to lend its colour", nullptr);
    return 0;
}

static void SavePicture(GafFrame* frame, unsigned char** saved, int* size)
{
    free(*saved);
    *saved = nullptr;
    *size = 0;
    if (frame == nullptr)
        return;
    int bytes = *(int*)((char*)frame - 4) - 8;
    *saved = (unsigned char*)malloc((size_t)bytes);
    if (*saved == nullptr)
        ReplayFail("no memory for a model picture", nullptr);
    memcpy(*saved, frame, (size_t)bytes);
    GafFrame* copy = (GafFrame*)*saved;
    copy->pixelsOrLayers = (unsigned char*)(copy->pixelsOrLayers - (unsigned char*)frame);
    if (copy->scratch != nullptr)
        copy->scratch = (unsigned char*)(copy->scratch - (unsigned char*)frame);
    *size = bytes;
}

static GafFrame* RestorePicture(const unsigned char* saved, int size, GafFrame** handle)
{
    *handle = nullptr;
    if (saved == nullptr)
        return nullptr;
    CMemoryCache* cache = (CMemoryCache*)g_game->unitTable;
    if (!cache->AllocHandle((void**)handle, size) || *handle == nullptr)
        ReplayFail("no room in the picture cache", nullptr);
    GafFrame* frame = *handle;
    memcpy(frame, saved, (size_t)size);
    frame->pixelsOrLayers = (unsigned char*)frame + (int)frame->pixelsOrLayers;
    if (frame->scratch != nullptr)
        frame->scratch = (unsigned char*)frame + (int)frame->scratch;
    return frame;
}

struct ModelFields {
    char* kind;
    char* model;
    char* light;
    char* key;
    char* attrs;
    char** pieces;
    int x;
    int y;
    int wx;
    int wy;
    int wz;
    int colour;
    float build;
    int cloak;
    int pieceCount;
    short ax;
    short ay;
    short az;
    char cache;
};

static int ParseModel(char** w, int n, ModelFields* m)
{
    if (n < 23)
        return 0;
    m->kind = w[0];
    m->model = w[7];
    m->x = atoi(w[8]);
    m->y = atoi(w[9]);
    m->wx = atoi(w[10]);
    m->wy = atoi(w[11]);
    m->wz = atoi(w[12]);
    m->ax = (short)atoi(w[13]);
    m->ay = (short)atoi(w[14]);
    m->az = (short)atoi(w[15]);
    m->colour = atoi(w[16]);
    m->build = HexFloat(w[17]);
    m->cloak = atoi(w[18]);
    m->light = w[19];
    m->key = w[20];
    m->cache = w[21][0];
    m->attrs = w[22];
    m->pieces = w + 23;
    m->pieceCount = n - 23;
    return 1;
}

static void SetUpModel(ModelSlot* slot, const ModelFields* m, int player)
{
    UnitDrawView* u = slot->unit;
    int structure = HasAttr(m->attrs, 's');
    int moving = HasAttr(m->attrs, 'm');
    u->pos_x = m->wx;
    u->pos_y = m->wy;
    u->pos_z = m->wz;
    u->carrier = 0;
    u->cargo = nullptr;
    u->cargoNext = nullptr;
    u->unitDefIndex = HasAttr(m->attrs, 'u') ? 1 : 0;
    u->id = m->key[0] >= '0' && m->key[0] <= '9' ? (unsigned short)atoi(m->key) : 0;
    u->playerIndex = (unsigned char)player;
    u->buildLeft = m->build;
    u->activateFlags = (unsigned char)((structure && moving ? 1 : 0) | (m->cloak ? 4 : 0));
    u->flags = (structure ? 0x20000000u : 0) | (HasAttr(m->attrs, 't') ? 0x200u : 0);
    u->zBufferFlag = HasAttr(m->attrs, 'z') ? 1 : 0;
    *(int*)(u->motion.get() + 0x20) = !structure && moving ? 1 : 0;
    UnitDef* def = u->def.get();
    def->flags1 = (HasAttr(m->attrs, 'n') ? 0x2000000u : 0) | (HasAttr(m->attrs, 'h') ? 0x40000000u : 0)
        | (HasAttr(m->attrs, 'v') ? 0x1000u : 0);
    ModelState* state = slot->state;
    if (m->pieceCount != state->count)
        ReplayFail("the model has another number of pieces: ", m->model);
    for (int i = 0; i < m->pieceCount; i++) {
        const char* field = m->pieces[i];
        const char* comma = strchr(field, ',');
        if (comma == nullptr)
            ReplayFail("a piece without its fields in ", m->model);
        int v[7];
        const char* p = comma + 1;
        for (int k = 0; k < 7; k++) {
            v[k] = atoi(p);
            p = strchr(p, ',');
            p = p != nullptr ? p + 1 : "0";
        }
        ModelPiece* piece = &state->pieces[i];
        char name[64];
        PieceName(field, name, sizeof name);
        if (_stricmp(name, piece->object->name) != 0)
            ReplayFail("a piece in another order in ", m->model);
        piece->flags.word = (unsigned short)((piece->flags.word & ~7) | (v[0] & 7));
        piece->short_10 = (short)v[1];
        piece->short_12 = (short)v[2];
        piece->short_14 = (short)v[3];
        piece->rot_x = v[4];
        piece->rot_y = v[5];
        piece->rot_z = v[6];
    }
    state->pos.x = m->ax;
    state->pos.y = m->ay;
    state->pos.z = m->az;
    RestorePieceVertices(state->root.get(), 1);
    PoseModel(state, state->root.get(), 0);
    state->animDirty = 0;
}

static void DrawPendingModel()
{
    if (s_pendingCount <= 0)
        return;
    int count = s_pendingCount;
    s_pendingCount = 0;
    static char* words[1 + kMaxCargo][kMaxWords];
    int n[1 + kMaxCargo];
    ModelFields fields[1 + kMaxCargo];
    for (int i = 0; i < count; i++) {
        n[i] = Split(s_pending[i], words[i]);
        if (!ParseModel(words[i], n[i], &fields[i]))
            ReplayFail("a model item without its fields", nullptr);
    }
    char** w = words[0];
    Target* t = FindTarget(w[1]);
    int x0 = atoi(w[2]), y0 = atoi(w[3]), x1 = atoi(w[4]), y1 = atoi(w[5]);
    if (t == nullptr || x0 >= x1 || y0 >= y1)
        return;
    int player = HasAttr(fields[0].attrs, 't') ? g_game->playerIndex : OtherPlayer();
    PlayerInfo* info = g_game->players_1b63[player].info.get();
    unsigned char oldColour = info->color;
    unsigned oldTicks = (unsigned)g_game->ticks;
    int oldScrollX = g_game->scrollX, oldScrollY = g_game->scrollY;
    float lx = g_lightX, ly = g_lightY, lz = g_lightZ;
    ModelSlot* slots[1 + kMaxCargo];
    for (int i = 0; i < count; i++) {
        slots[i] = SlotFor(fields[i].key, fields[i].model, fields[i].pieces, fields[i].pieceCount);
        SetUpModel(slots[i], &fields[i], player);
    }
    for (int i = 1; i < count; i++) {
        if (i == 1)
            slots[0]->unit->cargo = slots[i]->unit;
        else
            slots[i - 1]->unit->cargoNext = slots[i]->unit;
    }
    ModelSlot* slot = slots[0];
    const ModelFields* m = &fields[0];
    ModelState* state = slot->state;
    if (m->cache == 'n') {
        state->bitmap = nullptr;
        state->shadow = nullptr;
        state->cacheDrawCount = 0;
    } else if (m->cache == 'k') {
        if (slot->picture == nullptr)
            ReplayFail("a kept picture that was never made: ", m->key);
        RestorePicture(slot->picture, slot->pictureSize, &state->bitmap);
        RestorePicture(slot->shadow, slot->shadowSize, &state->shadow);
        state->cacheDrawCount = 1;
    } else {
        state->bitmap = nullptr;
        state->shadow = nullptr;
        state->cacheDrawCount = 1;
    }
    info->color = (unsigned char)m->colour;
    g_game->ticks = (int)s_tick;
    g_game->scrollX = (m->wx >> 16) - (m->x - 0x80);
    g_game->scrollY = (m->wz >> 16) - (m->y - 0x20 + ((short)(m->wy >> 16) >> 1));
    char light[64];
    strncpy(light, m->light, sizeof light - 1);
    light[sizeof light - 1] = '\0';
    char* second = strchr(light, ',');
    char* third = second != nullptr ? strchr(second + 1, ',') : nullptr;
    if (third == nullptr)
        ReplayFail("a model item without its light", nullptr);
    g_lightX = HexFloat(light);
    g_lightY = HexFloat(second + 1);
    g_lightZ = HexFloat(third + 1);
    Surface s;
    SurfaceFor(t, &s, x0, y0, x1, y1);
    ((CMemoryCache*)g_game->unitTable)->DrawObjectState(state, &s);
    SavePicture(state->bitmap, &slot->picture, &slot->pictureSize);
    SavePicture(state->shadow, &slot->shadow, &slot->shadowSize);
    state->bitmap = nullptr;
    state->shadow = nullptr;
    info->color = oldColour;
    g_game->ticks = (int)oldTicks;
    g_game->scrollX = oldScrollX;
    g_game->scrollY = oldScrollY;
    g_lightX = lx;
    g_lightY = ly;
    g_lightZ = lz;
}

static void DrawDebris(Surface* s, Object3D* piece, int colour, int ox, int oy, int oz, short ax, short ay, short az)
{
    static Vec3 turned[2000];
    static Point projected[2000];
    if (piece->vertexCount > 2000)
        ReplayFail("a debris piece with too many corners", piece->name);
    short angles[3] = {ax, ay, az};
    for (int i = piece->vertexCount - 1; i >= 0; i--)
        RotateByAngles(&piece->verts[i], &turned[i], angles);
    for (int i = 0; i < piece->vertexCount; i++) {
        projected[i].x = (short)((short)((turned[i].x + ox) >> 16) + 0x80);
        projected[i].y = (short)((short)((oz - turned[i].z) >> 16) - ((short)((turned[i].y + oy) >> 16) >> 1) + 0x20);
    }
    int i = piece->firstFace != -1 ? 1 : 0;
    for (Face* face = piece->faces + i; i < piece->faceCount; i++, face++) {
        Point poly[25];
        for (int j = 0; j < face->nverts && j < 25; j++)
            poly[j] = projected[face->vindex[j]];
        if (face->flags.bits.a) {
            FillPolygon(s, poly, face->nverts, (unsigned char)face->color);
        } else if (face->nverts == 4) {
            void* pic;
            if (face->flags.bits.b)
                pic = face->flags.bits.c ? GetGafFrame(face->tex.src, colour) : GetGafSequenceFrame(&face->tex);
            else
                pic = face->tex.pic;
            DrawFrameQuad(s, (GafFrame*)pic, (FrameQuad*)poly, nullptr);
        }
    }
}

static void DrawShard(Surface* s, char** w)
{
    DebrisPiece* box = &g_game->slots[299];
    Face* faces = box->faces.get();
    Vec3* verts = box->verts.get();
    FaceFlags flags;
    flags.raw = 0;
    int colour = 0;
    void* pic = nullptr;
    if (w[0][0] == '#') {
        flags.bits.a = 1;
        colour = atoi(w[0] + 1);
    } else if (strcmp(w[0], "-") == 0) {
        Skip("shard with no face");
        return;
    } else {
        pic = Picture(w[0]);
        if (pic == nullptr) {
            Skip("shard");
            return;
        }
    }
    for (int i = 0; i < box->faceCount; i++) {
        faces[i].flags = flags;
        faces[i].color = colour;
        faces[i].tex.pic = pic;
    }
    for (int i = 0; i < 8; i++) {
        const char* v = w[7 + i];
        verts[i].x = atoi(v);
        v = strchr(v, ',');
        verts[i].y = v != nullptr ? atoi(v + 1) : 0;
        v = v != nullptr ? strchr(v + 1, ',') : nullptr;
        verts[i].z = v != nullptr ? atoi(v + 1) : 0;
    }
    Vec3 offset;
    offset.x = atoi(w[1]);
    offset.y = atoi(w[2]);
    offset.z = atoi(w[3]);
    short angles[3] = {(short)atoi(w[4]), (short)atoi(w[5]), (short)atoi(w[6])};
    DrawDebrisProjected(s, &offset, box, angles);
}

static void Draw(char** w, int n)
{
    const char* kind = w[0];
    Target* t = FindTarget(w[1]);
    int x0 = atoi(w[2]), y0 = atoi(w[3]), x1 = atoi(w[4]), y1 = atoi(w[5]);
    if (t == nullptr || x0 >= x1 || y0 >= y1)
        return;
    Surface s;
    SurfaceFor(t, &s, x0, y0, x1, y1);
    Rect area;
    area.left = x0;
    area.top = y0;
    area.right = x1 - 1;
    area.bottom = y1 - 1;
    if (strncmp(kind, "sprite", 6) == 0 || strcmp(kind, "lens") == 0) {
        GafFrame* f = n >= 9 ? Picture(w[6]) : nullptr;
        if (f == nullptr) {
            Skip(kind);
            return;
        }
        int x = atoi(w[7]), y = atoi(w[8]);
        if (strcmp(kind, "sprite") == 0)
            DrawFrame(&s, f, x, y);
        else if (strcmp(kind, "sprite_opaque") == 0)
            DrawFrameOpaque(&s, f, x, y);
        else if (strcmp(kind, "sprite_blend") == 0)
            DrawFrameBlended(&s, f, x, y);
        else if (strcmp(kind, "sprite_gray") == 0)
            DrawFrameGray(&s, f, x, y);
        else if (strcmp(kind, "sprite_shadow") == 0)
            DrawFrameShadow(&s, f, x, y);
        else if (strcmp(kind, "sprite_lit") == 0 && n >= 10)
            DrawFrameLit(&s, f, x, y, atoi(w[9]));
        else if (strcmp(kind, "sprite_erase") == 0 && n >= 10)
            EraseFrameDithered(&s, f, x, y, atoi(w[9]));
        else if (strcmp(kind, "lens") == 0)
            DrawLens(&s, f, x, y);
        else
            Skip(kind);
    } else if (strcmp(kind, "quad") == 0 && n >= 12) {
        GafFrame* f = Picture(w[6]);
        if (f == nullptr) {
            Skip(kind);
            return;
        }
        FrameQuad corners;
        for (int i = 0; i < 4; i++)
            Corner(w[7 + i], &corners.p[i]);
        FrameQuad source;
        int whole = strcmp(w[11], "-") == 0;
        if (!whole)
            Corners(w[11], &source);
        DrawFrameQuad(&s, f, &corners, whole ? nullptr : &source);
    } else if (strcmp(kind, "blit") == 0 && n >= 9) {
        Target* from = FindTarget(w[6]);
        if (from == nullptr) {
            Skip(kind);
            return;
        }
        Surface src;
        SurfaceFor(from, &src, 0, 0, from->width, from->height);
        Surface inside = s;
        inside.pixels = s.pixels + (size_t)y0 * s.pitch + x0;
        inside.width = x1 - x0;
        inside.height = y1 - y0;
        DrawSurface(&inside, &src, atoi(w[7]) - x0, atoi(w[8]) - y0);
    } else if (strcmp(kind, "copy") == 0 && n >= 13) {
        Target* from = FindTarget(w[6]);
        if (from == nullptr) {
            Skip(kind);
            return;
        }
        Surface src;
        SurfaceFor(from, &src, 0, 0, from->width, from->height);
        Rect r;
        r.left = atoi(w[7]);
        r.top = atoi(w[8]);
        r.right = atoi(w[9]);
        r.bottom = atoi(w[10]);
        Point pos;
        pos.x = atoi(w[11]);
        pos.y = atoi(w[12]);
        CopySurfaceRect(&s, &src, &r, &pos);
    } else if (strcmp(kind, "pixels") == 0 && n >= 7) {
        GafFrame* f = Picture(w[6]);
        if (f->width != x1 - x0 || f->height != y1 - y0)
            ReplayFail("a pixels item whose picture does not fit its area: ", w[6]);
        for (int y = 0; y < f->height; y++)
            memcpy(t->pixels + (size_t)(y0 + y) * t->width + x0, f->pixelsOrLayers + (size_t)y * f->width, f->width);
    } else if (strcmp(kind, "clear") == 0 && n >= 7) {
        FillSurface(&s, atoi(w[6]));
    } else if (strcmp(kind, "fill") == 0 && n >= 7) {
        FillRectangle(&s, &area, atoi(w[6]));
    } else if (strcmp(kind, "fade") == 0 && n >= 7) {
        FadeRectangle(&s, &area, atoi(w[6]));
    } else if (strcmp(kind, "gray") == 0) {
        GrayRectangle(&s, &area);
    } else if (strcmp(kind, "dither") == 0 && n >= 7) {
        DitherRectangle(&s, &area, atoi(w[6]));
    } else if (strcmp(kind, "outline") == 0 && n >= 11) {
        Rect r;
        r.left = atoi(w[6]);
        r.top = atoi(w[7]);
        r.right = atoi(w[8]);
        r.bottom = atoi(w[9]);
        DrawRectangle(&s, &r, atoi(w[10]));
    } else if (strcmp(kind, "outline_lit") == 0 && n >= 11) {
        int r[4] = { atoi(w[6]), atoi(w[7]), atoi(w[8]), atoi(w[9]) };
        DrawLitRectangle(&s, r, atoi(w[10]));
    } else if (strcmp(kind, "line") == 0 && n >= 11) {
        DrawLine(&s, atoi(w[6]), atoi(w[7]), atoi(w[8]), atoi(w[9]), atoi(w[10]));
    } else if (strcmp(kind, "pixel") == 0 && n >= 9) {
        DrawPixel(&s, atoi(w[6]), atoi(w[7]), atoi(w[8]));
    } else if (strcmp(kind, "circle") == 0 && n >= 10) {
        DrawCircle(&s, atoi(w[6]), atoi(w[7]), atoi(w[8]), atoi(w[9]));
    } else if (strcmp(kind, "circle_dashed") == 0 && n >= 12) {
        DrawDashedCircle(&s, atoi(w[6]), atoi(w[7]), atoi(w[8]), atoi(w[9]), atoi(w[10]), atoi(w[11]));
    } else if (strcmp(kind, "polygon") == 0 && n >= 7) {
        Point points[256];
        int count = 0;
        for (int i = 7; i < n && count < 256; i++)
            Corner(w[i], &points[count++]);
        FillPolygon(&s, points, count, (unsigned char)atoi(w[6]));
    } else if (strcmp(kind, "text") == 0 && n >= 13) {
        Display* d = GetDisplay();
        Font* oldFont = d->font;
        int fore = d->foreColor, back = d->backColor, key = d->keyColor;
        d->font = FontNamed(w[6]);
        d->foreColor = atoi(w[7]);
        d->backColor = atoi(w[8]);
        d->keyColor = atoi(w[9]);
        char text[400];
        Unescape(w[12], text, sizeof text);
        s.clip.right = x1;
        s.clip.bottom = y1;
        DrawString(&s, (unsigned char*)text, atoi(w[10]), atoi(w[11]), -1);
        d->font = oldFont;
        d->foreColor = fore;
        d->backColor = back;
        d->keyColor = key;
    } else if (strcmp(kind, "model") == 0 || strcmp(kind, "cargo") == 0) {
        Skip(kind);
    } else if (strcmp(kind, "projectile") == 0 && n >= 13) {
        if (w[10][0] == '-') {
            Skip("projectile with no turn");
            return;
        }
        Vec3 offset;
        offset.x = atoi(w[7]);
        offset.y = atoi(w[8]);
        offset.z = atoi(w[9]);
        short angles[3] = {(short)atoi(w[10]), (short)atoi(w[11]), (short)atoi(w[12])};
        DrawModel3doProjected(&s, &offset, (ProjectedModel*)PieceNamed(w[6]), angles);
    } else if (strcmp(kind, "debris") == 0 && n >= 14) {
        DrawDebris(&s, (Object3D*)PieceNamed(w[6]), atoi(w[7]), atoi(w[8]), atoi(w[9]), atoi(w[10]),
                   (short)atoi(w[11]), (short)atoi(w[12]), (short)atoi(w[13]));
    } else if (strcmp(kind, "shard") == 0 && n >= 21) {
        DrawShard(&s, w + 6);
    } else if (strcmp(kind, "terrain") == 0 && n >= 9) {
        char map[200];
        Unescape(w[6], map, sizeof map);
        if (strcmp(map, HarnessMapName()) != 0)
            ReplayFail("the list draws another map: ", map);
        int left = g_game->rect.left, top = g_game->rect.top;
        int vw = g_game->viewWidth, vh = g_game->viewHeight;
        int sx = g_game->scrollX, sy = g_game->scrollY;
        g_game->rect.left = x0;
        g_game->rect.top = y0;
        g_game->viewWidth = x1 - x0;
        g_game->viewHeight = y1 - y0;
        g_game->scrollX = atoi(w[7]);
        g_game->scrollY = atoi(w[8]);
        DrawMapTiles(&s);
        g_game->rect.left = left;
        g_game->rect.top = top;
        g_game->viewWidth = vw;
        g_game->viewHeight = vh;
        g_game->scrollX = sx;
        g_game->scrollY = sy;
    } else {
        Skip(kind);
    }
}

static void LoadPalette(const char* hash)
{
    if (strcmp(hash, s_paletteHash) == 0)
        return;
    unsigned char* pal = (unsigned char*)HAPI_LoadFile("palettes\\PALETTE.PAL", nullptr);
    if (pal == nullptr)
        ReplayFail("cannot load palettes\\PALETTE.PAL", nullptr);
    for (int i = 0; i < 256; i++) {
        s_palette[i * 4] = pal[i * 4];
        s_palette[i * 4 + 1] = pal[i * 4 + 1];
        s_palette[i * 4 + 2] = pal[i * 4 + 2];
        s_palette[i * 4 + 3] = 0;
    }
    unsigned long long h = HashBytes(s_palette, 1024);
    char text[20];
    sprintf(text, "%08x%08x", (unsigned)(h >> 32), (unsigned)h);
    if (strcmp(text, hash) != 0)
        ReplayFail("no palette file has the hash ", hash);
    strcpy(s_paletteHash, hash);
}

static void WriteFrame(unsigned tick)
{
    Target* screen = FindTarget("screen");
    char directory[] = "frames";
    char path[40];
    MakeDirectoryPath(directory);
    sprintf(path, "frames\\%u.bin", tick);
    FILE* f = fopen(path, "wb");
    if (f == nullptr)
        ReplayFail("cannot write ", path);
    unsigned char header[16] = { 'B', 'T', 'F', 'R', 1, 0, 128, 2, 224, 1, 0, 0, (unsigned char)tick, (unsigned char)(tick >> 8),
        (unsigned char)(tick >> 16), (unsigned char)(tick >> 24) };
    fwrite(header, sizeof header, 1, f);
    fwrite(s_palette, sizeof s_palette, 1, f);
    for (int y = 0; y < 480; y++)
        fwrite(screen->pixels + (size_t)y * screen->width, 1, 640, f);
    fclose(f);
}

static char* ReadAll(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f == nullptr)
        ReplayFail("cannot read ", path);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0)
        ReplayFail("cannot read ", path);
    fseek(f, 0, SEEK_SET);
    char* text = (char*)malloc((size_t)size + 1);
    if (text == nullptr || fread(text, 1, (size_t)size, f) != (size_t)size)
        ReplayFail("cannot read ", path);
    text[size] = '\0';
    fclose(f);
    return text;
}

static void RestartSequences()
{
    for (int i = 0; i < g_game->count_148e7; i++) {
        Handle* v = (Handle*)((void**)g_game->data_148eb)[i];
        if (v->src == nullptr)
            ReplayFail("a picture sequence that has already ended", nullptr);
        InitGafSequence(v, v->src, 0);
    }
}

void DisplayReplayRun(const char* path, const unsigned* frameTicks, unsigned frameCount)
{
    DlEnter();
    char* text = ReadAll(path);
    Measure(text);
    RestartSequences();
    unsigned written = 0;
    unsigned tick = 0;
    unsigned frames = 0;
    char* words[kMaxWords];
    for (char* line = text; line != nullptr && *line != '\0';) {
        char* next = strchr(line, '\n');
        if (next != nullptr)
            *next = '\0';
        int queued = 0;
        if (strncmp(line, "model ", 6) == 0) {
            DrawPendingModel();
            queued = 1;
        } else if (strncmp(line, "cargo ", 6) == 0) {
            queued = s_pendingCount > 0 && s_pendingCount <= kMaxCargo;
        } else {
            DrawPendingModel();
        }
        if (queued) {
            if (strlen(line) >= sizeof s_pending[0])
                ReplayFail("a model item too long", nullptr);
            strcpy(s_pending[s_pendingCount++], line);
            line = next != nullptr ? next + 1 : nullptr;
            continue;
        }
        int n = Split(line, words);
        if (n == 0) {
        } else if (strcmp(words[0], "bt-display") == 0) {
        } else if (strcmp(words[0], "picture") == 0) {
            DefinePicture(words, n, line);
        } else if (strcmp(words[0], "frame") == 0 && n >= 5) {
            tick = (unsigned)atoi(words[2]);
            s_tick = tick;
            for (int step = atoi(words[3]); step > 0; step--)
                StepAllGafSequences();
            LoadPalette(words[4]);
            frames++;
        } else if (strcmp(words[0], "cursor") == 0) {
            for (unsigned i = 0; i < frameCount; i++) {
                if (frameTicks[i] == tick && (written & (1u << i)) == 0) {
                    WriteFrame(tick);
                    written |= 1u << i;
                }
            }
        } else if (n >= 6) {
            Draw(words, n);
        }
        line = next != nullptr ? next + 1 : nullptr;
    }
    char msg[200];
    sprintf(msg, "testreplay: %u frames replayed\n", frames);
    host_log(msg);
    for (int i = 0; i < s_skippedCount; i++) {
        sprintf(msg, "testreplay: %d %s items not drawn\n", s_skipped[i].count, s_skipped[i].kind);
        host_log(msg);
    }
    free(text);
    host_exit(0);
}
