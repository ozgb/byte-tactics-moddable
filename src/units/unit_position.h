#ifndef UNIT_POSITION_H
#define UNIT_POSITION_H
#include "../../interfaces/services.h"
struct OrderView;
struct Player;

class BitReader;
class BitWriter;
struct Player;
struct Unit;
struct Vec3;

class MotionSerializer {
public:
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual int v7();
    virtual void WriteTo(BitWriter* stream);
    virtual void ReadFrom(BitReader* reader);
};

void __stdcall SnapUnitToGround(Unit* unit);
void __stdcall UpdateUnitHeight(Unit* unit);
unsigned short __stdcall GetHeadingBetweenOrDefault(Vec3* from, Vec3* to, unsigned short def);
int __stdcall SetUnitPosition(Unit* unit, Vec3 pos, int);
void __stdcall UpdateAllUnits(void);
void __stdcall WriteUnitState(BitWriter* stream, Unit* u);
void __stdcall ReadUnitState(BitReader* reader, Unit* u);
void __stdcall SendUnitStates(Player* p);

void __stdcall ApplyAttachUnit(OrderView* order);
void __stdcall ReceiveUnitStates(Player* p, unsigned int* data);

#endif
