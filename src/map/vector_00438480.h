#ifndef MAP_VECTOR_00438480_H
#define MAP_VECTOR_00438480_H

#include <memory>

template <class T, class A = std::allocator<T> >
class MsvcVector {
public:
    explicit MsvcVector(const A& al = A())
        : allocator(al), first(0), last(0), end(0) {}
    ~MsvcVector()
    {
        allocator.deallocate(first, end - first);
        first = 0, last = 0, end = 0;
    }

    A allocator;
    T* first;
    T* last;
    T* end;
};

#endif
