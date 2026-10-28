#ifndef ORDERS_PANEL_FLAGS_H
#define ORDERS_PANEL_FLAGS_H

#pragma pack(push, 1)
struct OrdersPanelFlags {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short rest : 13;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(OrdersPanelFlags) == 2, "OrdersPanelFlags");
#endif

#endif
