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
    void Reset();
    bool Changed(const char* name, const char* value);
    void Set(const char* name, const char* value);

private:
    enum { CAPACITY = 64, NAME = 48, VALUE = 192 };
    int Find(const char* name) const;
    int count;
    char names[CAPACITY][NAME];
    char values[CAPACITY][VALUE];
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
