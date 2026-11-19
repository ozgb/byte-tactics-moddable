#ifndef ORDER_FX_H
#define ORDER_FX_H

#include "../../interfaces/ref.h"

struct Vec3;
struct Unit;
struct Order;
class HapiBank;
class BitWriter;

extern void* g_orderFxVtable[];

class __declspec(novtable) OrderFx {
public:
    Ref<Order> source;

    OrderFx() {}

    // FUNCTION: 0x44ce20
    OrderFx(Order* param_1)
    {
        SetVtable(g_orderFxVtable);
        source = param_1;
    }

    OrderFx(int arg1, int arg2, int arg3);

    virtual ~OrderFx();
    virtual int SerializeToSave(Order* order, HapiBank* file, char* name);
    virtual int GetType();
    virtual int IsFxStyle();
    virtual int ContainsUnit(Unit* unit);
    virtual int ContainsCell(int x, int y);
    virtual void FillGoalCells(void* cells);
    virtual int ApproxDist(int x, int y);
    virtual int FillWorldPos(Vec3* out);
    virtual int GetDesiredHeading(unsigned short* out);
    virtual void SerializeToBits(BitWriter* stream);
    virtual int KeepAfterComplete();

    void AddFlags(int flags);
    void* Destroy(unsigned char flag);
    void WriteBits(int);

protected:
    void SetVtable(void** table) { *(void***)this = table; }
};

#endif
