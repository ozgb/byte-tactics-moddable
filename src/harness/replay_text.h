#ifndef HARNESS_REPLAY_TEXT_H
#define HARNESS_REPLAY_TEXT_H

class ReplayText {
public:
    constexpr ReplayText() : buffer(nullptr), used(0), capacity(0) {}
    void Free();
    void Clear();
    void Add(const char* text);
    void AddChar(char c);
    void AddInt(int value);
    void AddUnsigned(unsigned value);
    void AddHex32(unsigned value);
    void AddF32(unsigned bits);
    void AddF64(unsigned low, unsigned high);
    void AddString(const char* text);
    void AddText(const ReplayText& other);
    const char* Data() const;
    unsigned Size() const;

private:
    ReplayText(const ReplayText&);
    ReplayText& operator=(const ReplayText&);
    void Reserve(unsigned extra);
    char* buffer;
    unsigned used;
    unsigned capacity;
};

class ReplayKnown {
public:
    ReplayKnown() : count(0), slotCount(0), entries(nullptr), slots(nullptr), capacity(0) {}
    void Reset();
    bool Changed(const char* name, const char* value);
    void Set(const char* name, const char* value);
    const char* Get(const char* name) const;
    char* Edit(const char* name);

private:
    ReplayKnown(const ReplayKnown&);
    ReplayKnown& operator=(const ReplayKnown&);
    struct Entry {
        char* name;
        char* value;
    };
    int Find(const char* name) const;
    void Grow();
    int count;
    unsigned slotCount;
    Entry* entries;
    int* slots;
    int capacity;
};

class ReplayOrdinals {
public:
    void Reset();
    unsigned Of(unsigned kind, const void* object);
    unsigned Find(unsigned kind, const void* object) const;
    void Bind(unsigned kind, unsigned ordinal, const void* object);

private:
    enum { KINDS = 16, CAPACITY = 4096 };
    unsigned count[KINDS];
    const void* objects[KINDS][CAPACITY];
};

#endif
