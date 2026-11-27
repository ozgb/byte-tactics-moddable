#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "load_data.h"
#include "../graphics/gaf_frame.h"

namespace {

enum Kind { KIND_GAF, KIND_TABLE, KIND_FRAME };

struct Registered {
    const unsigned char* native;
    unsigned size;
    int kind;
    int mapIndex;
};

struct Mapped {
    const unsigned char* native;
    unsigned size;
    unsigned linear;
    unsigned hash;
    int live;
};

unsigned char* s_region;
unsigned s_base;
unsigned s_size;
unsigned s_bump;
void (__stdcall* s_fatal)(const char* text);

Registered* s_registered;
unsigned s_registeredCount;
unsigned s_registeredCapacity;
Mapped* s_mapped;
unsigned s_mappedCount;
unsigned s_mappedCapacity;
unsigned s_live;

void Fatal(const char* text)
{
    if (s_fatal != nullptr)
        s_fatal(text);
    fputs(text, stderr);
    exit(2);
}

void FatalAddress(const char* what, unsigned address)
{
    char text[100];
    sprintf(text, "load_data: %x %s", address, what);
    Fatal(text);
}

unsigned FindRegistered(const void* p)
{
    unsigned lo = 0;
    unsigned hi = s_registeredCount;
    const unsigned char* at = (const unsigned char*)p;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (s_registered[mid].native <= at)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return (unsigned)-1;
    Registered& r = s_registered[lo - 1];
    return at < r.native + r.size ? lo - 1 : (unsigned)-1;
}

unsigned HashBytes(const unsigned char* bytes, unsigned size)
{
    unsigned hash = 0x811c9dc5;
    const unsigned* words = (const unsigned*)bytes;
    for (unsigned i = 0; i < size / 4; i++)
        hash = (hash ^ words[i]) * 0x1000193;
    return hash;
}

void Grow(void** array, unsigned* capacity, unsigned count, unsigned element)
{
    if (count < *capacity)
        return;
    unsigned grown = *capacity == 0 ? 256 : *capacity * 2;
    void* bigger = realloc(*array, (size_t)grown * element);
    if (bigger == nullptr)
        Fatal("load_data: out of memory");
    *array = bigger;
    *capacity = grown;
}

void Register(const void* buffer, unsigned size, int kind)
{
    if (buffer == nullptr || size == 0)
        return;
    unsigned lo = 0;
    unsigned hi = s_registeredCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (s_registered[mid].native < (const unsigned char*)buffer)
            lo = mid + 1;
        else
            hi = mid;
    }
    Grow((void**)&s_registered, &s_registeredCapacity, s_registeredCount, sizeof(Registered));
    memmove(s_registered + lo + 1, s_registered + lo, (s_registeredCount - lo) * sizeof(Registered));
    Registered& r = s_registered[lo];
    r.native = (const unsigned char*)buffer;
    r.size = size;
    r.kind = kind;
    r.mapIndex = -1;
    s_registeredCount++;
}

unsigned MapRegistered(unsigned at);

unsigned Rebase(const Registered& r, unsigned linear, unsigned value)
{
    unsigned offset = value - (unsigned)r.native;
    if (value < (unsigned)r.native || offset >= r.size)
        FatalAddress("points outside its load data buffer", value);
    return linear + offset;
}

int* Slot(unsigned char* copy, const Registered& r, unsigned nativeValue)
{
    unsigned offset = nativeValue - (unsigned)r.native;
    if (nativeValue < (unsigned)r.native || offset + 4 > r.size)
        FatalAddress("points outside its load data buffer", nativeValue);
    return (int*)(copy + offset);
}

void RelocateGaf(unsigned char* copy, const Registered& r, unsigned linear)
{
    const int* base = (const int*)r.native;
    int blocks = (short)base[1];
    for (int i = 0; i < blocks; i++) {
        unsigned blockAddress = (unsigned)base[3 + i];
        ((int*)copy)[3 + i] = (int)Rebase(r, linear, blockAddress);
        const unsigned char* block = r.native + (blockAddress - (unsigned)r.native);
        unsigned count = *(const unsigned short*)block;
        for (unsigned j = 0; j < count; j++) {
            unsigned entryAddress = *(const unsigned*)(block + 40 + j * 8);
            *Slot(copy, r, blockAddress + 40 + j * 8) = (int)Rebase(r, linear, entryAddress);
            const unsigned char* frame = r.native + (entryAddress - (unsigned)r.native);
            unsigned pixels = *(const unsigned*)(frame + 16);
            *Slot(copy, r, entryAddress + 16) = (int)Rebase(r, linear, pixels);
            unsigned layers = frame[0xa];
            for (unsigned k = 0; k < layers; k++) {
                unsigned subAddress = ((const unsigned*)(r.native + (pixels - (unsigned)r.native)))[k];
                *Slot(copy, r, pixels + k * 4) = (int)Rebase(r, linear, subAddress);
                const unsigned char* sub = r.native + (subAddress - (unsigned)r.native);
                *Slot(copy, r, subAddress + 16) = (int)Rebase(r, linear, *(const unsigned*)(sub + 16));
            }
        }
    }
}

void RelocateTable(unsigned char* copy, const Registered& r)
{
    unsigned count = *(const unsigned short*)r.native;
    if (0x28 + count * 8 > r.size)
        FatalAddress("is a frame table that overruns its buffer", (unsigned)r.native);
    for (unsigned j = 0; j < count; j++) {
        unsigned image = *(const unsigned*)(r.native + 0x28 + j * 8);
        unsigned at = FindRegistered((const void*)image);
        if (at == (unsigned)-1)
            FatalAddress("is not load data", image);
        *(unsigned*)(copy + 0x28 + j * 8) = MapRegistered(at);
    }
}

unsigned MapRegistered(unsigned at)
{
    Registered& r = s_registered[at];
    if (r.mapIndex >= 0)
        return s_mapped[r.mapIndex].linear;
    unsigned size = (r.size + 7) & ~7u;
    if (size > s_size || s_bump > s_size - size)
        Fatal("load_data: the region is full");
    unsigned linear = s_base + s_bump;
    unsigned char* copy = s_region + s_bump;
    s_bump += size;
    memcpy(copy, r.native, r.size);
    memset(copy + r.size, 0, size - r.size);
    Grow((void**)&s_mapped, &s_mappedCapacity, s_mappedCount, sizeof(Mapped));
    Mapped& m = s_mapped[s_mappedCount];
    m.native = r.native;
    m.size = r.size;
    m.linear = linear;
    m.live = 1;
    unsigned index = s_mappedCount++;
    r.mapIndex = (int)index;
    s_live++;
    Registered copyOfRegistered = r;
    if (r.kind == KIND_GAF)
        RelocateGaf(copy, copyOfRegistered, linear);
    else if (r.kind == KIND_TABLE)
        RelocateTable(copy, copyOfRegistered);
    else if (r.size >= sizeof(GafFrame)) {
        unsigned pixels = *(const unsigned*)(r.native + 16);
        if (pixels >= (unsigned)r.native && pixels - (unsigned)r.native < r.size)
            *(unsigned*)(copy + 16) = linear + (pixels - (unsigned)r.native);
    }
    s_mapped[index].hash = HashBytes(copy, size);
    return linear;
}

}

void __cdecl LoadDataAttach(unsigned char* native, unsigned base, unsigned size, void (__stdcall* fatal)(const char* text))
{
    s_region = native;
    s_base = base;
    s_size = size;
    s_fatal = fatal;
    s_bump = 0;
}

void __cdecl LoadDataRegister(const void* buffer, unsigned size)
{
    Register(buffer, size, KIND_GAF);
}

void __cdecl LoadDataRegisterTable(const void* table, unsigned size)
{
    Register(table, size, KIND_TABLE);
}

void __cdecl LoadDataRegisterFrame(const void* frame, unsigned size)
{
    Register(frame, size, KIND_FRAME);
}

void __cdecl LoadDataUnregister(const void* buffer)
{
    unsigned at = FindRegistered(buffer);
    if (at == (unsigned)-1 || s_registered[at].native != (const unsigned char*)buffer)
        return;
    int mapIndex = s_registered[at].mapIndex;
    if (mapIndex >= 0) {
        s_mapped[mapIndex].live = 0;
        if (--s_live == 0) {
            s_mappedCount = 0;
            s_bump = 0;
        }
    }
    memmove(s_registered + at, s_registered + at + 1, (s_registeredCount - at - 1) * sizeof(Registered));
    s_registeredCount--;
}

void __cdecl LoadDataStoreByte(unsigned char* native, unsigned char value)
{
    *native = value;
    if (s_region == nullptr)
        return;
    unsigned at = FindRegistered(native);
    if (at == (unsigned)-1 || s_registered[at].mapIndex < 0)
        return;
    Mapped& m = s_mapped[s_registered[at].mapIndex];
    unsigned char* copy = s_region + (m.linear - s_base);
    copy[native - m.native] = value;
    m.hash = HashBytes(copy, (m.size + 7) & ~7u);
}

unsigned __cdecl LoadAddress(const void* native)
{
    if (s_region == nullptr)
        return (unsigned)native;
    if (native == nullptr)
        return 0;
    unsigned at = FindRegistered(native);
    if (at == (unsigned)-1)
        FatalAddress("is not load data", (unsigned)native);
    unsigned linear = MapRegistered(at);
    return linear + (unsigned)((const unsigned char*)native - s_registered[at].native);
}

unsigned __cdecl LoadAddressIfLoadData(const void* native)
{
    if (s_region == nullptr || native == nullptr || FindRegistered(native) == (unsigned)-1)
        return (unsigned)native;
    return LoadAddress(native);
}

namespace {

const Mapped* FindMapped(unsigned address)
{
    if (address < s_base || address - s_base >= s_size)
        return nullptr;
    unsigned lo = 0;
    unsigned hi = s_mappedCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (s_mapped[mid].linear <= address)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo > 0) {
        const Mapped& m = s_mapped[lo - 1];
        if (m.live && address - m.linear < m.size)
            return &m;
    }
    return nullptr;
}

}

void* __cdecl LoadNative(unsigned address)
{
    if (s_region == nullptr || address == 0)
        return (void*)address;
    const Mapped* m = FindMapped(address);
    if (m == nullptr)
        FatalAddress("is not a load address", address);
    return (void*)(m->native + (address - m->linear));
}

void* __cdecl LoadNativeIfLoadData(unsigned address)
{
    if (s_region == nullptr || address == 0)
        return (void*)address;
    const Mapped* m = FindMapped(address);
    return m == nullptr ? (void*)address : (void*)(m->native + (address - m->linear));
}

void __cdecl LoadDataCheck(unsigned tick)
{
    if (s_region == nullptr)
        return;
    for (unsigned i = 0; i < s_mappedCount; i++) {
        const Mapped& m = s_mapped[i];
        if (m.live && HashBytes(s_region + (m.linear - s_base), (m.size + 7) & ~7u) != m.hash) {
            char text[100];
            sprintf(text, "load_data: the load data changed at tick %u", tick);
            Fatal(text);
        }
    }
}
