#ifndef ORDER_TYPE_ENTRY_H
#define ORDER_TYPE_ENTRY_H

struct Order;

#pragma pack(push, 1)
struct OrderTypeEntry {
    unsigned char index;
    char unknown_1[0x8 - 0x1];
    unsigned int flags_a;
    char unknown_c[0x11 - 0xc];
    unsigned int flags;
};

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

#pragma pack(pop)

#endif
