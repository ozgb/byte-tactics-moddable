#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display_log.h"

static int g_dlOn;
static int g_dlDepth;
static int g_dlOffX;
static int g_dlOffY;

static DWORD g_dlOwner;
static CRITICAL_SECTION g_dlSection;
static int g_dlLock;

struct DlLock {
    DlLock()
    {
        if (g_dlLock != 0)
            EnterCriticalSection(&g_dlSection);
    }
    ~DlLock()
    {
        if (g_dlLock != 0)
            LeaveCriticalSection(&g_dlSection);
    }
};

void DlLockRegistries()
{
    if (g_dlLock != 0)
        EnterCriticalSection(&g_dlSection);
}

void DlUnlockRegistries()
{
    if (g_dlLock != 0)
        LeaveCriticalSection(&g_dlSection);
}

static int OnOwner()
{
    return GetCurrentThreadId() == g_dlOwner;
}

int DlWants()
{
    return g_dlOn != 0 && g_dlDepth == 0 && OnOwner();
}

int DlIsOn()
{
    return g_dlOn;
}

void DlEnter()
{
    if (g_dlOn != 0 && OnOwner())
        g_dlDepth++;
}

void DlLeave()
{
    if (g_dlOn != 0 && OnOwner())
        g_dlDepth--;
}

int DlOffsetX()
{
    return g_dlOffX;
}

int DlOffsetY()
{
    return g_dlOffY;
}

static const int kMaxItems = 1 << 15;
static const int kTextBytes = 4 << 20;
static const int kMaxRegions = 2048;
static const int kRegionHashSize = 8192;
static const int kMaxGafRecords = 1 << 15;
static const int kGafHashSize = 1 << 16;
static const int kMaxGafBlocks = 1024;
static const int kMaxPieces = 16384;
static const int kPieceHashSize = 1 << 15;
static const int kMaxFonts = 64;
static const int kNameBytes = 1 << 18;
static const int kNameHashSize = 1 << 14;
static const int kMaxTargets = 256;
static const int kMaxBuckets = 1 << 16;
static const int kMaxNodes = 1 << 16;
static const int kBucketShift = 5;
static const int kBucketSize = 1 << kBucketShift;
static const int kLineGuard = 4096;

static const unsigned __int64 kHashOffset = 0xcbf29ce484222325ui64;
static const unsigned __int64 kHashPrime = 0x100000001b3ui64;

struct DlHash {
    unsigned __int64 h;
    unsigned char buf[8];
    unsigned n;
};

static void HashStart(DlHash* s)
{
    s->h = kHashOffset;
    s->n = 0;
}

static void HashFeed(DlHash* s, const unsigned char* p, unsigned n)
{
    while (n > 0) {
        if (s->n == 0 && n >= 8) {
            unsigned __int64 word;
            memcpy(&word, p, 8);
            s->h = (s->h ^ word) * kHashPrime;
            p += 8;
            n -= 8;
            continue;
        }
        s->buf[s->n++] = *p++;
        n--;
        if (s->n == 8) {
            unsigned __int64 word;
            memcpy(&word, s->buf, 8);
            s->h = (s->h ^ word) * kHashPrime;
            s->n = 0;
        }
    }
}

static unsigned __int64 HashEnd(DlHash* s)
{
    unsigned __int64 h = s->h;
    for (unsigned i = 0; i < s->n; i++)
        h = (h ^ s->buf[i]) * kHashPrime;
    return h;
}

static unsigned __int64 HashRun(const unsigned char* p, unsigned n)
{
    DlHash s;
    HashStart(&s);
    HashFeed(&s, p, n);
    return HashEnd(&s);
}

static void HexHash(char* out, unsigned __int64 h)
{
    sprintf(out, "%08x%08x", (unsigned)(h >> 32), (unsigned)h);
}

static void Die(const char* text)
{
    DWORD written;
    OutputDebugStringA(text);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, (DWORD)strlen(text), &written, 0);
    ExitProcess(2);
}

static void Fail(const char* what, const char* limit)
{
    char msg[200];
    sprintf(msg, "testmode: the display log is out of %s (limit %s)\n", what, limit);
    Die(msg);
}

static char s_nameBytes[kNameBytes];
static int s_nameUsed = 1;
static int s_nameHash[kNameHashSize];
static int s_nameNext[kNameHashSize];
static int s_nameCount = 0;
static int s_nameOffsets[kNameHashSize];

static int Escape(char* out, const char* p, int n)
{
    static const char digits[] = "0123456789ABCDEF";
    char* start = out;
    if (n == 0) {
        *out++ = '-';
        return 1;
    }
    if (n == 1 && p[0] == '-') {
        *out++ = '%';
        *out++ = '2';
        *out++ = 'D';
        return 3;
    }
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)p[i];
        if (c < 0x21 || c > 0x7e || c == '%' || c == ',') {
            *out++ = '%';
            *out++ = digits[c >> 4];
            *out++ = digits[c & 15];
        } else {
            *out++ = (char)c;
        }
    }
    return (int)(out - start);
}

static int Intern(const char* p, int n)
{
    char buf[1024];
    int written = Escape(buf, p, n > 300 ? 300 : n);
    buf[written] = '\0';
    unsigned h = 2166136261u;
    for (int i = 0; i < written; i++)
        h = (h ^ (unsigned char)buf[i]) * 16777619u;
    unsigned slot = h & (kNameHashSize - 1);
    int index = s_nameHash[slot] - 1;
    while (index >= 0) {
        if (strcmp(s_nameBytes + s_nameOffsets[index], buf) == 0)
            return s_nameOffsets[index];
        index = s_nameNext[index] - 1;
    }
    if (s_nameCount >= kNameHashSize)
        Fail("names", "16384");
    if (s_nameUsed + written + 1 > kNameBytes)
        Fail("name bytes", "256 KiB");
    int offset = s_nameUsed;
    memcpy(s_nameBytes + offset, buf, written + 1);
    s_nameUsed += written + 1;
    s_nameOffsets[s_nameCount] = offset;
    s_nameNext[s_nameCount] = s_nameHash[slot];
    s_nameHash[slot] = s_nameCount + 1;
    s_nameCount++;
    return offset;
}

static int InternLower(const char* p, int n, int slashes)
{
    char tmp[300];
    if (n > (int)sizeof tmp)
        n = (int)sizeof tmp;
    for (int i = 0; i < n; i++) {
        char c = p[i];
        if (c >= 'A' && c <= 'Z')
            c = (char)(c + 32);
        if (slashes && c == '\\')
            c = '/';
        tmp[i] = c;
    }
    return Intern(tmp, n);
}

struct Region {
    const void* block;
    const unsigned char* pixels;
    int width;
    int height;
    int pitch;
    int surface;
    int name;
    int live;
};

static Region s_regions[kMaxRegions];
static int s_regionCount = 0;
static int s_regionFree[kMaxRegions];
static int s_regionFreeCount = 0;
static int s_pixelHash[kRegionHashSize];
static int s_blockHash[kRegionHashSize];
static unsigned s_epoch = 1;

struct LookupCache {
    const unsigned char* pixels;
    int region;
    unsigned epoch;
};
static LookupCache s_cache[256];

static unsigned PtrHash(const void* p)
{
    unsigned __int64 v = (unsigned __int64)(unsigned long)p;
    v = (v ^ (v >> 15)) * 0x2545f4914f6cdd1dui64;
    return (unsigned)(v >> 20);
}

static int PixelKey(int index, const void** key)
{
    *key = s_regions[index].pixels;
    return s_regions[index].live;
}

static int BlockKey(int index, const void** key)
{
    *key = s_regions[index].block;
    return s_regions[index].live;
}

static void HashInsert(int* table, const void* key, int value)
{
    unsigned slot = PtrHash(key) & (kRegionHashSize - 1);
    while (table[slot] != 0)
        slot = (slot + 1) & (kRegionHashSize - 1);
    table[slot] = value + 1;
}

static void HashRemove(int* table, int (*keyOf)(int, const void**), int value)
{
    const void* key;
    keyOf(value, &key);
    unsigned mask = kRegionHashSize - 1;
    unsigned i = PtrHash(key) & mask;
    while (table[i] != value + 1)
        i = (i + 1) & mask;
    unsigned j = i;
    for (;;) {
        j = (j + 1) & mask;
        if (table[j] == 0)
            break;
        const void* other;
        keyOf(table[j] - 1, &other);
        unsigned k = PtrHash(other) & mask;
        int move = j > i ? (k <= i || k > j) : (k <= i && k > j);
        if (move) {
            table[i] = table[j];
            i = j;
        }
    }
    table[i] = 0;
}

static int HashFind(const int* table, const void* key, int (*keyOf)(int, const void**))
{
    unsigned slot = PtrHash(key) & (kRegionHashSize - 1);
    for (;;) {
        int e = table[slot];
        const void* existing;
        if (e == 0)
            return -1;
        keyOf(e - 1, &existing);
        if (existing == key)
            return e - 1;
        slot = (slot + 1) & (kRegionHashSize - 1);
    }
}

static int FindByPixels(const unsigned char* pixels)
{
    return HashFind(s_pixelHash, pixels, PixelKey);
}

static int FindByBlock(const void* block)
{
    return HashFind(s_blockHash, block, BlockKey);
}

static void RemoveRegion(int index)
{
    HashRemove(s_pixelHash, PixelKey, index);
    HashRemove(s_blockHash, BlockKey, index);
    s_regions[index].live = 0;
    s_regionFree[s_regionFreeCount++] = index;
    s_epoch++;
}

static void AddRegion(const void* block, const unsigned char* pixels, int width, int height, int pitch, int surface, const char* name)
{
    int old = FindByBlock(block);
    if (old >= 0)
        RemoveRegion(old);
    old = FindByPixels(pixels);
    if (old >= 0)
        RemoveRegion(old);
    int index;
    if (s_regionFreeCount > 0) {
        index = s_regionFree[--s_regionFreeCount];
    } else {
        if (s_regionCount >= kMaxRegions)
            Fail("live allocations", "2048");
        index = s_regionCount++;
    }
    Region* r = &s_regions[index];
    r->block = block;
    r->pixels = pixels;
    r->width = width;
    r->height = height;
    r->pitch = pitch;
    r->surface = surface;
    r->live = 1;
    if (surface != 0 && name != 0 && strcmp(name, "OFFSCREEN") == 0)
        name = "screen";
    r->name = Intern(name == 0 ? "" : name, name == 0 ? 0 : (int)strlen(name));
    HashInsert(s_pixelHash, pixels, index);
    HashInsert(s_blockHash, block, index);
    s_epoch++;
}

struct GafRecord {
    const void* key;
    int file;
    int entry;
    int index;
    int dead;
};

static unsigned s_gafEpoch = 1;
static GafRecord s_gaf[kMaxGafRecords];
static int s_gafCount = 0;
static int s_gafHash[kGafHashSize];

struct GafBlock {
    const void* base;
    int first;
    int count;
    int live;
};

static GafBlock s_gafBlocks[kMaxGafBlocks];
static int s_gafBlockCount = 0;
static int s_gafFile = 0;

static int GafFind(const void* key)
{
    unsigned slot = PtrHash(key) & (kGafHashSize - 1);
    for (;;) {
        int e = s_gafHash[slot];
        if (e == 0)
            return -1;
        if (s_gaf[e - 1].key == key)
            return s_gaf[e - 1].dead ? -1 : e - 1;
        slot = (slot + 1) & (kGafHashSize - 1);
    }
}

static void GafInsert(int index)
{
    unsigned slot = PtrHash(s_gaf[index].key) & (kGafHashSize - 1);
    for (;;) {
        int e = s_gafHash[slot];
        if (e == 0 || s_gaf[e - 1].key == s_gaf[index].key) {
            s_gafHash[slot] = index + 1;
            return;
        }
        slot = (slot + 1) & (kGafHashSize - 1);
    }
}

void DlGafBegin(const char* path, const void* base)
{
    DlLock lock;
    int i;
    s_gafFile = InternLower(path, (int)strlen(path), 1);
    for (i = 0; i < s_gafBlockCount; i++) {
        if (s_gafBlocks[i].base == base && s_gafBlocks[i].live) {
            s_gafBlocks[i].live = 0;
            for (int k = 0; k < s_gafBlocks[i].count; k++)
                s_gaf[s_gafBlocks[i].first + k].dead = 1;
            s_gafEpoch++;
        }
    }
    int slot = -1;
    for (i = 0; i < s_gafBlockCount; i++) {
        if (!s_gafBlocks[i].live) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        if (s_gafBlockCount >= kMaxGafBlocks)
            Fail("archives", "1024");
        slot = s_gafBlockCount++;
    }
    s_gafBlocks[slot].base = base;
    s_gafBlocks[slot].first = s_gafCount;
    s_gafBlocks[slot].count = 0;
    s_gafBlocks[slot].live = 1;
}

void DlGafFrame(const char* entry, int index, const void* frame)
{
    DlLock lock;
    if (s_gafCount >= kMaxGafRecords)
        Fail("archive frames", "32768");
    GafRecord* r = &s_gaf[s_gafCount];
    r->key = frame;
    r->file = s_gafFile;
    r->entry = Intern(entry, (int)strlen(entry));
    r->index = index;
    r->dead = 0;
    GafInsert(s_gafCount);
    s_gafCount++;
    s_gafEpoch++;
    for (int i = s_gafBlockCount - 1; i >= 0; i--) {
        if (s_gafBlocks[i].live && s_gafBlocks[i].first + s_gafBlocks[i].count == s_gafCount - 1) {
            s_gafBlocks[i].count++;
            break;
        }
    }
}

void DlRegisterSurface(const void* block, const unsigned char* pixels, int width, int height, int pitch, const char* name)
{
    DlLock lock;
    AddRegion(block, pixels, width, height, pitch, 1, name);
}

void DlRegisterFrame(const void* block, const unsigned char* pixels, int width, int height, const char* name)
{
    DlLock lock;
    int g = GafFind(block);
    if (g >= 0) {
        s_gaf[g].dead = 1;
        s_gafEpoch++;
    }
    AddRegion(block, pixels, width, height, width, 0, name);
}

struct Piece {
    const void* object;
    int model;
    int name;
    int live;
};

static Piece s_pieces[kMaxPieces];
static int s_pieceCount = 0;
static int s_pieceHash[kPieceHashSize];

void DlRegisterModelPiece(const void* object, const char* stem, const char* piece)
{
    DlLock lock;
    unsigned slot = PtrHash(object) & (kPieceHashSize - 1);
    int found = -1;
    for (;;) {
        int e = s_pieceHash[slot];
        if (e == 0)
            break;
        if (s_pieces[e - 1].object == object) {
            found = e - 1;
            break;
        }
        slot = (slot + 1) & (kPieceHashSize - 1);
    }
    if (found < 0) {
        if (s_pieceCount >= kMaxPieces)
            Fail("model pieces", "16384");
        found = s_pieceCount++;
        s_pieceHash[slot] = found + 1;
    }
    s_pieces[found].object = object;
    s_pieces[found].model = InternLower(stem, (int)strlen(stem), 0);
    s_pieces[found].name = Intern(piece, (int)strlen(piece));
    s_pieces[found].live = 1;
}

static const Piece* FindPiece(const void* object)
{
    unsigned slot = PtrHash(object) & (kPieceHashSize - 1);
    for (;;) {
        int e = s_pieceHash[slot];
        if (e == 0)
            return 0;
        if (s_pieces[e - 1].object == object)
            return &s_pieces[e - 1];
        slot = (slot + 1) & (kPieceHashSize - 1);
    }
}

const char* DlModelName(const void* object, int* known)
{
    DlLock lock;
    const Piece* p = FindPiece(object);
    *known = p != 0;
    return p != 0 ? s_nameBytes + p->model : "-";
}

const char* DlPieceName(const void* object)
{
    DlLock lock;
    const Piece* p = FindPiece(object);
    return p != 0 ? s_nameBytes + p->name : "-";
}

struct FontRecord {
    const void* font;
    int name;
};

static FontRecord s_fonts[kMaxFonts];
static int s_fontCount = 0;

void DlRegisterFont(const void* font, const char* name)
{
    DlLock lock;
    for (int i = 0; i < s_fontCount; i++) {
        if (s_fonts[i].font == font) {
            s_fonts[i].name = InternLower(name, (int)strlen(name), 0);
            return;
        }
    }
    if (s_fontCount >= kMaxFonts)
        Fail("fonts", "64");
    s_fonts[s_fontCount].font = font;
    s_fonts[s_fontCount].name = InternLower(name, (int)strlen(name), 0);
    s_fontCount++;
}

void DlFontName(const void* font, char* out, int cap)
{
    DlLock lock;
    const char* name = "?";
    for (int i = 0; i < s_fontCount; i++) {
        if (s_fonts[i].font == font)
            name = s_nameBytes + s_fonts[i].name;
    }
    strncpy(out, name, cap - 1);
    out[cap - 1] = '\0';
}

void DlFree(const void* block)
{
    DlLock lock;
    if (g_dlOn == 0)
        return;
    int r = FindByBlock(block);
    if (r >= 0)
        RemoveRegion(r);
    for (int i = 0; i < s_gafBlockCount; i++) {
        if (s_gafBlocks[i].live && s_gafBlocks[i].base == block) {
            s_gafBlocks[i].live = 0;
            for (int k = 0; k < s_gafBlocks[i].count; k++)
                s_gaf[s_gafBlocks[i].first + k].dead = 1;
            s_gafEpoch++;
        }
    }
}

static int FindContaining(const unsigned char* pixels)
{
    LookupCache* c = &s_cache[(PtrHash(pixels) >> 3) & 255];
    if (c->epoch == s_epoch && c->pixels == pixels)
        return c->region;
    int result = FindByPixels(pixels);
    if (result < 0) {
        for (int i = 0; i < s_regionCount; i++) {
            const Region* r = &s_regions[i];
            if (r->live && pixels > r->pixels && pixels < r->pixels + r->pitch * r->height) {
                result = i;
                break;
            }
        }
    }
    c->pixels = pixels;
    c->region = result;
    c->epoch = s_epoch;
    return result;
}

struct Resolved {
    const unsigned char* base;
    int offX;
    int offY;
    int width;
    int height;
    int name;
    int anonymous;
};

static int AnonymousName(int width, int height)
{
    char text[40];
    sprintf(text, "?%dx%d", width, height);
    return Intern(text, (int)strlen(text));
}

static void Resolve(const DlSurface* s, Resolved* out)
{
    int r = s->pixels != 0 ? FindContaining(s->pixels) : -1;
    if (r >= 0) {
        const Region* region = &s_regions[r];
        long off = (long)(s->pixels - region->pixels);
        out->base = region->pixels;
        out->offX = region->pitch > 0 ? (int)(off % region->pitch) : 0;
        out->offY = region->pitch > 0 ? (int)(off / region->pitch) : 0;
        out->width = region->width;
        out->height = region->height;
        out->name = region->name;
        out->anonymous = 0;
        return;
    }
    out->base = s->pixels;
    out->offX = 0;
    out->offY = 0;
    out->width = s->width;
    out->height = s->height;
    out->name = AnonymousName(s->width, s->height);
    out->anonymous = 1;
}

const unsigned char* DlSourceBase(const DlSurface* surface)
{
    DlLock lock;
    Resolved r;
    Resolve(surface, &r);
    return r.base;
}

void DlSourceOffset(const DlSurface* surface, int* x, int* y)
{
    DlLock lock;
    Resolved r;
    Resolve(surface, &r);
    *x = r.offX;
    *y = r.offY;
}

struct Bucket {
    int full;
    int head;
};

struct Node {
    int x0;
    int y0;
    int x1;
    int y1;
    int layer;
    int next;
};

struct TargetSlot {
    const unsigned char* base;
    int width;
    int height;
    int name;
    int bw;
    int bh;
    int first;
    int overflow;
    int maxDraw;
    int maxRead;
};

static TargetSlot s_targets[kMaxTargets];
static int s_targetCount = 0;
static Bucket s_buckets[kMaxBuckets];
static int s_bucketUsed = 0;
static Node s_nodes[kMaxNodes];
static int s_nodeUsed = 0;

struct Item {
    int start;
    int lineOff;
    int end;
};

static Item s_items[kMaxItems];
static int s_itemCount = 0;
static char s_text[kTextBytes];
static int s_textUsed = 0;
static int s_open = 0;
static int s_curStart = 0;
static int s_dropItem = 0;

static TargetSlot* SlotFor(const unsigned char* base, int width, int height, int name)
{
    int i;
    for (i = 0; i < s_targetCount; i++) {
        if (s_targets[i].base == base)
            return &s_targets[i];
    }
    if (s_targetCount >= kMaxTargets)
        Fail("targets in a frame", "256");
    TargetSlot* t = &s_targets[s_targetCount++];
    t->base = base;
    t->width = width;
    t->height = height;
    t->name = name;
    t->bw = (width + kBucketSize - 1) >> kBucketShift;
    t->bh = (height + kBucketSize - 1) >> kBucketShift;
    if (t->bw < 0 || t->bh < 0 || (long)t->bw * t->bh > kMaxBuckets - s_bucketUsed)
        Fail("grid buckets", "65536");
    t->first = s_bucketUsed;
    s_bucketUsed += t->bw * t->bh;
    for (i = 0; i < t->bw * t->bh; i++) {
        s_buckets[t->first + i].full = -1;
        s_buckets[t->first + i].head = -1;
    }
    t->overflow = -1;
    t->maxDraw = -1;
    t->maxRead = -1;
    return t;
}

static int NewNode(int x0, int y0, int x1, int y1, int layer, int next)
{
    if (s_nodeUsed >= kMaxNodes)
        Fail("grid nodes", "65536");
    Node* n = &s_nodes[s_nodeUsed];
    n->x0 = x0;
    n->y0 = y0;
    n->x1 = x1;
    n->y1 = y1;
    n->layer = layer;
    n->next = next;
    return s_nodeUsed++;
}

static int Overlaps(const Node* n, int x0, int y0, int x1, int y1)
{
    return n->x0 < x1 && x0 < n->x1 && n->y0 < y1 && y0 < n->y1;
}

static int Query(const TargetSlot* t, int x0, int y0, int x1, int y1)
{
    int best = -1;
    for (int n = t->overflow; n >= 0; n = s_nodes[n].next) {
        if (s_nodes[n].layer > best && Overlaps(&s_nodes[n], x0, y0, x1, y1))
            best = s_nodes[n].layer;
    }
    int cx0 = x0 < 0 ? 0 : x0;
    int cy0 = y0 < 0 ? 0 : y0;
    int cx1 = x1 > t->width ? t->width : x1;
    int cy1 = y1 > t->height ? t->height : y1;
    if (cx0 >= cx1 || cy0 >= cy1)
        return best;
    for (int by = cy0 >> kBucketShift; by <= (cy1 - 1) >> kBucketShift; by++) {
        for (int bx = cx0 >> kBucketShift; bx <= (cx1 - 1) >> kBucketShift; bx++) {
            const Bucket* b = &s_buckets[t->first + by * t->bw + bx];
            if (b->full > best)
                best = b->full;
            for (int n = b->head; n >= 0; n = s_nodes[n].next) {
                if (s_nodes[n].layer > best && Overlaps(&s_nodes[n], x0, y0, x1, y1))
                    best = s_nodes[n].layer;
            }
        }
    }
    return best;
}

static void Add(TargetSlot* t, int x0, int y0, int x1, int y1, int layer)
{
    if (layer > t->maxDraw)
        t->maxDraw = layer;
    if (x0 < 0 || y0 < 0 || x1 > t->width || y1 > t->height) {
        t->overflow = NewNode(x0, y0, x1, y1, layer, t->overflow);
        return;
    }
    for (int by = y0 >> kBucketShift; by <= (y1 - 1) >> kBucketShift; by++) {
        for (int bx = x0 >> kBucketShift; bx <= (x1 - 1) >> kBucketShift; bx++) {
            Bucket* b = &s_buckets[t->first + by * t->bw + bx];
            int bx0 = bx << kBucketShift;
            int by0 = by << kBucketShift;
            if (x0 <= bx0 && y0 <= by0 && x1 >= bx0 + kBucketSize && y1 >= by0 + kBucketSize) {
                if (layer > b->full)
                    b->full = layer;
            } else {
                b->head = NewNode(x0, y0, x1, y1, layer, b->head);
            }
        }
    }
}

void DlCutRect(const DlSurface* dst, int* x0, int* y0, int* x1, int* y1)
{
    if (*x0 < dst->clipLeft)
        *x0 = dst->clipLeft;
    if (*y0 < dst->clipTop)
        *y0 = dst->clipTop;
    if (*x1 > dst->clipRight + 1)
        *x1 = dst->clipRight + 1;
    if (*y1 > dst->clipBottom + 1)
        *y1 = dst->clipBottom + 1;
    if (*x0 >= *x1 || *y0 >= *y1) {
        *x0 = 0;
        *y0 = 0;
        *x1 = 0;
        *y1 = 0;
    }
}

static void Reserve(int bytes)
{
    if (s_textUsed + bytes + kLineGuard > kTextBytes)
        Fail("text bytes in a frame", "4194304");
}

void DlPutRaw(const char* text)
{
    int n = (int)strlen(text);
    Reserve(n + 1);
    strcpy(s_text + s_textUsed, text);
    s_textUsed += n;
}

void DlPutInt(int value)
{
    Reserve(16);
    char* out = s_text + s_textUsed;
    unsigned v = value < 0 ? 0u - (unsigned)value : (unsigned)value;
    char digits[12];
    int n = 0;
    do {
        digits[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v != 0);
    if (value < 0)
        *out++ = '-';
    while (n > 0)
        *out++ = digits[--n];
    s_textUsed = (int)(out - s_text);
}

void DlPutHex8(unsigned value)
{
    static const char hex[] = "0123456789abcdef";
    Reserve(16);
    char* out = s_text + s_textUsed;
    for (int i = 7; i >= 0; i--)
        *out++ = hex[(value >> (i * 4)) & 15];
    s_textUsed = (int)(out - s_text);
}

void DlPutName(const char* bytes, int length)
{
    Reserve(length * 3 + 4);
    s_textUsed += Escape(s_text + s_textUsed, bytes, length);
}

void DlPutTarget(const DlSurface* surface)
{
    DlLock lock;
    Resolved r;
    Resolve(surface, &r);
    DlPutRaw(s_nameBytes + r.name);
}

static const void* s_lastKey = 0;
static int s_lastGaf = -1;
static unsigned s_lastEpoch = 0;
static int GafLookup(const void* key)
{
    if (key == s_lastKey && s_lastEpoch == s_gafEpoch)
        return s_lastGaf;
    s_lastKey = key;
    s_lastGaf = GafFind(key);
    s_lastEpoch = s_gafEpoch;
    return s_lastGaf;
}

static void PutPictureName(const DlPicture* p)
{
    char text[400];
    int g = GafLookup(p->key);
    if (g >= 0) {
        DlPutRaw(s_nameBytes + s_gaf[g].file);
        DlPutRaw(":");
        DlPutRaw(s_nameBytes + s_gaf[g].entry);
        DlPutRaw(":");
        DlPutInt(s_gaf[g].index);
        return;
    }
    int r = FindByBlock(p->key);
    if (r < 0 && p->pixels != 0)
        r = FindContaining(p->pixels);
    if (r >= 0 && s_regions[r].surface != 0) {
        DlPutRaw("target:");
        DlPutRaw(s_nameBytes + s_regions[r].name);
        return;
    }
    char hash[24];
    if (p->compressed != 0 || p->layered != 0 || p->pixels == 0)
        strcpy(hash, "-");
    else
        HexHash(hash, HashRun(p->pixels, (unsigned)(p->width * p->height)));
    if (r >= 0)
        sprintf(text, "@%s:%dx%d:%s", s_nameBytes + s_regions[r].name, p->width, p->height, hash);
    else
        sprintf(text, "?%dx%d:%s", p->width, p->height, hash);
    DlPutRaw(text);
}

void DlPictureReads(const DlPicture* picture, const unsigned char** reads)
{
    DlLock lock;
    *reads = 0;
    if (GafLookup(picture->key) >= 0)
        return;
    int r = FindByBlock(picture->key);
    if (r < 0 && picture->pixels != 0)
        r = FindContaining(picture->pixels);
    if (r >= 0 && s_regions[r].surface != 0)
        *reads = s_regions[r].pixels;
}

void DlPutPicture(const DlPicture* picture)
{
    DlLock lock;
    PutPictureName(picture);
}

static void PutCursorName(const DlPicture* picture)
{
    int g = GafLookup(picture->key);
    if (g < 0) {
        PutPictureName(picture);
        return;
    }
    DlPutRaw(s_nameBytes + s_gaf[g].file);
    DlPutRaw(":");
    DlPutRaw(s_nameBytes + s_gaf[g].entry);
}

int DlBegin(const char* kind, const DlSurface* dst, int x0, int y0, int x1, int y1, const unsigned char* reads)
{
    DlLock lock;
    if (s_itemCount >= kMaxItems)
        Fail("items in a frame", "32768");
    Resolved r;
    Resolve(dst, &r);
    g_dlOffX = r.offX;
    g_dlOffY = r.offY;
    s_curStart = s_textUsed;
    s_dropItem = x0 >= x1 || y0 >= y1;
    if (s_dropItem)
        return -1;
    TargetSlot* t = SlotFor(r.base, r.width, r.height, r.name);
    x0 += r.offX;
    x1 += r.offX;
    y0 += r.offY;
    y1 += r.offY;
    int depends = Query(t, x0, y0, x1, y1);
    if (t->maxRead > depends)
        depends = t->maxRead;
    TargetSlot* source = 0;
    if (reads != 0) {
        int sr = FindContaining(reads);
        const Region* region = sr >= 0 ? &s_regions[sr] : 0;
        source = SlotFor(reads, region != 0 ? region->width : 0, region != 0 ? region->height : 0,
                         region != 0 ? region->name : AnonymousName(0, 0));
        if (source->maxDraw > depends)
            depends = source->maxDraw;
    }
    int layer = depends + 1;
    Add(t, x0, y0, x1, y1, layer);
    if (source != 0 && layer > source->maxRead)
        source->maxRead = layer;
    s_curStart = s_textUsed;
    DlPutInt(layer);
    DlPutRaw(" ");
    s_items[s_itemCount].start = s_curStart;
    s_items[s_itemCount].lineOff = s_textUsed;
    DlPutRaw(kind);
    DlPutRaw(" ");
    DlPutRaw(s_nameBytes + t->name);
    DlPutRaw(" ");
    DlPutInt(x0);
    DlPutRaw(" ");
    DlPutInt(y0);
    DlPutRaw(" ");
    DlPutInt(x1);
    DlPutRaw(" ");
    DlPutInt(y1);
    return layer;
}

void DlEnd()
{
    if (s_dropItem) {
        s_textUsed = s_curStart;
        s_dropItem = 0;
        return;
    }
    DlPutRaw("\n");
    s_items[s_itemCount].end = s_textUsed;
    s_itemCount++;
}

static FILE* s_digest = 0;
static FILE* s_list = 0;
static char s_digestBuffer[8192];
static char s_listBuffer[1 << 16];
static int s_listAll = 0;
static int s_order[kMaxItems];

static int CompareFrom(int ia, int ib, int depth)
{
    const Item* x = &s_items[ia];
    const Item* y = &s_items[ib];
    int lx = x->end - x->start;
    int ly = y->end - y->start;
    if (depth >= lx || depth >= ly)
        return lx < ly ? -1 : (lx > ly ? 1 : 0);
    int m = (lx < ly ? lx : ly) - depth;
    int c = memcmp(s_text + x->start + depth, s_text + y->start + depth, (size_t)m);
    if (c != 0)
        return c;
    return lx < ly ? -1 : (lx > ly ? 1 : 0);
}

static unsigned WordAt(int item, int depth)
{
    const Item* it = &s_items[item];
    const unsigned char* p = (const unsigned char*)s_text + it->start + depth;
    int n = it->end - it->start - depth;
    if (n >= 4)
        return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | (unsigned)p[3];
    unsigned w = 0;
    for (int i = 0; i < 4; i++)
        w = w << 8 | (i < n ? (unsigned)p[i] : 0u);
    return w;
}

static void SortLines(int* a, int n, int depth)
{
    while (n > 1) {
        if (n < 12) {
            for (int i = 1; i < n; i++) {
                int v = a[i];
                int j = i - 1;
                while (j >= 0 && CompareFrom(a[j], v, depth) > 0) {
                    a[j + 1] = a[j];
                    j--;
                }
                a[j + 1] = v;
            }
            return;
        }
        unsigned b0 = WordAt(a[0], depth);
        unsigned b1 = WordAt(a[n / 2], depth);
        unsigned b2 = WordAt(a[n - 1], depth);
        unsigned pivot = b0 < b1 ? (b1 < b2 ? b1 : (b0 < b2 ? b2 : b0)) : (b0 < b2 ? b0 : (b1 < b2 ? b2 : b1));
        int lt = 0;
        int i = 0;
        int gt = n;
        while (i < gt) {
            unsigned c = WordAt(a[i], depth);
            if (c < pivot) {
                int t = a[lt];
                a[lt] = a[i];
                a[i] = t;
                lt++;
                i++;
            } else if (c > pivot) {
                gt--;
                int t = a[gt];
                a[gt] = a[i];
                a[i] = t;
            } else {
                i++;
            }
        }
        SortLines(a, lt, depth);
        SortLines(a + gt, n - gt, depth);
        a += lt;
        n = gt - lt;
        if (n > 0 && s_items[a[0]].end - s_items[a[0]].start <= depth + 4)
            return;
        depth += 4;
    }
}

void DlOpen(int listAll)
{
    s_digest = fopen("display.txt", "wb");
    if (s_digest == 0) {
        Die("testmode: cannot write display.txt\n");
        return;
    }
    setvbuf(s_digest, s_digestBuffer, _IOFBF, sizeof s_digestBuffer);
    fprintf(s_digest, "bt-display-digest 1\nlog bt-display 1\n");
    s_list = fopen("displaylist.txt", "wb");
    if (s_list == 0) {
        Die("testmode: cannot write displaylist.txt\n");
        return;
    }
    setvbuf(s_list, s_listBuffer, _IOFBF, sizeof s_listBuffer);
    fprintf(s_list, "bt-display 1\n");
    s_listAll = listAll;
    g_dlOwner = GetCurrentThreadId();
    InitializeCriticalSection(&g_dlSection);
    g_dlLock = 1;
    g_dlOn = 1;
}

void DlFlush()
{
    if (s_digest != 0)
        fflush(s_digest);
    if (s_list != 0)
        fflush(s_list);
}

void DlClose()
{
    if (s_digest != 0)
        fclose(s_digest);
    if (s_list != 0)
        fclose(s_list);
    s_digest = 0;
    s_list = 0;
    g_dlOn = 0;
}

void DlFrameEnd(unsigned number, unsigned tick, unsigned steps, const unsigned char* palette, int list, const DlPicture* cursor,
                int cursorX, int cursorY)
{
    DlLock lock;
    int i;
    char line[200];
    unsigned __int64 paletteHash = HashRun(palette, 1024);
    char paletteText[20];
    HexHash(paletteText, paletteHash);
    sprintf(line, "frame %u %u %u %s\n", number, tick, steps, paletteText);
    char cursorLine[600];
    if (cursor == 0) {
        strcpy(cursorLine, "cursor - - -\n");
    } else {
        int before = s_textUsed;
        DlPutRaw("cursor ");
        PutCursorName(cursor);
        DlPutRaw(" ");
        DlPutInt(cursorX);
        DlPutRaw(" ");
        DlPutInt(cursorY);
        DlPutRaw("\n");
        int n = s_textUsed - before;
        if (n > (int)sizeof cursorLine - 1)
            n = (int)sizeof cursorLine - 1;
        memcpy(cursorLine, s_text + before, n);
        cursorLine[n] = '\0';
        s_textUsed = before;
    }

    DlHash ordered;
    HashStart(&ordered);
    HashFeed(&ordered, (const unsigned char*)line, (unsigned)strlen(line));
    for (i = 0; i < s_itemCount; i++)
        HashFeed(&ordered, (const unsigned char*)s_text + s_items[i].lineOff, (unsigned)(s_items[i].end - s_items[i].lineOff));
    HashFeed(&ordered, (const unsigned char*)cursorLine, (unsigned)strlen(cursorLine));

    for (i = 0; i < s_itemCount; i++)
        s_order[i] = i;
    SortLines(s_order, s_itemCount, 0);
    DlHash set;
    HashStart(&set);
    HashFeed(&set, (const unsigned char*)line, (unsigned)strlen(line));
    for (i = 0; i < s_itemCount; i++) {
        const Item* it = &s_items[s_order[i]];
        HashFeed(&set, (const unsigned char*)s_text + it->start, (unsigned)(it->end - it->start));
    }
    HashFeed(&set, (const unsigned char*)cursorLine, (unsigned)strlen(cursorLine));

    char orderedText[20];
    char setText[20];
    HexHash(orderedText, HashEnd(&ordered));
    HexHash(setText, HashEnd(&set));
    fprintf(s_digest, "%u %u %d %s %s\n", number, tick, s_itemCount, orderedText, setText);
    if (list || s_listAll) {
        fputs(line, s_list);
        for (i = 0; i < s_itemCount; i++)
            fwrite(s_text + s_items[i].lineOff, 1, (size_t)(s_items[i].end - s_items[i].lineOff), s_list);
        fputs(cursorLine, s_list);
    }
    s_itemCount = 0;
    s_textUsed = 0;
    s_targetCount = 0;
    s_bucketUsed = 0;
    s_nodeUsed = 0;
}
