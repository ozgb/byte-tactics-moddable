#include <stdio.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))

#pragma pack(push, 1)

union Fixed {
    int value;
    struct {
        unsigned short fraction;
        short whole;
    };
};

#include "../util/vec3.h"
#include "../util/angles.h"

struct Tail32 {
    int a;
    unsigned short b;
};

class CobScript;
class UnitMotion;
class PlayerData;
class BitWriter;
class BitReader;
struct Block;
struct Beacon;
struct OrderView;
struct Spawn;
struct UnitType;
struct Player;

#include "../map/mission.h"

#include "../network/player.h"

struct UnitType {
    char unknown_0[0x1fa];
    unsigned int f1fa;
    char unknown_1fe[0x200 - 0x1fe];
    unsigned short f200;
    char unknown_202[0x22c - 0x202];
    unsigned char draft;
    char unknown_22d[0x241 - 0x22d];
    union {
        struct {
            unsigned int low : 12;
            unsigned int floats : 1;
            unsigned int unknown_13 : 6;
            unsigned int on_water : 1;
            unsigned int over_water : 1;
            unsigned int unknown_21 : 11;
        } bits;
        unsigned int all;
    } f241;
};

#include "../orders/unit_motion.h"

class PlayerData {
public:
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual int v7();
    virtual void WriteTo(BitWriter* stream);
    virtual void ReadFrom(BitReader* reader);
};

class SpatialBucket {
public:
    char unknown_0[6];
    void* head;
    void UnlinkUnit(void* node);
    void PrependUnit(int node);
};

#include "unit.h"

#include "f14373.h"

#include "../game/game.h"

struct OrderView {
    unsigned char type;
    unsigned short id1;
    unsigned short id2;
    char param;
    unsigned char param2;
};

struct Spawn {
    unsigned char player;
    unsigned short type;
    unsigned short id;
    Vec3 pos;
    union {
        Angles16 words;
        Tail32 dword;
    } tail;
};

struct Block {
    char unknown_0[0x10];
    int field_10;
};

#include "cob_script.h"

#include "../sound/sound.h"
#include "../util/tdf.h"


#include "../network/bit_writer.h"

#include "../network/bit_reader.h"

#pragma pack(pop)

int __stdcall GetGroundHeight(Vec3* pos);
#include "unit_targets.h"
#include "../../interfaces/seams.h"
int __cdecl AngleOfVector(int x, int z);
#include "../network/net_game.h"
int __stdcall BroadcastPacket(int player, void* data, int size);
#include "unit_position.h"
#include "../ingame/selection.h"
#include "unit_motion.h"
#include "unit_position.h"
#include "units_485010.h"
#include "../ingame/control_panel.h"
#include "../gui/keyboard.h"
Unit* __stdcall CreateUnitFromPacket(unsigned char player, Spawn* spawn);

#include "../../link/globals.h"

// FUNCTION: 0x48a7f0
void __stdcall SnapUnitToGround(Unit* unit)
{
    extern Game* g_game;
    if (((UnitType*)unit->def.get())->f241.bits.floats) {
        unit->pos.y = max(GetGroundHeight(&unit->pos), g_game->seaLevel - ((UnitType*)unit->def.get())->draft) << 16;
    } else {
        unit->pos.y = GetGroundHeight(&unit->pos) << 16;
    }
}

// FUNCTION: 0x48a870
void __stdcall UpdateUnitHeight(Unit* unit)
{
    extern Game* g_game;
    if ((unit->flags & 0x10000) || ((UnitType*)unit->def.get())->f241.bits.floats) {
        unit->flags &= ~0x10000;
        if (unit->motion && (unit->flags & 3) == 1) {
            if (((UnitType*)unit->def.get())->f241.bits.over_water) {
                if (((UnitType*)unit->def.get())->f241.bits.floats) {
                    unit->pos.y = max(GetGroundHeight(&unit->pos), g_game->seaLevel - ((UnitType*)unit->def.get())->draft) << 16;
                } else {
                    unit->pos.y = GetGroundHeight(&unit->pos) << 16;
                }
            } else if (((UnitType*)unit->def.get())->f241.bits.on_water) {
                Fixed h;
                h.value = ((UnitType*)unit->def.get())->draft * 0xffff + g_game->seaLevel;
                h.value <<= 16;
                unit->pos.y = h.value;
            } else {
                AlignUnitToGround(unit);
            }
        }
    }
}

// FUNCTION: 0x48a980
int __stdcall GetHeadingBetween(Vec3* from, Vec3* to)
{
    return AngleOfVector(from->x - to->x, from->z - to->z);
}

// FUNCTION: 0x48a9b0
unsigned short __stdcall GetHeadingBetweenOrDefault(Vec3* from, Vec3* to, unsigned short def)
{
    int dx = from->x - to->x;
    int dz = from->z - to->z;
    if (dx == 0 && dz == 0)
        return def;
    return AngleOfVector(dx, dz);
}

static inline Point16 WorldToCell(Vec3 v, Point16 origin)
{
    Point16 c;
    c.x = (v.x - origin.x * 0x80000 + 0x80000) >> 20;
    c.y = (v.z - origin.y * 0x80000 + 0x80000) >> 20;
    return c;
}

// FUNCTION: 0x48a9f0
int __stdcall SetUnitPosition(Unit* unit, Vec3 pos, int param_5)
{
    Point16 cell = WorldToCell(pos, unit->footprint);
    if (cell.x == unit->cell.x && cell.y == unit->cell.y && param_5 == (unit->flags & 3)) {
        unit->pos = pos;
    } else {
        g_seams.map->RemoveUnitFromMap(unit);
        unit->pos = pos;
        unit->cell = cell;
        unit->flags = (unit->flags & 0xfffffffc) | (short)(param_5 & 3);
        g_seams.map->AddUnitToMap(unit);
        g_seams.los_radar->UpdateUnitLineOfSight(unit);
    }
    unit->flags |= 0x10000;
    return unit->flags;
}

// FUNCTION: 0x48aac0
void __stdcall AttachUnitToPiece(Unit* unit, Unit* target, char p3, char p4)
{
    OrderView packet;
    if ((unit->flags & 0x10000000) && !(unit->flags & 0x20000000) && unit->cargo == 0
        && (target == 0
            || ((target->flags & 0x10000000) && target != unit && target->carrier == 0))) {
        packet.type = 0xa;
        if (unit == 0)
            packet.id1 = 0;
        else
            packet.id1 = unit->id;
        if (target == 0)
            packet.id2 = 0;
        else
            packet.id2 = target->id;
        packet.param = p3;
        packet.param2 = p4;
        BroadcastPacket(GetLocalDpid(), &packet, 7);
        ApplyAttachUnit(&packet);
    }
}

// FUNCTION: 0x48ab70
void __stdcall ApplyAttachUnit(OrderView* order)
{
    extern Game* g_game;
    Unit* u = !order->id1 ? 0 : &g_game->units[order->id1];
    Unit* t = !order->id2 ? 0 : &g_game->units[order->id2];
    if (u) {
        unsigned int f = u->flags;
        if (f & 0x10000000) {
            if (!(f & 0x20000000)) {
                if (u->cargo == 0) {
                    if (t == 0
                        || ((t->flags & 0x10000000) && t != u && t->carrier == 0)) {
                        if (u->carrier != 0) {
                            Unit* n = u->carrier;
                            Ref<Unit>* link = &n->cargo;
                            n = n->cargo;
                            while (n != u) {
                                link = &n->cargoNext;
                                n = n->cargoNext;
                            }
                            *link = u->cargoNext;
                        } else {
                            g_seams.map->SpatialBucketUnlinkUnit((SpatialBucket*)u->spatialBucket.get(), 0, u);
                        }
                        u->transportPiece = order->param;
                        if (t) {
                            u->carrier = t;
                            u->cargoNext = t->cargo;
                            t->cargo = u;
                            bool v = ((order->param == -1) & 1);
                            u->carriedNoPiece = v;
                        } else {
                            u->cargoNext = 0;
                            u->carriedNoPiece = 0;
                            u->carrier = 0;
                            g_seams.map->SpatialBucketPrependUnit((SpatialBucket*)u->spatialBucket.get(), 0, u);
                        }
                        u->motion->mode = order->param2;
                        if (u->player->active) {
                            unsigned char k = u->player->type;
                            if (k == 1 || k == 2) {
                                if (t && !(((UnitType*)t->def.get())->f241.bits.low & 0x200))
                                    g_seams.orders->AddBeCarriedOrder(u);
                            }
                        }
                        DeselectIfIneligible(u);
                    }
                }
            }
        }
    }
}

static inline int PlayerMore(unsigned char i)
{
    if (i >= 10) return 0;
    return 1;
}

// FUNCTION: 0x48ad30
void __stdcall UpdateAllUnits(void)
{
    int* cnt;
    unsigned char i;
    int off;
    extern Game* g_game;
    cnt = &g_game->f14353;
    *cnt = 0;
    i = 0;
    off = 0;
    for (; i < 10; i++, off += 0x14b) {
        if (!PlayerMore(i)) continue;
        Player* p = (Player*)((char*)&g_game->players[0] + off);
        if (p->active == 0) continue;
        unsigned char k = p->type;
        if (k != 1 && k != 2 && k != 3) continue;
        if (p->index == 0xa) continue;
        {
            Unit* last = p->unitsEnd;
            Unit* u = p->unitsBegin;
            while (u <= last) {
                    if (u->unitDefIndex != 0) {
                        (*cnt)++;
                        UpdateWindGenerator(u);
                        if (p->active != 0) {
                            unsigned char k2 = p->type;
                            if (k2 == 1 || k2 == 2) {
                                g_seams.weapons->UpdateUnitWeapons(u);
                            }
                        }
                        if (u->script != 0) {
                            ((CobScript*)u->script.get())->RunScripts(1);
                        }
                        if (u->recentlyDamagedTimer != 0) {
                            u->recentlyDamagedTimer--;
                        }
                        if (u->postTransferHoldoff != 0) {
                            u->postTransferHoldoff--;
                        }
                        if (u->selected != 0) {
                            if (!(u->selectable) || u->buildLeft != 0.0f || u->postTransferHoldoff != 0
                                || (u->carrier != 0 && !(u->carrier->cargoSelectable))) {
                                u->selected = 0;
                            }
                        }
                        if ((unsigned int)g_game->ticks % 30 == 0) {
                            int v = u->health * 100 / ((UnitType*)u->def.get())->f1fa;
                            if (v < 0) {
                                v = 0;
                            }
                            if (v > 100) {
                                v = 100;
                            }
                            u->prevHealthPercent = u->healthPercent;
                            u->healthPercent = v;
                        }
                        Player* pl = u->player;
                        if (pl->active != 0) {
                            unsigned char k3 = pl->type;
                            if (k3 == 1 || k3 == 2) {
                                if (g_game->net->waterDoesDamage != 0
                                    && g_game->net->waterDamage != 0
                                    && (unsigned int)g_game->ticks % 30 == 0 && u->yWhole <= g_game->seaLevel
                                    && !((UnitType*)u->def.get())->f241.bits.floats) {
                                    g_seams.weapons->DamageUnit(0, u, g_game->net->waterDamage, 0xb, 0);
                                }
                                if (((UnitType*)u->def.get())->f200 != 0 && u->health < ((UnitType*)u->def.get())->f1fa
                                    && (g_game->ticks & 7) == 0) {
                                    int n = ((UnitType*)u->def.get())->f200 * 8;
                                    AddRepairProgress(u, u, (float)(n / 30));
                                }
                                g_seams.orders->RunOrders(u);
                                g_seams.orders->RunSecondaryOrders(u);
                                if (u->motion != 0) {
                                    g_seams.orders->UpdateMotion(u->motion, 0, u);
                                    UpdateUnitHeight(u);
                                }
                            }
                        }
                        if (u->killPending) {
                            KillUnit(u, u->lastDamageType);
                        }
                    }
                    u = (Unit*)((char*)u + 0x118);
                }
                if (g_game->netGameFlags & 1) {
                    if (p->active != 0) {
                        unsigned char k4 = p->type;
                        if (k4 == 1 || k4 == 2) {
                            SendUnitStates(p);
                        }
                    }
                }
            }
        }
    if (g_game->autoFollowFlags.bits.b1) {
        if (!IsKeyDown(0xf9)) {
            (*(short*)&g_game->autoFollowTicks)--;
            if ((short)g_game->autoFollowTicks <= 0) {
                (*(short*)&g_game->autoFollowTicks) = 0x5a;
                CycleSelection();
                CycleCameraFollow(0);
            }
        }
    }
}

// FUNCTION: 0x48b200
void __stdcall WriteUnitState(BitWriter* stream, Unit* u)
{
    extern Game* g_game;
    stream->WriteBits(u->unitDefIndex, g_game->unitDefCountBits);
    if (u->unitDefIndex == 0)
        return;
    stream->WriteBits(u->health, 0x10);
    stream->WriteBits((u->buildLeft != 0.0f) ? 1 - (int)(u->buildLeft * -254.0f) : 0, 8);
    stream->WriteBits(u->activateFlags, 8);
    stream->WriteBits(u->flags & 3, 2);
    if (u->carrier) {
        stream->data[stream->index] |= 1 << stream->bit;
        stream->bit++;
        if (stream->bit == 0x20) {
            stream->bit = 0;
            stream->index++;
            if (stream->index == stream->capacity) {
                stream->GrowBuffer();
            }
            stream->data[stream->index] = 0;
        }
        stream->WriteBits((!u->carrier ? 0 : u->carrier->id) & 0xffff, 0xf);
        stream->WriteBits((signed char)u->transportPiece, 8);
    } else {
        stream->bit++;
        if (stream->bit == 0x20) {
            stream->bit = 0;
            stream->index++;
            if (stream->index == stream->capacity) {
                stream->GrowBuffer();
            }
            stream->data[stream->index] = 0;
        }
        stream->WriteBits(u->pos.x, 0x20);
        stream->WriteBits(u->pos.y, 0x20);
        stream->WriteBits(u->pos.z, 0x20);
        stream->WriteBits(u->angles.heading, 0x10);
        stream->WriteBits((unsigned short)u->angles.pitch, 0x10);
        stream->WriteBits((unsigned short)u->angles.bank, 0x10);
        if (u->motion)
            stream->WriteBits(u->motion->speed, 0x20);
    }
}

// FUNCTION: 0x48b3f0
void __stdcall ReadUnitState(BitReader* reader, Unit* u)
{
    extern Game* g_game;
    unsigned short type = (unsigned short)reader->ReadBits(g_game->unitDefCountBits);
    int zero = 0;
    if (type == zero) {
        if (u->unitDefIndex != zero)
            u->killPending = 1;
        return;
    }
    if (u->unitDefIndex != type) {
        Spawn spawn;
        spawn.type = type;
        spawn.id = u->id;
        spawn.player = 9;
        spawn.pos = u->pos;
        spawn.tail.words = u->angles;
        CreateUnitFromPacket((u->angles.bank & ~0xff) | u->playerIndex, &spawn);
    }
    ((Block*)u->state.get())->field_10 = zero;
    u->health = reader->ReadBits(0x10);
    __int64 alpha = (unsigned int)reader->ReadBits(8);
    float scale = (float)alpha * g_buildPercentScale;
    if (scale != u->buildLeft) {
        u->buildLeft = scale;
        u->buildLeftChanged = 1;
    }
    unsigned char state = (unsigned char)reader->ReadBits(8);
    u->SetStateBits(state, 1);
    u->SetStateBits((unsigned char)~state, zero);
    int colour = reader->ReadBits(2);
    if (reader->ReadBit()) {
        OrderView order;
        order.id1 = u->id;
        order.id2 = (unsigned short)reader->ReadBits(0xf);
        order.param = (unsigned char)reader->ReadSignedBits(8);
        order.param2 = (unsigned char)colour;
        ApplyAttachUnit(&order);
        return;
    }
    if (u->carrier) {
        OrderView order;
        order.id1 = u->id;
        order.id2 = 0;
        order.param = 0xff;
        order.param2 = (unsigned char)colour;
        ApplyAttachUnit(&order);
    }
    Vec3 pos;
    pos.x = reader->ReadBits(0x20);
    pos.y = reader->ReadBits(0x20);
    pos.z = reader->ReadBits(0x20);
    Angles16 tail;
    tail.heading = (unsigned short)reader->ReadBits(0x10);
    tail.pitch = (unsigned short)reader->ReadBits(0x10);
    tail.bank = (unsigned short)reader->ReadBits(0x10);
    Point16 fixed = u->footprint;
    Point16 np;
    np.x = (short)((pos.x - fixed.x * 0x80000 + 0x80000) >> 20);
    np.y = (short)((pos.z - fixed.y * 0x80000 + 0x80000) >> 20);
    if (np.x == u->cell.x && np.y == u->cell.y && colour == (u->flags & 3)) {
        u->pos = pos;
    } else {
        g_seams.map->RemoveUnitFromMap(u);
        u->pos = pos;
        u->cell = np;
        u->mode = (unsigned int)(unsigned short)colour;
        g_seams.map->AddUnitToMap(u);
        g_seams.los_radar->UpdateUnitLineOfSight(u);
    }
    u->moved = 1;
    u->angles = tail;
    if (u->motion)
        u->motion->speed = reader->ReadBits(0x20);
}

// FUNCTION: 0x48b710
void __stdcall SendUnitStates(Player* p)
{
    extern Game* g_game;
    BitWriter stream;
    stream.WriteBits(0x2c, 8);
    stream.WriteBits(0, 0x10);
    stream.WriteBits(g_game->ticks, 0x20);
    p->syncTick = g_game->ticks;
    for (Unit* u = p->unitsBegin; u <= p->unitsEnd; u++) {
        if (!(u->flags & 0x10000000))
            continue;
        if (!u->motion)
            continue;
        if (!u->motion->player->v7())
            continue;
        stream.WriteBits(u->id - u->player->firstIndex, 0x10);
        stream.WriteBits(u->unitDefIndex, g_game->unitDefCountBits);
        u->motion->player->WriteTo(&stream);
        if (((stream.bit + 7) >> 3) + stream.index * 4 >= 0x200)
            break;
    }
    stream.WriteBits(-1, 0x10);
    int i = g_game->ticks % g_game->maxUnitsSetting;
    stream.data[stream.index] |= 1 << stream.bit;
    stream.bit++;
    if (stream.bit == 0x20) {
        stream.bit = 0;
        stream.index++;
        if (stream.index == stream.capacity)
            stream.GrowBuffer();
        stream.data[stream.index] = 0;
    }
    WriteUnitState(&stream, &p->unitsBegin[i]);
    stream.SetByteAt(
        1, (char)(((stream.bit + 7) >> 3) + (unsigned char)stream.index * 4));
    stream.SetByteAt(
        2, (char)((((stream.bit + 7) >> 3) + stream.index * 4) >> 8));
    BroadcastPacket(p->id, stream.data, ((stream.bit + 7) >> 3) + stream.index * 4);
    stream.FreeBuffer();
}

// FUNCTION: 0x48b920
void __stdcall ReceiveUnitStates(Player* p, unsigned int* data)
{
    extern Game* g_game;
    BitReader reader;

    reader.data = data;
    reader.index = 0;
    reader.bit = 0;
    reader.ReadBits(8);
    reader.ReadBits(0x10);
    int tick = reader.ReadBits(0x20);
    p->syncTick = tick;
    if (p->unitsBegin == 0)
        return;

    short index = (short)reader.ReadBits(0x10);
    while (index != -1) {
        Unit* unit = &p->unitsBegin[index];
        unsigned short type = (unsigned short)reader.ReadBits(g_game->unitDefCountBits);
        if (unit->unitDefIndex != type) {
            Spawn spawn;
            spawn.id = unit->id;
            spawn.type = type;
            spawn.player = 9;
            spawn.pos = unit->pos;
            spawn.tail.dword = *(Tail32*)&unit->angles;
            CreateUnitFromPacket(unit->playerIndex, &spawn);
        }
        unit->motion->player->ReadFrom(&reader);
        index = (short)reader.ReadBits(0x10);
    }

    for (Unit* u = p->unitsBegin; u <= p->unitsEnd;
         u = (Unit*)((char*)u + 0x118)) {
        if ((u->flags & 0x10000000) && u->motion) {
            g_seams.orders->UpdateMotion(u->motion, 0, u);
            UpdateUnitHeight(u);
        }
    }

    if (reader.ReadBit())
        ReadUnitState(&reader, &p->unitsBegin[tick % g_game->maxUnitsSetting]);
}
