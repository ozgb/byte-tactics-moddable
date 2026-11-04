#ifndef HARNESS_REPLAY_H
#define HARNESS_REPLAY_H

#include "../../interfaces/host.h"

inline int& ReplayCalls()
{
    static int calls;
    return calls;
}

inline unsigned int& ReplayOwner()
{
    static unsigned int owner;
    return owner;
}

inline unsigned int& ReplayNestedDepth()
{
    static unsigned int depth;
    return depth;
}

inline bool ReplayOwned()
{
    return ReplayCalls() != 0 && ReplayOwner() == host_thread_id();
}

#include "replay_services.h"

#pragma pack(push, 8)

struct ReplayArg {
    int kind;
    union {
        int i;
        const void* p;
        double d;
    } v;
};

struct ReplayImport;
void ReplayImportEnd(ReplayImport& call);
void ReplayFail(const char* message);

typedef bool (*ReplayAnswerFn)(ReplayImport& call);

inline ReplayAnswerFn& ReplayAnswerHook()
{
    static ReplayAnswerFn hook;
    return hook;
}

inline bool& ReplayAnswerBusy()
{
    static bool busy;
    return busy;
}

struct ReplayImport {
    explicit ReplayImport(int id) : id(id), active(ReplayOwned()), answering(ReplayAnswerHook() != nullptr && !ReplayAnswerBusy()), nested(false), count(0)
    {
        result.kind = -1;
        if (active)
            ReplayCalls() = 0;
    }
    ~ReplayImport()
    {
        if (nested)
            ReplayNestedDepth()--;
        if (active)
            ReplayImportEnd(*this);
    }
    void Nested()
    {
        if (!active)
            return;
        if (ReplayNestedDepth() != 0)
            ReplayFail("a command run inside a command of ExecuteCommandText is not recorded");
        nested = true;
        ReplayNestedDepth()++;
    }
    void Int(int value)
    {
        if (active || answering)
            AddArg(0)->v.i = value;
    }
    void Ptr(const void* value)
    {
        if (active || answering)
            AddArg(1)->v.p = value;
    }
    void Dbl(double value)
    {
        if (active || answering)
            AddArg(2)->v.d = value;
    }
    bool Answered()
    {
        if (!answering)
            return false;
        ReplayAnswerBusy() = true;
        bool answered = ReplayAnswerHook()(*this);
        ReplayAnswerBusy() = false;
        return answered;
    }
    void* AnsweredPointer() const { return const_cast<void*>(result.v.p); }
    void Return(int value)
    {
        result.kind = 0;
        result.v.i = value;
    }
    void Return(double value)
    {
        result.kind = 2;
        result.v.d = value;
    }
    void Return(const void* value)
    {
        result.kind = 1;
        result.v.p = value;
    }
    int Result(int value)
    {
        result.kind = 0;
        result.v.i = value;
        return value;
    }
    double Result(double value)
    {
        result.kind = 2;
        result.v.d = value;
        return value;
    }
    template <class T> T* Result(T* value)
    {
        result.kind = 1;
        result.v.p = value;
        return value;
    }
    ReplayArg* AddArg(int kind)
    {
        ReplayArg* arg = &args[count++];
        arg->kind = kind;
        return arg;
    }
    int id;
    bool active;
    bool answering;
    bool nested;
    int count;
    ReplayArg args[8];
    ReplayArg result;
};

#pragma pack(pop)

int ReplayConfigure(const char* value, char* error, int size);
void ReplayInstall();
int ReplayWrite(char* error, int size);

#endif
