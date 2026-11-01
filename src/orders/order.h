#ifndef ORDER_H
#define ORDER_H

#include "../util/vec3.h"
#include "mission_type.h"
#include "order_base.h"
#include "path_order_attach.h"
#include "../../interfaces/ref.h"

struct Unit;
enum SimSpeechText : unsigned short;
class OrderFx;
class HapiBank;
struct Feature;
struct Point;
struct OrderSaveRecord;
struct OrderTableEntryView;

#pragma pack(push, 1)
struct Order : public OrderBase {
    MissionType kind;
    unsigned char state;
    union {
        unsigned int flags;
        struct {
            unsigned int low : 15;
            unsigned int waiting : 1;
            unsigned int high : 16;
        };
        struct {
            unsigned int flag_0 : 1;
            unsigned int flag_1 : 1;
            unsigned int flag_2 : 1;
            unsigned int started : 1;
            unsigned int flag_rest : 28;
        } groups;
    };
    unsigned int wakeFrame;
    union {
        Ref<Unit> unit;
        Ref<Unit> source;
    };
    union {
        struct {
            char unknown_12[4];
            Ref<Unit> target;
            char unknown_1a[8];
        };
        PathOrderAttach link;
    };
    Vec3 pos;
    union {
        Point16 start;
        struct {
            short x;
            short z;
        };
    };
    union {
        Point16 cached;
    };
    union {
        int type;
        int id;
        int wait;
        int weapon;
        int elapsed;
        int unitType;
        int ticks;
        int mode;
        int done;
        int attempts;
        int time;
        int guardRange;
        int approachRadius;
        int angle;
        int side;
        int piece;
    };
    union {
        int radius;
        int step;
        int waitLimit;
        int count;
        int unused;
        int duration;
        int countdown;
        int parity;
        int misses;
    };
    union {
        int progress;
        int range;
        int retries;
        int capabilities;
    };
    unsigned int flags_42;
    unsigned int created;
    Ref<Order> next;
    unsigned int subFlags;
    Ref<OrderFx> attached;

    Order(unsigned char kind, Unit* unit, Vec3* pos, int a, int b, int c);
    Order(MissionType kind, Unit* unit, void* pos, int a, int b, int c);
    Order(unsigned char kind, int a, int b, int c, int d, int e);
    Order(Unit* unit, HapiBank* file, char* name);
    ~Order();
    void SetDeadlineTicks(int param);
    void ReattachFxToUnit();
    void MergeFlagsFromTable(int k);
    virtual void OrStatusFlags(unsigned int flags);
    void SetAttachedFx(OrderFx* obj);
    void AttachApproachRadiusGoal(Vec3* pos, int n);
    void AttachRingApproachGoal(Vec3* pos, int radius1, int radius2);
    void AttachBuildFootprintMarker(Point16 cell, Point16 size);
    void AnnounceStatusIfFlagged(SimSpeechText text);
    Unit* Target() { return target; }
    void Wait() { waiting = 1; }
    Vec3* Position() { return &pos; }
    int Advance(int distance);
    int SerializeToSave(Unit* unit, HapiBank* file, char* name);
};
#pragma pack(pop)

#endif
