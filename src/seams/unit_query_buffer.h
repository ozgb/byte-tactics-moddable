#ifndef UNIT_QUERY_BUFFER_H
#define UNIT_QUERY_BUFFER_H

#include <vector>

#include "../../interfaces/ref.h"

struct Unit;

class UnitQueryBuffer {
public:
    enum { INLINE_CAPACITY = 64 };

    Ref<Unit>* Data() { return heap.empty() ? local : &heap[0]; }
    int Capacity() const { return heap.empty() ? (int)INLINE_CAPACITY : (int)heap.size(); }
    bool Grow(int total)
    {
        if (total <= Capacity())
            return false;
        heap.resize(total);
        return true;
    }
    int Written(int total) const { return total < Capacity() ? total : Capacity(); }
    Ref<Unit>& operator[](int i) { return Data()[i]; }

private:
    Ref<Unit> local[INLINE_CAPACITY];
    std::vector<Ref<Unit> > heap;
};

#endif
