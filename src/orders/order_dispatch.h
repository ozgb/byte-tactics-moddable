#ifndef ORDER_DISPATCH_H
#define ORDER_DISPATCH_H

class MissionType;
struct FixedPosView;
struct UnitOrderTypeView;

MissionType __stdcall GetOrderType(unsigned char mode, UnitOrderTypeView* unit, UnitOrderTypeView* target, FixedPosView* pos);

#endif
