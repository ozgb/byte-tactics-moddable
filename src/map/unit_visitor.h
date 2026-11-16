#ifndef UNIT_VISITOR_H
#define UNIT_VISITOR_H

struct Unit;

class UnitVisitor {
public:
    virtual void Visit(Unit* unit) = 0;
};

#endif
