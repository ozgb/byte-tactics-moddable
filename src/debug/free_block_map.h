#ifndef FREE_BLOCK_MAP_H
#define FREE_BLOCK_MAP_H

struct Pair_004db000 {
    unsigned int offset;
    unsigned int length;
    Pair_004db000() {}
    Pair_004db000(unsigned int o, unsigned int l) : offset(o), length(l) {}
};

struct Node_004db000 {
    Node_004db000* left;
    Node_004db000* parent;
    Node_004db000* right;
    Pair_004db000 value;
    int color;
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
    FreeBlockIter Previous(int);
    FreeBlockIter Next(int);
};

class MapInsertResult {
public:
    FreeBlockIter first;
    unsigned char second;
    MapInsertResult() {}
};

class FreeBlockMap {
public:
    char unknown_0[4];
    Node_004db000* head;
    unsigned char rebuild;
    unsigned int count;
    unsigned int total;

    FreeBlockIter begin() { return Begin(); }
    FreeBlockIter end() { return FreeBlockIter(head); }
    unsigned int size() const { return count; }

    FreeBlockIter Begin();
    FreeBlockIter EraseCopyIter(FreeBlockIter it);
    MapInsertResult InsertOrFind(const Pair_004db000& v);
    FreeBlockIter lower_bound(const Pair_004db000& k);
    FreeBlockIter UpperBound(const unsigned int& k);
    void AddFreeBlock(Pair_004db000 p);
    unsigned int TakeFreeBlock(unsigned int bytes);
    bool GrowReservation(unsigned int size);
};

#endif
