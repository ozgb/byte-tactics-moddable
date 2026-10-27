#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Unit;
#include "../orders/unit_orders.h"
namespace std {
inline void _Construct(Unit** dest, Unit* const& src) { CopyDwordIfNonNull((int *)dest, (int *)&src); }
}
#include <vector>

#pragma pack(push, 1)

#include "../util/vec3.h"
#include "../util/point.h"
#include "../graphics/rect.h"
#include "../util/angles.h"
#include "../units/unit_def.h"

#include "../units/unit.h"
#include "../units/unit_type_set.h"

#include "../network/player.h"
#include "../network/player_info.h"

union Orders_37ebe {
    unsigned short flags;
    unsigned char byte;
    struct {
        unsigned short bits0 : 4;
        unsigned short bit4 : 1;
        unsigned short bits5 : 11;
    };
};

struct Slot_0048cd80 {
    unsigned short unitId;
    int x;
    int y;
};

struct Name_0048d630 {
    char unknown_0[0x22];
    char name[0x20];
    char unknown_42[0x232 - 0x42];
};

struct OrderTypeEntry {
    unsigned char index;
    char unknown_1[0x8 - 0x1];
    unsigned int flags_a;
    char unknown_c[0x11 - 0xc];
    unsigned int flags;
};

#include "../orders/order_type_entry.h"
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "keys.h"
#include "../game/game_state.h"
#include "../sound/sound.h"
#include "control_panel.h"
#include "../units/unit_commands.h"
#include "../orders/mission_type.h"
#include "../units/unit_scripts.h"

#include "../map/cell.h"

#include "../map/line_of_sight.h"
int __stdcall IsUnitVisibleToPlayer(Player* player, Unit* unit);

// FUNCTION: 0x48bae0
void CollectVisibleUnitIds(void)
{
    int count = 0;
    unsigned short* out = g_game->list_1435f;
    Rect* rect = &g_game->rect;
    Player* player = &g_game->players[g_game->playerIndex];
    for (Unit* u = g_game->units; u <= g_game->unitsEnd; u++) {
        if (u->unitDefIndex != 0) {
            UnitDef* def = u->def;
            int ux = u->xWhole;
            int uy = u->yWhole;
            int camy = g_game->scrollY;
            int x1 = ((short*)&def->modelMinX)[1] + ux - g_game->scrollX;
            int y2 = def->field_170 + uy;
            int z1 = ((short*)&def->modelMinZ)[1] + u->zWhole - camy;
            int x2 = ((short*)&def->modelMaxX)[1] + ux - g_game->scrollX;
            int y1 = ((short*)&def->modelMinY)[1] + uy;
            int z2 = ((short*)&def->modelMaxZ)[1] + u->zWhole - g_game->scrollY;
            if ((u->flags & 3) != 1) {
                Cell* c = GetMapCellAtPosition(&u->pos);
                if (c != 0) {
                    int h = c->height;
                    if (y1 > h)
                        y1 = h;
                }
            }
            int left = x1 + 0x80;
            int top = z1 - (y2 >> 1) + 0x20;
            int right = x2 + 0x80;
            int bottom = z2 - (y1 >> 1) + 0x20;
            if (left <= rect->right && right >= rect->left
                && top <= rect->bottom && bottom >= rect->top) {
                if (u->playerIndex == g_game->playerIndex
                    || IsUnitVisibleToPlayer(player, u)) {
                    *out++ = u->id;
                    count++;
                }
            }
        }
    }
    g_game->count_14367 = count;
}

// FUNCTION: 0x48bcb0
int __stdcall IsUnitVisible(char* unit)
{
    short* ids = (short*)g_game->list_1435f;
    int n = g_game->count_14367;
    for (int i = 0; i < n; i++) {
        if (ids[i] == *(short*)(unit + 0xa8))
            return 1;
    }
    return 0;
}

// FUNCTION: 0x48bd00
void ClearSelection(void)
{
    for (Unit* u = g_game->units; u <= g_game->unitsEnd; u++)
        u->flags &= 0xffffff2f;
    PopUntilNamedLayout(0);
}

// FUNCTION: 0x48bd50
void SelectAllIdleUnits(void)
{
    Player* pl = &g_game->players[g_game->localPlayer];
    Unit* u = pl->unitsBegin;
    if (u <= pl->unitsEnd) {
        do {
            if ((u->flags & 0x20) && u->buildLeft == 0.0f && u->postTransferHoldoff == 0
                && (u->carrier == 0 || (u->carrier->flags & 0x40000000))) {
                u->flags |= 0x10;
                g_game->unitIndex = 0;
            }
            u = (Unit*)((char*)u + 0x118);
        } while (u <= pl->unitsEnd);
    }
    SelectStopOrder();
    (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).byte |= 0x10;
}

static inline unsigned int* FlagsPtr(Unit* u)
{
    return (unsigned int*)&u->flags;
}

// FUNCTION: 0x48be00
void SelectUnitsOfSameTypes(void)
{
    Player* player = &g_game->players[g_game->localPlayer];
    unsigned int selected[16];
    memset(selected, 0, sizeof(selected));
    {
        for (Unit* u = player->unitsBegin; u <= player->unitsEnd; u++) {
            if (u->selected) {
                unsigned int v = u->unitDefIndex & 0xffff;
                selected[v >> 5] |= 1 << (v & 0x1f);
            }
        }
    }
    for (Unit* u = player->unitsBegin; u <= player->unitsEnd; u++) {
        unsigned int f = *FlagsPtr(u);
        if (f & 0x20) {
            if (u->buildLeft == 0.0f) {
                if (u->postTransferHoldoff == 0) {
                    if (u->carrier == 0 || (*FlagsPtr(u->carrier) & 0x40000000)) {
                        unsigned int v = u->unitDefIndex & 0xffff;
                        if (selected[v >> 5] & (1 << (v & 0x1f)))
                            *FlagsPtr(u) = f | 0x10;
                    }
                }
            }
        }
    }
    g_game->unitIndex = 0;
    SelectStopOrder();
    (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).bit4 = 1;
}

// FUNCTION: 0x48bf30
void __stdcall SelectUnitsByCategory(char* name, int param_2)
{
    int* mask = (int*)GetCategoryMask(name);
    int player = g_game->localPlayer;
    Player* p = &g_game->players[player];
    Player* q = &g_game->players[player];
    Unit* u = q->unitsBegin;

    for (; u <= p->unitsEnd; u++) {
        unsigned int flags = u->flags;
        if ((flags & 0x20) && u->buildLeft == 0.0f && u->postTransferHoldoff == 0
            && (u->carrier == 0 || (u->carrier->flags & 0x40000000))) {
            unsigned short bits = u->unitDefIndex;
            unsigned int bit = 1 << (bits & 0x1f);
            if (mask[bits >> 5] & bit) {
                u->flags = flags | 0x10;
            } else if (param_2 == 0) {
                u->flags = flags & ~0x10;
            }
        }
    }
    g_game->unitIndex = 0;
    SelectStopOrder();
    (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).bit4 = 1;
}

// FUNCTION: 0x48c030
void SelectAllVisibleUnits(void)
{
    int found = 0;
    for (Unit* u = g_game->units; u <= g_game->unitsEnd; u++)
        u->flags &= 0xffffff2f;
    PopUntilNamedLayout(0);
    unsigned short* list = g_game->list_1435f;
    for (int i = 0; i < g_game->count_14367; i++) {
        Unit* u = &g_game->units[list[i]];
        unsigned int flags = u->flags;
        if ((flags & 0x20) && u->buildLeft == 0.0f && u->postTransferHoldoff == 0
            && (u->carrier == 0 || (((unsigned char*)&u->carrier->flags)[3] & 0x40))
            && u->playerIndex == g_game->localPlayer) {
            found = 1;
            u->flags = flags | 0x10;
        }
    }
    if (found) {
        SelectStopOrder();
        g_game->unitIndex = 0;
        (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).byte |= 0x10;
    }
}

// FUNCTION: 0x48c150
void ClearUnitSelectFlags(void)
{
    for (Unit* u = g_game->units; u <= g_game->unitsEnd; u++)
        u->flags &= 0xffffff3f;
}

static inline Unit* GetUnit_0048c190(unsigned short i)
{
    return i == 0 ? 0 : g_game->units + i;
}

// FUNCTION: 0x48c190
Unit* __stdcall FindNextSelectedUnit(Unit* unit, int dir)
{
    Player* p = &g_game->players[g_game->localPlayer];
    unsigned short x = unit == 0 ? 0 : unit->id;
    if (x < p->firstIndex || x > p->lastIndex) {
        x = p->firstIndex;
    }
    if (dir) {
        short j = x;
        while ((unsigned short)j != p->firstIndex) {
            --j;
            Unit* v = GetUnit_0048c190((unsigned short)j);
            if (v->selected) {
                return v;
            }
        }
        j = p->lastIndex + 1;
        while ((unsigned short)j != x) {
            --j;
            Unit* v = GetUnit_0048c190((unsigned short)j);
            if (v->selected) {
                return v;
            }
        }
    } else {
        short j;
        for (j = x; (unsigned short)j != p->lastIndex; j++) {
            Unit* v = GetUnit_0048c190((unsigned short)(j + 1));
            if (v->selected) {
                return v;
            }
        }
        for (j = p->firstIndex - 1; (unsigned short)j != x; j++) {
            Unit* v = GetUnit_0048c190((unsigned short)(j + 1));
            if (v->selected) {
                return v;
            }
        }
    }
    return 0;
}

// FUNCTION: 0x48c320
void MarkLocalVisibleUnits(void)
{
    unsigned short* list = g_game->list_1435f;
    for (int i = 0; i < g_game->count_14367; i++) {
        Unit* u = &g_game->units[list[i]];
        if (u->playerIndex == g_game->localPlayer)
            u->selectState = 1;
    }
}

struct Select_0048c390 {
    char unknown_0[8];
    unsigned int flags;
};

int __stdcall PlaySoundByName(char* msg, int a);

// FUNCTION: 0x48c390
int __stdcall SelectUnitsInBox(void* param_1)
{
    int found = 0;
    int ymin = (g_game->boxStartX - g_game->scrollX) + 0x80;
    int xmin = ((g_game->boxStartZ - (g_game->boxStartHeight >> 1)) - g_game->scrollY) + 0x20;
    int ymax = (g_game->boxEndX - g_game->scrollX) + 0x80;
    int xmax = ((g_game->boxEndZ - (g_game->boxEndHeight >> 1)) - g_game->scrollY) + 0x20;
    int t;
    if (ymin > ymax) { t = ymin; ymin = ymax; ymax = t; }
    if (xmin > xmax) { t = xmin; xmin = xmax; xmax = t; }
    int toggle = (((Select_0048c390*)param_1)->flags >> 2) & 1;
    if (!toggle) {
        for (Unit* v = g_game->units; v <= g_game->unitsEnd; v++)
            v->flags &= 0xffffff2f;
        PopUntilNamedLayout(0);
    }
    Player* p = &g_game->players[g_game->localPlayer];
    int count = 0;
    Unit* last;
    for (Unit* u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if ((u->flags & 0x20) && u->buildLeft == 0.0f && !u->postTransferHoldoff &&
            (!u->carrier || (u->carrier->flags & 0x40000000))) {
            int sy = (u->xWhole - g_game->scrollX) + 0x80;
            int sx = (u->zWhole - g_game->scrollY - (u->yWhole >> 1)) + 0x20;
            if (sy >= ymin && sy <= ymax && sx >= xmin && sx <= xmax) {
                if (!toggle)
                    u->selected = 1;
                else
                    u->selected = !u->selected;
                found = 1;
            }
            if (u->selected) { last = u; count++; }
        }
    }
    g_game->unitIndex = 0;
    for (Unit* v = g_game->units; v <= g_game->unitsEnd; v++)
        v->flags &= 0xffffff3f;
    unsigned short* list = g_game->list_1435f;
    for (int i = 0; i < g_game->count_14367; i++) {
        Unit* v = &g_game->units[list[i]];
        if (v->playerIndex == g_game->localPlayer)
            v->selectState = 1;
    }
    if (found)
        (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).byte |= 0x10;
    if (!count)
        return 0;
    if (count == 1) {
        QueueUnitSpeech(last, 1, 0);
        return 1;
    }
    PlaySoundByName(s_SelectMultipleUnits_00508d8c, 0);
    return 1;
}

void __stdcall RotateByAngles(Vec3* in, Vec3* out, short* angles);
void __stdcall GetObjectBounds(void* obj, Vec3* lo, Vec3* hi, int arg);
#include "../graphics/draw.h"

// FUNCTION: 0x48c6a0
int __stdcall HitTestUnitScreenHull(Unit* obj, Point* p)
{
    void** models = (void **)g_game->types.get();
    Vec3 corners[4];
    Point pts[4];
    int i;
    Vec3 o;
    Vec3 lo;
    Vec3 hi;
    GetObjectBounds(models[(unsigned short)obj->unitDefIndex], &lo, &hi, 0);
    corners[0].x = lo.x;
    corners[0].y = lo.y;
    corners[0].z = lo.z;
    corners[1].x = hi.x;
    corners[1].y = lo.y;
    corners[1].z = lo.z;
    corners[2].x = hi.x;
    corners[2].y = lo.y;
    corners[2].z = hi.z;
    corners[3].x = lo.x;
    corners[3].y = lo.y;
    corners[3].z = hi.z;
    o.x = obj->pos.x - (g_game->scrollX << 16);
    o.y = obj->pos.y;
    o.z = obj->pos.z - (g_game->scrollY << 16);
    short* angles = (short*)&obj->angles;
    for (i = 0; i < 4; i++) {
        Vec3 v;
        RotateByAngles(&corners[i], &v, angles);
        pts[i].x = (short)((v.x + o.x) >> 16) + 0x80;
        pts[i].y = ((short)((o.z - v.z) >> 16) - ((short)((v.y + o.y) >> 16) >> 1)) + 0x20;
    }
    return PointInPolygon(pts, 4, p->x, p->y) != 0;
}

struct Param_0048c7f0 {
    char unknown_0[8];
    unsigned char flags;
};

// FUNCTION: 0x48c7f0
void __stdcall ClickSelectHoverUnit(Param_0048c7f0* param)
{
    Unit* unit = !g_game->hoverUnitId ? 0 : &g_game->units[g_game->hoverUnitId];
    if (unit == 0)
        return;
    if (unit->playerIndex != g_game->localPlayer)
        return;
    if (!(unit->flags & 0x20))
        return;
    if (unit->buildLeft != 0.0f)
        return;
    if (unit->postTransferHoldoff != 0)
        return;
    if (unit->carrier != 0 && !(unit->carrier->flags & 0x40000000))
        return;

    if (param->flags & 4) {
        unit->selected = !unit->selected;
        if (unit->selected)
            QueueUnitSpeech(unit, 1, 0);
        g_game->unitIndex = 0;
        (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).flags |= 0x10;
        return;
    }

    for (Unit* u = g_game->units; u <= g_game->unitsEnd; u++)
        u->flags &= 0xffffff2f;
    PopUntilNamedLayout(0);
    unsigned short* list = g_game->list_1435f;
    for (int i = 0; i < g_game->count_14367; i++) {
        Unit* u = &g_game->units[list[i]];
        if (u->playerIndex == g_game->localPlayer)
            u->flags = u->flags & 0xffffff7f | 0x40;
    }
    unit->flags |= 0x10;
    QueueUnitSpeech(unit, 1, 0);
    (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).flags |= 0x10;
}

// FUNCTION: 0x48c9b0
void __stdcall DeselectIfIneligible(Unit* unit)
{
    unsigned int flags = unit->flags;
    if (flags & 0x10) {
        if (!(flags & 0x20) || unit->buildLeft != 0.0f || unit->postTransferHoldoff != 0
            || (unit->carrier != 0 && !(unit->carrier->flags & 0x40000000))) {
            unit->flags = flags & ~0x10;
            g_game->unitIndex = 0;
            (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).byte |= 0x10;
        }
    }
}

// FUNCTION: 0x48ca20
void __stdcall CollectSelectedUnits(std::vector<Unit*>* list)
{
    list->clear();
    Player* team = &g_game->players[g_game->localPlayer];
    for (Unit* u = team->unitsBegin; u <= team->unitsEnd; u++) {
        if (u->selected)
            list->push_back(u);
    }
}

#include "../orders/order_list.h"

// FUNCTION: 0x48cc30
void __stdcall DrawSelectedUnitOrderOverlays(void* obj, Unit** sel)
{
    Player* team = &g_game->players[g_game->localPlayer];
    Unit* sel1unit = !g_game->unitIndex ? 0 : &g_game->units[g_game->unitIndex];
    Unit* sel2unit = !g_game->hoverUnitId ? 0 : &g_game->units[g_game->hoverUnitId];
    Unit* selunit = *sel;
    bool flag = (selunit && selunit->def->ids)
        || (sel1unit && sel1unit->def->ids)
        || (sel2unit && sel2unit->def->ids);
    for (Unit* u = team->unitsBegin; u <= team->unitsEnd; u++) {
        if ((u->flags & 0x10000000) && !(u->flags & 0x4000)) {
            if (u == *sel || u->id == g_game->unitIndex || u->id == g_game->hoverUnitId)
                DrawOrderOverlays(u, 0x1f, obj, sel, 1);
            else if (u->selected)
                DrawOrderOverlays(u, 0x1f, obj, sel, 0);
            else if (flag)
                DrawOrderOverlays(u, 1, obj, sel, 1);
        }
    }
}

int __stdcall PointInRect(Rect* rect, int x, int y);

static inline int FixMul(int a, int b)
{
    return (int)(((__int64)a * b) >> 16);
}

// FUNCTION: 0x48cd80
unsigned short __stdcall PickUnitUnderCursor(void)
{
    Point* p = &g_game->view_2c76;
    unsigned short result = 0;
    if (PointInRect(&g_game->rect, p->x, p->y)) {
        int best = 0x7fff0000;
        unsigned short* ids = g_game->list_1435f;
        if (ids == 0)
            return 0;
        for (int i = 0; i < g_game->count_14367; i++, ids++) {
            Unit* u = &g_game->units[*ids];
            if (u->unitDefIndex != 0) {
                if (HitTestUnitScreenHull(u, p)) {
                    UnitDef* def = u->def;
                    int v = FixMul(def->modelSizeY, 0x8000) + def->modelSizeZ;
                    v = FixMul(v, def->modelSizeX);
                    if (v < best) {
                        result = u->id;
                        best = v;
                    }
                }
            }
        }
    } else if (PointInRect(&g_game->viewLimit, p->x, p->y)) {
        int best = 99999;
        Slot_0048cd80* s = (Slot_0048cd80 *)g_game->hotRadar;
        for (int i = g_game->hotRadarCount; i > 0; i--) {
            int dy = s->y - p->y;
            int dx = s->x - p->x;
            int d = dx * dx + dy * dy;
            if (d < 4 && d < best) {
                best = d;
                result = s->unitId;
            }
            s++;
        }
    }
    return result;
}

static inline void Dummy(void) {}

int __stdcall GetOrderCursor(unsigned char type, Unit* unit, Unit* target, int* out);

class IntDynArray {
public:
    int EraseByValue(int value);
};

MissionType __stdcall GetOrderType(unsigned char mode, Unit* unit,
                                       Unit* target, void* pos);
void __stdcall IssueOrCancelOrder(OrderType kind, int flag, Unit* unit,
                            Unit* target, int* pos, int typeId, int amount);

static inline MissionType Order(const char* name) { return MissionType(name); }

// FUNCTION: 0x48cf30
void __stdcall IssueOrderToSelection(OrderTypeEntry* entry, unsigned char mode,
                            OrderType kind, int* pos, int typeId, int amount)
{
    int flag_a = (entry->flags_a >> 2) & 1;
    Unit* except = 0;
    int flag_b;
    if (mode)
        flag_b = OrderModeTakesTarget(mode);
    else
        flag_b = (kind.GetTableEntry()->flags >> 9) & 1;
    if (flag_b) {
        if (!g_game->hoverUnitId)
            except = 0;
        else
            except = (Unit*)((char*)g_game->units.get() + 280 * g_game->hoverUnitId);
    }
    Player* p = &g_game->players[g_game->localPlayer];
    int count = 0;
    int sum_x = 0;
    int sum_z = 0;
    Unit* u;
    for (u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if ((((unsigned char)u->flags) & 0x10) && u != except) {
            count++;
            sum_x += (short)(u->pos.x >> 16);
            sum_z += (short)(u->pos.z >> 16);
        }
    }
    if (!count)
        return;
    int avg[3];
    avg[0] = (sum_x / count) * 65536.0;
    avg[2] = (sum_z / count) * 65536.0;
    int range = count * 3000;
    for (u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if (!(((unsigned char)u->flags) & 0x10) || u == except)
            continue;
        if (mode)
            kind.index = GetOrderType(mode, u, except, &g_game->pos_2caa).index;
        if (!kind.index)
            continue;
        MissionType fire = Order("Standing_FireOrder");
        if (kind.index != fire.index || (u->def->flags2Byte & 2)) {
            MissionType move = Order("Standing_MoveOrder");
            if (kind.index != move.index || (u->def->flags2Byte & 1)) {
                if (pos && (kind.GetTableEntry()->flags & 2)) {
                    int dz = u->pos.z;
                    dz -= avg[2];
                    int dx = u->pos.x;
                    dx -= avg[0];
                    if ((int)(((__int64)dx * dx) >> 32)
                        + (int)(((__int64)dz * dz) >> 32) <= range) {
                        int here[3];
                        here[0] = pos[0] + u->pos.x - avg[0];
                        here[1] = pos[1];
                        here[2] = pos[2] + u->pos.z - avg[2];
                        IssueOrCancelOrder(kind, flag_a, u, except, here, typeId, amount);
                        continue;
                    }
                }
                IssueOrCancelOrder(kind, flag_a, u, except, pos, typeId, amount);
            }
        }
    }
}

// FUNCTION: 0x48d220
int __stdcall ResolveCursorModeForSelection(char arg)
{
    Unit* target;
    if (g_game->hoverUnitId != 0)
        target = &g_game->units[g_game->hoverUnitId];
    else
        target = 0;

    std::vector<Unit*> vec;
    vec.clear();

    Player* player = &g_game->players[g_game->localPlayer];
    for (Unit* u = player->unitsBegin; u <= player->unitsEnd; u++) {
        if (u->selected)
            vec.push_back(u);
    }

    if (target != 0)
        ((IntDynArray*)&vec)->EraseByValue((int)target);

    if (vec.empty()) {
        if (arg == 1 && target != 0
            && target->playerIndex == g_game->localPlayer
            && target->selectable
            && target->buildLeft == 0.0f
            && target->postTransferHoldoff == 0
            && (target->carrier == 0 || (target->carrier->flags & 0x40000000)))
            return 0x0f;
        return 0x13;
    }

    int result = 0x13;
    for (std::vector<Unit*>::iterator it = vec.begin(); it != vec.end(); ++it) {
        int r = GetOrderCursor(g_game->orderMode, *it, target, &(*(int*)&g_game->pos_2caa));
        if (r < result)
            result = r;
    }

    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    Dummy();
    return result;
}

// FUNCTION: 0x48d420
Unit* FindNextUnmarkedLocalUnit(void)
{
    Player* player = &g_game->players[g_game->localPlayer];
    Unit* u;
    for (u = player->unitsBegin; u <= player->unitsEnd; u++) {
        if (u->unitDefIndex != 0) {
            if (u->selectState == 0)
                return u;
        }
    }
    for (Unit* p = g_game->units; p <= g_game->unitsEnd; p++) {
        p->selectState = 0;
    }
    for (u = player->unitsBegin; u <= player->unitsEnd; u++) {
        if (u->unitDefIndex != 0) {
            if (u->selectState == 0)
                return u;
        }
    }
    return 0;
}

static inline Unit* PickUnit(Player* player)
{
    Unit* u = player->unitsBegin;
    while (u <= player->unitsEnd) {
        if (u->unitDefIndex != 0) {
            if (u->selectState == 0)
                return u;
        }
        u++;
    }
    Unit* q = g_game->units;
    while (q <= g_game->unitsEnd) {
        q->selectState = 0;
        q++;
    }
    u = player->unitsBegin;
    while (u <= player->unitsEnd) {
        if (u->unitDefIndex != 0) {
            if (u->selectState == 0)
                return u;
        }
        u++;
    }
    return 0;
}

// FUNCTION: 0x48d4d0
void FocusNextLocalUnit(void)
{
    Unit* u = PickUnit(&g_game->players[g_game->localPlayer]);
    if (u == 0)
        return;
    g_game->focusUnitId = u->id;
    CenterCameraOnMapPosition(&u->pos, 1);
    CollectVisibleUnitIds();
    unsigned short* list = g_game->list_1435f;
    for (int i = 0; i < g_game->count_14367; i++) {
        Unit* unit = &g_game->units[list[i]];
        if (unit->playerIndex == g_game->localPlayer)
            unit->selectState = 1;
    }
    u->selectState = 1;
}

// FUNCTION: 0x48d630
void __stdcall FocusCommander(int param_1)
{
    Player* team = &g_game->players[g_game->playerIndex];
    char* playerName = (*(Name_0048d630 (*)[5])&g_game->sideDefs)[team->info->side].name;
    Unit* last = team->unitsEnd;
    for (Unit* u = team->unitsBegin; u <= last; u++) {
        if (u->selectable && u->buildLeft == 0.0f && u->postTransferHoldoff == 0
            && (u->carrier == 0 || u->carrier->cargoSelectable)) {
            if (strcmp(u->def->unitname, playerName) == 0) {
                ClearCameraFollowState();
                CenterCameraOnMapPosition(&u->pos, 1);
                if (param_1 == 0)
                    return;
                SelectStopOrder();
                for (Unit* v = g_game->units; v <= g_game->unitsEnd; v++) {
                    v->selected = 0;
                    v->selectState = 0;
                }
                PopUntilNamedLayout(0);
                u->selected = 1;
                (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).bit4 = 1;
                return;
            }
        }
    }
}

// FUNCTION: 0x48d790
void __stdcall CycleSelection(void)
{
    Unit* found = 0;
    Player* t = &g_game->players[g_game->playerIndex];
    Unit* u = t->unitsBegin;
    Unit* last = t->unitsEnd;

    for (; u <= last; u++) {
        if (u->flags & 0x20) {
            if (u->buildLeft == 0.0f && u->postTransferHoldoff == 0) {
                Unit* owner = u->carrier;
                if (owner == 0 || (owner->flags & 0x40000000)) {
                    if (found == 0) {
                        found = u;
                    }
                    if (u->selected) {
                        for (Unit* q = g_game->units;
                             q <= g_game->unitsEnd; q++) {
                            q->flags &= 0xffffff2f;
                        }
                        PopUntilNamedLayout(0);
                        break;
                    }
                }
            }
        }
    }

    unsigned short flag = 0x10;
    if (found != 0) {
        u++;
        if (u <= t->unitsEnd) {
            do {
                if ((u->flags & 0x20) && u->buildLeft == 0.0f && u->postTransferHoldoff == 0) {
                    Unit* owner = u->carrier;
                    if (owner == 0 || (owner->flags & 0x40000000)) {
                        u->flags |= flag;
                        g_game->unitIndex = 0;
                        goto done;
                    }
                }
                u++;
            } while (u <= t->unitsEnd);
        }
        found->flags |= flag;
    }
done:
    (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).flags |= flag;
}

#include "../units/squads.h"

// FUNCTION: 0x48d920
void __stdcall CreateSquad(int param_1)
{
    Player* player = &g_game->players[g_game->localPlayer];
    for (Unit* u = player->unitsBegin; u <= player->unitsEnd; u++) {
        if (u->unitDefIndex != 0) {
            if (u->selected) {
                SetUnitSquad(u, param_1);
            } else if (u->group == param_1) {
                SetUnitSquad(u, 0);
            }
        }
    }
}

static inline int TestBit(UnitTypeSet* set, unsigned short n)
{
    return set->bits[n >> 5] & (1 << (n & 0x1f));
}

// FUNCTION: 0x48d9a0
bool __stdcall SelectSquad(int id, int param_2)
{
    UnitTypeSet* setA = GetCategoryMask("CTRL_F");
    int cnt = 0;
    Player* player = &g_game->players[g_game->localPlayer];
    UnitTypeSet* setB = GetCategoryMask("CTRL_F");

    Unit* u;
    Unit* v;
    Unit* w;
    int found;
    Player* q = &g_game->players[g_game->localPlayer];
    for (u = q->unitsBegin; u <= q->unitsEnd; u++) {
        if ((*(unsigned char*)&u->flags & 0x20) && u->buildLeft == 0.0f && u->postTransferHoldoff == 0
            && (u->carrier == 0 || (u->carrier->flags & 0x40000000))
            && u->group == id && TestBit(setB, u->unitDefIndex)) {
            {
                Player* r = &g_game->players[g_game->localPlayer];
                for (v = r->unitsBegin;
                     v <= r->unitsEnd; v++) {
                    if ((v->flags & 0x20) && v->buildLeft == 0.0f && v->postTransferHoldoff == 0
                        && (v->carrier == 0 || (v->carrier->flags & 0x40000000))
                        && v->group == id && (v->flags & 0x80000000)) {
                        goto found_match;
                    }
                }
            }
            break;
        }
    }

    found = 0;
    goto select_units;
found_match:
    found = 1;
select_units:
    for (w = player->unitsBegin;
         w <= player->unitsEnd; w++) {
        if ((w->flags & 0x20) && w->buildLeft == 0.0f && w->postTransferHoldoff == 0
            && (w->carrier == 0 || (w->carrier->flags & 0x40000000))) {
            if (w->group == id) {
                if (found) {
                    if (TestBit(setA, w->unitDefIndex)) {
                        w->flags &= ~0x10;
                    } else {
                        w->flags |= 0x10;
                        cnt++;
                    }
                } else {
                    w->flags |= 0x10;
                    cnt++;
                }
            } else if (param_2 == 0) {
                w->flags &= ~0x10;
            }
        }
    }
    SelectStopOrder();
    g_game->unitIndex = 0;
    (*(Orders_37ebe*)&g_game->ordersPanelFlags_37ebe).bit4 = 1;
    return cnt > 0;
}

// FUNCTION: 0x48dc30
int __stdcall SquadHasCtrlFMember(int id)
{
    UnitTypeSet* set = GetCategoryMask("CTRL_F");
    Player* p = &g_game->players[g_game->localPlayer];
    Unit* u = p->unitsBegin;
    Unit* end = p->unitsEnd;
    if (u > end)
        return 0;
    while (u <= end) {
        if ((*(unsigned char*)&u->flags & 0x20) && u->buildLeft == 0.0f && u->postTransferHoldoff == 0
            && (u->carrier == 0 || (u->carrier->flags & 0x40000000))
            && u->group == id && TestBit(set, u->unitDefIndex))
            return 1;
        u++;
    }
    return 0;
}

// FUNCTION: 0x48dd10
int __stdcall SquadHasReadyMember(int param_1)
{
    Player* player = &g_game->players[g_game->localPlayer];
    for (Unit* u = player->unitsBegin; u <= player->unitsEnd; u++) {
        unsigned int flags = u->flags;
        if (flags & 0x20) {
            if (u->buildLeft == 0.0f) {
                if (u->postTransferHoldoff == 0) {
                    Unit* owner = u->carrier;
                    if (owner == 0 || (owner->flags & 0x40000000)) {
                        if (u->group == param_1) {
                            if (flags & 0x80000000)
                                return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

typedef std::vector<Unit*> Vec_0048ddc0;
typedef Vec_0048ddc0::iterator (Vec_0048ddc0::*InsertFn_0048ddc0)(
    Vec_0048ddc0::iterator, Unit* const&);

// FUNCTION: 0x48ddc0 ?insert@?$vector@PAUUnit@@V?$allocator@PAUUnit@@@std@@@std@@QAEPAPAUUnit@@PAPAU3@ABQAU3@@Z
InsertFn_0048ddc0 g_insert_0048ddc0 = &Vec_0048ddc0::insert;

#include "../game/mission_conditions.h"

// FUNCTION: 0x48df90
MissionConditions::MissionConditions()
{
    active = 1;
    victoryCount = 0;
    defeatCount = 0;
}
