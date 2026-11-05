#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../interfaces/host.h"
#include "access_log.h"
#include "classifier.h"
#include "../../src/state/alloc_hooks.h"
#include "../../src/state/sim_events.h"

#define NS __attribute__((no_sanitize("address")))

void* __cdecl SimBlockBase();
void __cdecl SimBlockInit();

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
};
struct Alloc {
    unsigned start, size, item, type, stride, seq, next;
};
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
unsigned g_idUnresolvedImage, g_idUnresolvedBlock, g_idRng[3], g_idSound, g_idUi;
Type* g_types;
unsigned g_typeCount;
Region* g_statics;
unsigned g_staticCount;
Base* g_bases;
unsigned g_baseCount;
Site* g_sites;
unsigned g_siteCount;

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
    g_itemNames = (const char**)Zeroed(sizeof(char*) * (strings + 8));
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
    g_itemNames[g_idUnresolvedImage] = "unresolved:image";
    g_itemNames[g_idUnresolvedBlock] = "unresolved:block";
    g_itemNames[g_idRng[0]] = "rng.parkMiller";
    g_itemNames[g_idRng[1]] = "rng.crtHoldrand";
    g_itemNames[g_idRng[2]] = "rng.crtLoad";
    g_itemNames[g_idSound] = "sound";
    g_itemNames[g_idUi] = "ui";
    g_itemCount = strings + 7;
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
            RefineInRegion(al->item, al->type, al->stride, al->start, al->size, a - al->start, a, h);
            h->entA = al->seq;
            if (al->type == kNone || al->stride == 0)
                h->end = al->start + al->size;
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

NS void Record(int store, unsigned a, unsigned size, void* ra)
{
    unsigned sub = Attribute(ra);
    if (sub == kIgnored) {
        g_stats.ignored++;
        return;
    }
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
            g_classifier.Read(h.item, h.entA, h.entB, sub, g_tick);
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

NS inline void Touch(int store, void* p, unsigned size, void* ra)
{
    if (!g_on || g_busy || ThreadId() != g_simThread)
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

NS void __cdecl OnAdded(void* p, unsigned size)
{
    Quiet quiet;
    unsigned a = (unsigned)p;
    if (!g_ready || a - g_blockLo >= kBlock)
        return;
    host_mutex_lock(g_allocLock);
    unsigned site = g_depth > 0 ? g_curSite : 0;
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
    } else {
        al->item = g_idUnresolvedBlock;
        al->type = kNone;
        al->stride = 0;
    }
    al->seq = ++g_allocSeq;
    unsigned g = (a - g_blockLo) / 8;
    g_granuleIndex[g] = index;
    SetBit(g);
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
const char* const kEvidenceNames[] = {"read-after-write", "read-unwritten", "write-unread", "ordered"};

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
    g_raw = getenv("ACCESSLOG_RAW") != 0;
    if (g_raw) {
        g_rawFile = host_file_create("access.raw.txt", 0);
        g_rawBuf = (char*)Zeroed(kRawCap);
    }
    g_allocHooks = &g_hooks;
    g_ready = true;
}

NS void AccessLogWindow(int window)
{
    if (!g_ready)
        return;
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
    for (unsigned i = 0; i < n; i++) {
        const ClassifierEdge& e = rows[i].e;
        out.Add(SubName(e.writer));
        out.Add(",");
        out.Add(SubName(e.reader));
        out.Add(",\"");
        out.Add(g_itemNames[e.item]);
        out.Add("\",");
        out.Add(kPhaseNames[e.label]);
        out.Add(",");
        out.Add(kEvidenceNames[e.evidence]);
        out.Add(",");
        out.Num(e.count);
        out.Add(",");
        out.Num(e.firstTick);
        out.Add("\n");
    }
    WriteWhole("access.edges.csv", out);
    Out stats = {0, 0, 0};
    struct {
        const char* name;
        unsigned long long value;
    } lines[] = {{"accesses", g_stats.accesses}, {"unknown_subsystem", g_stats.noSubsystem}, {"ignored_helpers", g_stats.ignored},
                 {"code", g_stats.code}, {"constants_unnamed", g_stats.constants}, {"other_memory", g_stats.other},
                 {"unresolved_image", g_stats.unresolvedImage}, {"unresolved_block", g_stats.unresolvedBlock},
                 {"writes", g_stats.writes}, {"range_calls", g_stats.ranges}, {"range_capped", g_stats.rangeCapped},
                 {"random_draws", g_stats.draws}, {"sound_records", g_stats.sounds}, {"ui_records", g_stats.uiRecords},
                 {"edges", n}, {"overflow", g_classifier.Overflowed() ? 1u : 0u}, {"allocations", g_allocSeq},
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
    WriteWhole("access.stats.txt", stats);
    if (g_pres != 0)
        WritePresentation();
}
