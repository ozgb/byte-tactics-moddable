#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block_scan.h"
#include "block_spans.h"
#include "../../interfaces/host.h"
#include "../state/block.h"
#include "../state/sim_globals.h"

#include "../debug/debug_lib.h"

namespace {

const unsigned kRegistryBits = 19;
const unsigned kRegistrySize = 1u << kRegistryBits;
const unsigned kRowBits = 21;
const unsigned kRowSize = 1u << kRowBits;

struct Entry {
    unsigned addr;
    unsigned size;
    unsigned site;
    const char* tag;
};

struct Row {
    unsigned holder;
    const char* holderTag;
    unsigned offset;
    unsigned target;
    const char* targetTag;
    unsigned ticks;
    unsigned lastTick;
    unsigned targetMin;
    unsigned targetMax;
    unsigned globals;
};

Entry* g_registry;
unsigned g_registryCount;
Span* g_spans;
unsigned g_spanBase;
bool g_verify;
Span* g_refSpans;
unsigned g_refSpanCount;
unsigned g_refSpanMin;
unsigned g_refSpanMax;
unsigned* g_refPages;
Row* g_refRows;
unsigned* g_refIndex;
unsigned g_refRowCount;
unsigned g_refCursor;
unsigned g_spanCount;
unsigned g_spanMin;
unsigned g_spanMax;
unsigned* g_pages;
const unsigned kPageShift = 12;
const unsigned kPageWords = (1u << (32 - kPageShift)) / 32;
bool g_dirty = true;
Row* g_rows;
unsigned* g_index;
unsigned g_rowCount;
unsigned g_cursor;
unsigned g_scanned;
__declspec(thread) unsigned g_depth;
__declspec(thread) unsigned g_curSite;
__declspec(thread) const char* g_curTag;
void* g_scanLock;
unsigned g_imageLo;
unsigned g_imageHi;
unsigned long long g_scanTotal;
unsigned long long g_scanMax;
unsigned g_words;
unsigned g_hits;
unsigned g_every = 1;
const unsigned kLastTicks = 64;

void Die(const char* text)
{
    host_log(text);
    host_exit(2);
}

void* AllocZeroed(unsigned bytes)
{
    void* p = host_mem_reserve(bytes);
    if (p != 0)
        p = host_mem_commit(p, bytes);
    if (p == 0)
        Die("blockscan: cannot allocate the scan tables\n");
    return p;
}

unsigned Slot(unsigned addr)
{
    return ((addr >> 3) * 2654435761u) >> (32 - kRegistryBits);
}

void __cdecl OnEnter(void* site, const char* tag)
{
    unsigned t = (unsigned)tag;
    if (t < g_imageLo || t >= g_imageHi)
        tag = 0;
    if (g_depth++ == 0) {
        g_curSite = (unsigned)site;
        g_curTag = tag;
    } else if (g_curTag == 0) {
        g_curTag = tag;
    }
}

void __cdecl OnLeave()
{
    if (g_depth > 0)
        g_depth--;
}

bool Inside(unsigned addr)
{
    return SimOwns((const void*)addr);
}

unsigned SpanLowerBound(unsigned addr)
{
    unsigned lo = 0;
    unsigned hi = g_spanCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (g_spans[mid].start < addr)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

void SpanErase(unsigned addr)
{
    unsigned i = SpanLowerBound(addr);
    if (i >= g_spanCount || g_spans[i].start != addr)
        return;
    memmove(&g_spans[i], &g_spans[i + 1], (g_spanCount - i - 1) * sizeof(Span));
    g_spanCount--;
    g_dirty = true;
}

void SpanInsert(unsigned addr, unsigned size, unsigned slot)
{
    unsigned i = SpanLowerBound(addr);
    memmove(&g_spans[i + 1], &g_spans[i], (g_spanCount - i) * sizeof(Span));
    g_spans[i].start = addr;
    g_spans[i].end = addr + size;
    g_spans[i].index = slot;
    g_spanCount++;
    g_dirty = true;
}

void SpanMoved(unsigned addr, unsigned slot)
{
    unsigned i = SpanLowerBound(addr);
    if (i < g_spanCount && g_spans[i].start == addr)
        g_spans[i].index = slot;
}

void __cdecl OnAdded(void* p, unsigned size)
{
    unsigned addr = (unsigned)p;
    host_mutex_lock(g_scanLock);
    unsigned i = Slot(addr);
    while (g_registry[i].addr != 0 && g_registry[i].addr != addr)
        i = (i + 1) & (kRegistrySize - 1);
    if (g_registry[i].addr == 0) {
        if (g_registryCount >= kRegistrySize - kRegistrySize / 8)
            Die("blockscan: allocation registry is full, raise kRegistryBits in src/harness/block_scan.cpp\n");
        g_registryCount++;
    }
    bool live = g_depth > 0;
    g_registry[i].addr = addr;
    g_registry[i].size = size;
    g_registry[i].site = live ? g_curSite : 0;
    g_registry[i].tag = live ? g_curTag : 0;
    if (!Inside(addr)) {
        SpanErase(addr);
        if (size != 0)
            SpanInsert(addr, size, i);
    }
    host_mutex_unlock(g_scanLock);
}

void __cdecl OnRemoved(void* p)
{
    unsigned addr = (unsigned)p;
    host_mutex_lock(g_scanLock);
    unsigned i = Slot(addr);
    while (g_registry[i].addr != 0 && g_registry[i].addr != addr)
        i = (i + 1) & (kRegistrySize - 1);
    if (g_registry[i].addr == 0) {
        host_mutex_unlock(g_scanLock);
        return;
    }
    if (!Inside(addr))
        SpanErase(addr);
    unsigned hole = i;
    unsigned j = i;
    for (;;) {
        j = (j + 1) & (kRegistrySize - 1);
        if (g_registry[j].addr == 0)
            break;
        unsigned home = Slot(g_registry[j].addr);
        bool movable = hole <= j ? (home <= hole || home > j) : (home <= hole && home > j);
        if (movable) {
            g_registry[hole] = g_registry[j];
            if (!Inside(g_registry[hole].addr))
                SpanMoved(g_registry[hole].addr, hole);
            hole = j;
        }
    }
    g_registry[hole].addr = 0;
    g_registryCount--;
    host_mutex_unlock(g_scanLock);
}

const AllocHooks g_hooks = { OnEnter, OnLeave, OnAdded, OnRemoved };

const Entry* Find(unsigned addr)
{
    unsigned i = Slot(addr);
    while (g_registry[i].addr != 0) {
        if (g_registry[i].addr == addr)
            return &g_registry[i];
        i = (i + 1) & (kRegistrySize - 1);
    }
    return 0;
}

int __cdecl CompareSpans(const void* a, const void* b)
{
    unsigned x = ((const Span*)a)->start;
    unsigned y = ((const Span*)b)->start;
    return x < y ? -1 : x > y ? 1 : 0;
}

void RebuildSpans()
{
    g_spanCount = 0;
    for (unsigned i = 0; i < kRegistrySize; i++) {
        const Entry& e = g_registry[i];
        if (e.addr == 0 || e.size == 0 || Inside(e.addr))
            continue;
        g_spans[g_spanCount].start = e.addr;
        g_spans[g_spanCount].end = e.addr + e.size;
        g_spans[g_spanCount].index = i;
        g_spanCount++;
    }
    qsort(g_spans, g_spanCount, sizeof(Span), CompareSpans);
}

void SummarizeSpans()
{
    g_spanMin = 0xffffffffu;
    g_spanMax = 0;
    memset(g_pages, 0, kPageWords * sizeof(unsigned));
    for (unsigned i = 0; i < g_spanCount; i++) {
        const Span& sp = g_spans[i];
        for (unsigned pg = sp.start >> kPageShift; pg <= (sp.end - 1) >> kPageShift; pg++)
            g_pages[pg >> 5] |= 1u << (pg & 31);
        if (sp.start < g_spanMin)
            g_spanMin = sp.start;
        if (sp.end > g_spanMax)
            g_spanMax = sp.end;
    }
    g_dirty = false;
}

template <class T> void Swap(T& a, T& b)
{
    T t = a;
    a = b;
    b = t;
}

void SwapBanks()
{
    Swap(g_spans, g_refSpans);
    Swap(g_spanCount, g_refSpanCount);
    Swap(g_spanMin, g_refSpanMin);
    Swap(g_spanMax, g_refSpanMax);
    Swap(g_pages, g_refPages);
    Swap(g_rows, g_refRows);
    Swap(g_index, g_refIndex);
    Swap(g_rowCount, g_refRowCount);
    Swap(g_cursor, g_refCursor);
}

unsigned HashRow(const Row& r)
{
    unsigned h = r.holder * 2654435761u;
    h = (h ^ (unsigned)r.holderTag) * 2246822519u;
    h = (h ^ r.offset) * 3266489917u;
    h = (h ^ r.target) * 668265263u;
    h = (h ^ (unsigned)r.targetTag) * 374761393u;
    return (h ^ (h >> 15)) >> (32 - kRowBits);
}

struct HolderCount {
    unsigned holder;
    const char* tag;
    unsigned rows;
};

void CleanTag(const char* tag, char* out);

void ReportTopHolders()
{
    const unsigned kSlots = 4096;
    const unsigned kTop = 10;
    HolderCount* counts = (HolderCount*)calloc(kSlots, sizeof(HolderCount));
    if (counts == 0)
        return;
    for (unsigned i = 0; i < g_rowCount; i++) {
        const Row& r = g_rows[i];
        unsigned h = ((r.holder * 2654435761u) ^ (unsigned)r.holderTag) * 2246822519u;
        unsigned j = (h ^ (h >> 15)) & (kSlots - 1);
        for (unsigned probes = 0; probes < kSlots; probes++) {
            HolderCount& c = counts[j];
            if (c.rows == 0) {
                c.holder = r.holder;
                c.tag = r.holderTag;
                c.rows = 1;
                break;
            }
            if (c.holder == r.holder && c.tag == r.holderTag) {
                c.rows++;
                break;
            }
            j = (j + 1) & (kSlots - 1);
        }
    }
    host_log("blockscan: holders with the most rows:\n");
    for (unsigned k = 0; k < kTop; k++) {
        unsigned best = 0;
        for (unsigned j = 1; j < kSlots; j++)
            if (counts[j].rows > counts[best].rows)
                best = j;
        if (counts[best].rows == 0)
            break;
        char tag[96];
        char msg[200];
        CleanTag(counts[best].tag, tag);
        sprintf(msg, "blockscan:   0x%08x %s: %u rows\n", counts[best].holder, tag, counts[best].rows);
        host_log(msg);
        counts[best].rows = 0;
    }
    free(counts);
}

bool SameKey(const Row& r, const Row& key)
{
    return r.holder == key.holder && r.holderTag == key.holderTag && r.offset == key.offset && r.target == key.target
        && r.targetTag == key.targetTag && r.globals == key.globals;
}

void Touch(Row& r, unsigned targetOffset, unsigned tick)
{
    if (r.lastTick != tick) {
        r.lastTick = tick;
        r.ticks++;
    }
    if (targetOffset < r.targetMin)
        r.targetMin = targetOffset;
    if (targetOffset > r.targetMax)
        r.targetMax = targetOffset;
}

void Note(const Row& key, unsigned targetOffset, unsigned tick)
{
    if (g_cursor < g_rowCount && SameKey(g_rows[g_cursor], key)) {
        Touch(g_rows[g_cursor++], targetOffset, tick);
        return;
    }
    unsigned i = HashRow(key);
    for (;;) {
        unsigned n = g_index[i];
        if (n == 0) {
            if (g_rowCount >= kRowSize - kRowSize / 4) {
                ReportTopHolders();
                Die("blockscan: row table is full, raise kRowBits in src/harness/block_scan.cpp\n");
            }
            Row& r = g_rows[g_rowCount];
            r = key;
            r.ticks = 1;
            r.lastTick = tick;
            r.targetMin = targetOffset;
            r.targetMax = targetOffset;
            g_index[i] = ++g_rowCount;
            g_cursor = g_rowCount;
            return;
        }
        if (SameKey(g_rows[n - 1], key)) {
            Touch(g_rows[n - 1], targetOffset, tick);
            g_cursor = n;
            return;
        }
        i = (i + 1) & (kRowSize - 1);
    }
}

struct WalkCtx {
    unsigned tick;
};

void Probe(unsigned v, unsigned off, Row& key, unsigned tick)
{
    unsigned before = (v - 1) >> kPageShift;
    unsigned pg = v >> kPageShift;
    if ((g_pages[before >> 5] & (1u << (before & 31))) == 0 && (g_pages[pg >> 5] & (1u << (pg & 31))) == 0)
        return;
    unsigned targetOffset;
    const Span* s = LocateSpan(g_spans, g_spanCount, v, &targetOffset);
    if (s == 0)
        return;
    g_hits++;
    const Entry& t = g_registry[s->index];
    key.offset = off;
    key.target = t.site;
    key.targetTag = t.tag;
    Note(key, targetOffset, tick);
}

inline void ProbeWord(unsigned a, unsigned b, unsigned off, Row& key, unsigned tick)
{
    if ((a | b) == 0)
        return;
    const unsigned spanMin = g_spanMin;
    const unsigned spanRange = g_spanMax - g_spanMin;
    unsigned v1 = (a >> 8) | (b << 24);
    unsigned v2 = (a >> 16) | (b << 16);
    unsigned v3 = (a >> 24) | (b << 8);
    if (a - spanMin <= spanRange)
        Probe(a, off, key, tick);
    if (v1 - spanMin <= spanRange)
        Probe(v1, off + 1, key, tick);
    if (v2 - spanMin <= spanRange)
        Probe(v2, off + 2, key, tick);
    if (v3 - spanMin <= spanRange)
        Probe(v3, off + 3, key, tick);
}

typedef int Lanes __attribute__((vector_size(16)));
typedef float Floats __attribute__((vector_size(16)));

__attribute__((target("sse2"))) unsigned ScanLanes(const unsigned char* bytes, unsigned size, Row& key, unsigned tick)
{
    const unsigned bound = (g_spanMax - g_spanMin) ^ 0x80000000u;
    const Lanes bias = { (int)0x80000000, (int)0x80000000, (int)0x80000000, (int)0x80000000 };
    const Lanes lows = { (int)g_spanMin, (int)g_spanMin, (int)g_spanMin, (int)g_spanMin };
    const Lanes bounds = { (int)bound, (int)bound, (int)bound, (int)bound };
    unsigned off = 0;
    for (; off + 19 <= size; off += 16) {
        Lanes v[4];
        unsigned outside = 0;
        for (unsigned k = 0; k < 4; k++) {
            memcpy(&v[k], bytes + off + k, sizeof v[k]);
            Lanes out = ((v[k] - lows) ^ bias) > bounds;
            outside |= (unsigned)__builtin_ia32_movmskps((Floats)out) << (4 * k);
        }
        unsigned candidates = ~outside & 0xffff;
        if (candidates == 0)
            continue;
        unsigned values[16];
        memcpy(values, v, sizeof values);
        do {
            unsigned bit = __builtin_ctz(candidates);
            candidates &= candidates - 1;
            unsigned value = values[bit];
            unsigned page = value >> kPageShift;
            unsigned before = (value - 1) >> kPageShift;
            if ((g_pages[page >> 5] >> (page & 31) | g_pages[before >> 5] >> (before & 31)) & 1)
                Probe(value, off + (bit >> 2) + 4 * (bit & 3), key, tick);
        } while (candidates != 0);
    }
    return off;
}

void ScanFast(const unsigned char* bytes, unsigned size, Row& key, unsigned tick)
{
    const unsigned spanMin = g_spanMin;
    const unsigned spanRange = g_spanMax - g_spanMin;
    unsigned off = ScanLanes(bytes, size, key, tick);
    for (; off + 8 <= size; off += 4) {
        unsigned q[2];
        memcpy(q, bytes + off, sizeof q);
        ProbeWord(q[0], q[1], off, key, tick);
    }
    for (; off + 4 <= size; off++) {
        unsigned v;
        memcpy(&v, bytes + off, 4);
        if (v - spanMin <= spanRange)
            Probe(v, off, key, tick);
    }
}

void ScanPlain(const unsigned char* bytes, unsigned size, Row& key, unsigned tick)
{
    for (unsigned off = 0; off + 4 <= size; off++) {
        unsigned v;
        memcpy(&v, bytes + off, 4);
        if (v < g_spanMin || v > g_spanMax)
            continue;
        Probe(v, off, key, tick);
    }
}

void __cdecl Visit(void* payload, unsigned size, void* ctx)
{
    unsigned tick = ((WalkCtx*)ctx)->tick;
    const Entry* holder = Find((unsigned)payload);
    Row key;
    memset(&key, 0, sizeof key);
    key.globals = payload == (void*)g_simGlobals;
    key.holder = holder != 0 ? holder->site : 0;
    key.holderTag = holder != 0 ? holder->tag : 0;
    if (size >= 4)
        g_words += size - 3;
    ScanFast((const unsigned char*)payload, size, key, tick);
    if (g_verify) {
        SwapBanks();
        ScanPlain((const unsigned char*)payload, size, key, tick);
        SwapBanks();
    }
}

void CleanTag(const char* tag, char* out)
{
    if (tag == 0 || !host_mem_readable(tag, 1)) {
        strcpy(out, "-");
        return;
    }
    unsigned n = 0;
    while (n < 80 && host_mem_readable(tag + n, 1) && tag[n] != '\0') {
        unsigned char c = (unsigned char)tag[n];
        out[n] = c < 32 || c > 126 ? ' ' : (char)c;
        n++;
    }
    out[n] = '\0';
    if (n == 0)
        strcpy(out, "-");
}

void PrepareSpans()
{
    if ((unsigned)SimBlockBase() != g_spanBase) {
        g_spanBase = (unsigned)SimBlockBase();
        RebuildSpans();
        g_dirty = true;
    }
    if (g_dirty) {
        SummarizeSpans();
        g_dirty = false;
    }
    if (!g_verify)
        return;
    SwapBanks();
    RebuildSpans();
    SummarizeSpans();
    SwapBanks();
    bool same = g_spanCount == g_refSpanCount && g_spanMin == g_refSpanMin && g_spanMax == g_refSpanMax;
    for (unsigned i = 0; same && i < g_spanCount; i++)
        same = g_spans[i].start == g_refSpans[i].start && g_spans[i].end == g_refSpans[i].end && g_spans[i].index == g_refSpans[i].index;
    if (!same)
        Die("blockscan: the maintained spans differ from the spans rebuilt from the registry\n");
}

struct OutRow {
    char holder[16];
    char holderTag[96];
    unsigned offset;
    char target[16];
    char targetTag[96];
    unsigned ticks;
    unsigned targetMin;
    unsigned targetMax;
};

int __cdecl CompareOut(const void* a, const void* b)
{
    const OutRow* x = (const OutRow*)a;
    const OutRow* y = (const OutRow*)b;
    int c = strcmp(x->holder, y->holder);
    if (c == 0)
        c = strcmp(x->holderTag, y->holderTag);
    if (c == 0)
        c = x->offset < y->offset ? -1 : x->offset > y->offset ? 1 : 0;
    if (c == 0)
        c = strcmp(x->target, y->target);
    if (c == 0)
        c = strcmp(x->targetTag, y->targetTag);
    return c;
}

OutRow* CollectRows(unsigned* count)
{
    OutRow* out = (OutRow*)malloc(sizeof(OutRow) * (g_rowCount + 1));
    if (out == 0)
        Die("blockscan: out of memory\n");
    unsigned n = 0;
    for (unsigned i = 0; i < g_rowCount; i++) {
        const Row& r = g_rows[i];
        OutRow& o = out[n++];
        if (r.globals)
            strcpy(o.holder, "SimGlobals");
        else
            sprintf(o.holder, "0x%08x", r.holder);
        CleanTag(r.holderTag, o.holderTag);
        o.offset = r.offset;
        sprintf(o.target, "0x%08x", r.target);
        CleanTag(r.targetTag, o.targetTag);
        o.ticks = r.ticks;
        o.targetMin = r.targetMin;
        o.targetMax = r.targetMax;
    }
    qsort(out, n, sizeof(OutRow), CompareOut);
    unsigned m = 0;
    for (unsigned i = 0; i < n; i++) {
        if (m > 0 && CompareOut(&out[m - 1], &out[i]) == 0) {
            if (out[i].ticks > out[m - 1].ticks)
                out[m - 1].ticks = out[i].ticks;
            if (out[i].targetMin < out[m - 1].targetMin)
                out[m - 1].targetMin = out[i].targetMin;
            if (out[i].targetMax > out[m - 1].targetMax)
                out[m - 1].targetMax = out[i].targetMax;
        } else {
            out[m++] = out[i];
        }
    }
    *count = m;
    return out;
}

bool SameRow(const OutRow& x, const OutRow& y)
{
    return CompareOut(&x, &y) == 0 && x.ticks == y.ticks && x.targetMin == y.targetMin && x.targetMax == y.targetMax;
}

}

void BlockScanEnable(unsigned every, bool verify)
{
    g_verify = verify;
    if (verify) {
        g_refSpans = (Span*)AllocZeroed(sizeof(Span) * kRegistrySize);
        g_refRows = (Row*)AllocZeroed(sizeof(Row) * kRowSize + sizeof(unsigned) * kRowSize);
        g_refIndex = (unsigned*)(g_refRows + kRowSize);
        g_refPages = (unsigned*)AllocZeroed(kPageWords * sizeof(unsigned));
    }
    g_every = every != 0 ? every : 1;
    g_registry = (Entry*)AllocZeroed(sizeof(Entry) * kRegistrySize);
    g_spans = (Span*)AllocZeroed(sizeof(Span) * kRegistrySize);
    g_rows = (Row*)AllocZeroed(sizeof(Row) * kRowSize + sizeof(unsigned) * kRowSize);
    g_index = (unsigned*)(g_rows + kRowSize);
    g_pages = (unsigned*)AllocZeroed(kPageWords * sizeof(unsigned));
    g_scanLock = host_mutex_create();
    const void* base = 0;
    unsigned int size = 0;
    host_image_range(&base, &size);
    g_imageLo = (unsigned)base;
    g_imageHi = (unsigned)base + size;
    g_allocHooks = &g_hooks;
}

void BlockScanTick(unsigned tick, unsigned endTick)
{
    if (g_allocHooks == 0)
        return;
    if (tick % g_every != 0 && tick + kLastTicks <= endTick)
        return;
    unsigned long long t0 = host_time_counter();
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    host_mutex_lock(g_scanLock);
    PrepareSpans();
    host_mutex_unlock(g_scanLock);
    g_cursor = 0;
    g_refCursor = 0;
    WalkCtx ctx;
    ctx.tick = tick;
    SimBlockForEach(Visit, &ctx);
    host_mutex_unlock(cs);
    unsigned long long dt = host_time_counter() - t0;
    g_scanTotal += dt;
    if (dt > g_scanMax)
        g_scanMax = dt;
    g_scanned++;
}

void BlockScanWrite(const char* commit)
{
    if (g_allocHooks == 0)
        return;
    unsigned n = 0;
    OutRow* out = CollectRows(&n);
    if (g_verify) {
        unsigned m = 0;
        SwapBanks();
        OutRow* plain = CollectRows(&m);
        SwapBanks();
        bool same = n == m;
        for (unsigned i = 0; same && i < n; i++)
            same = SameRow(out[i], plain[i]);
        if (!same)
            Die("blockscan: the fast scan and the plain scan found different rows\n");
        free(plain);
        host_log("blockscan: the fast scan and the plain scan found the same rows\n");
    }
    FILE* f = fopen("blockptrs.txt", "wb");
    if (f == 0)
        Die("blockscan: cannot write blockptrs.txt\n");
    fprintf(f, "bt-blockptrs 2\ncommit\t%s\nevery\t%u\nticks\t%u\n", commit, g_every, g_scanned);
    fprintf(f, "holder\tholder_tag\toffset\ttarget\ttarget_tag\tticks\ttarget_offset_min\ttarget_offset_max\n");
    for (unsigned i = 0; i < n; i++)
        fprintf(f, "%s\t%s\t0x%x\t%s\t%s\t%u\t0x%x\t0x%x\n", out[i].holder, out[i].holderTag, out[i].offset, out[i].target,
                out[i].targetTag, out[i].ticks, out[i].targetMin, out[i].targetMax);
    fclose(f);
    free(out);
    char msg[256];
    sprintf(msg, "blockscan: %u ticks, %u rows, %u table rows, scan counter total %I64u max %I64u, %u words, %u hits, %u live allocations\n",
            g_scanned, n, g_rowCount, g_scanTotal, g_scanMax, g_words, g_hits, g_registryCount);
    host_log(msg);
}
