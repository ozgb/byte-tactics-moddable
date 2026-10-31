#ifndef PATH_ORDER_ATTACH_H
#define PATH_ORDER_ATTACH_H

#include "../../interfaces/ref.h"

struct Unit;
struct Order;

class PathOrderAttach {
public:
    Ref<Unit> owner;
    Ref<PathOrderAttach> next;
    Ref<Order> value;

    PathOrderAttach(Unit* o, Order* v);
    virtual ~PathOrderAttach();
    void SetUnit(Unit* o);
    void SetValue(Order* v) { value = v; }
    Unit* Get() { return owner; }
};

#endif
