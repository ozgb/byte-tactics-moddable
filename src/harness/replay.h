#ifndef HARNESS_REPLAY_H
#define HARNESS_REPLAY_H

#include "../../interfaces/host.h"

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
typedef void (*ReplayEndFn)(ReplayImport& call);

struct ReplayGate {
    int calls;
    unsigned int owner;
    unsigned int nestedDepth;
    bool busy;
    ReplayAnswerFn answer;
    ReplayEndFn end;
};

extern ReplayGate g_replayGate;

inline int& ReplayCalls()
{
    return g_replayGate.calls;
}

inline unsigned int& ReplayOwner()
{
    return g_replayGate.owner;
}

inline unsigned int& ReplayNestedDepth()
{
    return g_replayGate.nestedDepth;
}

inline ReplayAnswerFn& ReplayAnswerHook()
{
    return g_replayGate.answer;
}

inline bool& ReplayAnswerBusy()
{
    return g_replayGate.busy;
}

inline ReplayEndFn& ReplayEndHook()
{
    return g_replayGate.end;
}

inline bool ReplayOwned()
{
    return ReplayCalls() != 0 && ReplayOwner() == host_thread_id();
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
        if (active && ReplayEndHook() != nullptr)
            ReplayEndHook()(*this);
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
