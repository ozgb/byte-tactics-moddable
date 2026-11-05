#ifndef ORDER_TYPE_ENTRY_H
#define ORDER_TYPE_ENTRY_H

struct Order;
struct Unit;
struct View;
struct Pos;

#pragma pack(push, 1)
struct OrderTableEntry {
    const char* value;
    int (__stdcall* notify)(Unit* unit, Order* order, int code);
    void (__stdcall* draw)(void* surface, View* view, Order* order, Pos* out, int flag);
    int drawMask;
    unsigned char markerAnim;
    unsigned int flags;
    char* name;

    int operator<(const OrderTableEntry&) const { return 0; }
    int operator==(const OrderTableEntry&) const { return 0; }
    int operator!=(const OrderTableEntry&) const { return 0; }
};

#pragma pack(pop)

#endif
