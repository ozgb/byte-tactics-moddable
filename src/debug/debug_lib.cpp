#define NOMINMAX
#define _Charalloc Allocate
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <functional>
#include <new>
#include <stdio.h>
#include "../state/block.h"
#include "../../interfaces/host.h"
#include "command_line.h"

extern char g_memFussyQueried;
extern char g_memFussyDefault;

#pragma auto_inline(off)
// FUNCTION: 0x4d80d0
char IsMemFussy()
{
    g_memFussyQueried = 1;
    static CommandLineSwitch memFussy("memfussy", 1, g_memFussyDefault, "-memfussy", "-memnofussy", "-memfrontalign", 0);
    return memFussy.on;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8130
void MemFussyAtexitHandler(void)
{
}

#pragma auto_inline(off)
// FUNCTION: 0x4d8140
char IsGonzo()
{
    static CommandLineSwitch gonzo("gonzo", 1, 1, 0, "-gonzo", 0, 0);
    return gonzo.on;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8190
void GonzoAtexitHandler(void)
{
}

#include "debug_lib.h"
#include <memory>

class FreeBlockMap;
template <class T, class A = std::allocator<T> >
class FreedBlockRing;

static unsigned int __cdecl AllocDebugBlock(unsigned int n, unsigned int arg2);
static void __cdecl FreeDebugBlock(void* p, int flags);
static FreeBlockMap* __stdcall GetFreeBlockSet();

#pragma auto_inline(off)
// FUNCTION: 0x4d8200
char IsMemSet()
{
    static CommandLineSwitch memSet("setmemory", 1, 0, "-memset", "-memnoset", 0, 0);
    return memSet.on;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8250
void MemSetAtexitHandler(void)
{
}

class CommandLineNumber {
public:
    int value;
    CommandLineNumber(char* name, int a, int def, char* sw);
    ~CommandLineNumber() {}
};

#pragma auto_inline(off)
// FUNCTION: 0x4d8260
int GetMemSetValue()
{
    static CommandLineNumber setValue("setvalue", 0xdeadbeef, 0xdeadbeef, "-memset");
    return setValue.value;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d82b0
void SetValueAtexitHandler(void)
{
}

#pragma auto_inline(off)
// FUNCTION: 0x4d82c0
void __cdecl FillPattern(void* p, unsigned int pattern, unsigned int n)
{
    unsigned int* d = (unsigned int*)p;
    while (n >= 4) {
        *d++ = pattern;
        n -= 4;
    }
    if (n > 0)
        memcpy(d, &pattern, n);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d8360
size_t __cdecl GetBlockSize(void* p)
{
    if (p == 0)
        return 0;

    if (IsMemFussy())
        return LookupBlockSize((unsigned int)p);

    return _msize(p);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d8390
void NopAbortHook(void)
{
}
#pragma auto_inline(on)

// FUNCTION: 0x4d83a0
void __cdecl AllocNotifyNop(int)
{
}

// FUNCTION: 0x4d83b0
void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size)
{
    AllocSite site(_ReturnAddress(), name);
    return GameAlloc(size);
}

extern void (*g_outOfMemoryHandler)();

#pragma auto_inline(off)
// FUNCTION: 0x4d83c0
void* __cdecl GameAlloc(unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    void* p;
    do {
        if (IsMemFussy()) {
            p = (void *)AllocDebugBlock(size, 0);
        } else {
            p = malloc(size);
            if (p != 0)
                CountAlloc(size);
        }
        if (p == 0 && g_outOfMemoryHandler != 0)
            g_outOfMemoryHandler();
    } while (p == 0 && g_outOfMemoryHandler != 0);
    if (p != 0) {
        if (IsMemSet())
            FillPattern(p, GetMemSetValue(), size);
    }
    if (p != 0 && g_allocHooks != 0)
        g_allocHooks->added(p, size);
    host_mutex_unlock(cs);
    return p;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8450
void* __cdecl GameAllocShared(unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAlloc(size);
}

// FUNCTION: 0x4d8460
void* __cdecl GameCalloc(unsigned int count, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    unsigned int total = size * count;
    void* p = GameAlloc(total);
    if (p != 0) {
        memset(p, 0, total);
    }
    return p;
}

#pragma auto_inline(off)
// FUNCTION: 0x4d84a0
void* __cdecl GameReallocTagged(void* block, int unused_param_2, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameRealloc(block, size);
}
#pragma auto_inline(on)

#include "../../interfaces/ref.h"

#pragma auto_inline(off)
// FUNCTION: 0x4d84c0
void* __cdecl GameRealloc(void* block, unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    if (SimOwns(block))
        return SimRealloc(block, size);
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    size_t grownFrom = 0;
    if (HarnessTestMode())
        grownFrom = GetBlockSize(block);
    void* p;
    do {
        if (IsMemFussy()) {
            p = ReallocDebugBlock(block, size, 0);
        } else {
            size_t old = GetBlockSize(block);
            if (block == 0 && size == 0)
                p = 0;
            else
                p = realloc(block, size);
            if (p != 0 || size == 0) {
                if (block != 0)
                    CountFree(old);
                if (size != 0)
                    CountAlloc(size);
            }
        }
        if (p == 0 && size != 0 && g_outOfMemoryHandler != 0)
            g_outOfMemoryHandler();
    } while (p == 0 && size != 0 && g_outOfMemoryHandler != 0);
    if (HarnessTestMode() && p != 0 && IsMemSet() && size > grownFrom)
        FillPattern((char*)p + grownFrom, GetMemSetValue(), size - grownFrom);
    if (g_allocHooks != 0 && (p != 0 || size == 0)) {
        if (block != 0)
            g_allocHooks->removed(block);
        if (p != 0)
            g_allocHooks->added(p, size);
    }
    host_mutex_unlock(cs);
    return p;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8580
void* __cdecl GameReallocIgnoreTag(int* block, int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameReallocTagged(block, 0, size);
}

#pragma auto_inline(off)
// FUNCTION: 0x4d85a0
void __cdecl GameFreeThunk(int* p)
{
    GameFree(p);
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d85b0
void __cdecl GameFree(void* p)
{
    if (SimOwns(p)) {
        SimFree(p);
        return;
    }
    if (p) {
        void* cs = GetAllocLock();
        host_mutex_lock(cs);
        if (g_allocHooks != 0)
            g_allocHooks->removed(p);
        if (IsMemFussy()) {
            FreeDebugBlock(p, 0);
        } else {
            CountFree(GetBlockSize(p));
            free(p);
        }
        host_mutex_unlock(cs);
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8610
char* __cdecl GameStrdup(char* s)
{
    if (!s)
        return 0;
    AllocSite site(_ReturnAddress(), 0);
    char* p = (char*)GameAlloc(strlen(s) + 1);
    if (p)
        strcpy(p, s);
    return p;
}

// FUNCTION: 0x4d8660
void* __cdecl GameAllocThunk(unsigned int size)
{
    AllocSite site(_ReturnAddress(), 0);
    return GameAlloc(size);
}

// FUNCTION: 0x4d8670
void __cdecl GameFreeIndirectThunk(int* p)
{
    GameFreeThunk(p);
}

#pragma auto_inline(off)
// FUNCTION: 0x4d8680
char __cdecl AvWriteDetectStub(unsigned long)
{
    return 0;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d86b0
void __cdecl ProtectPages(int addr, int size, int protect)
{
    if (IsGonzo()) {
        unsigned int start = (addr + 0xfff) & 0xfffff000;
        unsigned int end = (addr + size) & 0xfffff000;
        if (start < end)
            host_mem_protect((void*)start, end - start, protect);
    }
}
#pragma auto_inline(on)


// FUNCTION: 0x4d8710
void __cdecl ProtectBlockReadOnly(void* p)
{
    ProtectBlock(p, HOST_MEM_READONLY);
}

#pragma auto_inline(off)
// FUNCTION: 0x4d8720
void __cdecl ProtectBlock(void* p, int protect)
{
    unsigned int start = (unsigned int)p;
    size_t size = GetBlockSize(p);
    unsigned int end = start + size;
    if (IsMemFussy())
        RoundRangeToPages(&start, &end);
    ProtectPages(start, end - start, protect);
}
#pragma auto_inline(on)

// FUNCTION: 0x4d8780
void __cdecl ProtectBlockReadWrite(void* p)
{
    ProtectBlock(p, HOST_MEM_READWRITE);
}

class BlockInfo {
public:
    unsigned int address;
    long size;
    int allocNumber;
    char name[0x20];
    unsigned int tag;

    BlockInfo(void);
    BlockInfo(int p1, int p2, int p3, int p4, const char* p5);
    void SetName(char* newName);
};

#pragma auto_inline(off)
// FUNCTION: 0x4d8820 ??0BlockInfo@@QAE@HHHHPBD@Z
BlockInfo::BlockInfo(int p1, int p2, int p3, int p4, const char* p5)
{
    address = p1;
    size = p2;
    allocNumber = p3;
    SetName((char*)p5);
    tag = p4;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d8850
void BlockInfo::SetName(char* newName)
{
    if (newName != 0) {
        int i = 0;
        for (; i < 0x1f && newName[i] != 0; i++)
            name[i] = newName[i];
        name[i] = 0;
    } else {
        name[0] = 0;
    }
}
#pragma auto_inline(on)

#define NOMINMAX
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <yvals.h>
#include <memory>
#include <io.h>

extern void (*g_outOfMemoryHandler)();

// FUNCTION: 0x4d8e50
int __cdecl SetOutOfMemoryHandler(int handler)
{
    int temp = (int)g_outOfMemoryHandler;
    g_outOfMemoryHandler = (void (*)())handler;
    return temp;
}

#pragma auto_inline(off)
// FUNCTION: 0x4d9f60
char* __cdecl FindCommandLineSwitch(char* name)
{
    if (!name) return 0;
    char* cmd = (char*)host_command_line();
    if (!cmd) return 0;
    size_t len = strlen(name);
    if (len == 0) return 0;
    while (*cmd) {
        if (_strnicmp(cmd, name, len) == 0) {
            char c = cmd[len];
            if (c == 0 || isspace(c) || c == '=')
                return cmd + len;
        }
        cmd++;
    }
    return 0;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4d9fe0
CommandLineSwitch::CommandLineSwitch(char* name, int a, char def, char* onSwitch,
                               char* offSwitch, char* onSwitch2, char* offSwitch2)
{
    on = def;
    if (FindCommandLineSwitch(onSwitch) || FindCommandLineSwitch(onSwitch2))
        on = 1;
    if (FindCommandLineSwitch(offSwitch) || FindCommandLineSwitch(offSwitch2))
        on = 0;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4da040
CommandLineNumber::CommandLineNumber(char* name, int a, int def, char* sw)
{
    value = def;
    char* p = FindCommandLineSwitch(sw);
    if (p) {
        while (*p == ' ' || *p == '\t' || *p == '=')
            p++;
        if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
            sscanf(p, "%x", &value);
            return;
        }
        sscanf(p, "%d", &value);
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4da0c0
void AbortProgram(void)
{
    NopAbortHook();
    raise(SIGABRT);
    _exit(3);
}

extern char* g_debugKeywords[19];

// FUNCTION: 0x4da0e0
bool __cdecl IsDebugKeyword(const char* arg)
{
    for (char** p = g_debugKeywords; p < g_debugKeywords + 19; p++) {
        if (_strnicmp(*p, arg, strlen(*p)) == 0)
            return true;
    }
    return false;
}

extern char* __cdecl strrchr(const char*, int);

#pragma auto_inline(off)
// FUNCTION: 0x4da780
void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}
#pragma auto_inline(on)

// FUNCTION: 0x4da7c0
void AllocLockAtexitHandler(void)
{
}

extern unsigned int g_allocSerial;
extern unsigned int g_liveAllocCount;
extern unsigned int g_liveAllocPeak;
extern unsigned int g_bytesRequestedLo;
extern unsigned int g_bytesRequestedHi;
extern unsigned int g_currentBytes;
extern unsigned int g_currentBytesPeak;
extern unsigned int g_committedBytesPeak;
extern unsigned int g_committedBytes;
extern unsigned int g_freeBlockWraps;

#pragma auto_inline(off)
// FUNCTION: 0x4da7d0
void __cdecl CountAlloc(unsigned int size)
{
    g_allocSerial++;
    g_liveAllocCount++;
    if (g_liveAllocCount > g_liveAllocPeak)
        g_liveAllocPeak = g_liveAllocCount;
    g_bytesRequestedLo += size;
    if (g_bytesRequestedLo >= 1000000000) {
        g_bytesRequestedLo -= 1000000000;
        g_bytesRequestedHi++;
    }
    g_currentBytes += size;
    if (g_currentBytes > g_currentBytesPeak)
        g_currentBytesPeak = g_currentBytes;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4da840
void __cdecl CountFree(int size) {
    g_liveAllocCount--;
    g_currentBytes -= size;
}
#pragma auto_inline(on)


// FUNCTION: 0x4da8a0
int __cdecl RoundUpToDoublePage(int size)
{
    return (size + 0xfff & ~0xfff) * 2;
}

// FUNCTION: 0x4da8c0
unsigned int __cdecl RoundUpToPage(int size)
{
    return (size + 0xfff) & 0xfffff000;
}

extern void* g_blockMapFreeList;
extern void (*g_outOfMemoryHandler)();
extern int g_blockMapLiveCount;
extern void* g_blockMap;

struct BlockMapRawNode {
    BlockMapRawNode* left;
    BlockMapRawNode* parent;
    BlockMapRawNode* right;
    char value[0x30];
    int color;
};

extern void* g_blockMapNil;

class BlockMapAllocator {
public:
    void* Allocate(unsigned int n);
};

class BlockMapInitView {
public:
    char allocator;
    char key_compare;
    BlockMapRawNode* head;
    unsigned char multi;
    int size;
    void* operator new(unsigned int n) { return host_mem_alloc(n); }
    BlockMapInitView(const char& a, const char& b);
    void Init();
};

static inline BlockMapRawNode* AllocNode()
{
    if (g_blockMapFreeList == 0) {
        BlockMapRawNode* block;
        do {
            block = (BlockMapRawNode*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        BlockMapRawNode* head = (BlockMapRawNode*)g_blockMapFreeList;
        for (int i = 0; i < 0x80; i++) {
            block->left = head;
            head = block;
            block++;
        }
        g_blockMapFreeList = head;
    }
    BlockMapRawNode* node = (BlockMapRawNode*)g_blockMapFreeList;
    g_blockMapFreeList = node->left;
    return node;
}

void BlockMapInitView::Init()
{
    std::_Lockit lock;
    if (g_blockMapNil == 0) {
        BlockMapRawNode* nil =
            (BlockMapRawNode*)((BlockMapAllocator*)this)->Allocate(0x40);
        nil->parent = 0;
        nil->color = 1;
        g_blockMapNil = nil;
        nil->left = 0;
        ((BlockMapRawNode*)g_blockMapNil)->right = 0;
    }
    BlockMapRawNode* nil = (BlockMapRawNode*)g_blockMapNil;
    g_blockMapLiveCount++;
    BlockMapRawNode* node = AllocNode();
    node->color = 0;
    node->parent = nil;
    head = node;
    size = 0;
    node->left = node;
    head->right = head;
}

inline BlockMapInitView::BlockMapInitView(const char& a, const char& b)
{
    allocator = a;
    key_compare = b;
    multi = 0;
    Init();
}

#pragma auto_inline(off)
// FUNCTION: 0x4da8d0
static void* GetBlockMap()
{
    if (g_blockMap == 0) {
        char s0, s1;
        g_blockMap = new BlockMapInitView(s0, s1);
    }
    return g_blockMap;
}
#pragma auto_inline(on)

#include <vector>

struct FreedBlockRecord {
    unsigned int w[0xc];
};

class FreedBlockAllocator {
public:
    typedef unsigned int size_type;
    typedef int difference_type;
    typedef FreedBlockRecord* pointer;
    typedef const FreedBlockRecord* const_pointer;
    typedef FreedBlockRecord& reference;
    typedef const FreedBlockRecord& const_reference;
    typedef FreedBlockRecord value_type;

    pointer allocate(size_type _N, const void* = 0)
    {
        pointer _P;
        do {
            _P = (pointer)host_mem_alloc(_N * sizeof(value_type));
            if (_P == 0 && g_outOfMemoryHandler != 0)
                g_outOfMemoryHandler();
        } while (_P == 0 && g_outOfMemoryHandler != 0);
        return _P;
    }
    void deallocate(pointer _P, size_type)
    {
        if (_P != 0)
            host_mem_free(_P);
    }
    void construct(pointer _P, const value_type& _V)
    {
        std::_Construct(_P, _V);
    }
    void destroy(pointer) {}
    size_type max_size() const { return (size_type)(-1) / sizeof(value_type); }
};

typedef std::vector<FreedBlockRecord, FreedBlockAllocator> FreedBlocks;

template <class T, class A>
class FreedBlockRing {
public:
    explicit FreedBlockRing(const A& al = A())
        : ring(), count(0) {}
    static void* operator new(size_t size) { return host_mem_alloc(size); }

    FreedBlocks ring;
    int count;
};

extern FreedBlockRing<int>* g_freedBlockRing;

#pragma auto_inline(off)
// FUNCTION: 0x4da9f0
static FreedBlockRing<int>* GetFreedBlockRing()
{
    if (g_freedBlockRing == 0) {
        g_freedBlockRing = new FreedBlockRing<int>;
    }
    return g_freedBlockRing;
}
#pragma auto_inline(on)
#include <string.h>
#include <set>
#include <yvals.h>
#include <map>
#include <list>
#include <time.h>
#include <algorithm>
#include "free_block_map.h"

class FreeBlockMap;

extern void* g_blockMapNil;
extern void* g_freeBlockFreeList;
extern void* g_blockMapFreeList;
extern void (*g_outOfMemoryHandler)();
extern unsigned int g_lastAllocOffset;
extern unsigned int g_freeBlockWraps;
extern unsigned int g_committedBytes;
extern unsigned int g_committedBytesPeak;
extern unsigned int g_allocSerial;
extern FreeBlockMap* g_freeBlockSet;

struct BlockMapNode {
    BlockMapNode* left;
    BlockMapNode* parent;
    BlockMapNode* right;
    BlockInfo value;
    unsigned int color;
};

static inline FreeBlockNode* FreeBlockMaxNode(FreeBlockNode* p)
{
    std::_Lockit lock;
    while (p->right != DAT_00528a54) {
        p = p->right;
    }
    return p;
}

static inline FreeBlockNode* Min(FreeBlockNode* p)
{
    std::_Lockit lock;
    while (p->left != DAT_00528a54)
        p = p->left;
    return p;
}

static inline FreeBlockNode* FreeBlockMinForInc(FreeBlockNode* p)
{
    std::_Lockit lock;
    while (p->left != DAT_00528a54) {
        p = p->left;
    }
    return p;
}

static inline FreeBlockNode* FreeBlockMaxForDec(FreeBlockNode* p)
{
    std::_Lockit lock;
    while (p->right != DAT_00528a54) {
        p = p->right;
    }
    return p;
}

static inline void IncByMin(FreeBlockIter& it)
{
    std::_Lockit lock;
    if (it.ptr->right != DAT_00528a54) {
        it.ptr = FreeBlockMinForInc(it.ptr->right);
    } else {
        FreeBlockNode* p;
        while (it.ptr == (p = it.ptr->parent)->right) {
            it.ptr = p;
        }
        if (it.ptr->right != p) {
            it.ptr = p;
        }
    }
}

inline void FreeBlockIter::Dec()
{
    std::_Lockit lock;
    if (ptr->color == 0 && ptr->parent->parent == ptr) {
        ptr = ptr->right;
    } else if (ptr->left != DAT_00528a54) {
        ptr = FreeBlockMaxForDec(ptr->left);
    } else {
        FreeBlockNode* p;
        while (ptr == (p = ptr->parent)->left) {
            ptr = p;
        }
        ptr = p;
    }
}

class BlockMapIter {
public:
    BlockMapNode* ptr;

    BlockMapIter() {}
    BlockMapIter(BlockMapNode* q) : ptr(q) {}
    bool operator==(const BlockMapIter& o) const { return ptr == o.ptr; }
    BlockInfo& operator*() const { return ptr->value; }
    BlockInfo* operator->() const { return &ptr->value; }
    BlockMapIter operator++(int)
    {
        BlockMapIter _Tmp = *this;
        NextNode();
        return _Tmp;
    }
    BlockMapNode* _Mynode() const { return ptr; }
    void PrevNode();
    void NextNode();
};

inline MapInsertResult::MapInsertResult(const BlockMapIter& i, unsigned char b) : first((FreeBlockNode*)i.ptr), second(b) {}

static inline BlockMapNode* BlockMapMaxNode(BlockMapNode* p)
{
    std::_Lockit lock;
    while (p->right != g_blockMapNil) {
        p = p->right;
    }
    return p;
}

class FreeBlockUpperBoundIter {
public:
    FreeBlockNode* ptr;
    FreeBlockUpperBoundIter() : ptr(0) {}
    FreeBlockUpperBoundIter(FreeBlockNode* p) : ptr(p) {}
};

class FreeBlockEraseIter {
public:
    FreeBlockNode* ptr;

    FreeBlockEraseIter() {}
    FreeBlockEraseIter(FreeBlockNode* p) : ptr(p) {}
};

class FreeBlockEraseConstIter : public FreeBlockEraseIter {
public:
    FreeBlockEraseConstIter() {}
    FreeBlockEraseConstIter(FreeBlockNode* p) : FreeBlockEraseIter(p) {}
    FreeBlockEraseConstIter(const FreeBlockEraseIter& x) : FreeBlockEraseIter(x) {}
};

class BlockMap {
public:
    BlockMapAllocator allocator;
    UnsignedLess key_compare;
    BlockMapNode* head;
    unsigned char multi;
    int size;

    BlockMapIter Begin() { return BlockMapIter(head->left); }
    BlockMapIter End() { return BlockMapIter(head); }
    BlockMapIter end() { return BlockMapIter(head); }
    BlockMapIter find(const unsigned int& key)
    {
        BlockMapIter it = lower_bound(key);
        return (it == End() || key_compare(key, it.ptr->value.address)) ? End() : it;
    }

    BlockMapNode* Lbound(const unsigned int& key) const
    {
        std::_Lockit lock;
        BlockMapNode* x = head->parent;
        BlockMapNode* y = head;
        while (x != g_blockMapNil) {
            if (key_compare(x->value.address, key))
                x = x->right;
            else
                y = x, x = x->left;
        }
        return y;
    }

    BlockMapNode*& _Root() { return head->parent; }
    BlockMapNode*& _Lmost() { return head->left; }
    BlockMapNode*& _Rmost() { return head->right; }

    static BlockMapNode* _Min(BlockMapNode* _P)
        { std::_Lockit _Lk;
          while (_P->left != g_blockMapNil)
              _P = _P->left;
          return _P; }

    static BlockMapNode* _Max(BlockMapNode* _P)
        { std::_Lockit _Lk;
          while (_P->right != g_blockMapNil)
              _P = _P->right;
          return _P; }

    void _Lrotate(BlockMapNode* _X)
        { std::_Lockit _Lk;
          BlockMapNode* _Y = _X->right;
          _X->right = _Y->left;
          if (_Y->left != g_blockMapNil)
              _Y->left->parent = _X;
          _Y->parent = _X->parent;
          if (_X == _Root())
              _Root() = _Y;
          else if (_X == _X->parent->left)
              _X->parent->left = _Y;
          else
              _X->parent->right = _Y;
          _Y->left = _X;
          _X->parent = _Y; }

    void _Rrotate(BlockMapNode* _X)
        { std::_Lockit _Lk;
          BlockMapNode* _Y = _X->left;
          _X->left = _Y->right;
          if (_Y->right != g_blockMapNil)
              _Y->right->parent = _X;
          _Y->parent = _X->parent;
          if (_X == _Root())
              _Root() = _Y;
          else if (_X == _X->parent->right)
              _X->parent->right = _Y;
          else
              _X->parent->left = _Y;
          _Y->right = _X;
          _X->parent = _Y; }

    void Lrotate(BlockMapNode* x)
    {
        std::_Lockit lock;
        BlockMapNode* y = x->right;
        x->right = y->left;
        if (y->left != g_blockMapNil)
            y->left->parent = x;
        y->parent = x->parent;
        if (x == head->parent)
            head->parent = y;
        else if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
        y->left = x;
        x->parent = y;
    }

    void Rrotate(BlockMapNode* x)
    {
        std::_Lockit lock;
        BlockMapNode* y = x->left;
        x->left = y->right;
        if (y->right != g_blockMapNil)
            y->right->parent = x;
        y->parent = x->parent;
        if (x == head->parent)
            head->parent = y;
        else if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
        y->right = x;
        x->parent = y;
    }

    MapInsertResult Insert(BlockInfo* p);
    BlockMapIter InsertHinted(BlockMapNode* x, BlockMapNode* y, const BlockInfo& v);
    BlockMapIter* InsertHinted(BlockMapIter* out, BlockMapNode* x, BlockMapNode* y, const BlockInfo* v);
    BlockMapIter erase(BlockMapIter _P);
    BlockMapIter Find(const unsigned int& key);
    BlockMapIter lower_bound(const unsigned int& key);
    BlockMapNode* LowerBound(const unsigned int& key);
    BlockMapNode* Ubound(const unsigned int& key);
    BlockMapNode* CreateNode(int parent, int color);
    void RotateLeft(BlockMapNode* x);
    void RotateRight(BlockMapNode* x);
};

class FreeBlockLowerBoundIter {
public:
    FreeBlockNode* ptr;
    FreeBlockLowerBoundIter() : ptr(0) {}
    FreeBlockLowerBoundIter(FreeBlockNode* p) : ptr(p) {}
};

struct FreeBlockSignedRange {
    unsigned int offset;
    int length;
};

struct FreeBlockInsertResult { FreeBlockIter first; int second; };

struct FreeBlockTailFrame { FreeBlockInsertResult r; FreeBlockSignedRange p; };

inline FreeBlockNode* FreeBlockMap::Min(FreeBlockNode* p)
{
    std::_Lockit _Lk;
    while (Left(p) != DAT_00528a54)
        p = Left(p);
    return p;
}

inline FreeBlockNode* FreeBlockMap::Max(FreeBlockNode* p)
{
    std::_Lockit _Lk;
    while (Right(p) != DAT_00528a54)
        p = Right(p);
    return p;
}

inline void FreeBlockMap::Lrotate(FreeBlockNode* x)
{
    std::_Lockit _Lk;
    FreeBlockNode* y = Right(x);
    Right(x) = Left(y);
    if (Left(y) != DAT_00528a54)
        Parent(Left(y)) = x;
    Parent(y) = Parent(x);
    if (x == Root())
        Root() = y;
    else if (x == Left(Parent(x)))
        Left(Parent(x)) = y;
    else
        Right(Parent(x)) = y;
    Left(y) = x;
    Parent(x) = y;
}

inline void FreeBlockMap::Rrotate(FreeBlockNode* x)
{
    std::_Lockit _Lk;
    FreeBlockNode* y = Left(x);
    Left(x) = Right(y);
    if (Right(y) != DAT_00528a54)
        Parent(Right(y)) = x;
    Parent(y) = Parent(x);
    if (x == Root())
        Root() = y;
    else if (x == Right(Parent(x)))
        Right(Parent(x)) = y;
    else
        Left(Parent(x)) = y;
    Right(y) = x;
    Parent(x) = y;
}

inline void FreeBlockMap::Freenode(FreeBlockNode* p)
{
    if (p != 0) {
        *(void**)p = g_freeBlockFreeList;
        g_freeBlockFreeList = p;
    }
}

inline FreeBlockNode* FreeBlockMap::Ubound(const unsigned int& kv)
{
    std::_Lockit lock;
    FreeBlockNode* x = head->parent;
    FreeBlockNode* y = head;
    while (x != DAT_00528a54)
        if (key_compare(kv, x->value.offset))
            y = x, x = x->left;
        else
            x = x->right;
    return y;
}

inline void FreeBlockMap::Tail(unsigned base, unsigned len, FreeBlockIter& it)
{
    total += len;
    FreeBlockIter n;
    FreeBlockTailFrame f;
    f.p.offset = base;
    f.p.length = len;
    lower_bound(&n, f.p.offset);
    it = n;
    Begin((int*)&f.r);
    if (it == f.r.first)
        it.ptr = head;
    else
        it.PrevNode();
    if (Neq(n, FreeBlockIter(head))) {
        if (n.ptr->value.offset == f.p.offset + f.p.length) {
            f.p.length = f.p.length + n.ptr->value.length;
            Erase(&f.r.first, n);
        }
    }
    if (Neq(it, FreeBlockIter(head))) {
        if (it.ptr->value.offset + it.ptr->value.length == f.p.offset) {
            f.p.length = f.p.length + it.ptr->value.length;
            f.p.offset = it.ptr->value.offset;
            Erase(&f.r.first, it);
        }
    }
    InsertOrFindInline(&f.r, &f.p);
}

typedef void (FreedBlocks::*FreedBlocksInsertFn)(
    FreedBlocks::iterator, FreedBlocks::size_type, const FreedBlockRecord&);


// FUNCTION: 0x4db000
void FreeBlockMap::AddFreeBlock(FreeBlockRange p)
{
    FreeBlockIter it;
    FreeBlockIter it2;
    MapInsertResult result;
    unsigned char inserted;

    FreeBlockIter n(LowerBound(p));
    it = n;
    if (n == begin()) {
        it = end();
    } else {
        it.PrevNode();
    }
    if (Neq(n, end())) {
        if (n->offset == p.length + p.offset) {
            p.length = p.length + n->length;
            Erase(n);
        }
    }
    if (Neq(it, end())) {
        FreeBlockNode* m = it.ptr;
        if (m->value.length + m->value.offset == p.offset) {
            p.length = p.length + m->value.length;
            p.offset = m->value.offset;
            Erase(it);
        }
    }

    FreeBlockNode* y = head;
    bool less = true;
    FreeBlockNode* x = y->parent;
    {
        std::_Lockit lock;
        while (x != DAT_00528a54) {
            y = x;
            less = p.offset < x->value.offset;
            x = less ? x->left : x->right;
        }
    }

    if (multi) {
        FreeBlockIter t(Insert(x, y, &p));
        return;
    }
    it2.ptr = y;
    if (less) {
        if (FreeBlockIter(y) == begin()) {
            inserted = 1;
            result.Assign(Insert(x, y, &p), inserted);
            goto done;
        }
        it2.PrevNode();
    }
    if (key_compare(it2->offset, p.offset)) {
        inserted = 1;
        result.Assign(Insert(x, y, &p), inserted);
    } else {
        inserted = 0;
        result.Assign(it2, inserted);
    }
done: ;
}

// FUNCTION: 0x4db450
bool FreeBlockMap::GrowReservation(unsigned int size)
{
    unsigned int len = 0x10000000;
    unsigned int base;
    FreeBlockIter& it = *(FreeBlockIter*)&size;

    if (2 * size > len && size < 0x40000000u)
        len = ((size + 0x1fff) & 0xffffe000) * 2;
    if (size > len)
        len = (size + 0x1fff) & 0xffffe000;
    base = (unsigned int)host_mem_reserve(len + 0x2000);
    for (;;) {
        if (base != 0 && base + len <= 0x80000000u)
            break;
        if (base != 0)
            host_mem_release((void*)base);
        len = (len >> 1) & 0x7fffe000;
        if (len < 0x10000 || len < size)
            return false;
        base = (unsigned int)host_mem_reserve(len + 0x2000);
    }
    Tail(base, len, *(FreeBlockIter*)&size);
    return true;
}

// FUNCTION: 0x4db760
char IsBackAlign(void)
{
    static CommandLineSwitch g_backAlign("backalign", 1, 1, 0, "-memfrontalign", 0, 0);
    return g_backAlign.on;
}

// FUNCTION: 0x4db7b0
void BackAlignAtexitHandler(void)
{
}

// FUNCTION: 0x4db7c0
int GetDebugFillPattern(void)
{
    return 0xcccccccc;
}

#pragma auto_inline(off)
// FUNCTION: 0x4dba40
void* __cdecl ReallocDebugBlock(void* p, unsigned int size, int flags)
{
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    void* q = 0;
    size_t old = 0;
    if (p)
        old = GetBlockSize(p);
    if (size > 0) {
        q = (void *)AllocDebugBlock(size, flags);
        if (!q) {
            host_mutex_unlock(cs);
            return 0;
        }
        unsigned int n = old >= size ? size : old;
        if (n > 0)
            memcpy(q, p, n);
    }
    if (p)
        FreeDebugBlock(p, flags);
    host_mutex_unlock(cs);
    return q;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4dbae0
unsigned int __cdecl LookupBlockSize(unsigned int key)
{
    void* cs = GetAllocLock();
    host_mutex_lock(cs);
    BlockMap* tree = (BlockMap*)GetBlockMap();
    BlockInfo rec(key, 0, 0, 0, 0);
    BlockMapIter it = tree->find(rec.address);
    if (it == ((BlockMap*)GetBlockMap())->End()) {
        host_mutex_unlock(cs);
        return 0;
    }
    unsigned int value = it.ptr->value.size;
    host_mutex_unlock(cs);
    return value;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4dbb90
void __cdecl RoundRangeToPages(unsigned int* start, unsigned int* end)
{
    *start &= 0xfffff000;
    *end = (*end + 0xfff) & 0xfffff000;
}
#pragma auto_inline(on)

inline MapInsertResult FreeBlockMap::TreeInsert(const FreeBlockRange& V)
{
    FreeBlockNode* X = Root();
    FreeBlockNode* Y = head;
    bool Ans = true;
    {
        std::_Lockit Lk;
        while (X != DAT_00528a54) {
            Y = X;
            Ans = key_compare(FreeBlockOffsetKey()(V), Key(X));
            X = Ans ? Left(X) : Right(X);
        }
    }
    if (multi)
        return MapInsertResult(Insert(X, Y, &V), true);
    FreeBlockIter P = FreeBlockIter(Y);
    if (!Ans)
        ;
    else if (P == begin())
        return MapInsertResult(Insert(X, Y, &V), true);
    else
        P.PrevNode();
    if (key_compare(Key(P.Mynode()), FreeBlockOffsetKey()(V)))
        return MapInsertResult(Insert(X, Y, &V), true);
    MapInsertResult res;
    res.Assign(P, false);
    return res;
}

// FUNCTION: 0x4dbbc0
MapInsertResult FreeBlockMap::InsertOrFind(const FreeBlockRange& V)
{
    MapInsertResult ans = TreeInsert(V);
    return MapInsertResult(ans.first, ans.second);
}

// FUNCTION: 0x4dce60 ?Insert@FreeBlockMap@@QAE?AVFreeBlockIter@@PAUFreeBlockNode@@0PBUFreeBlockRange@@@Z
FreeBlockIter FreeBlockMap::Insert(FreeBlockNode* x, FreeBlockNode* y,
                                         const FreeBlockRange* v)
{
    std::_Lockit lock;
    FreeBlockNode* p = (FreeBlockNode*)allocator.Allocate(0x18);
    p->parent = y;
    p->color = 0;
    p->left = (FreeBlockNode*)DAT_00528a54;
    p->right = (FreeBlockNode*)DAT_00528a54;
    new (&p->value) FreeBlockRange(*v);
    ++count;

    if (y == head || x != DAT_00528a54 || key_compare(v->offset, y->value.offset)) {
        y->left = p;
        if (y == head) {
            head->parent = p;
            head->right = p;
        } else if (y == head->left) {
            head->left = p;
        }
    } else {
        y->right = p;
        if (y == head->right)
            head->right = p;
    }

    FreeBlockNode* z = p;
    while (z != head->parent) {
        if (z->parent->color != 0)
            break;

        if (z->parent == z->parent->parent->left) {
            FreeBlockNode* u = z->parent->parent->right;
            if (u->color == 0) {
                z->parent->color = 1;
                u->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    Lrotate(z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                Rrotate(z->parent->parent);
            }
        } else {
            FreeBlockNode* u = z->parent->parent->left;
            if (u->color == 0) {
                z->parent->color = 1;
                u->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    Rrotate(z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                Lrotate(z->parent->parent);
            }
        }
    }
    head->parent->color = 1;
    return FreeBlockIter(p);
}

// FUNCTION: 0x4dbd00
FreeBlockEraseConstIter FreeBlockMap::EraseCopyIter(FreeBlockEraseConstIter it)
{
    return Erase((FreeBlockEraseIter&)it);
}

// FUNCTION: 0x4dbd20
FreeBlockUpperBoundIter FreeBlockMap::UpperBound(const unsigned int& kv)
{
    return FreeBlockUpperBoundIter(Ubound(kv));
}

// FUNCTION: 0x4dbd80
FreeBlockIter FreeBlockIter::Next(int)
{
    FreeBlockIter tmp = *this;
    IncByMin(*this);
    return tmp;
}

// FUNCTION: 0x4dbe10
FreeBlockIter FreeBlockIter::Previous(int)
{
    FreeBlockIter tmp = *this;
    Dec();
    return tmp;
}

#pragma auto_inline(off)
// FUNCTION: 0x4dbeb0
int* FreeBlockMap::Begin(int* out)
{
    *out = (int)head->left;
    return out;
}
#pragma auto_inline(on)

// FUNCTION: 0x4dbec0
MapInsertResult FreeBlockMap::InsertOrFindInline(FreeBlockRange* p)
{
    FreeBlockNode* y = head;
    bool less = true;
    FreeBlockNode* x = y->parent;
    FreeBlockIter it2;
    FreeBlockIter it;
    {
        std::_Lockit lock;
        while (x != DAT_00528a54) {
            y = x;
            less = p->offset < x->value.offset;
            x = less ? x->left : x->right;
        }
    }
    if (multi) {
        {
            std::_Lockit lock;
            it = FreeBlockIter(CreateNode((int)y, 0));
            FreeBlockNode* z = it.ptr;
            z->left = (FreeBlockNode*)DAT_00528a54;
            z->right = (FreeBlockNode*)DAT_00528a54;
            new (&z->value) FreeBlockRange(*p);
            count++;
            if (y == head || x != DAT_00528a54 || key_compare(p->offset, y->value.offset)) {
                y->left = z;
                if (y == head) {
                    head->parent = z;
                    head->right = z;
                } else if (y == head->left) {
                    head->left = z;
                }
            } else {
                y->right = z;
                if (y == head->right) {
                    head->right = z;
                }
            }
            for (FreeBlockNode* q = z; q != head->parent && q->parent->color == 0; ) {
                if (q->parent == q->parent->parent->left) {
                    FreeBlockNode* w = q->parent->parent->right;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->right) {
                            q = q->parent;
                            RotateLeft(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateRight(q->parent->parent);
                    }
                } else {
                    FreeBlockNode* w = q->parent->parent->left;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->left) {
                            q = q->parent;
                            RotateRight(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateLeft(q->parent->parent);
                    }
                }
            }
            head->parent->color = 1;
        }
        return MapInsertResult(it, 1);
    }
    it2 = FreeBlockIter(y);
    if (less) {
        if (FreeBlockIter(y) == Begin())
            return MapInsertResult(Insert(x, y, p), 1);
        it2.PrevNode();
    }
    if (key_compare(it2.ptr->value.offset, p->offset))
        return MapInsertResult(Insert(x, y, p), 1);
    return MapInsertResult(it2, 0);
}

// FUNCTION: 0x4dc130 ?Erase@FreeBlockMap@@QAE?AVFreeBlockIter@@V2@@Z
FreeBlockIter FreeBlockMap::Erase(FreeBlockIter _P)
{
    FreeBlockNode* _X;
    FreeBlockNode* _Y = (_P++)._Mynode();
    FreeBlockNode* _Z = _Y;
    std::_Lockit _Lk;
    if (Left(_Y) == DAT_00528a54)
        _X = Right(_Y);
    else if (Right(_Y) == DAT_00528a54)
        _X = Left(_Y);
    else
        _Y = Min(Right(_Y)), _X = Right(_Y);
    if (_Y != _Z) {
        Parent(Left(_Z)) = _Y;
        Left(_Y) = Left(_Z);
        if (_Y == Right(_Z))
            Parent(_X) = _Y;
        else {
            Parent(_X) = Parent(_Y);
            Left(Parent(_Y)) = _X;
            Right(_Y) = Right(_Z);
            Parent(Right(_Z)) = _Y;
        }
        if (Root() == _Z)
            Root() = _Y;
        else if (Left(Parent(_Z)) == _Z)
            Left(Parent(_Z)) = _Y;
        else
            Right(Parent(_Z)) = _Y;
        Parent(_Y) = Parent(_Z);
        std::swap(Color(_Y), Color(_Z));
        _Y = _Z;
    } else {
        Parent(_X) = Parent(_Y);
        if (Root() == _Z)
            Root() = _X;
        else if (Left(Parent(_Z)) == _Z)
            Left(Parent(_Z)) = _X;
        else
            Right(Parent(_Z)) = _X;
        if (Lmost() != _Z)
            ;
        else if (Right(_Z) == DAT_00528a54)
            Lmost() = Parent(_Z);
        else
            Lmost() = Min(_X);
        if (Rmost() != _Z)
            ;
        else if (Left(_Z) == DAT_00528a54)
            Rmost() = Parent(_Z);
        else
            Rmost() = Max(_X);
    }
    if (Color(_Y) == _Black) {
        while (_X != Root() && Color(_X) == _Black)
            if (_X == Left(Parent(_X))) {
                FreeBlockNode* _W = Right(Parent(_X));
                if (Color(_W) == _Red) {
                    Color(_W) = _Black;
                    Color(Parent(_X)) = _Red;
                    Lrotate(Parent(_X));
                    _W = Right(Parent(_X));
                }
                if (Color(Left(_W)) == _Black && Color(Right(_W)) == _Black) {
                    Color(_W) = _Red;
                    _X = Parent(_X);
                } else {
                    if (Color(Right(_W)) == _Black) {
                        Color(Left(_W)) = _Black;
                        Color(_W) = _Red;
                        Rrotate(_W);
                        _W = Right(Parent(_X));
                    }
                    Color(_W) = Color(Parent(_X));
                    Color(Parent(_X)) = _Black;
                    Color(Right(_W)) = _Black;
                    Lrotate(Parent(_X));
                    break;
                }
            } else {
                FreeBlockNode* _W = Left(Parent(_X));
                if (Color(_W) == _Red) {
                    Color(_W) = _Black;
                    Color(Parent(_X)) = _Red;
                    Rrotate(Parent(_X));
                    _W = Left(Parent(_X));
                }
                if (Color(Right(_W)) == _Black && Color(Left(_W)) == _Black) {
                    Color(_W) = _Red;
                    _X = Parent(_X);
                } else {
                    if (Color(Left(_W)) == _Black) {
                        Color(Right(_W)) = _Black;
                        Color(_W) = _Red;
                        Lrotate(_W);
                        _W = Left(Parent(_X));
                    }
                    Color(_W) = Color(Parent(_X));
                    Color(Parent(_X)) = _Black;
                    Color(Left(_W)) = _Black;
                    Rrotate(Parent(_X));
                    break;
                }
            }
        Color(_X) = _Black;
    }
    Destval(_Value(_Y));
    Freenode(_Y);
    --count;
    return (_P);
}

// FUNCTION: 0x4dc620
FreeBlockLowerBoundIter FreeBlockMap::lower_bound(const unsigned int& kv)
{
    FreeBlockLowerBoundIter y;
    {
        std::_Lockit lock;
        FreeBlockNode* x = head->parent;
        y.ptr = head;
        while (x != DAT_00528a54)
            if (key_compare(kv, x->value.offset))
                y.ptr = x, x = x->left;
            else
                x = x->right;
    }
    return y;
}

// FUNCTION: 0x4dc680 ?Insert@BlockMap@@QAE?AVMapInsertResult@@PAVBlockInfo@@@Z
MapInsertResult BlockMap::Insert(BlockInfo* p)
{
    BlockMapNode* y = head;
    bool less = true;
    BlockMapNode* x = y->parent;
    BlockMapIter it2;
    BlockMapIter it;
    {
        std::_Lockit lock;
        while (x != g_blockMapNil) {
            y = x;
            less = p->address < x->value.address;
            x = less ? x->left : x->right;
        }
    }
    if (multi) {
        {
            std::_Lockit lock;
            it = BlockMapIter(CreateNode((int)y, 0));
            BlockMapNode* z = it.ptr;
            z->left = (BlockMapNode*)g_blockMapNil;
            z->right = (BlockMapNode*)g_blockMapNil;
            new (&z->value) BlockInfo(*p);
            size++;
            if (y == head || x != g_blockMapNil || key_compare(p->address, y->value.address)) {
                y->left = z;
                if (y == head) {
                    head->parent = z;
                    head->right = z;
                } else if (y == head->left) {
                    head->left = z;
                }
            } else {
                y->right = z;
                if (y == head->right) {
                    head->right = z;
                }
            }
            for (BlockMapNode* q = z; q != head->parent && q->parent->color == 0; ) {
                if (q->parent == q->parent->parent->left) {
                    BlockMapNode* w = q->parent->parent->right;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->right) {
                            q = q->parent;
                            RotateLeft(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateRight(q->parent->parent);
                    }
                } else {
                    BlockMapNode* w = q->parent->parent->left;
                    if (w->color == 0) {
                        q->parent->color = 1;
                        w->color = 1;
                        q->parent->parent->color = 0;
                        q = q->parent->parent;
                    } else {
                        if (q == q->parent->left) {
                            q = q->parent;
                            RotateRight(q);
                        }
                        q->parent->color = 1;
                        q->parent->parent->color = 0;
                        RotateLeft(q->parent->parent);
                    }
                }
            }
            head->parent->color = 1;
        }
        return MapInsertResult(it, 1);
    }
    it2 = BlockMapIter(y);
    if (less) {
        if (BlockMapIter(y) == Begin())
            return MapInsertResult(*InsertHinted((BlockMapIter*)&p, x, y, p), 1);
        it2.PrevNode();
    }
    if (key_compare(it2.ptr->value.address, p->address))
        return MapInsertResult(*InsertHinted((BlockMapIter*)&p, x, y, p), 1);
    return MapInsertResult(it2, 0);
}

// FUNCTION: 0x4dc910
BlockMapIter BlockMap::erase(BlockMapIter _P)
{
    BlockMapNode* _X;
    BlockMapNode* _Y = (_P++)._Mynode();
    BlockMapNode* _Z = _Y;
    std::_Lockit _Lk;
    if (_Y->left == g_blockMapNil)
        _X = _Y->right;
    else if (_Y->right == g_blockMapNil)
        _X = _Y->left;
    else
        _Y = _Min(_Y->right), _X = _Y->right;
    if (_Y != _Z)
        {
        _Z->left->parent = _Y;
        _Y->left = _Z->left;
        if (_Y == _Z->right)
            _X->parent = _Y;
        else
            {
            _X->parent = _Y->parent;
            _Y->parent->left = _X;
            _Y->right = _Z->right;
            _Z->right->parent = _Y;
            }
        if (_Root() == _Z)
            _Root() = _Y;
        else if (_Z->parent->left == _Z)
            _Z->parent->left = _Y;
        else
            _Z->parent->right = _Y;
        _Y->parent = _Z->parent;
        std::swap(_Y->color, _Z->color);
        _Y = _Z;
        }
    else
        {
        _X->parent = _Y->parent;
        if (_Root() == _Z)
            _Root() = _X;
        else if (_Z->parent->left == _Z)
            _Z->parent->left = _X;
        else
            _Z->parent->right = _X;
        if (_Lmost() != _Z)
            ;
        else if (_Z->right == g_blockMapNil)
            _Lmost() = _Z->parent;
        else
            _Lmost() = _Min(_X);
        if (_Rmost() != _Z)
            ;
        else if (_Z->left == g_blockMapNil)
            _Rmost() = _Z->parent;
        else
            _Rmost() = _Max(_X);
        }
    if (_Y->color == 1)
        {
        while (_X != _Root() && _X->color == 1)
            if (_X == _X->parent->left)
                {
                BlockMapNode* _W = _X->parent->right;
                if (_W->color == 0)
                    {
                    _W->color = 1;
                    _X->parent->color = 0;
                    _Lrotate(_X->parent);
                    _W = _X->parent->right;
                    }
                if (_W->left->color == 1 && _W->right->color == 1)
                    {
                    _W->color = 0;
                    _X = _X->parent;
                    }
                else
                    {
                    if (_W->right->color == 1)
                        {
                        _W->left->color = 1;
                        _W->color = 0;
                        _Rrotate(_W);
                        _W = _X->parent->right;
                        }
                    _W->color = _X->parent->color;
                    _X->parent->color = 1;
                    _W->right->color = 1;
                    _Lrotate(_X->parent);
                    break;
                    }
                }
            else
                {
                BlockMapNode* _W = _X->parent->left;
                if (_W->color == 0)
                    {
                    _W->color = 1;
                    _X->parent->color = 0;
                    _Rrotate(_X->parent);
                    _W = _X->parent->left;
                    }
                if (_W->right->color == 1 && _W->left->color == 1)
                    {
                    _W->color = 0;
                    _X = _X->parent;
                    }
                else
                    {
                    if (_W->left->color == 1)
                        {
                        _W->right->color = 1;
                        _W->color = 0;
                        _Lrotate(_W);
                        _W = _X->parent->left;
                        }
                    _W->color = _X->parent->color;
                    _X->parent->color = 1;
                    _W->left->color = 1;
                    _Rrotate(_X->parent);
                    break;
                    }
                }
        _X->color = 1;
        }
    if (_Y != 0)
        {
        *(void**)_Y = g_blockMapFreeList;
        g_blockMapFreeList = _Y;
        }
    --size;
    return (_P);
}

// FUNCTION: 0x4dce00
BlockMapIter BlockMap::Find(const unsigned int& key)
{
    BlockMapIter p = BlockMapIter(LowerBound(key));
    return (p == End() || key_compare(key, p.ptr->value.address)) ? End() : p;
}

// FUNCTION: 0x4dd150
void FreeBlockMap::RotateLeft(FreeBlockNode* x)
{
    std::_Lockit lock;
    FreeBlockNode* y = x->right;
    x->right = y->left;
    if (y->left != DAT_00528a54)
        y->left->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

#pragma auto_inline(off)
// FUNCTION: 0x4dd1b0
LeftChainNode* __cdecl FindLeftmost(LeftChainNode* p)
{
    std::_Lockit lock;
    while (p->left != DAT_00528a54)
        p = p->left;
    return p;
}
#pragma auto_inline(on)

// FUNCTION: 0x4dd1f0
void FreeBlockMap::RotateRight(FreeBlockNode* x)
{
    std::_Lockit lock;
    FreeBlockNode* y = x->left;
    x->left = y->right;
    if (y->right != DAT_00528a54)
        y->right->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;
    y->right = x;
    x->parent = y;
}

// FUNCTION: 0x4dd250
FreeBlockNode* FreeBlockMap::LowerBound(const unsigned int& kv)
{
    std::_Lockit lock;
    FreeBlockNode* x = head->parent;
    FreeBlockNode* y = head;
    while (x != DAT_00528a54)
        if (key_compare(kv, x->value.offset))
            y = x, x = x->left;
        else
            x = x->right;
    return y;
}

#pragma auto_inline(off)
// FUNCTION: 0x4dd2a0
void FreeBlockIter::PrevNode()
{
    std::_Lockit lock;
    if (ptr->color == 0 && ptr->parent->parent == ptr) {
        ptr = ptr->right;
    } else if (ptr->left != DAT_00528a54) {
        ptr = FreeBlockMaxNode(ptr->left);
    } else {
        FreeBlockNode* p;
        while (ptr == (p = ptr->parent)->left) {
            ptr = p;
        }
        ptr = p;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4dd340
void FreeBlockIter::NextNode()
{
    std::_Lockit lock;
    if (ptr->right != DAT_00528a54)
        ptr = Min(ptr->right);
    else {
        FreeBlockNode* p;
        while (ptr == (p = ptr->parent)->right)
            ptr = p;
        if (ptr->right != p)
            ptr = p;
    }
}

#pragma auto_inline(off)
// FUNCTION: 0x4dd3d0
BlockMapIter BlockMap::lower_bound(const unsigned int& key)
{
    return BlockMapIter(Lbound(key));
}
#pragma auto_inline(on)

// FUNCTION: 0x4dd430
BlockMapIter BlockMap::InsertHinted(BlockMapNode* x, BlockMapNode* y,
                                    const BlockInfo& v)
{
    std::_Lockit lock;
    BlockMapNode* z = (BlockMapNode*)allocator.Allocate(0x40);
    z->parent = y;
    z->color = 0;
    z->left = (BlockMapNode*)g_blockMapNil;
    z->right = (BlockMapNode*)g_blockMapNil;
    new (&z->value) BlockInfo(v);
    size++;
    if (y == head || x != g_blockMapNil || key_compare(v.address, y->value.address)) {
        y->left = z;
        if (y == head) {
            head->parent = z;
            head->right = z;
        } else if (y == head->left) {
            head->left = z;
        }
    } else {
        y->right = z;
        if (y == head->right) {
            head->right = z;
        }
    }
    for (x = z; x != head->parent && x->parent->color == 0; ) {
        if (x->parent == x->parent->parent->left) {
            BlockMapNode* w = x->parent->parent->right;
            if (w->color == 0) {
                x->parent->color = 1;
                w->color = 1;
                x->parent->parent->color = 0;
                x = x->parent->parent;
            } else {
                if (x == x->parent->right) {
                    x = x->parent;
                    Lrotate(x);
                }
                x->parent->color = 1;
                x->parent->parent->color = 0;
                Rrotate(x->parent->parent);
            }
        } else {
            BlockMapNode* w = x->parent->parent->left;
            if (w->color == 0) {
                x->parent->color = 1;
                w->color = 1;
                x->parent->parent->color = 0;
                x = x->parent->parent;
            } else {
                if (x == x->parent->left) {
                    x = x->parent;
                    Rrotate(x);
                }
                x->parent->color = 1;
                x->parent->parent->color = 0;
                Lrotate(x->parent->parent);
            }
        }
    }
    head->parent->color = 1;
    return BlockMapIter(z);
}

// FUNCTION: 0x4dd710
void BlockMap::RotateLeft(BlockMapNode* x)
{
    std::_Lockit lock;
    BlockMapNode* y = x->right;
    x->right = y->left;
    if (y->left != g_blockMapNil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

// FUNCTION: 0x4dd770
void BlockMap::RotateRight(BlockMapNode* x)
{
    std::_Lockit lock;
    BlockMapNode* y = x->left;
    x->left = y->right;
    if (y->right != g_blockMapNil)
        y->right->parent = x;
    y->parent = x->parent;
    if (x == head->parent)
        head->parent = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;
    y->right = x;
    x->parent = y;
}

// FUNCTION: 0x4dd820
void BlockMapIter::PrevNode()
{
    std::_Lockit lock;
    if (ptr->color == 0 && ptr->parent->parent == ptr) {
        ptr = ptr->right;
    } else if (ptr->left != g_blockMapNil) {
        ptr = BlockMapMaxNode(ptr->left);
    } else {
        BlockMapNode* p;
        while (ptr == (p = ptr->parent)->left) {
            ptr = p;
        }
        ptr = p;
    }
}

// FUNCTION: 0x4dd8c0 ?insert@?$vector@UFreedBlockRecord@@VFreedBlockAllocator@@@std@@QAEXPAUFreedBlockRecord@@IABU3@@Z
FreedBlocksInsertFn g_insert_004dd8c0 = &FreedBlocks::insert;

// FUNCTION: 0x4ddbe0
MapInsertResult* MapInsertResult::Assign(int* iter, unsigned char* inserted)
{
    MapInsertResult* eax = this;
    int* ecx = iter;
    int edx = *ecx;
    unsigned char* ecx2 = (unsigned char*)inserted;
    eax->first.ptr = (FreeBlockNode*)edx;
    unsigned char dl = *ecx2;
    eax->second = dl;
    return eax;
}
#include <math.h>
#include <yvals.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include "../state/sim_globals.h"

extern void* g_freeBlockFreeList;
extern void (*g_outOfMemoryHandler)();

static inline FreeBlockNode* NewFreeBlockNode()
{
    if (g_freeBlockFreeList == 0) {
        FreeBlockNode* block;
        do {
            block = (FreeBlockNode*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        FreeBlockNode* head = (FreeBlockNode*)g_freeBlockFreeList;
        for (int i = 0; i < 0x155; i++) {
            block->left = head;
            head = block;
            block++;
        }
        g_freeBlockFreeList = head;
    }
    FreeBlockNode* node = (FreeBlockNode*)g_freeBlockFreeList;
    g_freeBlockFreeList = node->left;
    return node;
}

#pragma auto_inline(off)
// FUNCTION: 0x4ddc00
FreeBlockNode* FreeBlockMap::CreateNode(int parent, int color)
{
    FreeBlockNode* node = NewFreeBlockNode();
    node->parent = (FreeBlockNode*)parent;
    node->color = color;
    return node;
}
#pragma auto_inline(on)

extern void* g_blockMapNil;

#pragma auto_inline(off)
// FUNCTION: 0x4ddc90
BlockMapNode* BlockMap::LowerBound(const unsigned int& key)
{
    std::_Lockit lock;
    BlockMapNode* x = head->parent;
    BlockMapNode* y = head;
    while (x != g_blockMapNil) {
        if (key_compare(x->value.address, key))
            x = x->right;
        else
            y = x, x = x->left;
    }
    return y;
}
#pragma auto_inline(on)

extern void* g_blockMapFreeList;

static inline BlockMapNode* NewBlockMapNode()
{
    if (g_blockMapFreeList == 0) {
        BlockMapNode* block;
        do {
            block = (BlockMapNode*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        BlockMapNode* head = (BlockMapNode*)g_blockMapFreeList;
        for (int i = 0; i < 0x80; i++) {
            block->left = head;
            head = block;
            block++;
        }
        g_blockMapFreeList = head;
    }
    BlockMapNode* node = (BlockMapNode*)g_blockMapFreeList;
    g_blockMapFreeList = node->left;
    return node;
}

#pragma auto_inline(off)
// FUNCTION: 0x4ddce0
BlockMapNode* BlockMap::CreateNode(int parent, int color)
{
    BlockMapNode* node = NewBlockMapNode();
    node->parent = (BlockMapNode*)parent;
    node->color = color;
    return node;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4ddd70
void* FreeBlockAllocator::Allocate(unsigned int n)
{
    if (g_freeBlockFreeList == 0) {
        unsigned int rem = 0x2000;
        char* block;
        do {
            block = (char*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        for (; rem >= n; rem -= n) {
            *(void**)block = g_freeBlockFreeList;
            g_freeBlockFreeList = block;
            block += n;
        }
    }
    void* p = g_freeBlockFreeList;
    g_freeBlockFreeList = *(void**)g_freeBlockFreeList;
    return p;
}
#pragma auto_inline(on)

#pragma auto_inline(off)
// FUNCTION: 0x4dddf0
void* BlockMapAllocator::Allocate(unsigned int n)
{
    if (g_blockMapFreeList == 0) {
        unsigned int rem = 0x2000;
        char* block;
        do {
            block = (char*)host_mem_alloc(0x2000);
            if (block == 0 && g_outOfMemoryHandler != 0) {
                g_outOfMemoryHandler();
            }
        } while (block == 0 && g_outOfMemoryHandler != 0);
        if (block == 0) {
            return 0;
        }
        for (; rem >= n; rem -= n) {
            *(void**)block = g_blockMapFreeList;
            g_blockMapFreeList = block;
            block += n;
        }
    }
    void* p = g_blockMapFreeList;
    g_blockMapFreeList = *(void**)g_blockMapFreeList;
    return p;
}
#pragma auto_inline(on)

static inline BlockMapNode* BlockMapMinNode(BlockMapNode* p)
{
    std::_Lockit lock;
    while (p->left != g_blockMapNil)
        p = p->left;
    return p;
}

#pragma auto_inline(off)
// FUNCTION: 0x4dde70
void BlockMapIter::NextNode()
{
    std::_Lockit lock;
    if (ptr->right != g_blockMapNil)
        ptr = BlockMapMinNode(ptr->right);
    else {
        BlockMapNode* p;
        while (ptr == (p = ptr->parent)->right)
            ptr = p;
        if (ptr->right != p)
            ptr = p;
    }
}
#pragma auto_inline(on)

// FUNCTION: 0x4df160
void SetFpuFussy()
{
    static CommandLineSwitch fussy("fpufussy", 1, 0, "-fpufussy", "-fpunofussy", 0, 0);
    if (fussy.on)
        _controlfp(0, _EM_ZERODIVIDE | _EM_INVALID);
    else
        _controlfp(_EM_ZERODIVIDE | _EM_INVALID, _EM_ZERODIVIDE | _EM_INVALID);
}

// FUNCTION: 0x4df1d0
void FpuFussyAtexitHandler(void)
{
}

// FUNCTION: 0x4e3e10
int StringMaxSize(void)
{
    return 0xfffffffd;
}

#include "../data/exe_only.h"

// FUNCTION: 0x4d8310
static void __cdecl CheckFillPattern(unsigned int* block, unsigned int fill, unsigned int size)
{
    unsigned int* p = block;
    while (size >= 4) {
        if (*p != fill)
            __asm int 3
        p++;
        size -= 4;
    }
    unsigned char* bytes = (unsigned char*)p;
    unsigned char* pattern = (unsigned char*)&fill;
    for (unsigned int n = size; n > 0; n--, bytes++, pattern++)
        if (*bytes != *pattern)
            __asm int 3
}

// FUNCTION: 0x4da1d0
void __cdecl InitDebugSupport(unsigned int flags)
{
    if (g_debugSupportInited)
        return;
    g_debugSupportInited = 1;
    char* option = FindCommandLineSwitch("-debughelper");
    if (option) {
        int value = 0;
        sscanf(option, "=%d", &value);
        host_debug_helper(value);
        AbortProgram();
    }
    getenv("windir");
    if (!(flags & 2))
        host_crash_install();
    if (!(flags & 4))
        SetFpuFussy();
}

// FUNCTION: 0x4dacf0
static unsigned int __cdecl AllocDebugBlock(unsigned int n, unsigned int arg2)
{
    void* lock = GetAllocLock();
    host_mutex_lock(lock);
    unsigned int size = n;
    unsigned int res = 0;
    if (size == 0)
        size = 1;
    unsigned int need = RoundUpToPage(size);
    unsigned int want = RoundUpToDoublePage(size);
    if (want < need) {
        host_mutex_unlock(lock);
        return 0;
    }
    unsigned int base = GetFreeBlockSet()->TakeFreeBlock(want);
    if (base != 0) {
        res = (unsigned int)host_mem_commit((void*)base, need);
        if (res == 0) {
            FreeBlockMap* pool = GetFreeBlockSet();
            pool->AddFreeBlock(FreeBlockRange(base, want));
        }
    }
    if (res == 0) {
        host_mutex_unlock(lock);
        return 0;
    }
    unsigned int pad = (0 - (n & 0xfff)) & 0xfff;
    if (IsBackAlign()) {
        FillPattern((void*)res, GetDebugFillPattern(), pad);
        res += pad;
    } else {
        FillPattern((void*)(res + n), GetDebugFillPattern(), pad);
    }
    BlockInfo rec(res, n, g_allocSerial, arg2, nullptr);
    BlockMap* blocks = (BlockMap*)GetBlockMap();
    blocks->Insert(&rec);
    CountAlloc(n);
    g_committedBytes += (n + 0xfff) & 0xfffff000;
    if (g_committedBytes > g_committedBytesPeak)
        g_committedBytesPeak = g_committedBytes;
    host_mutex_unlock(lock);
    return res;
}

class FreeBlockPoolAllocator : public std::allocator<int> {
public:
    char* _Charalloc(size_t n)
    {
        return (char*)((FreeBlockAllocator*)this)->Allocate(n);
    }
};

typedef std::map<unsigned int, int, std::less<unsigned int>, FreeBlockPoolAllocator> FreeBlockStdMap;

FreeBlockMap::FreeBlockMap() : total(0)
{
    new (this) FreeBlockStdMap;
    int n = 4;
    do {
        unsigned int len = 0x10000000;
        void* m = host_mem_reserve(len + 0x2000);
        bool reserved = true;
        for (;;) {
            if (m != nullptr && (unsigned int)m + len <= 0x80000000u)
                break;
            if (m != nullptr)
                host_mem_release(m);
            len = (len >> 1) & 0x7fffe000;
            if (len < 0x10000) {
                reserved = false;
                break;
            }
            m = host_mem_reserve(len + 0x2000);
        }
        if (reserved) {
            total = total + len;
            FreeBlockRange pair;
            pair.offset = (unsigned int)m;
            pair.length = len;
            AddFreeBlock(pair);
        }
    } while (--n);
}

// FUNCTION: 0x4db610
static FreeBlockMap* __stdcall GetFreeBlockSet()
{
    if (g_freeBlockSet == nullptr)
        g_freeBlockSet = new FreeBlockMap;
    return g_freeBlockSet;
}

// FUNCTION: 0x4db7d0
static void __cdecl FreeDebugBlock(void* p, int flags)
{
    if (p == nullptr)
        return;
    void* lock = GetAllocLock();
    host_mutex_lock(lock);
    BlockMap* live = (BlockMap*)GetBlockMap();
    BlockInfo rec((unsigned int)p, 0, 0, 0, nullptr);
    BlockMapIter it = live->Find(rec.address);
    if (it == ((BlockMap*)GetBlockMap())->end()) {
        host_mutex_unlock(lock);
        return;
    }
    unsigned int size = (unsigned int)it->size;
    unsigned int pad = (0 - (size & 0xfff)) & 0xfff;
    if (IsBackAlign())
        CheckFillPattern((unsigned int *)((char*)p - pad), GetDebugFillPattern(), pad);
    else
        CheckFillPattern((unsigned int *)((char*)p + size), GetDebugFillPattern(), pad);
    FreedBlockRecord* old = (FreedBlockRecord*)&*it;
    FreedBlockRing<int>* arena = GetFreedBlockRing();
    FreedBlocks& ring = arena->ring;
    if ((unsigned int)arena->count < 0x2000)
        ring.push_back(*old);
    else
        ring[(unsigned int)arena->count & 0x1fff] = *old;
    arena->count++;
    ((BlockMap*)GetBlockMap())->erase(it);
    CountFree(size);
    g_committedBytes -= (size + 0xfff) & 0xfffff000;
    p = (void*)((unsigned int)p & 0xfffff000);
    if (size == 0)
        size = 1;
    unsigned int commit = RoundUpToPage(size);
    host_mem_decommit(p, commit);
    unsigned int reserve = RoundUpToDoublePage(size);
    FreeBlockMap* blocks = GetFreeBlockSet();
    blocks->AddFreeBlock(FreeBlockRange((unsigned int)p, reserve));
    host_mutex_unlock(lock);
}
