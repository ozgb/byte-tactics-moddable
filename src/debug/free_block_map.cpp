#include "../../interfaces/host.h"
#include <string>
#include <yvals.h>
#include <map>
#include <list>
#include <time.h>
#include <malloc.h>

#include "debug_lib.h"

Node_004dd1b0* __cdecl FindLeftmost(Node_004dd1b0* p);

extern Node_004db000* DAT_00528a54;
#include "../../link/globals.h"

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
    void PrevNode();
    void Inc()
    {
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
    MapInsertResult* Assign(const FreeBlockIter& first, unsigned char& second);
};


struct Less_004db000 {
    bool operator()(const unsigned int& a, const unsigned int& b) const { return a < b; }
};

class FreeBlockMap {
public:
    Less_004db000 key_compare;
    Node_004db000* head;
    unsigned char rebuild;
    unsigned int count;
    unsigned int total;

    FreeBlockIter begin() { return FreeBlockIter(head->left); }
    FreeBlockIter end() { return FreeBlockIter(head); }
    unsigned int size() const { return count; }
    bool Neq(FreeBlockIter a, FreeBlockIter b) { return !(a == b); }
    FreeBlockIter upper_bound(const Pair_004db000& k)
    {
        return FreeBlockIter(LowerBound(k));
    }

    Node_004db000* LowerBound(const Pair_004db000& k);
    FreeBlockIter Erase(FreeBlockIter it);
    MapInsertResult InsertOrFindInline(const Pair_004db000& v);
    FreeBlockIter Insert(Node_004db000* x, Node_004db000* y,
                                 Pair_004db000* v);

    void AddFreeBlock(Pair_004db000 p);
    unsigned int TakeFreeBlock(unsigned int bytes);

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
};

// FUNCTION: 0x4db1c0
unsigned int FreeBlockMap::TakeFreeBlock(unsigned int bytes)
{
    Pair_004db000 k;
    if (size() > 0) {
        k.offset = g_lastAllocOffset;
        k.length = 0;
        FreeBlockIter lb = upper_bound(k);
        if (lb != begin()) {
            FreeBlockIter it = lb;
            it--;
            if (g_lastAllocOffset >= it->offset && g_lastAllocOffset + bytes <= it->offset + it->length)
                lb = it;
        }
        FreeBlockIter cur = lb;
        int tries = 0;
        do {
            if (cur == end()) {
                cur = begin();
                g_lastAllocOffset = 0;
                g_freeBlockWraps++;
                tries++;
            }
            if (cur->length >= bytes) {
                unsigned int len, base;
                base = cur->offset;
                len = cur->length;
                Erase(cur);
                if (g_lastAllocOffset == 0)
                    g_lastAllocOffset = base;
                unsigned int mark;
                if (g_lastAllocOffset >= base && g_lastAllocOffset + bytes <= base + len)
                    mark = g_lastAllocOffset;
                else
                    mark = base;
                if (mark > base)
                    InsertOrFindInline(Pair_004db000(base, mark - base));
                unsigned int end = mark + bytes;
                if (end < base + len)
                    InsertOrFindInline(Pair_004db000(end, base - mark + len - bytes));
                g_lastAllocOffset = end;
                return mark;
            }
            cur++;
        } while (tries < 2);
    }
    if (Grow(bytes))
        return TakeFreeBlock(bytes);
    return 0;
}
