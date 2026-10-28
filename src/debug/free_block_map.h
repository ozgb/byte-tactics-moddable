#ifndef FREE_BLOCK_MAP_H
#define FREE_BLOCK_MAP_H

#include <stddef.h>
#include <yvals.h>
#include "debug_lib.h"
#include "../../interfaces/host.h"

extern void* DAT_00528a54;

class BlockMapIter;

struct Less_004db000 {
    bool operator()(const unsigned int& a, const unsigned int& b) const { return a < b; }
};

struct Kfn_004db000 {
    const unsigned int& operator()(const Pair_004db000& x) const { return x.offset; }
};

class FreeBlockAllocator {
public:
    void* Allocate(unsigned int n);
};

class FreeBlockIter {
public:
    Node_004db000* ptr;

    FreeBlockIter() {}
    FreeBlockIter(Node_004db000* q) : ptr(q) {}
    bool operator==(const FreeBlockIter& o) const { return ptr == o.ptr; }
    bool operator!=(const FreeBlockIter& o) const { return !(*this == o); }
    Pair_004db000& operator*() const { return ptr->value; }
    Pair_004db000* operator->() const { return &ptr->value; }
    FreeBlockIter& operator++() { Inc(); return *this; }
    FreeBlockIter operator++(int) { FreeBlockIter tmp = *this; ++*this; return tmp; }
    FreeBlockIter& operator--() { PrevNode(); return *this; }
    FreeBlockIter operator--(int) { FreeBlockIter tmp = *this; --*this; return tmp; }
    Node_004db000* Mynode() const { return ptr; }
    Node_004db000* _Mynode() const { return ptr; }
    void PrevNode();
    void NextNode();
    FreeBlockIter Previous(int);
    FreeBlockIter Next(int);
    void Dec();

    void Inc()
    {
        Node_004dd1b0* __cdecl FindLeftmost(Node_004dd1b0*);
        std::_Lockit lock;
        if (ptr->right != DAT_00528a54)
            ptr = (Node_004db000*)FindLeftmost((Node_004dd1b0*)ptr->right);
        else {
            Node_004db000* p;
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
    Less_004db000 key_compare;
    Node_004db000* head;
    bool multi;
    unsigned int count;
    unsigned int total;

    static Node_004db000*& Left(Node_004db000* p) { return p->left; }
    static Node_004db000*& Parent(Node_004db000* p) { return p->parent; }
    static Node_004db000*& Right(Node_004db000* p) { return p->right; }
    static int& Color(Node_004db000* p) { return p->color; }
    static unsigned int* _Value(Node_004db000* p) { return (unsigned int*)&p->value; }
    static const unsigned int& Key(Node_004db000* p) { return Kfn_004db000()(p->value); }

    Node_004db000*& Root() { return Parent(head); }
    Node_004db000*& Lmost() { return Left(head); }
    Node_004db000*& Rmost() { return Right(head); }

    static Node_004db000* Min(Node_004db000* p);
    static Node_004db000* Max(Node_004db000* p);
    void Lrotate(Node_004db000* x);
    void Rrotate(Node_004db000* x);

    static void Destval(unsigned int*) { }

    static void Freenode(Node_004db000* p);

    FreeBlockIter begin() { return FreeBlockIter(head->left); }
    FreeBlockIter end() { return FreeBlockIter(head); }
    unsigned int size() const { return count; }
    bool Neq(FreeBlockIter a, FreeBlockIter b) { return !(a == b); }
    FreeBlockIter upper_bound(const Pair_004db000& k)
    {
        return FreeBlockIter(LowerBound(k));
    }

    Node_004db000* Ubound(const unsigned int& kv);
    void Tail(unsigned base, unsigned len, FreeBlockIter& it);

    void* operator new(size_t n) { return host_mem_alloc(n); }
    FreeBlockMap();

    void AddFreeBlock(Pair_004db000 p);
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
        AddFreeBlock(Pair_004db000(base, len));
        return true;
    }

    FreeBlockIter Begin() { return FreeBlockIter(head->left); }
    int* Begin(int* out);
    Node_004db000* CreateNode(int parent, int color);
    MapInsertResult TreeInsert(const Pair_004db000& V);
    MapInsertResult InsertOrFind(const Pair_004db000& V);
    MapInsertResult InsertOrFindInline(Pair_004db000* p);
    MapInsertResult InsertOrFindInline(const Pair_004db000& v);
    void InsertOrFindInline(struct InsertResult_004db450* it, struct Pair_004db450* p);
    FreeBlockIter Insert(Node_004db000* x, Node_004db000* y, const Pair_004db000* v);
    FreeBlockIter Erase(FreeBlockIter _P);
    void Erase(FreeBlockIter* out, FreeBlockIter it);
    class Iter_004dbd00 Erase(Iter_004dbd00 it);
    class ConstIter_004dbd00 EraseCopyIter(ConstIter_004dbd00 it);
    class Iter_004dbd20 UpperBound(const unsigned int& kv);
    class Iter_004dc620 lower_bound(const unsigned int& kv);
    void lower_bound(FreeBlockIter* out, const unsigned int& kv);
    Node_004db000* LowerBound(const Pair_004db000& k);
    Node_004db000* LowerBound(const unsigned int& kv);
    void RotateLeft(Node_004db000* x);
    void RotateRight(Node_004db000* x);
};

#endif
