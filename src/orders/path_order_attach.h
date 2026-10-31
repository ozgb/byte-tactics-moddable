#ifndef PATH_ORDER_ATTACH_H
#define PATH_ORDER_ATTACH_H

#include "../../interfaces/ref.h"

struct Unit;
class OrderBase;

class PathOrderAttach {
public:
    Ref<Unit> owner;
    Ref<PathOrderAttach> next;
    Ref<OrderBase> value;

    PathOrderAttach(Unit* o, OrderBase* v);
    virtual ~PathOrderAttach();
    void SetUnit(Unit* o);
    void SetValue(OrderBase* v) { value = v; }
    Unit* Get() { return owner; }
};

#endif
