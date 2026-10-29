#ifndef ORDER_DISPATCH_H
#define ORDER_DISPATCH_H

class MissionType;
struct Unit;
struct Vec3;

MissionType __stdcall GetOrderType(unsigned char mode, Unit* unit, Unit* target, Vec3* pos);

#endif
