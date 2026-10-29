#ifndef PATH_ORDER_H
#define PATH_ORDER_H

#include "order_fx.h"
#include "path_order_attach.h"
#include "../util/vec3.h"
#include "../../interfaces/ref.h"

struct Order;
class BitReader;
class BitWriter;
class HapiBank;

#pragma pack(push, 2)
class __declspec(novtable) PathOrder : public OrderFx {
public:
    union {
        unsigned short flags;
        short field_8;
        struct {
            unsigned short : 6;
            unsigned short headingFlag : 1;
            unsigned short : 9;
        };
        struct {
            unsigned short : 4;
            unsigned short radiusFlag : 1;
            unsigned short : 11;
        };
    };
    short approachRadius;
    short altitude;
    short heading;
    short piece;
    Ref<Unit> unit;
    PathOrderAttach ref;
    union {
        Vec3 pos;
        struct {
            int pos_x;
            int pos_y;
            int pos_z;
        };
    };
    int followOffset;

    PathOrder(int owner, HapiBank* file, char* name);
    PathOrder(Unit* owner_, BitReader* reader);
    PathOrder(Order* order, Unit* unit);
    PathOrder(Order* source, Unit* unit, short value);
    PathOrder(Order* source, const Vec3& p);
    PathOrder(Order* source, Unit* unit, const Vec3& p);
    void SetAltitude(int param_1);
    void SetHeading(short v);
    void SetApproachRadius(short v);

    void SerializeToBits(BitWriter* stream);
    int GetType();
    int IsFxStyle();
    void* Destroy(unsigned char flag);
    int SerializeToSave(int unused, HapiBank* file, char* name);
    int KeepAfterComplete();
    int FillWorldPos(Vec3* out);
    int GetDesiredHeading(unsigned short* out);
    int IsComplete(Unit* arg);
};
#pragma pack(pop)

#endif
