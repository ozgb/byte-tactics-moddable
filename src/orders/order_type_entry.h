#ifndef ORDER_TYPE_ENTRY_H
#define ORDER_TYPE_ENTRY_H

struct Order;

#pragma pack(push, 1)
struct OrderTableEntry {
    int value;
    int (__stdcall* notify)(void* unit, Order* obj, unsigned int code);
    char unknown_8[0xc - 8];
    int drawMask;
    char unknown_10[0x11 - 0x10];
    unsigned int flags;
    char* name;

    int operator<(const OrderTableEntry&) const { return 0; }
    int operator==(const OrderTableEntry&) const { return 0; }
    int operator!=(const OrderTableEntry&) const { return 0; }
};

struct OrderType {
    unsigned char index;
    OrderTableEntry* GetTableEntry();
};
#pragma pack(pop)

#endif
