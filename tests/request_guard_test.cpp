#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../interfaces/services.h"
#include "../interfaces/economy.h"
#include "../src/state/sim_globals.h"
#include "../src/game/scheduler.h"
#include "../src/game/economy_events.h"

struct Game;
Game* g_game;
Seams g_seams;
SimGlobals* g_simGlobals = 0;
void (*g_outOfMemoryHandler)() = 0;

static int g_failures;
static int s_testMode = 1;
static int s_fatals;
static int s_granted;

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
    return s_testMode;
}

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    printf("FAIL: host_fatal: %s: %s\n", title, text);
    g_failures++;
}

void __stdcall FatalError(char*)
{
    s_fatals++;
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}

void __stdcall HarnessEconomyEvent(const EconomyEventHeader*, unsigned int, int) {}

static int __stdcall DiscardEvent(const EconomyEventHeader*, unsigned int)
{
    return 1;
}

static int __stdcall CountGranted(EconUnit, unsigned int)
{
    s_granted++;
    return 1;
}

static int Ask(unsigned int unit)
{
    s_fatals = 0;
    s_granted = 0;
    ScheduleEconomyRequestGranted(unit, 5);
    return s_fatals;
}

int main()
{
    SimBlockInit();
    static EconomySeam economy;
    memset(&economy, 0, sizeof economy);
    economy.RequestGranted = CountGranted;
    economy.Apply = DiscardEvent;
    g_seams.economy = &economy;

    int count = 0;
    const SchedRoute* routes = SchedRoutes(&count);
    int queued = 0;
    for (int i = 0; i < count; i++)
        if (routes[i].outbox == SCHED_UNITS && routes[i].kind == ECONOMY_RESET && routes[i].subscriber == SCHED_ECONOMY && !routes[i].synchronous)
            queued = 1;
    Check(queued, "units' RESET of economy is a queued route");

    const unsigned int kUnit = 7;
    const unsigned int kOther = 9;

    SchedEndTick();
    Check(Ask(kUnit) == 0 && s_granted == 1, "with nothing pending the request is granted");

    SchedBeginTick();
    EconomyEventHeader other = EconHeader(ECONOMY_RESET, 0, kOther);
    SchedPublish(SCHED_UNITS, &other, sizeof other);
    Check(Ask(kUnit) == 0, "a RESET of another unit pending does not fail the request");
    Check(Ask(kOther) == 1, "a RESET of the asking unit pending fails the request");
    SchedPhaseBoundary();
    Check(Ask(kOther) == 0, "a RESET that was delivered does not fail the request");

    EconomyEventHeader reset = EconHeader(ECONOMY_RESET, 0, kUnit);
    SchedPublish(SCHED_UNITS, &reset, sizeof reset);
    Check(Ask(kUnit) == 1, "a RESET of the asking unit pending in economy's inbox fails the request");
    s_testMode = 0;
    Check(Ask(kUnit) == 0, "with test mode off a pending RESET does not fail the request");
    s_testMode = 1;
    SchedEndTick();

    SchedBeginTick();
    EconAccount account;
    memset(&account, 0, sizeof account);
    EconPublishLoad(SCHED_UNITS, 0, kUnit, account);
    Check(Ask(kUnit) == 1, "a LOAD of the asking unit pending fails the request");
    Check(Ask(kOther) == 0, "a LOAD of another unit pending does not fail the request");
    SchedEndTick();

    printf(g_failures ? "FAILED\n" : "request_guard_test: all passed\n");
    return g_failures != 0;
}
