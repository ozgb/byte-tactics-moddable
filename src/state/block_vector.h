#ifndef STATE_BLOCK_VECTOR_H
#define STATE_BLOCK_VECTOR_H

#include <new>

#include "block.h"
#include "../../interfaces/ref.h"

#pragma pack(push, 4)

// A growable array in the state block: Ref words for begin, end and capacity, the growth rule of VC5's std::vector.
template <class T>
class BlockVector {
public:
    typedef unsigned int size_type;
    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    // An empty vector that owns no storage.
    BlockVector() : first_(0), last_(0), end_(0) {}
    // A copy of x in storage of exactly x.size() elements.
    BlockVector(const BlockVector& x)
    {
        first_ = Allocate(x.size());
        last_ = Copy(x.begin(), x.end(), first_.get());
        end_ = last_;
    }
    // Destroys the elements and returns the storage to the block.
    ~BlockVector()
    {
        Destroy(first_.get(), last_.get());
        SimFree(first_.get());
        first_ = 0;
        last_ = 0;
        end_ = 0;
    }
    // Copies x in place when it fits, else into new storage.
    BlockVector& operator=(const BlockVector& x)
    {
        if (this == &x) {
        } else if (x.size() <= size()) {
            iterator s = CopyAssign(x.begin(), x.end(), first_.get());
            Destroy(s, last_.get());
            last_ = first_.get() + x.size();
        } else if (x.size() <= capacity()) {
            const_iterator s = x.begin() + size();
            CopyAssign(x.begin(), s, first_.get());
            Copy(s, x.end(), last_.get());
            last_ = first_.get() + x.size();
        } else {
            Destroy(first_.get(), last_.get());
            SimFree(first_.get());
            first_ = Allocate(x.size());
            last_ = Copy(x.begin(), x.end(), first_.get());
            end_ = last_;
        }
        return *this;
    }
    // Grows the storage to n elements when it is smaller.
    void reserve(size_type n)
    {
        if (capacity() < n) {
            iterator s = Allocate(n);
            Copy(first_.get(), last_.get(), s);
            Destroy(first_.get(), last_.get());
            SimFree(first_.get());
            end_ = s + n;
            last_ = s + size();
            first_ = s;
        }
    }
    // The elements the storage holds.
    size_type capacity() const { return first_.get() == 0 ? 0 : end_.get() - first_.get(); }
    // The first element.
    iterator begin() { return first_.get(); }
    // The first element.
    const_iterator begin() const { return first_.get(); }
    // One past the last element.
    iterator end() { return last_.get(); }
    // One past the last element.
    const_iterator end() const { return last_.get(); }
    // Grows with copies of x or shrinks to n elements.
    void resize(size_type n, const T& x = T())
    {
        if (size() < n)
            insert(end(), n - size(), x);
        else if (n < size())
            erase(begin() + n, end());
    }
    // The elements held.
    size_type size() const { return first_.get() == 0 ? 0 : last_.get() - first_.get(); }
    // True when no element is held.
    bool empty() const { return size() == 0; }
    // The element at p.
    T& operator[](size_type p) { return *(begin() + p); }
    // The element at p.
    const T& operator[](size_type p) const { return *(begin() + p); }
    // Appends a copy of x.
    void push_back(const T& x) { insert(end(), x); }
    // Inserts a copy of x before p and returns where it went.
    iterator insert(iterator p, const T& x)
    {
        size_type o = p - begin();
        insert(p, 1, x);
        return begin() + o;
    }
    // Inserts m copies of x before p, growing to twice the size when the storage is full.
    void insert(iterator p, size_type m, const T& x)
    {
        if (end_.get() - last_.get() < m) {
            size_type n = size() + (m < size() ? size() : m);
            iterator s = Allocate(n);
            iterator q = Copy(first_.get(), p, s);
            Fill(q, m, x);
            Copy(p, last_.get(), q + m);
            Destroy(first_.get(), last_.get());
            SimFree(first_.get());
            end_ = s + n;
            last_ = s + size() + m;
            first_ = s;
        } else if (last_.get() - p < m) {
            iterator l = last_.get();
            Copy(p, l, p + m);
            Fill(l, m - (l - p), x);
            for (iterator i = p; i != l; ++i)
                *i = x;
            last_ = l + m;
        } else if (0 < m) {
            iterator l = last_.get();
            Copy(l - m, l, l);
            for (iterator from = l - m, to = l; from != p;)
                *--to = *--from;
            for (iterator i = p; i != p + m; ++i)
                *i = x;
            last_ = l + m;
        }
    }
    // Removes the element at p and returns where the next one went.
    iterator erase(iterator p)
    {
        iterator l = last_.get();
        CopyAssign(p + 1, l, p);
        Destroy(l - 1, l);
        last_ = l - 1;
        return p;
    }
    // Removes the elements from f up to l and returns where the next one went.
    iterator erase(iterator f, iterator l)
    {
        iterator s = CopyAssign(l, end(), f);
        Destroy(s, end());
        last_ = s;
        return f;
    }
    // Removes every element and keeps the storage.
    void clear() { erase(begin(), end()); }

private:
    struct Empty {};

    static iterator Allocate(size_type n) { return (iterator)SimAlloc((unsigned int)(n * sizeof(T))); }
    static void Destroy(iterator f, iterator l)
    {
        for (; f != l; ++f)
            f->~T();
    }
    static iterator Copy(const_iterator f, const_iterator l, iterator p)
    {
        for (; f != l; ++p, ++f)
            new ((void*)p) T(*f);
        return p;
    }
    static iterator CopyAssign(const_iterator f, const_iterator l, iterator p)
    {
        for (; f != l; ++p, ++f)
            *p = *f;
        return p;
    }
    static void Fill(iterator f, size_type n, const T& x)
    {
        for (; 0 < n; --n, ++f)
            new ((void*)f) T(x);
    }

    Empty allocator_;
    Ref<T> first_;
    Ref<T> last_;
    Ref<T> end_;
};

#pragma pack(pop)

static_assert(sizeof(BlockVector<int>) == 16, "a block vector is an empty member padded to 4 and three words");

#endif
