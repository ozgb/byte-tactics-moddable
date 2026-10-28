#ifndef PATH_FX_H
#define PATH_FX_H

#include "order_fx.h"
#include "path_order_attach.h"
#include "../../interfaces/ref.h"

#pragma pack(push, 1)
class PathOrderPosView : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    int field_12;
    PathOrderAttach ref;
    Vec3 pos;
    char unknown_32[4];

    PathOrderPosView(Order* order, const Vec3& pos);
    void SetAltitude(int altitude);
    void SetApproachRadius(short radius);
};

class PathOrderUnitView : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Ref<Unit> field_12;
    PathOrderAttach ref;
    Vec3 pos;
    int field_32;

    PathOrderUnitView(Order* order, Unit* unit);
};

class PathOrderMoveToUnitView : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Ref<Unit> field_12;
    PathOrderAttach ref;
    Vec3 pos;
    char unknown_32[4];

    PathOrderMoveToUnitView(Order* order, Unit* unit, int value);
};

class PathOrderCoordsView : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    int field_12;
    PathOrderAttach ref;
    Vec3 pos;
    char unknown_32[4];

    PathOrderCoordsView(Order* order, Unit* unit, const Vec3& p);
};

class HapiBank;

class PathOrderLoadView : public OrderFx {
public:
    short field_8;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Ref<Unit> field_12;
    PathOrderAttach ref;
    Vec3 pos;
    int field_32;

    PathOrderLoadView(int owner, HapiBank* file, char* name);
};
#pragma pack(pop)

#endif
