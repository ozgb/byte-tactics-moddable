#ifndef STATE_BLOCK_VECTOR_H
#define STATE_BLOCK_VECTOR_H

#include <new>

#include "block.h"
#include "../../interfaces/ref.h"

#pragma pack(push, 4)

template <class T>
class BlockVector {
public:
    typedef unsigned int size_type;
    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    BlockVector() : padding(0), firstWord(0), lastWord(0), endWord(0) {}
    BlockVector(const BlockVector& x) : padding(0)
    {
        firstWord = Allocate(x.Size());
        lastWord = Copy(x.Begin(), x.End(), firstWord.get());
        endWord = lastWord;
    }
    ~BlockVector()
    {
        Destroy(firstWord.get(), lastWord.get());
        SimFree(firstWord.get());
        firstWord = nullptr;
        lastWord = nullptr;
        endWord = nullptr;
    }
    BlockVector& operator=(const BlockVector& x)
    {
        if (this == &x) {
        } else if (x.Size() <= Size()) {
            iterator s = CopyAssign(x.Begin(), x.End(), firstWord.get());
            Destroy(s, lastWord.get());
            lastWord = firstWord.get() + x.Size();
        } else if (x.Size() <= Capacity()) {
            const_iterator s = x.Begin() + Size();
            CopyAssign(x.Begin(), s, firstWord.get());
            Copy(s, x.End(), lastWord.get());
            lastWord = firstWord.get() + x.Size();
        } else {
            Destroy(firstWord.get(), lastWord.get());
            SimFree(firstWord.get());
            firstWord = Allocate(x.Size());
            lastWord = Copy(x.Begin(), x.End(), firstWord.get());
            endWord = lastWord;
        }
        return *this;
    }
    void Reserve(size_type n)
    {
        if (Capacity() < n) {
            iterator s = Allocate(n);
            Copy(firstWord.get(), lastWord.get(), s);
            Destroy(firstWord.get(), lastWord.get());
            SimFree(firstWord.get());
            endWord = s + n;
            lastWord = s + Size();
            firstWord = s;
        }
    }
    size_type Capacity() const { return firstWord.get() == nullptr ? 0 : endWord.get() - firstWord.get(); }
    iterator Begin() { return firstWord.get(); }
    const_iterator Begin() const { return firstWord.get(); }
    iterator End() { return lastWord.get(); }
    const_iterator End() const { return lastWord.get(); }
    void Resize(size_type n, const T& x = T())
    {
        if (Size() < n)
            Insert(End(), n - Size(), x);
        else if (n < Size())
            Erase(Begin() + n, End());
    }
    size_type Size() const { return firstWord.get() == nullptr ? 0 : lastWord.get() - firstWord.get(); }
    bool Empty() const { return Size() == 0; }
    T& operator[](size_type p) { return *(Begin() + p); }
    const T& operator[](size_type p) const { return *(Begin() + p); }
    void PushBack(const T& x) { Insert(End(), x); }
    iterator Insert(iterator p, const T& x)
    {
        size_type o = p - Begin();
        Insert(p, 1, x);
        return Begin() + o;
    }
    void Insert(iterator p, size_type m, const T& x)
    {
        if (endWord.get() - lastWord.get() < m) {
            size_type n = Size() + (m < Size() ? Size() : m);
            iterator s = Allocate(n);
            iterator q = Copy(firstWord.get(), p, s);
            Fill(q, m, x);
            Copy(p, lastWord.get(), q + m);
            Destroy(firstWord.get(), lastWord.get());
            SimFree(firstWord.get());
            endWord = s + n;
            lastWord = s + Size() + m;
            firstWord = s;
        } else if (lastWord.get() - p < m) {
            iterator l = lastWord.get();
            Copy(p, l, p + m);
            Fill(l, m - (l - p), x);
            for (iterator i = p; i != l; ++i)
                *i = x;
            lastWord = l + m;
        } else if (0 < m) {
            iterator l = lastWord.get();
            Copy(l - m, l, l);
            for (iterator from = l - m, to = l; from != p;)
                *--to = *--from;
            for (iterator i = p; i != p + m; ++i)
                *i = x;
            lastWord = l + m;
        }
    }
    iterator Erase(iterator p)
    {
        iterator l = lastWord.get();
        CopyAssign(p + 1, l, p);
        Destroy(l - 1, l);
        lastWord = l - 1;
        return p;
    }
    iterator Erase(iterator f, iterator l)
    {
        iterator s = CopyAssign(l, End(), f);
        Destroy(s, End());
        lastWord = s;
        return f;
    }
    void Clear() { Erase(Begin(), End()); }

private:
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

    unsigned char padding;
    Ref<T> firstWord;
    Ref<T> lastWord;
    Ref<T> endWord;
};

#pragma pack(pop)

static_assert(sizeof(BlockVector<int>) == 16, "a block vector is a byte padded to 4 and three words");

#endif
