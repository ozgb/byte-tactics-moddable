#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../../interfaces/host.h"
#include "access_log.h"
#include "classifier.h"
#include "../../src/state/alloc_hooks.h"
#include "../../src/state/sim_events.h"
#include "../../src/state/sim_radar.h"
#include "../../src/state/block.h"

#define NS __attribute__((no_sanitize("address")))

void* __cdecl SimBlockBase();
void __cdecl SimBlockInit();

AccessMarks g_accessMarks;

namespace {

const unsigned kBlock = 32u << 20;
const unsigned kNone = 0xFFFFFFFFu;
const unsigned kIgnored = 0xFFFF;
const unsigned kGranules = kBlock / 8;
const unsigned kEdgeBits = 21;
const unsigned kSegmentLimit = 4096;
const unsigned kSectionExecute = 0x20000000u;
const unsigned kSectionWrite = 0x80000000u;

struct Type {
    unsigned size, count;
    const unsigned* fields;
    unsigned short* byteMap;
};
struct Region {
    unsigned start, end, item, type, stride;
};
struct Base {
    unsigned pointer, size, count;
    Region* regions;
    unsigned short* map;
};
struct Site {
    unsigned ra, item, type, stride;
    const unsigned* parts;
    unsigned partCount;
};
struct Alloc {
    unsigned start, size, item, type, stride, seq, next;
    const unsigned* parts;
    unsigned partCount;
};
struct ReadCell {
    unsigned count, firstTick;
};
const unsigned kPartWords = 7;
struct Hit {
    unsigned item, container, entA, entB, end;
};
struct Range {
    unsigned start, end;
    unsigned short sub, pad;
};
struct Section {
    unsigned start, end, flags;
};
struct Stats {
    unsigned long long accesses, noSubsystem, ignored, code, constants, other, unresolvedImage, unresolvedBlock, writes, ranges, rangeCapped, draws, sounds, uiRecords;
};
struct Slot {
    unsigned ra, sub;
};
struct Func {
    unsigned start, end;
    const char* name;
};
struct EntDraw {
    unsigned pass, stream, sub, func, count, tainted, taintN, taint[4];
};
struct DrawAgg {
    unsigned used, pass, stream, sub, func, minDraws, maxDraws, ticks, maxEntities, entities, lastTick, firstTick, tainted, taintN, taint[4];
};
struct PassRead {
    unsigned used, pass, item, sub, func, evidence, count, firstTick;
};
const unsigned kDrawBits = 13;
const unsigned kPassBits = 16;
const unsigned kEntDraws = 32;
const char* const kPassNames[] = {"other", "units", "projectiles", "explosions", "players", "features", "wind", "meteors", "tail",
                                  "features.anim", "explosions.pieces", "explosions.debris", "units.players", "players.los",
                                  "presentation"};
const unsigned kPresentationPass = ACCESS_PASS_COUNT;
static_assert(sizeof(kPassNames) / sizeof(kPassNames[0]) == kPresentationPass + 1, "a name for each pass and presentation");
static_assert(ACCESS_MARK_PASS + ACCESS_PASS_COUNT <= ACCESS_MARK_COUNT, "a mark for each pass");
const unsigned kPassDepth = 8;
struct PassFrame {
    int pass;
    unsigned entOrd;
};
const unsigned kKinds = 40;
const unsigned kPresBits = 16;
struct PresEntry {
    unsigned item, container, func, sub, stamp, windows;
    unsigned kinds[kKinds];
};

volatile int g_on;
int g_window = ACCESS_WINDOW_OFF;
unsigned g_tick;
unsigned g_lastMarkerTick;
unsigned g_simThread;
bool g_started;
bool g_ready;
bool g_raw;

unsigned g_subCount;
const char** g_subNames;
Range* g_ranges;
unsigned g_rangeCount;
Slot g_attrCache[1 << 16];

Func* g_funcs;
unsigned g_funcCount;
Slot g_funcCache[1 << 14];
PresEntry* g_pres;
unsigned g_presUsed;
unsigned g_presStamp;
unsigned g_presWindows, g_presActiveWindows;
bool g_presActive;
bool g_presOverflow;
unsigned char g_tickKinds[kKinds];
unsigned char g_prevKinds[kKinds];
unsigned g_seenTail;
unsigned g_kindWindows[kKinds];

unsigned g_itemCount;
const char** g_itemNames;
unsigned g_idUnresolvedImage, g_idUnresolvedBlock, g_idRng[3], g_idSound, g_idUi, g_idSimEvent;
int g_pass;
unsigned g_entOrd;
unsigned g_ordCounter;
unsigned g_passDepth;
PassFrame g_passStack[kPassDepth];
EntDraw g_ent[kEntDraws];
unsigned g_entN;
bool g_entTaint;
unsigned g_taintN;
unsigned g_taint[4];
DrawAgg* g_draws;
unsigned g_drawsUsed;
PassRead* g_passReads;
unsigned g_passReadsUsed;
bool g_phaseOverflow;
bool g_simPending;
unsigned char* g_funcRng;
Type* g_types;
unsigned g_typeCount;
Region* g_statics;
unsigned g_staticCount;
Base* g_bases;
unsigned g_baseCount;
Site* g_sites;
unsigned g_siteCount;
ReadCell* g_reads;

unsigned g_imageLo, g_imageSize;
Section g_sections[16];
unsigned g_sectionCount;
unsigned g_blockLo;

unsigned* g_granuleIndex;
Alloc* g_pool;
unsigned g_poolCount, g_poolFree, g_poolCap;
unsigned g_allocSeq;
unsigned* g_bits0;
unsigned* g_bits1;
unsigned* g_bits2;
void* g_allocLock;
__declspec(thread) int g_busy;
__declspec(thread) unsigned g_depth;
__declspec(thread) unsigned g_curSite;

Classifier g_classifier;
Stats g_stats;
void* g_rawFile;
char* g_rawBuf;
unsigned g_rawLen;
const unsigned kRawCap = 1u << 20;

struct Quiet {
    NS Quiet() { g_busy++; }
    NS ~Quiet() { g_busy--; }
};

NS void* Zeroed(unsigned bytes)
{
    Quiet quiet;
    void* p = host_mem_alloc(bytes);
    if (p != 0)
        memset(p, 0, bytes);
    return p;
}

NS void Release(void* p)
{
    Quiet quiet;
    host_mem_free(p);
}

NS inline unsigned ThreadId()
{
    unsigned id;
    __asm__("movl %%fs:0x24, %0" : "=r"(id));
    return id;
}

NS unsigned char* ReadWhole(const char* path, unsigned* size)
{
    Quiet quiet;
    void* h = host_file_open_read(path);
    if (h == 0)
        return 0;
    long n = host_file_size(h);
    unsigned char* p = n < 0 ? 0 : (unsigned char*)Zeroed((unsigned)n + 1);
    if (p == 0 || host_file_read(h, p, (unsigned)n) != (unsigned)n)
        p = 0;
    host_file_close(h);
    *size = (unsigned)n;
    return p;
}

NS void Die(const char* text)
{
    host_log(text);
    host_exit(3);
}

struct Reader {
    const unsigned char* p;
    NS unsigned U32()
    {
        unsigned v;
        memcpy(&v, p, 4);
        p += 4;
        return v;
    }
    NS unsigned U16()
    {
        unsigned v = p[0] | p[1] << 8;
        p += 2;
        return v;
    }
};

NS const char* CopyString(const unsigned char* p, unsigned n)
{
    char* s = (char*)Zeroed(n + 1);
    memcpy(s, p, n);
    return s;
}

NS void LoadAttr(const unsigned char* data)
{
    Reader r = {data + 4};
    g_subCount = r.U32();
    g_subNames = (const char**)Zeroed(sizeof(char*) * (g_subCount + 1));
    for (unsigned i = 0; i < g_subCount; i++) {
        unsigned n = *r.p++;
        g_subNames[i] = CopyString(r.p, n);
        r.p += n;
    }
    g_rangeCount = r.U32();
    g_ranges = (Range*)Zeroed(sizeof(Range) * (g_rangeCount + 1));
    for (unsigned i = 0; i < g_rangeCount; i++) {
        g_ranges[i].start = r.U32();
        g_ranges[i].end = r.U32();
        unsigned both = r.U32();
        g_ranges[i].sub = (unsigned short)(both & 0xFFFF);
    }
}

NS unsigned short* PaintFields(const unsigned* fields, unsigned count, unsigned size)
{
    unsigned short* map = (unsigned short*)Zeroed(2 * size);
    for (unsigned i = 0; i < size; i++)
        map[i] = 0xFFFF;
    for (unsigned f = 0; f < count; f++)
        for (unsigned b = fields[3 * f]; b < fields[3 * f + 1] && b < size; b++)
            map[b] = (unsigned short)f;
    return map;
}

NS void LoadItems(const unsigned char* data)
{
    Reader r = {data + 4};
    unsigned strings = r.U32();
    g_itemNames = (const char**)Zeroed(sizeof(char*) * (strings + 9));
    for (unsigned i = 0; i < strings; i++) {
        unsigned n = r.U16();
        g_itemNames[i] = CopyString(r.p, n);
        r.p += n;
    }
    g_idUnresolvedImage = strings;
    g_idUnresolvedBlock = strings + 1;
    g_idRng[0] = strings + 2;
    g_idRng[1] = strings + 3;
    g_idRng[2] = strings + 4;
    g_idSound = strings + 5;
    g_idUi = strings + 6;
    g_idSimEvent = strings + 7;
    g_itemNames[g_idUnresolvedImage] = "unresolved:image";
    g_itemNames[g_idUnresolvedBlock] = "unresolved:block";
    g_itemNames[g_idRng[0]] = "rng.parkMiller";
    g_itemNames[g_idRng[1]] = "rng.crtHoldrand";
    g_itemNames[g_idRng[2]] = "rng.crtLoad";
    g_itemNames[g_idSound] = "sound";
    g_itemNames[g_idUi] = "ui";
    g_itemNames[g_idSimEvent] = "sim-event";
    g_itemCount = strings + 8;
    g_typeCount = r.U32();
    g_types = (Type*)Zeroed(sizeof(Type) * (g_typeCount + 1));
    for (unsigned i = 0; i < g_typeCount; i++) {
        g_types[i].size = r.U32();
        g_types[i].count = r.U32();
        g_types[i].fields = (const unsigned*)r.p;
        r.p += 12 * g_types[i].count;
        if (g_types[i].size <= (1u << 16))
            g_types[i].byteMap = PaintFields(g_types[i].fields, g_types[i].count, g_types[i].size);
    }
    g_staticCount = r.U32();
    g_statics = (Region*)Zeroed(sizeof(Region) * (g_staticCount + 1));
    for (unsigned i = 0; i < g_staticCount; i++) {
        g_statics[i].start = r.U32();
        g_statics[i].end = r.U32();
        g_statics[i].item = r.U32();
        g_statics[i].type = r.U32();
        g_statics[i].stride = r.U32();
    }
    g_baseCount = r.U32();
    g_bases = (Base*)Zeroed(sizeof(Base) * (g_baseCount + 1));
    for (unsigned i = 0; i < g_baseCount; i++) {
        g_bases[i].pointer = r.U32();
        g_bases[i].size = r.U32();
        g_bases[i].count = r.U32();
        g_bases[i].regions = (Region*)Zeroed(sizeof(Region) * (g_bases[i].count + 1));
        g_bases[i].map = (unsigned short*)Zeroed(2 * g_bases[i].size);
        for (unsigned b = 0; b < g_bases[i].size; b++)
            g_bases[i].map[b] = 0xFFFF;
        for (unsigned k = 0; k < g_bases[i].count; k++) {
            Region* region = &g_bases[i].regions[k];
            region->start = r.U32();
            region->end = r.U32();
            region->item = r.U32();
            region->type = r.U32();
            region->stride = r.U32();
            for (unsigned b = region->start; b < region->end && b < g_bases[i].size; b++)
                g_bases[i].map[b] = (unsigned short)k;
        }
    }
    g_siteCount = r.U32();
    g_sites = (Site*)Zeroed(sizeof(Site) * (g_siteCount + 1));
    for (unsigned i = 0; i < g_siteCount; i++) {
        g_sites[i].ra = r.U32();
        g_sites[i].item = r.U32();
        g_sites[i].type = r.U32();
        g_sites[i].stride = r.U32();
    }
    unsigned partSets = r.U32();
    for (unsigned i = 0; i < partSets; i++) {
        unsigned ra = r.U32();
        unsigned n = r.U32();
        const unsigned* words = (const unsigned*)r.p;
        r.p += 4 * kPartWords * n;
        for (unsigned k = 0; k < g_siteCount; k++) {
            if (g_sites[k].ra == ra) {
                g_sites[k].parts = words;
                g_sites[k].partCount = n;
            }
        }
    }
}

NS void LoadFuncs(const unsigned char* data)
{
    Reader r = {data + 4};
    g_funcCount = r.U32();
    g_funcs = (Func*)Zeroed(sizeof(Func) * (g_funcCount + 1));
    for (unsigned i = 0; i < g_funcCount; i++) {
        g_funcs[i].start = r.U32();
        g_funcs[i].end = r.U32();
        unsigned n = r.U16();
        g_funcs[i].name = CopyString(r.p, n);
        r.p += n;
    }
    g_pres = (PresEntry*)Zeroed(sizeof(PresEntry) << kPresBits);
    g_funcRng = (unsigned char*)Zeroed(g_funcCount + 1);
    const char* rng[] = {"RandomInt", "GameRand", "GameHoldrand", "GameSrand", "SeedRandom"};
    for (unsigned i = 0; i < g_funcCount; i++)
        for (unsigned k = 0; k < sizeof rng / sizeof rng[0]; k++)
            if (strcmp(g_funcs[i].name, rng[k]) == 0)
                g_funcRng[i + 1] = 1;
}

NS void LoadImage()
{
    const void* base = 0;
    unsigned size = 0;
    if (!host_image_range(&base, &size))
        Die("accesslog: no image range\n");
    const unsigned char* image = (const unsigned char*)base;
    unsigned nt, count, optional;
    memcpy(&nt, image + 0x3C, 4);
    unsigned short sections, optionalSize;
    memcpy(&sections, image + nt + 6, 2);
    memcpy(&optionalSize, image + nt + 20, 2);
    count = sections;
    optional = optionalSize;
    g_imageLo = (unsigned)image;
    g_imageSize = size;
    const unsigned char* header = image + nt + 24 + optional;
    for (unsigned i = 0; i < count && i < 16; i++, header += 40) {
        unsigned virtualSize, virtualAddress, characteristics;
        memcpy(&virtualSize, header + 8, 4);
        memcpy(&virtualAddress, header + 12, 4);
        memcpy(&characteristics, header + 36, 4);
        g_sections[g_sectionCount].start = g_imageLo + virtualAddress;
        g_sections[g_sectionCount].end = g_imageLo + virtualAddress + virtualSize;
        g_sections[g_sectionCount].flags = characteristics;
        g_sectionCount++;
    }
}

NS inline unsigned Attribute(void* ra)
{
    unsigned r = (unsigned)ra;
    Slot* slot = &g_attrCache[(r * 2654435761u) >> 16];
    if (slot->ra == r)
        return slot->sub;
    unsigned a = r - 1;
    unsigned lo = 0, hi = g_rangeCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (g_ranges[mid].start <= a)
            lo = mid + 1;
        else
            hi = mid;
    }
    unsigned sub = 0;
    if (lo > 0 && a < g_ranges[lo - 1].end)
        sub = g_ranges[lo - 1].sub;
    slot->ra = r;
    slot->sub = sub;
    return sub;
}

NS unsigned FunctionOf(void* ra)
{
    unsigned r = (unsigned)ra;
    Slot* slot = &g_funcCache[(r * 2654435761u) >> 18];
    if (slot->ra == r)
        return slot->sub;
    unsigned a = r - 1;
    unsigned lo = 0, hi = g_funcCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (g_funcs[mid].start <= a)
            lo = mid + 1;
        else
            hi = mid;
    }
    unsigned f = lo > 0 && a < g_funcs[lo - 1].end ? lo : 0;
    slot->ra = r;
    slot->sub = f;
    return f;
}

NS void NotePresentationRead(unsigned item, unsigned container, void* ra, unsigned sub)
{
    unsigned f = FunctionOf(ra);
    unsigned h = (item * 40503u) ^ (f * 2654435761u) ^ (sub * 2246822519u) ^ (container * 3266489917u);
    h ^= h >> 15;
    unsigned mask = (1u << kPresBits) - 1;
    for (unsigned n = 0; n <= mask; n++) {
        PresEntry* e = &g_pres[(h + n) & mask];
        if (e->func == 0) {
            if (g_presUsed >= mask - (mask >> 3)) {
                g_presOverflow = true;
                return;
            }
            e->item = item;
            e->container = container;
            e->func = f + 1;
            e->sub = sub;
            g_presUsed++;
        } else if (e->item != item || e->container != container || e->func != f + 1 || e->sub != sub) {
            continue;
        }
        if (e->stamp != g_presStamp) {
            e->stamp = g_presStamp;
            e->windows++;
            for (unsigned k = 0; k < kKinds; k++)
                e->kinds[k] += g_prevKinds[k];
        }
        g_presActive = true;
        return;
    }
    g_presOverflow = true;
}

NS void ReadEventKinds(bool atMarker)
{
    Quiet quiet;
    SimEventQueue& q = SimEvents();
    unsigned tail = q.tail;
    if (atMarker) {
        for (unsigned i = q.head; i != tail; i++) {
            const SimEvent& e = q.records[i % SIM_EVENT_CAPACITY];
            if (e.flags & SIM_EVENT_CONTINUED)
                continue;
            g_tickKinds[e.kind < kKinds ? e.kind : kKinds - 1] = 1;
        }
    } else if (tail > g_seenTail) {
        g_tickKinds[0] = 1;
    }
    g_seenTail = tail;
}

NS inline unsigned FieldIndex(const Type* t, unsigned off)
{
    if (t->byteMap != 0)
        return off < t->size ? t->byteMap[off] : 0xFFFF;
    unsigned lo = 0, hi = t->count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (t->fields[3 * mid] <= off)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo > 0 && off < t->fields[3 * (lo - 1) + 1] ? lo - 1 : 0xFFFF;
}

NS void RefineInRegion(unsigned item, unsigned type, unsigned stride, unsigned start, unsigned end, unsigned off, unsigned a, Hit* h)
{
    h->item = item;
    h->container = item;
    h->entB = 0;
    h->end = start + end;
    if (type == kNone || stride == 0)
        return;
    unsigned idx = off / stride, within = off % stride;
    unsigned f = FieldIndex(&g_types[type], within);
    h->entB = idx;
    unsigned elementStart = a - within;
    if (f != 0xFFFF) {
        h->item = g_types[type].fields[3 * f + 2];
        h->end = elementStart + g_types[type].fields[3 * f + 1];
    } else {
        h->end = elementStart + stride;
    }
}

NS unsigned PartBound(const Alloc* al, unsigned at, unsigned fixed)
{
    if (at == kNone)
        return fixed == kNone ? al->size : fixed;
    return at + 4 <= al->size ? *(const unsigned*)(al->start + at) : 0;
}

NS bool ResolvePart(const Alloc* al, unsigned a, Hit* h)
{
    unsigned off = a - al->start;
    for (unsigned i = 0; i < al->partCount; i++) {
        const unsigned* w = al->parts + kPartWords * i;
        unsigned start = PartBound(al, w[0], w[1]);
        unsigned end = PartBound(al, w[2], w[3]);
        if (end > al->size)
            end = al->size;
        if (off < start || off >= end)
            continue;
        RefineInRegion(w[4], w[5], w[6], al->start + start, end - start, off - start, a, h);
        if (w[5] == kNone || w[6] == 0)
            h->end = al->start + end;
        return true;
    }
    return false;
}

NS bool FindAlloc(unsigned a, Alloc** out)
{
    unsigned g = (a - g_blockLo) / 8;
    unsigned w0 = g >> 5;
    unsigned bit = g & 31;
    unsigned m = g_bits0[w0] & (bit == 31 ? 0xFFFFFFFFu : ((2u << bit) - 1));
    unsigned found = kNone;
    if (m != 0) {
        found = (w0 << 5) | (31 - __builtin_clz(m));
    } else {
        unsigned s1 = w0 >> 5, b1 = w0 & 31;
        unsigned m1 = b1 == 0 ? 0 : g_bits1[s1] & ((1u << b1) - 1);
        if (m1 != 0) {
            unsigned word = (s1 << 5) | (31 - __builtin_clz(m1));
            found = (word << 5) | (31 - __builtin_clz(g_bits0[word]));
        } else {
            unsigned s2 = s1 >> 5, b2 = s1 & 31;
            unsigned m2 = b2 == 0 ? 0 : g_bits2[s2] & ((1u << b2) - 1);
            unsigned sw = kNone;
            if (m2 != 0) {
                sw = (s2 << 5) | (31 - __builtin_clz(m2));
            } else {
                for (unsigned t = s2; t-- > 0;) {
                    if (g_bits2[t] != 0) {
                        sw = (t << 5) | (31 - __builtin_clz(g_bits2[t]));
                        break;
                    }
                }
            }
            if (sw != kNone) {
                unsigned word = (sw << 5) | (31 - __builtin_clz(g_bits1[sw]));
                found = (word << 5) | (31 - __builtin_clz(g_bits0[word]));
            }
        }
    }
    if (found == kNone)
        return false;
    Alloc* al = &g_pool[g_granuleIndex[found] - 1];
    if (a >= al->start + al->size)
        return false;
    *out = al;
    return true;
}

NS inline void SetBit(unsigned g)
{
    g_bits0[g >> 5] |= 1u << (g & 31);
    g_bits1[g >> 10] |= 1u << ((g >> 5) & 31);
    g_bits2[g >> 15] |= 1u << ((g >> 10) & 31);
}

NS inline void ClearBit(unsigned g)
{
    g_bits0[g >> 5] &= ~(1u << (g & 31));
    if (g_bits0[g >> 5] == 0) {
        g_bits1[g >> 10] &= ~(1u << ((g >> 5) & 31));
        if (g_bits1[g >> 10] == 0)
            g_bits2[g >> 15] &= ~(1u << ((g >> 10) & 31));
    }
}

NS const Section* SectionOf(unsigned a)
{
    for (unsigned i = 0; i < g_sectionCount; i++)
        if (a - g_sections[i].start < g_sections[i].end - g_sections[i].start)
            return &g_sections[i];
    return 0;
}

NS bool Resolve(unsigned a, Hit* h)
{
    h->entA = 0;
    if (a - g_imageLo < g_imageSize) {
        const Section* s = SectionOf(a);
        if (s == 0 || (s->flags & kSectionExecute)) {
            g_stats.code++;
            return false;
        }
        unsigned lo = 0, hi = g_staticCount;
        while (lo < hi) {
            unsigned mid = (lo + hi) / 2;
            if (g_statics[mid].start <= a)
                lo = mid + 1;
            else
                hi = mid;
        }
        if (lo > 0 && a < g_statics[lo - 1].end) {
            const Region* r = &g_statics[lo - 1];
            RefineInRegion(r->item, r->type, r->stride, r->start, r->end - r->start, a - r->start, a, h);
            if (r->type == kNone || r->stride == 0)
                h->end = r->end;
            return true;
        }
        if (!(s->flags & kSectionWrite)) {
            g_stats.constants++;
            return false;
        }
        h->item = h->container = g_idUnresolvedImage;
        h->entB = 0;
        h->end = a + 1;
        g_stats.unresolvedImage++;
        return true;
    }
    if (a - g_blockLo < kBlock) {
        for (unsigned i = 0; i < g_baseCount; i++) {
            unsigned base = *(unsigned*)g_bases[i].pointer;
            if (base != 0 && a - base < g_bases[i].size) {
                unsigned off = a - base;
                unsigned k = g_bases[i].map[off];
                if (k == 0xFFFF)
                    break;
                const Region* r = &g_bases[i].regions[k];
                RefineInRegion(r->item, r->type, r->stride, base + r->start, r->end - r->start, off - r->start, a, h);
                if (r->type == kNone || r->stride == 0)
                    h->end = base + r->end;
                return true;
            }
        }
        Alloc* al;
        if (FindAlloc(a, &al)) {
            if (!ResolvePart(al, a, h)) {
                RefineInRegion(al->item, al->type, al->stride, al->start, al->size, a - al->start, a, h);
                if (al->type == kNone || al->stride == 0)
                    h->end = al->start + al->size;
            }
            h->entA = al->seq;
            if (al->item == g_idUnresolvedBlock)
                g_stats.unresolvedBlock++;
            return true;
        }
        h->item = h->container = g_idUnresolvedBlock;
        h->entB = 0;
        h->end = a + 1;
        g_stats.unresolvedBlock++;
        return true;
    }
    g_stats.other++;
    return false;
}

NS void RawLine(const char* kind, unsigned sub, unsigned item, const Hit& h, unsigned a)
{
    if (g_rawFile == 0)
        return;
    Quiet quiet;
    if (g_rawLen + 256 > kRawCap) {
        host_file_write(g_rawFile, g_rawBuf, g_rawLen);
        g_rawLen = 0;
    }
    g_rawLen += sprintf(g_rawBuf + g_rawLen, "%u %d %s %s %s %08x %u %u\n", g_tick, g_window, kind,
                          sub < g_subCount ? g_subNames[sub] : "-", g_itemNames[item], a, h.entA, h.entB);
}

NS DrawAgg* FindDraw(unsigned pass, unsigned stream, unsigned sub, unsigned func)
{
    unsigned h = (pass * 40503u) ^ (stream * 3266489917u) ^ (sub * 2246822519u) ^ (func * 2654435761u);
    h ^= h >> 15;
    unsigned mask = (1u << kDrawBits) - 1;
    for (unsigned n = 0; n <= mask; n++) {
        DrawAgg* a = &g_draws[(h + n) & mask];
        if (a->used == 0) {
            if (g_drawsUsed >= mask - (mask >> 3)) {
                g_phaseOverflow = true;
                return 0;
            }
            a->used = 1;
            a->pass = pass;
            a->stream = stream;
            a->sub = sub;
            a->func = func;
            g_drawsUsed++;
            return a;
        }
        if (a->pass == pass && a->stream == stream && a->sub == sub && a->func == func)
            return a;
    }
    g_phaseOverflow = true;
    return 0;
}

NS void AddTaint(unsigned* list, unsigned* n, unsigned item)
{
    for (unsigned i = 0; i < *n; i++)
        if (list[i] == item)
            return;
    if (*n < 4)
        list[(*n)++] = item;
}

NS void FlushEntity()
{
    for (unsigned i = 0; i < g_entN; i++) {
        const EntDraw& e = g_ent[i];
        DrawAgg* a = FindDraw(e.pass, e.stream, e.sub, e.func);
        if (a == 0)
            continue;
        if (a->ticks == 0 || a->lastTick != g_tick + 1) {
            a->ticks++;
            a->entities = 0;
            a->lastTick = g_tick + 1;
            if (a->ticks == 1) {
                a->firstTick = g_tick;
                a->minDraws = e.count;
            }
        }
        if (e.count < a->minDraws)
            a->minDraws = e.count;
        if (e.count > a->maxDraws)
            a->maxDraws = e.count;
        a->entities++;
        if (a->entities > a->maxEntities)
            a->maxEntities = a->entities;
        if (e.tainted) {
            a->tainted = 1;
            for (unsigned k = 0; k < e.taintN; k++)
                AddTaint(a->taint, &a->taintN, e.taint[k]);
        }
    }
    g_entN = 0;
    g_taintN = 0;
    g_entTaint = false;
}

NS void NotePassRead(unsigned item, unsigned sub, void* ra, unsigned flags, unsigned level)
{
    unsigned f = FunctionOf(ra);
    if (g_funcRng != 0 && g_funcRng[f])
        return;
    unsigned evidence = (flags & READ_SAME_RECORD) ? 0 : 1;
    if (!g_entTaint)
        g_entTaint = true;
    AddTaint(g_taint, &g_taintN, item);
    unsigned pass = (unsigned)(level < g_passDepth ? g_passStack[level].pass : g_pass);
    unsigned h = (pass * 40503u) ^ (item * 3266489917u) ^ (sub * 2246822519u) ^ (f * 2654435761u) ^ (evidence * 668265263u);
    h ^= h >> 15;
    unsigned mask = (1u << kPassBits) - 1;
    for (unsigned n = 0; n <= mask; n++) {
        PassRead* r = &g_passReads[(h + n) & mask];
        if (r->used == 0) {
            if (g_passReadsUsed >= mask - (mask >> 3)) {
                g_phaseOverflow = true;
                return;
            }
            r->used = 1;
            r->pass = pass;
            r->item = item;
            r->sub = sub;
            r->func = f;
            r->evidence = evidence;
            r->firstTick = g_tick;
            g_passReadsUsed++;
        } else if (r->pass != pass || r->item != item || r->sub != sub || r->func != f || r->evidence != evidence) {
            continue;
        }
        r->count++;
        return;
    }
    g_phaseOverflow = true;
}
#if BT_DEFER_WRITES
const unsigned kMaxDeferEdges = 32;
const unsigned kDeferNew = 1;
const unsigned kDeferOld = 2;

struct DeferEdge {
    unsigned writer, reader, phase;
};
struct DeferEntry {
    unsigned addr, slot;
    unsigned short edge;
    unsigned char oldv, newv, state;
};
struct DeferStats {
    unsigned long long created, held, applied, superseded, thirdStores, freed;
};

DeferEdge g_deferEdges[kMaxDeferEdges];
unsigned g_deferEdgeCount;
unsigned* g_itemEdges;
bool* g_isWriter;
DeferEntry* g_dEntries;
unsigned g_dCount, g_dCap;
unsigned* g_dTable;
unsigned g_dTableSize;
DeferStats g_dStats;

NS inline unsigned DeferHash(unsigned addr) { return (addr * 2654435761u) >> 7; }

NS DeferEntry* DeferFind(unsigned addr)
{
    if (g_dCount == 0)
        return 0;
    unsigned mask = g_dTableSize - 1;
    for (unsigned s = DeferHash(addr) & mask;; s = (s + 1) & mask) {
        unsigned v = g_dTable[s];
        if (v == 0)
            return 0;
        if (g_dEntries[v - 1].addr == addr)
            return &g_dEntries[v - 1];
    }
}

NS void DeferPlace(unsigned index)
{
    unsigned mask = g_dTableSize - 1;
    unsigned s = DeferHash(g_dEntries[index].addr) & mask;
    while (g_dTable[s] != 0)
        s = (s + 1) & mask;
    g_dTable[s] = index + 1;
    g_dEntries[index].slot = s;
}

NS DeferEntry* DeferAdd(unsigned addr)
{
    if (g_dCount == g_dCap) {
        g_dCap = g_dCap ? g_dCap * 2 : 1u << 12;
        DeferEntry* bigger = (DeferEntry*)Zeroed(sizeof(DeferEntry) * g_dCap);
        if (g_dEntries != 0) {
            memcpy(bigger, g_dEntries, sizeof(DeferEntry) * g_dCount);
            Release(g_dEntries);
        }
        g_dEntries = bigger;
    }
    if ((g_dCount + 1) * 2 > g_dTableSize) {
        unsigned size = g_dTableSize ? g_dTableSize * 2 : 1u << 13;
        if (g_dTable != 0)
            Release(g_dTable);
        g_dTable = (unsigned*)Zeroed(4 * size);
        g_dTableSize = size;
        for (unsigned i = 0; i < g_dCount; i++)
            DeferPlace(i);
    }
    DeferEntry* e = &g_dEntries[g_dCount];
    e->addr = addr;
    DeferPlace(g_dCount);
    g_dCount++;
    return e;
}

NS inline volatile unsigned char* DeferByte(const DeferEntry* e) { return (volatile unsigned char*)e->addr; }

NS void DeferFlush()
{
    for (unsigned i = 0; i < g_dCount; i++) {
        DeferEntry* e = &g_dEntries[i];
        if (e->state == kDeferOld) {
            *DeferByte(e) = e->newv;
            g_dStats.applied++;
        }
        g_dTable[e->slot] = 0;
        e->state = 0;
    }
    g_dCount = 0;
}

NS void DeferTouch(DeferEntry* e, int store, unsigned sub)
{
    if (e->state == 0)
        return;
    if (sub == g_deferEdges[e->edge].reader) {
        if (!store) {
            if (e->state == kDeferNew) {
                e->newv = *DeferByte(e);
                *DeferByte(e) = e->oldv;
                e->state = kDeferOld;
                g_dStats.held++;
            }
        } else {
            if (e->state == kDeferOld) {
                *DeferByte(e) = e->newv;
                g_dStats.applied++;
            }
            e->state = 0;
            g_dStats.superseded++;
        }
        return;
    }
    if (e->state == kDeferOld) {
        *DeferByte(e) = e->newv;
        e->state = kDeferNew;
        g_dStats.applied++;
    }
    if (store && sub != g_deferEdges[e->edge].writer)
        g_dStats.thirdStores++;
}

NS void DeferCreate(unsigned addr, unsigned edge)
{
    DeferEntry* e = DeferFind(addr);
    if (e == 0)
        e = DeferAdd(addr);
    else if (e->state != 0)
        return;
    e->edge = (unsigned short)edge;
    e->oldv = *(volatile unsigned char*)addr;
    e->newv = e->oldv;
    e->state = kDeferNew;
    g_dStats.created++;
}

NS unsigned DeferPhase()
{
    if (g_window == ACCESS_WINDOW_PENDING)
        return 1;
    return g_window >= 1 && g_window <= ACCESS_WINDOW_TAIL ? (unsigned)g_window : 0;
}

NS void DeferAccess(int store, unsigned a, unsigned size, unsigned sub)
{
    unsigned phase = DeferPhase();
    if (phase == 0 || g_deferEdgeCount == 0)
        return;
    unsigned n = size ? size : 1;
    unsigned end = a + n;
    if (g_dCount != 0) {
        if (n > g_dCount) {
            for (unsigned i = 0; i < g_dCount; i++)
                if (g_dEntries[i].addr - a < n)
                    DeferTouch(&g_dEntries[i], store, sub);
        } else {
            for (unsigned b = a; b < end; b++) {
                DeferEntry* e = DeferFind(b);
                if (e != 0)
                    DeferTouch(e, store, sub);
            }
        }
    }
    if (!store || sub >= g_subCount || !g_isWriter[sub])
        return;
    for (unsigned cursor = a; cursor < end;) {
        Hit h;
        if (!Resolve(cursor, &h)) {
            cursor = n <= 16 ? cursor + 1 : (cursor | 0xFFF) + 1;
            continue;
        }
        unsigned stop = h.end > cursor ? h.end : cursor + 1;
        if (stop > end)
            stop = end;
        unsigned mask = g_itemEdges[h.item];
        for (unsigned e = 0; mask != 0; e++, mask >>= 1) {
            if ((mask & 1) && g_deferEdges[e].writer == sub && g_deferEdges[e].phase == phase)
                for (unsigned b = cursor; b < stop; b++)
                    DeferCreate(b, e);
        }
        cursor = stop;
    }
}

NS void DeferDrop(unsigned start, unsigned size)
{
    for (unsigned i = 0; i < g_dCount; i++) {
        DeferEntry* e = &g_dEntries[i];
        if (e->state != 0 && e->addr - start < size) {
            if (e->state == kDeferOld)
                *DeferByte(e) = e->newv;
            e->state = 0;
            g_dStats.freed++;
        }
    }
}

NS unsigned DeferSub(const char* name, unsigned n)
{
    for (unsigned i = 0; i < g_subCount; i++)
        if (strlen(g_subNames[i]) == n && memcmp(g_subNames[i], name, n) == 0)
            return i;
    Die("accesslog: a subsystem in the defer file is not in accesslog.attr\n");
    return 0;
}

NS unsigned DeferPhaseOf(const char* name, unsigned n)
{
    static const char* const names[] = {"net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors", "tail"};
    for (unsigned i = 0; i < 9; i++)
        if (strlen(names[i]) == n && memcmp(names[i], name, n) == 0)
            return i;
    Die("accesslog: a phase in the defer file is not a phase name\n");
    return 0;
}

NS void DeferLoad()
{
    g_itemEdges = (unsigned*)Zeroed(4 * (g_itemCount + 1));
    g_isWriter = (bool*)Zeroed(g_subCount + 1);
    const char* path = getenv("ACCESSLOG_DEFER");
    if (path == 0)
        path = "accesslog.defer";
    unsigned size;
    unsigned char* text = ReadWhole(path, &size);
    if (text == 0)
        Die("accesslog: the defer file accesslog.defer (or $ACCESSLOG_DEFER) is missing; BT_DEFER_WRITES builds read the edges to hold back from it\n");
    const unsigned char* p = text;
    const unsigned char* stop = text + size;
    while (p < stop) {
        const unsigned char* lineEnd = p;
        while (lineEnd < stop && *lineEnd != '\n')
            lineEnd++;
        const unsigned char* line = p;
        const unsigned char* next = lineEnd < stop ? lineEnd + 1 : lineEnd;
        if (lineEnd > line && lineEnd[-1] == '\r')
            lineEnd--;
        p = next;
        if (lineEnd == line || (lineEnd - line >= 7 && memcmp(line, "writer,", 7) == 0))
            continue;
        const unsigned char* c1 = line;
        while (c1 < lineEnd && *c1 != ',')
            c1++;
        const unsigned char* c2 = c1 + 1;
        while (c2 < lineEnd && *c2 != ',')
            c2++;
        const unsigned char* c4 = lineEnd;
        while (c4 > c2 && *c4 != ',')
            c4--;
        if (c1 >= lineEnd || c2 >= lineEnd || c4 <= c2)
            Die("accesslog: a line of the defer file is not writer,reader,item,phase\n");
        if (g_deferEdgeCount == kMaxDeferEdges)
            Die("accesslog: too many edges in the defer file\n");
        DeferEdge* edge = &g_deferEdges[g_deferEdgeCount];
        edge->writer = DeferSub((const char*)line, (unsigned)(c1 - line));
        edge->reader = DeferSub((const char*)c1 + 1, (unsigned)(c2 - c1 - 1));
        edge->phase = DeferPhaseOf((const char*)c4 + 1, (unsigned)(lineEnd - c4 - 1));
        const char* item = (const char*)c2 + 1;
        unsigned itemLen = (unsigned)(c4 - c2 - 1);
        if (itemLen >= 2 && item[0] == '"' && item[itemLen - 1] == '"') {
            item++;
            itemLen -= 2;
        }
        bool found = false;
        for (unsigned i = 0; i < g_itemCount; i++) {
            if (g_itemNames[i] == 0 || strlen(g_itemNames[i]) != itemLen || memcmp(g_itemNames[i], item, itemLen) != 0)
                continue;
            for (unsigned other = 0; other < g_deferEdgeCount; other++)
                if ((g_itemEdges[i] >> other & 1) && g_deferEdges[other].phase == edge->phase)
                    Die("accesslog: two edges of the defer file share an item and a phase\n");
            g_itemEdges[i] |= 1u << g_deferEdgeCount;
            found = true;
        }
        if (!found)
            Die("accesslog: an item in the defer file is not in accesslog.items\n");
        g_isWriter[edge->writer] = true;
        g_deferEdgeCount++;
    }
}
#endif

NS void NoteRead(unsigned item, unsigned sub)
{
    if (g_reads == 0 || sub >= g_subCount)
        return;
    ReadCell* c = &g_reads[item * g_subCount + sub];
    if (c->count++ == 0)
        c->firstTick = g_tick;
}

NS void NoteReadSpan(unsigned from, unsigned end, unsigned sub)
{
    Stats saved = g_stats;
    for (unsigned cursor = from; cursor < end;) {
        Hit h;
        if (!Resolve(cursor, &h)) {
            cursor++;
            continue;
        }
        NoteRead(h.item, sub);
        cursor = h.end > cursor ? h.end : cursor + 1;
    }
    g_stats = saved;
}

NS void Record(int store, unsigned a, unsigned size, void* ra)
{
    unsigned sub = Attribute(ra);
    if (sub == kIgnored) {
        g_stats.ignored++;
        return;
    }
#if BT_DEFER_WRITES
    DeferAccess(store, a, size, sub);
    return;
#endif
    unsigned end = a + (size ? size : 1);
    unsigned cursor = a;
    for (unsigned n = 0; cursor < end; n++) {
        Hit h;
        if (!Resolve(cursor, &h)) {
            if (size <= 16)
                return;
            cursor = (cursor | 0xFFF) + 1;
            continue;
        }
        g_stats.accesses++;
        if (!store && g_window == ACCESS_WINDOW_PRESENTATION && g_pres != 0)
            NotePresentationRead(h.item, h.container, ra, sub);
        if (sub == 0)
            g_stats.noSubsystem++;
        if (store) {
            g_stats.writes++;
            g_classifier.Write(h.item, h.entA, h.entB, sub, g_tick);
        } else {
            if (g_window != ACCESS_WINDOW_PRESENTATION) {
                NoteRead(h.item, sub);
                if (size <= 16 && h.end < end)
                    NoteReadSpan(h.end, end, sub);
            }
            unsigned flags = g_classifier.Read(h.item, h.entA, h.entB, sub, g_tick);
            if ((flags & READ_EARLIER_ENTITY) != 0 && g_passReads != 0)
                NotePassRead(h.item, sub, ra, flags, g_classifier.MatchLevel());
        }
        if (g_raw)
            RawLine(store ? "W" : "R", sub, h.item, h, cursor);
        if (size <= 16)
            return;
        if (n >= kSegmentLimit) {
            g_stats.rangeCapped++;
            return;
        }
        cursor = h.end > cursor ? h.end : cursor + 1;
    }
}

NS void ProcessSimEvent()
{
    if (!g_simPending)
        return;
    g_simPending = false;
    unsigned sub = Attribute((void*)g_accessMarks.simCaller);
    if (sub != kIgnored)
        g_classifier.Ordered(5, g_idSimEvent, sub, g_tick);
}

NS void EnterPass(int pass)
{
    if (g_draws != 0)
        FlushEntity();
    if (g_passDepth == kPassDepth)
        Die("accesslog: passes nest deeper than the pass stack\n");
    PassFrame* f = &g_passStack[g_passDepth++];
    f->pass = g_pass;
    f->entOrd = g_entOrd;
    g_pass = pass;
    g_entOrd = ++g_ordCounter;
    g_classifier.PushLevel();
    g_classifier.Entity(g_entOrd, g_entOrd);
}

NS void LeavePass()
{
    if (g_draws != 0)
        FlushEntity();
    if (g_passDepth == 0) {
        g_pass = ACCESS_PASS_NONE;
        g_entOrd = 0;
        g_classifier.Entity(0, 0);
        return;
    }
    PassFrame* f = &g_passStack[--g_passDepth];
    g_pass = f->pass;
    g_entOrd = f->entOrd;
    g_classifier.PopLevel();
}

NS void NextEntity()
{
    if (g_draws != 0)
        FlushEntity();
    g_entOrd = ++g_ordCounter;
    g_classifier.Entity(g_entOrd, g_classifier.Floor());
}

NS bool Mark(unsigned a)
{
    unsigned offset = a - (unsigned)&g_accessMarks;
    if (offset >= sizeof g_accessMarks)
        return false;
    if (offset == offsetof(AccessMarks, simCaller)) {
        ProcessSimEvent();
        g_simPending = true;
    } else if (offset == ACCESS_MARK_ENTITY) {
        NextEntity();
    } else if (offset == ACCESS_MARK_PASS_END) {
        LeavePass();
    } else if (offset >= ACCESS_MARK_PASS) {
        EnterPass((int)(offset - ACCESS_MARK_PASS));
    }
    return true;
}

NS inline void Touch(int store, void* p, unsigned size, void* ra)
{
    if (!g_on || g_busy || ThreadId() != g_simThread)
        return;
    if (store && Mark((unsigned)p))
        return;
    Record(store, (unsigned)p, size, ra);
}

NS void Range_(int store, const void* p, unsigned n, void* ra)
{
    if (n == 0 || !g_on || g_busy || ThreadId() != g_simThread)
        return;
    g_stats.ranges++;
    Record(store, (unsigned)p, n, ra);
}

NS void __cdecl OnEnter(void* site, const char*)
{
    if (g_depth++ == 0)
        g_curSite = (unsigned)site;
}

NS void __cdecl OnLeave()
{
    if (g_depth > 0)
        g_depth--;
}

NS Alloc* NewAlloc()
{
    if (g_poolFree != 0) {
        Alloc* a = &g_pool[g_poolFree - 1];
        g_poolFree = a->next;
        return a;
    }
    if (g_poolCount == g_poolCap) {
        g_poolCap = g_poolCap ? g_poolCap * 2 : 1u << 16;
        Alloc* bigger = (Alloc*)Zeroed(sizeof(Alloc) * g_poolCap);
        if (g_pool != 0) {
            memcpy(bigger, g_pool, sizeof(Alloc) * g_poolCount);
            Release(g_pool);
        }
        g_pool = bigger;
    }
    return &g_pool[g_poolCount++];
}

NS void AddAlloc(unsigned a, unsigned size, unsigned site)
{
    unsigned lo = 0, hi = g_siteCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (g_sites[mid].ra < site)
            lo = mid + 1;
        else
            hi = mid;
    }
    Alloc* al = NewAlloc();
    unsigned index = (unsigned)(al - g_pool) + 1;
    al->start = a;
    al->size = size ? size : 1;
    if (lo < g_siteCount && g_sites[lo].ra == site) {
        al->item = g_sites[lo].item;
        al->type = g_sites[lo].type;
        al->stride = g_sites[lo].stride;
        al->parts = g_sites[lo].parts;
        al->partCount = g_sites[lo].partCount;
    } else {
        al->parts = 0;
        al->partCount = 0;
        al->item = g_idUnresolvedBlock;
        al->type = kNone;
        al->stride = 0;
    }
    al->seq = ++g_allocSeq;
    unsigned g = (a - g_blockLo) / 8;
    g_granuleIndex[g] = index;
    SetBit(g);
}

NS void __cdecl OnAdded(void* p, unsigned size)
{
    Quiet quiet;
    unsigned a = (unsigned)p;
    if (!g_ready || a - g_blockLo >= kBlock)
        return;
    host_mutex_lock(g_allocLock);
    AddAlloc(a, size, g_depth > 0 ? g_curSite : 0);
    host_mutex_unlock(g_allocLock);
}

NS void __cdecl OnRemoved(void* p)
{
    Quiet quiet;
    unsigned a = (unsigned)p;
    if (!g_ready || a - g_blockLo >= kBlock)
        return;
    host_mutex_lock(g_allocLock);
    unsigned g = (a - g_blockLo) / 8;
    unsigned index = g_granuleIndex[g];
    if (index != 0) {
#if BT_DEFER_WRITES
        DeferDrop(g_pool[index - 1].start, g_pool[index - 1].size);
#endif
        g_pool[index - 1].next = g_poolFree;
        g_poolFree = index;
        g_granuleIndex[g] = 0;
        ClearBit(g);
    }
    host_mutex_unlock(g_allocLock);
}

const AllocHooks g_hooks = {OnEnter, OnLeave, OnAdded, OnRemoved};

struct Out {
    char* p;
    unsigned len, cap;
    NS void Add(const char* s, unsigned n)
    {
        if (len + n + 1 > cap) {
            cap = (len + n + 1) * 2;
            char* bigger = (char*)Zeroed(cap);
            memcpy(bigger, p, len);
            if (p != 0)
                Release(p);
            p = bigger;
        }
        memcpy(p + len, s, n);
        len += n;
    }
    NS void Add(const char* s) { Add(s, (unsigned)strlen(s)); }
    NS void Num(unsigned long long v)
    {
        char t[24];
        int n = 0;
        do {
            t[n++] = (char)('0' + v % 10);
            v /= 10;
        } while (v != 0);
        char r[24];
        for (int i = 0; i < n; i++)
            r[i] = t[n - 1 - i];
        Add(r, (unsigned)n);
    }
};

NS void WriteWhole(const char* path, const Out& out)
{
    Quiet quiet;
    void* h = host_file_create(path, 0);
    if (h == 0)
        return;
    host_file_write(h, out.p, out.len);
    host_file_close(h);
}

const char* const kPhaseNames[] = {"net", "units", "projectiles", "explosions", "players", "features", "wind", "meteors",
                                   "tail", "presentation", "pending"};
const char* const kEvidenceNames[] = {"read-after-write", "read-unwritten", "write-unread", "ordered", "read-after-write",
                                      "read-before-write"};

const char* SubName(unsigned sub)
{
    return sub < g_subCount ? g_subNames[sub] : "-";
}

struct Row {
    ClassifierEdge e;
};

int __cdecl CompareRows(const void* x, const void* y)
{
    const ClassifierEdge& a = ((const Row*)x)->e;
    const ClassifierEdge& b = ((const Row*)y)->e;
    int c = strcmp(SubName(a.writer), SubName(b.writer));
    if (c == 0)
        c = strcmp(SubName(a.reader), SubName(b.reader));
    if (c == 0)
        c = strcmp(g_itemNames[a.item], g_itemNames[b.item]);
    if (c == 0)
        c = strcmp(kPhaseNames[a.label], kPhaseNames[b.label]);
    if (c == 0)
        c = strcmp(kPhaseNames[a.from], kPhaseNames[b.from]);
    if (c == 0)
        c = strcmp(kEvidenceNames[a.evidence], kEvidenceNames[b.evidence]);
    return c;
}

int __cdecl ComparePres(const void* x, const void* y)
{
    const PresEntry* a = *(const PresEntry* const*)x;
    const PresEntry* b = *(const PresEntry* const*)y;
    int c = strcmp(g_itemNames[a->item], g_itemNames[b->item]);
    if (c == 0)
        c = strcmp(SubName(a->sub), SubName(b->sub));
    if (c == 0)
        c = strcmp(g_itemNames[a->container], g_itemNames[b->container]);
    if (c == 0)
        c = strcmp(a->func == 1 ? "-" : g_funcs[a->func - 2].name, b->func == 1 ? "-" : g_funcs[b->func - 2].name);
    return c;
}

NS void WritePresentation()
{
    PresEntry** rows = (PresEntry**)Zeroed(sizeof(PresEntry*) * (g_presUsed + 1));
    unsigned n = 0;
    for (unsigned i = 0; i < (1u << kPresBits); i++)
        if (g_pres[i].func != 0)
            rows[n++] = &g_pres[i];
    qsort(rows, n, sizeof(PresEntry*), ComparePres);
    Out out = {0, 0, 0};
    out.Add("item,reader,function,windows,events,container\n");
    for (unsigned i = 0; i < n; i++) {
        const PresEntry* e = rows[i];
        out.Add("\"");
        out.Add(g_itemNames[e->item]);
        out.Add("\",");
        out.Add(SubName(e->sub));
        out.Add(",\"");
        out.Add(e->func == 1 ? "-" : g_funcs[e->func - 2].name);
        out.Add("\",");
        out.Num(e->windows);
        out.Add(",");
        bool first = true;
        for (unsigned k = 0; k < kKinds; k++) {
            if (e->kinds[k] == 0)
                continue;
            if (!first)
                out.Add(";");
            first = false;
            out.Num(k);
            out.Add(":");
            out.Num(e->kinds[k]);
        }
        out.Add(",\"");
        out.Add(g_itemNames[e->container]);
        out.Add("\"\n");
    }
    WriteWhole("access.presentation.csv", out);
}

NS void WriteDraws()
{
    Out out = {0, 0, 0};
    out.Add("pass,stream,subsystem,function,entities,draws_min,draws_max,ticks,first_tick,tainted,items\n");
    for (unsigned i = 0; i < (1u << kDrawBits); i++) {
        const DrawAgg* a = &g_draws[i];
        if (a->used == 0)
            continue;
        out.Add(kPassNames[a->pass]);
        out.Add(",");
        out.Add(g_itemNames[g_idRng[a->stream]]);
        out.Add(",");
        out.Add(SubName(a->sub));
        out.Add(",\"");
        out.Add(a->func == 0 ? "-" : g_funcs[a->func - 1].name);
        out.Add("\",");
        out.Num(a->maxEntities);
        out.Add(",");
        out.Num(a->minDraws);
        out.Add(",");
        out.Num(a->maxDraws);
        out.Add(",");
        out.Num(a->ticks);
        out.Add(",");
        out.Num(a->firstTick);
        out.Add(",");
        out.Num(a->tainted);
        out.Add(",\"");
        for (unsigned k = 0; k < a->taintN; k++) {
            if (k != 0)
                out.Add(";");
            out.Add(g_itemNames[a->taint[k]]);
        }
        out.Add("\"\n");
    }
    WriteWhole("access.draws.csv", out);
}

NS void WritePassReads()
{
    Out out = {0, 0, 0};
    out.Add("pass,item,reader,function,evidence,count,first_tick\n");
    for (unsigned i = 0; i < (1u << kPassBits); i++) {
        const PassRead* r = &g_passReads[i];
        if (r->used == 0)
            continue;
        out.Add(kPassNames[r->pass]);
        out.Add(",\"");
        out.Add(g_itemNames[r->item]);
        out.Add("\",");
        out.Add(SubName(r->sub));
        out.Add(",\"");
        out.Add(r->func == 0 ? "-" : g_funcs[r->func - 1].name);
        out.Add("\",");
        out.Add(r->evidence == 0 ? "same-record" : "other-record");
        out.Add(",");
        out.Num(r->count);
        out.Add(",");
        out.Num(r->firstTick);
        out.Add("\n");
    }
    WriteWhole("access.pass.csv", out);
}

NS void WriteReads()
{
    Out out = {0, 0, 0};
    out.Add("item,reader,count,first_tick\n");
    for (unsigned i = 0; i < g_itemCount; i++) {
        for (unsigned sub = 0; sub < g_subCount; sub++) {
            const ReadCell* c = &g_reads[i * g_subCount + sub];
            if (c->count == 0)
                continue;
            out.Add("\"");
            out.Add(g_itemNames[i]);
            out.Add("\",");
            out.Add(SubName(sub));
            out.Add(",");
            out.Num(c->count);
            out.Add(",");
            out.Num(c->firstTick);
            out.Add("\n");
        }
    }
    WriteWhole("access.reads.csv", out);
}

}

#define ACCESS_CALLBACK(N)                                                                                 \
    extern "C" NS void __cdecl __asan_load##N(void* p) { Touch(0, p, N, __builtin_return_address(0)); } \
    extern "C" NS void __cdecl __asan_store##N(void* p) { Touch(1, p, N, __builtin_return_address(0)); }
ACCESS_CALLBACK(1)
ACCESS_CALLBACK(2)
ACCESS_CALLBACK(4)
ACCESS_CALLBACK(8)
ACCESS_CALLBACK(16)

extern "C" NS void __cdecl __asan_loadN(void* p, unsigned n) { Range_(0, p, n, __builtin_return_address(0)); }
extern "C" NS void __cdecl __asan_storeN(void* p, unsigned n) { Range_(1, p, n, __builtin_return_address(0)); }

extern "C" NS void* __cdecl __asan_memcpy(void* d, const void* s, unsigned n)
{
    Range_(0, s, n, __builtin_return_address(0));
    Range_(1, d, n, __builtin_return_address(0));
    void* r = d;
    __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n)::"memory");
    return r;
}

extern "C" NS void* __cdecl __asan_memset(void* d, int c, unsigned n)
{
    Range_(1, d, n, __builtin_return_address(0));
    void* r = d;
    __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
    return r;
}

extern "C" NS void* __cdecl __asan_memmove(void* d, const void* s, unsigned n)
{
    Range_(0, s, n, __builtin_return_address(0));
    Range_(1, d, n, __builtin_return_address(0));
    void* r = d;
    if ((unsigned)d <= (unsigned)s) {
        __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n)::"memory");
    } else {
        char* dd = (char*)d + n - 1;
        const char* ss = (const char*)s + n - 1;
        __asm__ volatile("std\n rep movsb\n cld" : "+D"(dd), "+S"(ss), "+c"(n)::"memory");
    }
    return r;
}

extern "C" NS void __cdecl __asan_init(void) {}
extern "C" NS void __cdecl __asan_version_mismatch_check_v8(void) {}
extern "C" NS void __cdecl __asan_report_load1(void*) {}
extern "C" NS void __cdecl __asan_after_dynamic_init(void) {}
extern "C" NS void __cdecl __asan_before_dynamic_init(const char*) {}
extern "C" NS void __cdecl __asan_alloca_poison(unsigned, unsigned) {}
extern "C" NS void __cdecl __asan_allocas_unpoison(unsigned, unsigned) {}
extern "C" NS void __cdecl __asan_handle_no_return(void) {}
extern "C" NS void __cdecl __asan_set_shadow_00(unsigned, unsigned) {}
extern "C" NS void __cdecl __asan_set_shadow_f8(unsigned, unsigned) {}

NS unsigned SiteWithin(void (*function)())
{
    unsigned start = (unsigned)function;
    for (unsigned i = 0; i < g_siteCount; i++)
        if (g_sites[i].ra > start && g_sites[i].ra - start < 0x100)
            return g_sites[i].ra;
    return 0;
}

struct PreHook {
    unsigned queues, arenas;
};

NS void __cdecl CountExisting(void*, unsigned size, void* ctx)
{
    PreHook* c = (PreHook*)ctx;
    if (size == sizeof(SimEventQueue))
        c->queues++;
    else if (size == sizeof(SimRadarArena))
        c->arenas++;
}

NS unsigned SiteOfCreator(void (*function)(), unsigned size)
{
    unsigned site = SiteWithin(function);
    if (site == 0) {
        char text[128];
        sprintf(text, "accesslog: no allocation site follows the creator of the pre-hook chunk of %u bytes\n", size);
        Die(text);
    }
    return site;
}

NS void __cdecl AdoptExisting(void* payload, unsigned size, void*)
{
    unsigned site = 0;
    if (size == sizeof(SimEventQueue))
        site = SiteOfCreator(SimEventsCreate, size);
    else if (size == sizeof(SimRadarArena))
        site = SiteOfCreator(SimCaptureCreate, size);
    AddAlloc((unsigned)payload, size, site);
}

NS void AdoptPreHookChunks()
{
    PreHook c = {0, 0};
    SimBlockForEach(CountExisting, &c);
    char text[128];
    if (c.queues > 1) {
        sprintf(text, "accesslog: %u pre-hook chunks have the size of the sim event queue (%u bytes)\n", c.queues, (unsigned)sizeof(SimEventQueue));
        Die(text);
    }
    if (c.arenas > 1) {
        sprintf(text, "accesslog: %u pre-hook chunks have the size of the sim radar arena (%u bytes)\n", c.arenas, (unsigned)sizeof(SimRadarArena));
        Die(text);
    }
    SimBlockForEach(AdoptExisting, 0);
}

NS void AccessLogInit()
{
    unsigned size;
    unsigned char* attr = ReadWhole("accesslog.attr", &size);
    unsigned char* items = ReadWhole("accesslog.items", &size);
    if (attr == 0 || items == 0)
        Die("accesslog: accesslog.attr and accesslog.items must be beside the exe (tools/accesslog.py tables)\n");
    LoadAttr(attr);
    LoadItems(items);
    unsigned funcSize;
    unsigned char* funcs = ReadWhole("accesslog.funcs", &funcSize);
    if (funcs != 0)
        LoadFuncs(funcs);
    LoadImage();
    g_allocLock = host_mutex_create();
    g_granuleIndex = (unsigned*)Zeroed(4 * kGranules);
    g_bits0 = (unsigned*)Zeroed(4 * (kGranules / 32 + 1));
    g_bits1 = (unsigned*)Zeroed(4 * (kGranules / 1024 + 1));
    g_bits2 = (unsigned*)Zeroed(4 * (kGranules / 32768 + 1));
    SimBlockInit();
    g_blockLo = (unsigned)SimBlockBase();
    if (!g_classifier.Init(g_itemCount, kEdgeBits, Zeroed))
        Die("accesslog: out of memory\n");
    g_draws = (DrawAgg*)Zeroed(sizeof(DrawAgg) << kDrawBits);
    g_passReads = (PassRead*)Zeroed(sizeof(PassRead) << kPassBits);
    g_reads = (ReadCell*)Zeroed(sizeof(ReadCell) * g_itemCount * g_subCount);
    g_raw = getenv("ACCESSLOG_RAW") != 0;
    if (g_raw) {
        g_rawFile = host_file_create("access.raw.txt", 0);
        g_rawBuf = (char*)Zeroed(kRawCap);
    }
#if BT_DEFER_WRITES
    DeferLoad();
#endif
    AdoptPreHookChunks();
    g_allocHooks = &g_hooks;
    g_ready = true;
}

NS void AccessLogWindow(int window)
{
    if (!g_ready)
        return;
    ProcessSimEvent();
    if (g_draws != 0)
        FlushEntity();
    g_pass = ACCESS_PASS_NONE;
    g_entOrd = 0;
    g_ordCounter = 0;
    g_passDepth = 0;
#if BT_DEFER_WRITES
    DeferFlush();
#endif
    if (g_started && g_window != ACCESS_WINDOW_OFF)
        g_classifier.EndWindow(g_tick);
    if (g_window == ACCESS_WINDOW_PRESENTATION && window != ACCESS_WINDOW_PRESENTATION) {
        g_presWindows++;
        g_presActiveWindows += g_presActive;
        for (unsigned k = 0; k < kKinds; k++)
            g_kindWindows[k] += g_prevKinds[k];
    }
    if (window == ACCESS_WINDOW_PRESENTATION && g_pres != 0) {
        ReadEventKinds(false);
        memcpy(g_prevKinds, g_tickKinds, sizeof g_prevKinds);
        g_presStamp++;
        g_presActive = false;
    }
    if (window == ACCESS_WINDOW_PENDING)
        memset(g_tickKinds, 0, sizeof g_tickKinds);
    if (window == ACCESS_WINDOW_PENDING)
        g_tick = g_lastMarkerTick + 1;
    g_window = window;
    if (window == ACCESS_WINDOW_OFF) {
        g_on = 0;
        return;
    }
    if (!g_started) {
        g_started = true;
        g_simThread = ThreadId();
    }
    g_blockLo = (unsigned)SimBlockBase();
    g_classifier.BeginWindow(window);
    g_on = 1;
}

NS void AccessLogMarker(int phase, unsigned tick)
{
    if (!g_ready)
        return;
#if BT_DEFER_WRITES
    DeferFlush();
#endif
    g_tick = tick;
    g_lastMarkerTick = tick;
    if (g_pres != 0)
        ReadEventKinds(true);
    if (g_window == ACCESS_WINDOW_PENDING)
        g_classifier.ResolveLabel(phase == 0 ? 0 : 1);
    g_on = 0;
}

NS void AccessLogEmit(int kind, void* ra)
{
    if (!g_on || ThreadId() != g_simThread)
        return;
    unsigned sub = Attribute(ra);
    if (sub == kIgnored)
        return;
    (kind == 0 ? g_stats.sounds : g_stats.uiRecords)++;
    g_classifier.Ordered(kind == 0 ? 3 : 4, kind == 0 ? g_idSound : g_idUi, sub, g_tick);
}

NS void AccessLogDraw(int stream, void* ra)
{
    if (!g_on || ThreadId() != g_simThread)
        return;
    unsigned sub = Attribute(ra);
    if (sub == kIgnored)
        return;
    unsigned s = (unsigned)stream < 3 ? (unsigned)stream : 2;
    g_stats.draws++;
    g_classifier.Ordered(s, g_idRng[s], sub, g_tick);
    if (g_draws == 0)
        return;
    unsigned pass = g_pass != ACCESS_PASS_NONE ? (unsigned)g_pass : g_window == ACCESS_WINDOW_PRESENTATION ? kPresentationPass : 0u;
    unsigned func = FunctionOf(ra);
    EntDraw* e = 0;
    for (unsigned i = 0; i < g_entN; i++)
        if (g_ent[i].pass == pass && g_ent[i].stream == s && g_ent[i].sub == sub && g_ent[i].func == func)
            e = &g_ent[i];
    if (e == 0) {
        if (g_entN == kEntDraws) {
            g_phaseOverflow = true;
            return;
        }
        e = &g_ent[g_entN++];
        e->pass = pass;
        e->stream = s;
        e->sub = sub;
        e->func = func;
        e->count = 0;
        e->tainted = 0;
        e->taintN = 0;
    }
    e->count++;
    if (g_entTaint) {
        e->tainted = 1;
        for (unsigned k = 0; k < g_taintN; k++)
            AddTaint(e->taint, &e->taintN, g_taint[k]);
    }
}

NS void AccessLogFinish()
{
    if (!g_ready)
        return;
    AccessLogWindow(ACCESS_WINDOW_OFF);
    g_ready = false;
    if (g_rawFile != 0) {
        host_file_write(g_rawFile, g_rawBuf, g_rawLen);
        host_file_close(g_rawFile);
    }
    unsigned total = g_classifier.Slots();
    Row* rows = (Row*)Zeroed(sizeof(Row) * (total / 2 + 1));
    unsigned n = 0;
    for (unsigned i = 0; i < total && n < total / 2; i++) {
        if (g_classifier.Edge(i, &rows[n].e))
            n++;
    }
    qsort(rows, n, sizeof(Row), CompareRows);
    Out out = {0, 0, 0};
    out.Add("writer,reader,item,phase,evidence,count,first_tick\n");
    Out cross = {0, 0, 0};
    cross.Add("writer,reader,item,phase_from,phase_to,evidence,count,first_tick\n");
    Out events = {0, 0, 0};
    events.Add("writer,reader,item,phase,evidence,count,first_tick\n");
    unsigned crossRows = 0, edgeRows = 0;
    for (unsigned i = 0; i < n; i++) {
        const ClassifierEdge& e = rows[i].e;
        bool isEvent = e.item == g_idSimEvent;
        Out& o = isEvent ? events : e.evidence >= EVIDENCE_CROSS ? cross : out;
        crossRows += !isEvent && e.evidence >= EVIDENCE_CROSS;
        edgeRows += !isEvent && e.evidence < EVIDENCE_CROSS;
        o.Add(SubName(e.writer));
        o.Add(",");
        o.Add(SubName(e.reader));
        o.Add(",\"");
        o.Add(g_itemNames[e.item]);
        o.Add("\",");
        if (!isEvent && e.evidence >= EVIDENCE_CROSS) {
            o.Add(kPhaseNames[e.from]);
            o.Add(",");
        }
        o.Add(kPhaseNames[e.label]);
        o.Add(",");
        o.Add(kEvidenceNames[e.evidence]);
        o.Add(",");
        o.Num(e.count);
        o.Add(",");
        o.Num(e.firstTick);
        o.Add("\n");
    }
    WriteWhole("access.edges.csv", out);
    WriteWhole("access.cross.csv", cross);
    WriteWhole("access.ordered.csv", events);
    FlushEntity();
    WriteDraws();
    WritePassReads();
    WriteReads();
    Out stats = {0, 0, 0};
    struct {
        const char* name;
        unsigned long long value;
    } lines[] = {{"accesses", g_stats.accesses}, {"unknown_subsystem", g_stats.noSubsystem}, {"ignored_helpers", g_stats.ignored},
                 {"code", g_stats.code}, {"constants_unnamed", g_stats.constants}, {"other_memory", g_stats.other},
                 {"unresolved_image", g_stats.unresolvedImage}, {"unresolved_block", g_stats.unresolvedBlock},
                 {"writes", g_stats.writes}, {"range_calls", g_stats.ranges}, {"range_capped", g_stats.rangeCapped},
                 {"random_draws", g_stats.draws}, {"sound_records", g_stats.sounds}, {"ui_records", g_stats.uiRecords},
                 {"edges", edgeRows}, {"cross_rows", crossRows}, {"draw_rows", g_drawsUsed}, {"pass_read_rows", g_passReadsUsed},
                 {"phase_overflow", g_phaseOverflow ? 1u : 0u}, {"overflow", g_classifier.Overflowed() ? 1u : 0u}, {"allocations", g_allocSeq},
                 {"presentation_windows", g_presWindows}, {"presentation_active_windows", g_presActiveWindows},
                 {"presentation_rows", g_presUsed}, {"presentation_overflow", g_presOverflow ? 1u : 0u}};
    for (unsigned i = 0; i < sizeof lines / sizeof lines[0]; i++) {
        stats.Add(lines[i].name);
        stats.Add(" ");
        stats.Num(lines[i].value);
        stats.Add("\n");
    }
    for (unsigned k = 0; k < kKinds; k++) {
        stats.Add("presentation_kind_");
        stats.Num(k);
        stats.Add(" ");
        stats.Num(g_kindWindows[k]);
        stats.Add("\n");
    }
#if BT_DEFER_WRITES
    struct {
        const char* name;
        unsigned long long value;
    } held[] = {{"defer_edges", g_deferEdgeCount}, {"defer_created", g_dStats.created}, {"defer_held", g_dStats.held},
                {"defer_applied", g_dStats.applied}, {"defer_superseded", g_dStats.superseded},
                {"defer_third_stores", g_dStats.thirdStores}, {"defer_freed", g_dStats.freed}};
    for (unsigned i = 0; i < sizeof held / sizeof held[0]; i++) {
        stats.Add(held[i].name);
        stats.Add(" ");
        stats.Num(held[i].value);
        stats.Add("\n");
    }
#endif
    WriteWhole("access.stats.txt", stats);
    if (g_pres != 0)
        WritePresentation();
}
