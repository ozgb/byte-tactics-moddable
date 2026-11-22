#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/orders.h"
#include "../interfaces/units.h"
#include "../interfaces/weapons.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/units/unit_type_set.h"
#include "../src/util/vec3.h"
#pragma pack(push, 1)
#include "../src/weapons/weapon_def.h"
#include "../src/weapons/unit_weapon_slot.h"
#include "../src/orders/order.h"
#include "../src/map/cell.h"
#include "../src/map/spatial_bucket.h"
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#include "../src/orders/order_type_entry.h"
#pragma pack(pop)
#include "../src/game/scheduler.h"
#include "../src/game/weapons_events.h"
#include "../src/orders/order_list.h"
#include "../src/orders/orders_seam.h"
#include "../src/seams/orders_world.h"
#include "../interfaces/state_writer.h"

void __stdcall RegisterOrderTypes(OrderTableEntry* from, int count);

Game* g_game;
Seams g_seams;
SimGlobals* g_simGlobals = 0;
void (*g_outOfMemoryHandler)() = 0;
char g_paralyzeMissionName[12] = "paralyze";

static int g_failures;

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

int __stdcall HarnessTestMode()
{
    return 0;
}

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    printf("FAIL: host_fatal: %s: %s\n", title, text);
    g_failures++;
}

void __stdcall FatalError(const char* text)
{
    printf("FAIL: FatalError: %s\n", text);
    g_failures++;
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}

int __stdcall RandomInt(int n)
{
    return n > 0 ? n - 1 : 0;
}

void __stdcall RecordSimEvent(SimEventKind, unsigned int, const unsigned int*, unsigned int) {}
void __stdcall HarnessOrdersEvent(const OrdersEventHeader*, unsigned int, int) {}

MissionType::MissionType(const char* name)
{
    OrderTableEntry* first = g_missionOrderTableVec.Begin();
    OrderTableEntry* last = g_missionOrderTableVec.End();
    index = 0;
    for (OrderTableEntry* e = first; e != last; e++)
        if (_strcmpi(e->name, name) == 0)
            index = (unsigned char)(e - first);
}

Order::Order(MissionType k, Unit* o, void* p, int a, int b, int c) : Order(k.index, o, (Vec3*)p, a, b, c) {}

enum {
    kTicks = 40,
    kTamperTick = 12,
    kUnits = 1,
    kUnit = 1
};

static Unit* s_units;
static UnitDef* s_defs;
static UnitTypeSet* s_noTypes;
static int s_tick;
static int s_tamperAt = -1;
static int s_dropped = -1;
static int s_applied;

static bool IsHandled(SchedHandler handler)
{
    return handler == OrdersOnWeaponsParalyze;
}

void __stdcall SchedPublish(int outbox, const void* data, unsigned int size)
{
    int kind = *(const int*)data;
    int count = 0;
    const SchedRoute* routes = SchedRoutes(&count);
    for (int i = 0; i < count; i++)
        if (routes[i].outbox == outbox && routes[i].kind == kind && routes[i].synchronous && IsHandled(routes[i].handler))
            routes[i].handler(data, size);
}

static void __stdcall FakeUnitRefInit(UnitRef*, Unit*, OrderBase*) {}
static void __stdcall FakeUnitRefSetUnit(UnitRef*, Unit*) {}
static void __stdcall FakeUnitRefSetValue(UnitRef*, OrderBase*) {}

static int __stdcall PlainApply(const OrdersEventHeader* e, unsigned int size)
{
    s_applied++;
    return OrdersApply(e, size);
}

static int __stdcall TamperedApply(const OrdersEventHeader* e, unsigned int size)
{
    if (e->kind == ORDERS_PARALYZE && s_tick == s_tamperAt && s_dropped < 0) {
        s_dropped = s_tick;
        return 0;
    }
    return PlainApply(e, size);
}

static int Ticks(int tick)
{
    return 3 + tick % 5;
}

static OrderTableEntry s_paralyzeEntry;

static void Setup()
{
    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    s_units = (Unit*)SimAlloc((kUnits + 1) * sizeof(Unit));
    memset((void*)s_units, 0, (kUnits + 1) * sizeof(Unit));
    s_defs = (UnitDef*)SimAlloc((kUnits + 1) * sizeof(UnitDef));
    memset((void*)s_defs, 0, (kUnits + 1) * sizeof(UnitDef));
    s_noTypes = new (SimAlloc(sizeof(UnitTypeSet))) UnitTypeSet();
    g_game->units = s_units;
    g_game->unitsEnd = &s_units[kUnits];
    g_game->players[0].index = 0;
    g_game->players[0].active = 1;
    g_game->players[0].type = 1;
    s_defs[kUnit].flags1 = 0x10000000;
    s_defs[kUnit].categories = s_noTypes;
    s_units[kUnit].def = &s_defs[kUnit];
    s_units[kUnit].unitDefIndex = 11;
    s_units[kUnit].player = &g_game->players[0];
    s_units[kUnit].playerIndex = 0;
    s_units[kUnit].id = kUnit;

    static UnitsSeam units;
    memset(&units, 0, sizeof units);
    units.UnitRefInit = FakeUnitRefInit;
    units.UnitRefSetUnit = FakeUnitRefSetUnit;
    units.UnitRefSetValue = FakeUnitRefSetValue;
    g_seams.units = &units;

    memset(&s_paralyzeEntry, 0, sizeof s_paralyzeEntry);
    s_paralyzeEntry.name = g_paralyzeMissionName;
    RegisterOrderTypes(&s_paralyzeEntry, 1);
}

static void Restart()
{
    s_units[kUnit].list = (Order*)0;
    s_dropped = -1;
    s_applied = 0;
}

struct Run {
    int ticks[kTicks];
    int orders[kTicks];
    int tampered;
    int applied;
};

static int OrderCount(Unit* unit)
{
    int n = 0;
    for (Order* o = unit->list; o != 0; o = o->next)
        n++;
    return n;
}

static void Play(Run* run, bool tamper)
{
    Restart();
    s_tamperAt = tamper ? kTamperTick : -1;
    for (int t = 0; t < kTicks; t++) {
        s_tick = t;
        g_game->ticks = t;
        WeaponsParalyze e;
        memset(&e, 0, sizeof e);
        e.h = WeaponsHeaderOf(WEAPONS_EFFECT_PARALYZE, &s_units[kUnit]);
        e.ticks = Ticks(t);
        SchedPublish(SCHED_WEAPONS, &e, sizeof e);
        Order* first = s_units[kUnit].list;
        run->ticks[t] = first ? first->ticks : -1;
        run->orders[t] = OrderCount(&s_units[kUnit]);
    }
    run->tampered = s_dropped;
    run->applied = s_applied;
}

struct Line {
    char record[64];
    char field[64];
    int index;
    int size;
    unsigned int value;
};
static Line s_lines[2048];
static int s_lineCount;
static char s_record[64];

static void AddLine(const char* field, int index, int size, unsigned int value)
{
    if (s_lineCount >= 2048)
        return;
    Line* l = &s_lines[s_lineCount++];
    strcpy(l->record, s_record);
    strcpy(l->field, field);
    l->index = index;
    l->size = size;
    l->value = value;
}

static void __stdcall RecBegin(int, const char* name)
{
    strcpy(s_record, name);
}

static void __stdcall RecInt(const char* field, int index, int size, unsigned int value)
{
    AddLine(field, index, size, value);
}

static void __stdcall RecBytes(const char* field, int index, const void*, int size)
{
    AddLine(field, index, -size, 0);
}

static void __stdcall RecHandle(const char* field, int index, int, unsigned int value)
{
    AddLine(field, index, 0, value);
}

static void __stdcall RecRun(int) {}

static const Line* Find(const char* record, const char* field, int index)
{
    for (int i = 0; i < s_lineCount; i++)
        if (strcmp(s_lines[i].record, record) == 0 && strcmp(s_lines[i].field, field) == 0 && s_lines[i].index == index)
            return &s_lines[i];
    return nullptr;
}

static void Dump(void (__stdcall* writeState)(const StateWriter*))
{
    StateWriter writer = { RecBegin, RecInt, RecBytes, RecHandle, RecRun };
    s_lineCount = 0;
    s_record[0] = 0;
    writeState(&writer);
}

static Order* NewOrder(int kind, unsigned int wake, int arg0)
{
    Order* o = (Order*)SimAlloc(sizeof(Order));
    memset((void*)o, 0, sizeof(Order));
    o->kind = MissionType((unsigned char)kind);
    o->wakeFrame = wake;
    o->type = arg0;
    return o;
}

static void CheckStateRecords()
{
    g_game->poolCount = kUnits + 1;
    Order* first = NewOrder(3, 40, 5);
    Order* second = NewOrder(4, 50, 6);
    first->next = second;
    s_units[kUnit].list = first;
    Dump(OrdersWriteState);
    Check(Find("ordersState.units[1]", "list[%u].wakeFrame", 0) != nullptr && Find("ordersState.units[1]", "list[%u].wakeFrame", 0)->value == 40, "the unit's record holds the wake frame of its first order");
    Check(Find("ordersState.units[1]", "list[%u].arg0", 1) != nullptr && Find("ordersState.units[1]", "list[%u].arg0", 1)->value == 6, "the unit's record holds the first argument of its second order");
    Check(Find("ordersState.units[1]", "list[%u].kind", 1) != nullptr && Find("ordersState.units[1]", "list[%u].kind", 1)->value == 4, "the unit's record holds the kind of its second order");
    Check(Find("ordersState.units[1]", "list[%u].kind", 2) == nullptr && Find("ordersState.units[1]", "list2[%u].kind", 0) == nullptr, "the record stops at the end of each list");
    first->wakeFrame = 41;
    second->type = 9;
    Dump(OrdersWriteState);
    Check(Find("ordersState.units[1]", "list[%u].wakeFrame", 0)->value == 41, "tampering with an order's wake frame changes its record");
    Check(Find("ordersState.units[1]", "list[%u].arg0", 1)->value == 9, "tampering with an order's first argument changes its record");
    s_units[kUnit].list = 0;
    Dump(OrdersWriteState);
    Check(s_lineCount == 0, "a unit without orders has no record");
}

int main()
{
    Setup();
    static OrdersSeam plainSeam, tamperedSeam;
    memset(&plainSeam, 0, sizeof plainSeam);
    plainSeam.Apply = PlainApply;
    tamperedSeam = plainSeam;
    tamperedSeam.Apply = TamperedApply;

    static Run plain, again, tampered;
    g_seams.orders = &plainSeam;
    Play(&plain, false);
    Play(&again, false);
    g_seams.orders = &tamperedSeam;
    Play(&tampered, true);

    CheckStateRecords();
    int sum = 0;
    for (int t = 0; t < kTicks; t++)
        sum += Ticks(t);
    Check(plain.tampered < 0, "the default drops no event");
    Check(plain.orders[0] == 1 && plain.ticks[0] == Ticks(0), "the first paralysis appends a paralyze order with its ticks");
    Check(plain.orders[kTicks - 1] == 1, "later paralyses add to the first order and append no other");
    Check(plain.ticks[1] == Ticks(0) + Ticks(1), "a later paralysis adds its ticks to the paralyze order");
    Check(plain.ticks[kTicks - 1] == sum, "the paralyze order holds the ticks of every paralysis");
    Check(plain.applied == kTicks, "every paralysis reaches the orders module");
    Check(memcmp(&plain, &again, sizeof plain) == 0, "two runs of the default give the same orders at every tick");
    Check(tampered.tampered == kTamperTick, "the tampered module drops a paralysis at the tampered tick");
    Check(tampered.applied == plain.applied - 1, "the tampered module drops exactly one event");
    Check(tampered.ticks[kTicks - 1] == sum - Ticks(kTamperTick), "the tampered module's order lacks exactly the dropped ticks");
    int first = -1;
    int differing = 0;
    for (int t = 0; t < kTicks; t++) {
        if (plain.ticks[t] != tampered.ticks[t]) {
            if (first < 0)
                first = t;
            differing++;
        }
    }
    Check(first == kTamperTick, "the order's ticks differ first at the tampered tick");
    Check(differing == kTicks - kTamperTick, "the order's ticks differ at every tick from the tampered tick on");
    printf("tampered tick %d, first differing tick %d, %d differing ticks\n", tampered.tampered, first, differing);

    printf(g_failures ? "FAILED\n" : "orders_writer_test: all passed\n");
    return g_failures != 0;
}
