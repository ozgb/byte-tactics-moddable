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
    ReplayKnown() : count(0), names(nullptr), values(nullptr), slots(nullptr) {}
    void Reset();
    bool Changed(const char* name, const char* value);
    void Set(const char* name, const char* value);
    const char* Get(const char* name) const;

private:
    ReplayKnown(const ReplayKnown&);
    ReplayKnown& operator=(const ReplayKnown&);
    enum { CAPACITY = 8192, SLOTS = 16384, NAME = 64, VALUE = 192 };
    int Find(const char* name) const;
    int count;
    char (*names)[NAME];
    char (*values)[VALUE];
    short* slots;
};

class ReplayOrdinals {
public:
    void Reset();
    unsigned Of(unsigned kind, const void* object);

private:
    enum { KINDS = 4, CAPACITY = 64 };
    unsigned count[KINDS];
    const void* objects[KINDS][CAPACITY];
};

#endif
