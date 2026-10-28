#ifndef UNIT_REF_H
#define UNIT_REF_H

#include "../../interfaces/ref.h"

struct Unit;
class UnitRefListener;

class UnitRef {
public:
    void* table;
    Ref<Unit> owner;
    Ref<UnitRef> next;
    UnitRefListener* listener;

    Unit* Get() { do {} while (0); return owner; }

    void ClearRef(void);
    void LinkToUnit(Unit* o);
    void UnlinkFromUnit();
    void Unlink();
};

#endif
