#include <stdio.h>
#include <string.h>

#include "../interfaces/ref.h"
#include "../src/state/block.h"
#include "../src/ai/energy_rating.h"
#include "../src/network/resource_share.h"
#include "../src/util/x87_compare.h"

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

void (*g_outOfMemoryHandler)() = 0;

static int g_failures;
static int g_testMode;
static int g_fatals;
static char g_fatalText[512];

int HarnessTestMode()
{
    return g_testMode;
}

extern "C" void HOST_CALL host_fatal(void*, const char*, const char* text)
{
    g_fatals++;
    strncpy(g_fatalText, text, sizeof(g_fatalText) - 1);
}

static void Check(bool ok, const char* what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        g_failures++;
    }
}

struct Thing {
    int a;
    int b;
};

static_assert(sizeof(Ref<Thing>) == 4, "size");
static_assert(__is_trivial(Ref<Thing>) && __is_trivially_copyable(Ref<Thing>), "a Ref is plain data");

static double g_energyUse;

static double __stdcall FakeEnergyUse(UnitDef*)
{
    return g_energyUse;
}

static unsigned BitsOf(float value)
{
    unsigned bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static int RatingAt(float energyCost, float windStrength, float windGenerator)
{
    g_energyUse = -((double)windStrength * (double)windGenerator);
    return ComputeEnergyRating(energyCost, FakeEnergyUse, 0);
}

int main()
{
    Ref<Thing> zeroInit = {};
    Check(zeroInit.get() == 0, "empty braces give null");
    SimBlockInit();
    Thing* t = (Thing*)SimAlloc(sizeof(Thing));
    t->a = 7;
    t->b = 9;
    Thing* u = (Thing*)SimAlloc(sizeof(Thing));
    Thing outside;
    Thing* base = (Thing*)SimBlockBase();

    Ref<Thing> r = t;
    Check(r.offset == (unsigned)((char*)t - (char*)base), "a Ref holds the offset from the base");
    Check(r.offset != 0, "an object is never at offset 0");
    Check((Thing*)r == t && r.get() == t, "a Ref round-trips to its pointer");
    Check(r->a == 7 && (*r).b == 9, "-> and * reach the object");
    r->a = 11;
    Check(t->a == 11, "writing through -> writes the object");

    Ref<Thing> n;
    memset(&n, 0, sizeof(n));
    Check(n.get() == 0 && !n, "a zeroed Ref is null");
    Ref<Thing> n2 = 0;
    Ref<Thing> n3 = nullptr;
    Ref<Thing> n4 = (Thing*)0;
    Check(n2.offset == 0 && n3.offset == 0 && n4.offset == 0, "null converts to offset 0");
    Check(n == nullptr && nullptr == n && !(n != nullptr) && n == 0, "null compares equal to null");
    Check(r != nullptr && nullptr != r && r != 0, "an object is not null");
    Check(r, "a Ref to an object is true");

    Ref<Thing> r2 = t;
    Ref<Thing> r3 = u;
    Check(r == r2 && !(r != r2), "equal Refs compare equal");
    Check(r != r3 && !(r == r3), "different Refs compare unequal");
    Check(r == t && t == r && r3 != t && t != r3, "a Ref compares with a pointer");
    Check(r != n && n != r, "a Ref differs from null Ref");

    Thing* raw = r;
    Check(raw == t, "implicit conversion to a pointer");

    struct Holder {
        Ref<Thing> first;
        Ref<Thing> second;
        int tail;
    };
    Holder h = {};
    Check(h.first == nullptr && h.second == nullptr && h.tail == 0, "aggregate initialisation gives null");
    Holder h2 = { t, u, 3 };
    Check(h2.first == t && h2.second == u && sizeof(Holder) == 12, "aggregate initialisation from pointers");

    g_testMode = 0;
    Ref<Thing> quiet = &outside;
    Check(g_fatals == 0, "an outside pointer is stored quietly outside test mode");
    Check(quiet.offset == (unsigned)((char*)&outside - (char*)base), "an outside pointer wraps to its offset");
    Check(quiet.get() == &outside, "an outside pointer round-trips on this 32-bit target");

    g_testMode = 1;
    Ref<Thing> inside = t;
    Check(g_fatals == 0, "an inside pointer is fine in test mode");
    Ref<Thing> loud = &outside;
    Check(g_fatals == 1, "an outside pointer reaches host_fatal in test mode");
    Check(strstr(g_fatalText, "Thing") != 0 && strstr(g_fatalText, "pointer outside the state block") != 0, "the message names the type");
    Ref<Thing> nul = nullptr;
    Check(g_fatals == 1, "null is fine in test mode");
    Ref<Thing> below = (Thing*)((char*)base - 64);
    Check(g_fatals == 2, "a pointer below the block reaches host_fatal");
    Check(loud.offset == (unsigned)((char*)&outside - (char*)base), "the offset is stored after the report");
    (void)inside;
    (void)nul;
    (void)below;

    Check(RatingAt(536.0f, 0.0156f, 30.0f) == 1, "the cost product is stored to 32 bits before the subtraction");
    Check(RatingAt(104.0f, 0.0084f, 30.0f) == 0, "the energy use is subtracted unrounded");

    Check(BitsOf(ShareMetalAmount(860641.4375f, 300444.46875f, 2000000.0f, 0.0f)) == 0x48365b15u, "the metal share is the unrounded difference times a third, stored once");
    Check(BitsOf(ShareMetalAmount(918230.75f, 290541.28125f, 209229.84375f, 0.0f)) == 0x484c5375u, "a metal share just under the capacity room is stored from the unrounded product");
    Check(BitsOf(ShareMetalAmount(813324.125f, 220410.484375f, 197637.890625f, 0.0f)) == 0x48410179u, "a metal share a hair under the capacity room is stored from the unrounded product");
    Check(BitsOf(ShareMetalAmount(860641.4375f, 300444.46875f, 1000.0f, 600.25f)) == 0x43c7e000u, "a metal share above the room is limited to the room");
    Check(BitsOf(ShareEnergyAmount(1048576.25f, 100.1328125f, 4000000.0f, 0.0f)) == 0x48fff9c2u, "the energy share is half the difference");

    const float nan = __builtin_nanf("");
    const float inf = __builtin_inff();
    Check(StallsAllowRequest(nan, 0.0f), "a NaN energy stall grants the request");
    Check(StallsAllowRequest(0.0f, nan), "a NaN metal stall grants the request");
    Check(StallsAllowRequest(0.0f, -0.0f) && StallsAllowRequest(-1.0f, -inf), "stalls at or below zero grant the request");
    Check(!StallsAllowRequest(1.0f, 0.0f) && !StallsAllowRequest(0.0f, 1.0f) && !StallsAllowRequest(inf, 0.0f), "a positive stall refuses the request");
    Check(CloakCostPayable(5.0f, nan), "a NaN energy stock pays the cloak");
    Check(CloakCostPayable(5.0f, 5.0f) && CloakCostPayable(5.0f, inf) && CloakCostPayable(0.0f, -0.0f), "an energy stock covering the cloak pays");
    Check(!CloakCostPayable(5.0f, 4.0f) && !CloakCostPayable(5.0f, -inf), "an energy stock below the cloak refuses");
    Check(SettlementBacklogCovered(nan, 1.0f) && SettlementBacklogCovered(1.0f, nan), "a NaN backlog or supply covers the backlog");
    Check(SettlementBacklogCovered(1.0f, 1.0f) && SettlementBacklogCovered(0.0f, -0.0f) && SettlementBacklogCovered(1.0f, inf), "a backlog within the supply is covered");
    Check(!SettlementBacklogCovered(2.0f, 1.0f) && !SettlementBacklogCovered(inf, 1.0), "a backlog above the supply is not covered");
    Check(SettlementDemandCovered(nan, 1.0) && SettlementDemandCovered(1.0f, (double)nan), "a NaN demand or remainder covers the demand");
    Check(SettlementDemandCovered(1.0f, 1.0) && SettlementDemandCovered(0.0f, -0.0) && SettlementDemandCovered(1.0f, (double)inf), "a demand within the remainder is covered");
    Check(!SettlementDemandCovered(2.0f, 1.0) && !SettlementDemandCovered(inf, 1.0), "a demand above the remainder is not covered");
    Check(MetalRecipientPoorer(nan, 5.0f) && MetalRecipientPoorer(5.0f, nan), "a NaN metal stock is chosen as the recipient");
    Check(MetalRecipientPoorer(1.0f, 2.0f) && MetalRecipientPoorer(-inf, 0.0f), "a poorer metal stock is chosen");
    Check(!MetalRecipientPoorer(2.0f, 2.0f) && !MetalRecipientPoorer(0.0f, -0.0f) && !MetalRecipientPoorer(inf, 1.0f), "an equal or richer metal stock is not chosen");
    Check(EnergyRecipientPoorer(nan, 5.0f) && EnergyRecipientPoorer(5.0f, nan), "a NaN energy stock is chosen as the recipient");
    Check(EnergyRecipientPoorer(1.0f, 2.0f) && EnergyRecipientPoorer(-inf, 0.0f), "a poorer energy stock is chosen");
    Check(!EnergyRecipientPoorer(2.0f, 2.0f) && !EnergyRecipientPoorer(0.0f, -0.0f) && !EnergyRecipientPoorer(inf, 1.0f), "an equal or richer energy stock is not chosen");
    Check(BitsOf(ShareMetalAmount(300.0f, 0.0f, nan, 0.0f)) == 0x42c80000u, "a NaN capacity room takes the amount");
    float nanShare = ShareEnergyAmount(nan, 0.0f, 1000.0f, 0.0f);
    Check(nanShare != nanShare, "a NaN product takes the amount, not the room");
    Check(BitsOf(ShareMetalAmount(300.0f, 0.0f, 1000.0f, 0.0f)) == 0x42c80000u && BitsOf(ShareMetalAmount(300.0f, 0.0f, 50.0f, 0.0f)) == 0x42480000u && BitsOf(ShareMetalAmount(300.0f, 0.0f, 100.0f, 0.0f)) == 0x42c80000u, "ordered shares take the smaller of amount and room");
    Check(X87EqTest40(nan, 0.0f) && X87EqTest40(0.0f, -0.0f) && !X87EqTest40(1.0f, 0.0f) && !X87EqTest40(inf, 0.0f), "a NaN build left counts as zero");

    printf(g_failures ? "FAILED\n" : "ref_test: all passed\n");
    return g_failures != 0;
}
