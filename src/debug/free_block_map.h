#ifndef FREE_BLOCK_MAP_H
#define FREE_BLOCK_MAP_H

#include <stddef.h>
#include <yvals.h>
#include "debug_lib.h"
#include "../../interfaces/host.h"

extern void* DAT_00528a54;

class BlockMapIter;

struct UnsignedLess {
    bool operator()(const unsigned int& a, const unsigned int& b) const { return a < b; }
};

struct FreeBlockOffsetKey {
    const unsigned int& operator()(const FreeBlockRange& x) const { return x.offset; }
};

class FreeBlockAllocator {
public:
    void* Allocate(unsigned int n);
};

class FreeBlockIter {
public:
    FreeBlockNode* ptr;

    FreeBlockIter() {}
    FreeBlockIter(FreeBlockNode* q) : ptr(q) {}
    bool operator==(const FreeBlockIter& o) const { return ptr == o.ptr; }
    bool operator!=(const FreeBlockIter& o) const { return !(*this == o); }
    FreeBlockRange& operator*() const { return ptr->value; }
    FreeBlockRange* operator->() const { return &ptr->value; }
    FreeBlockIter& operator++() { Inc(); return *this; }
    FreeBlockIter operator++(int) { FreeBlockIter tmp = *this; ++*this; return tmp; }
    FreeBlockIter& operator--() { PrevNode(); return *this; }
    FreeBlockIter operator--(int) { FreeBlockIter tmp = *this; --*this; return tmp; }
    FreeBlockNode* Mynode() const { return ptr; }
    FreeBlockNode* _Mynode() const { return ptr; }
    void PrevNode();
    void NextNode();
    FreeBlockIter Previous(int);
    FreeBlockIter Next(int);
    void Dec();

    void Inc()
    {
        LeftChainNode* __cdecl FindLeftmost(LeftChainNode*);
        std::_Lockit lock;
        if (ptr->right != DAT_00528a54)
            ptr = (FreeBlockNode*)FindLeftmost((LeftChainNode*)ptr->right);
        else {
            FreeBlockNode* p;
            while (ptr == (p = ptr->parent)->right)
                ptr = p;
            if (ptr->right != p)
                ptr = p;
        }
    }
};

class MapInsertResult {
public:
    FreeBlockIter first;
    unsigned char second;

    MapInsertResult() {}
    MapInsertResult(const FreeBlockIter& i, const unsigned char& b) : first(i), second(b) {}
    MapInsertResult(const BlockMapIter& i, unsigned char b);
    inline MapInsertResult(const MapInsertResult& o)
    {
        second = o.second;
        first = o.first;
    }
    MapInsertResult* Assign(int* iter, unsigned char* inserted);
    MapInsertResult* Assign(const FreeBlockIter& first, unsigned char& second);
    MapInsertResult* Assign(const FreeBlockIter& first, const bool& second);
};

class FreeBlockMap {
public:
    enum Redbl { _Red, _Black };

    FreeBlockAllocator allocator;
    UnsignedLess key_compare;
    FreeBlockNode* head;
    bool multi;
    unsigned int count;
    unsigned int total;

    static FreeBlockNode*& Left(FreeBlockNode* p) { return p->left; }
    static FreeBlockNode*& Parent(FreeBlockNode* p) { return p->parent; }
    static FreeBlockNode*& Right(FreeBlockNode* p) { return p->right; }
    static int& Color(FreeBlockNode* p) { return p->color; }
    static unsigned int* _Value(FreeBlockNode* p) { return (unsigned int*)&p->value; }
    static const unsigned int& Key(FreeBlockNode* p) { return FreeBlockOffsetKey()(p->value); }

    FreeBlockNode*& Root() { return Parent(head); }
    FreeBlockNode*& Lmost() { return Left(head); }
    FreeBlockNode*& Rmost() { return Right(head); }

    static FreeBlockNode* Min(FreeBlockNode* p);
    static FreeBlockNode* Max(FreeBlockNode* p);
    void Lrotate(FreeBlockNode* x);
    void Rrotate(FreeBlockNode* x);

    static void Destval(unsigned int*) { }

    static void Freenode(FreeBlockNode* p);

    FreeBlockIter begin() { return FreeBlockIter(head->left); }
    FreeBlockIter end() { return FreeBlockIter(head); }
    unsigned int size() const { return count; }
    bool Neq(FreeBlockIter a, FreeBlockIter b) { return !(a == b); }
    FreeBlockIter upper_bound(const FreeBlockRange& k)
    {
        return FreeBlockIter(LowerBound(k));
    }

    FreeBlockNode* Ubound(const unsigned int& kv);
    void Tail(unsigned base, unsigned len, FreeBlockIter& it);

    void* operator new(size_t n) { return host_mem_alloc(n); }
    FreeBlockMap();

    void AddFreeBlock(FreeBlockRange p);
    unsigned int TakeFreeBlock(unsigned int bytes);
    bool GrowReservation(unsigned int size);
    bool Grow(unsigned int size)
    {
        unsigned int len = 0x10000000;
        unsigned int base;

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
        total += len;
        AddFreeBlock(FreeBlockRange(base, len));
        return true;
    }

    FreeBlockIter Begin() { return FreeBlockIter(head->left); }
    int* Begin(int* out);
    FreeBlockNode* CreateNode(int parent, int color);
    MapInsertResult TreeInsert(const FreeBlockRange& V);
    MapInsertResult InsertOrFind(const FreeBlockRange& V);
    MapInsertResult InsertOrFindInline(FreeBlockRange* p);
    MapInsertResult InsertOrFindInline(const FreeBlockRange& v);
    void InsertOrFindInline(struct FreeBlockInsertResult* it, struct FreeBlockSignedRange* p);
    FreeBlockIter Insert(FreeBlockNode* x, FreeBlockNode* y, const FreeBlockRange* v);
    FreeBlockIter Erase(FreeBlockIter _P);
    void Erase(FreeBlockIter* out, FreeBlockIter it);
    class FreeBlockEraseIter Erase(FreeBlockEraseIter it);
    class FreeBlockEraseConstIter EraseCopyIter(FreeBlockEraseConstIter it);
    class FreeBlockUpperBoundIter UpperBound(const unsigned int& kv);
    class FreeBlockLowerBoundIter lower_bound(const unsigned int& kv);
    void lower_bound(FreeBlockIter* out, const unsigned int& kv);
    FreeBlockNode* LowerBound(const FreeBlockRange& k);
    FreeBlockNode* LowerBound(const unsigned int& kv);
    void RotateLeft(FreeBlockNode* x);
    void RotateRight(FreeBlockNode* x);
};

#endif
