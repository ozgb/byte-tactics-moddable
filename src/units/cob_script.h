#ifndef COB_SCRIPT_H
#define COB_SCRIPT_H
#include "../../interfaces/ref.h"

struct ScriptTable;
struct Piece;
struct Callback;
class HapiBank;

struct Channel
{
    unsigned int state;
    int pc;
    int sp;
    int delay;
    int piece;
    int axis;
    int waiting;
    int signal;
    Ref<Callback> callback;
    int stack[32];
    int Pop()
    {
        return stack[sp--];
    }
    void Push(int value)
    {
        stack[++sp] = value;
    }
    void XorOp()
    {
        int a = Pop();
        Push(a ^ Pop());
    }
};

class CobScript
{
  public:
    int scale;
    ScriptTable *table;
    int unknown_c;
    Ref<int> statics;
    Ref<Piece> pieces;
    int changed;
    Channel channels[8];
    int activeCount;

    CobScript();

    virtual void SetPieceTranslation(int, int, int) = 0;
    virtual void SetPieceRotation(int, int, int) = 0;
    virtual void SetPieceVisible(int, int) = 0;
    virtual void SetPieceCached(int, int) = 0;
    virtual void SetPieceShaded(int, int) = 0;
    virtual int GetPieceTranslation(int, int) = 0;
    virtual int GetPieceRotation(int, int) = 0;
    virtual int IsPieceVisible(int);
    virtual int IsPieceCached(int);
    virtual int IsPieceShaded(int);
    virtual void ExplodeLegacy(int, int, int);
    virtual void PlaySoundNoop(int);
    virtual void EmitSfx(int, int);
    virtual void ExplodePiece(int, unsigned int);
    virtual void AttachUnit(unsigned short, int, int);
    virtual void DropUnit(unsigned short);
    virtual void SetUnitValue(int, int);
    virtual int GetUnitValue(int, int, int, int, int);
    virtual int IsCarryingUnit(int);
    virtual int GetTransporterId();
    virtual ~CobScript();

    void SetCob(ScriptTable* data);
    ScriptTable* GetCob();
    int FUN_004b07b0(int, int);
    int FindScript(const char* name);
    int StartThreadByName(const char* name);
    int StartThread(int id);
    int StartScript(const char* name, Callback* callback, int update);
    int StartScriptByIndex(int id, Callback* callback, int update);
    int StartScriptWithArgs(char* name, Callback* callback, int update, int count, int a, int b, int c, int d);
    int StartScriptWithArgsByIndex(int index, Callback* callback, int update, int count, int a, int b, int c, int d);
    int QueryScript(char* name, int* p2, int* p3, int* p4, int* p5);
    int QueryScriptByIndex(int index, int* p2, int* p3, int* p4, int* p5);
    void RemoveCallback(Callback* callback);
    void RunScripts(int elapsed);
    void RunThread(unsigned int channel, int elapsed);
    void Wake(unsigned int index)
    {
        for (int i = 0; i < 8; i++)
            if ((channels[i].state & 0xfff00000) == 0x2800000 && channels[i].waiting == index)
                channels[i].state = 0x1000000;
    }
    void AnimatePieces(int elapsed);
    void SaveScriptState(HapiBank* file);
    int LoadScriptState(HapiBank* file);
};

#endif
