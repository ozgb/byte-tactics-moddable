#ifndef PATH_ORDER_ATTACH_H
#define PATH_ORDER_ATTACH_H

#include "../../interfaces/ref.h"

struct Unit;

class PathOrderAttach {
public:
    Ref<Unit> owner;
    Ref<PathOrderAttach> next;
    void* value;

    PathOrderAttach(Unit* o, int v);
    virtual ~PathOrderAttach();
    void SetUnit(Unit* o);
    void SetValue(void* v) { value = v; }
    Unit* Get() { return owner; }
};

#endif
