#include <stdio.h>
#include "../state/block.h"
#include "../../interfaces/ref.h"
#include <string.h>

#include "../map/mission.h"
#include "mission_conditions.h"
#include "../game/unit_resources.h"
#include "../files/files_api.h"
#include "players.h"

struct Player;
struct Unit;
class Pathfinder;

#pragma pack(push, 1)

#include "../ai/squad_manager.h"

#include "../util/vec3.h"

struct Map {
    char unknown_0[0x620];
    int field_620;
};

struct UnitDef;
#include "../units/unit_def.h"

#include "../units/unit.h"

#include "../network/player_info.h"
#include "../../interfaces/seams.h"

#include "../network/player.h"

struct SkirmishPlayerSlot {
    int controller;
    char unknown_4[0xc - 0x4];
    int metal;
    int energy;
    int unknown_14;
};

struct Form {
    char unknown_0[0xcc];
    char choice1[0x10];
    char choice2[0x20];
};

struct Gadget;

struct Screen {
    char unknown_0[4];
    Form* form;
    void (__stdcall *callback)(Gadget*);
};

struct Gadget {
    char unknown_0[0x18];
    Screen* screen;
    char unknown_1c[0x60 - 0x1c];
    int selected;
};

#include "game.h"
#include "los_helpers.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

#include "../../link/globals.h"

const char* __stdcall Translate(const char* text);

// FUNCTION: 0x464290
void __stdcall SetupPlayerSlot(int player, char type)
{
    Player* p = &g_game->players[player & 0xff];
    int idx = player & 0xff;

    memset(p->allied, 0, 11);
    memset(p->alliedBy, 0, 11);
    memset(p->shareLos, 0, 11);
    memset(p->shareVision, 0, 11);
    memset(p->shareMapping, 0, 11);
    int t = type;
    p->type = t;
    if (t != 3) {
        p->info->kind = t;
    }
    p->alliedBy[idx] = 1;
    p->ai = 0;
    p->index = (char)player;
    p->rank = (char)player;
    p->allied[idx] = 1;
    p->active = 1;
    p->startPos = (char)player;
    p->alliance = 5;
    p->info->bit5 = 0;
    p->id = player & 0xff;
    p->lobbyDataSynced = 0;
    p->rejectReason = 0;

    if (p->active != 0 && (p->type == 1 || p->type == 2)) {
        p->info->memory = ((Map*)g_game->displayContext)->field_620 / 0x100000 + 1;
    }

    if (g_seams.map->GetGameType(g_game->net) == 1) {
        if (type == 1) {
            sprintf(p->name, Translate("Player"));
        } else if (type == 2) {
            if (p->info->side == 0) {
                sprintf(p->name, "Arm");
            } else {
                sprintf(p->name, "Core");
            }
        }
        strcpy(p->fullName, p->name);
    }

    if (g_seams.map->GetGameType(g_game->net) == 2) {
        if (type == 1) {
            sprintf(p->name, Translate("Player"));
        } else if (type == 2) {
            if (p->info->side == 0) {
                sprintf(p->name, Translate("Arm"));
            } else {
                sprintf(p->name, Translate("Core"));
            }
        }
        strcpy(p->fullName, p->name);
    }
}


// FUNCTION: 0x4644d0
void ResetPlayerSlots()
{
    Player* p;
    int i;

    for (i = 0; i <= 10; i++) {
        PlayerInfo* t = g_game->players[i].info;
        memset(&g_game->players[i], 0, 0x14b);
        g_game->players[i].info = t;
    }

    g_game->localPlayer = 0;
    g_game->playerIndex = 0;
    memset(g_game->playerIds, 0, 0x2c);

    p = &g_game->players[0];
    for (i = 0; i <= 10; i++, p++) {
        memset(p->allied, 0, 11);
        memset(p->alliedBy, 0, 11);
        memset(p->info, 0, 0xb9);
        sprintf(p->name, "Player %d First", i);
        sprintf(p->fullName, "Player %d Second", i);
        p->allied[i] = 1;
        p->alliedBy[i] = 1;
        p->Clear();
        p->info->kind = 0;
        p->info->memory = 0;
        p->info->started = 0;
        p->info->host = 0;
        p->info->color = (char)i;
        p->info->side = 0;
        strcpy(p->info->map, DAT_005119b8);
        p->ai = 0;
        p->lobbyDataSynced = 0;
        p->syncTick = 0;
        p->unitsBegin = 0;
        p->unitsEnd = 0;
        p->firstIndex = 0;
        p->lastIndex = 0;
        p->unitCount = 0;
        p->keepaliveFlags &= 0xfe;
        p->unitsCreated = 0;
        p->id = -1;
        p->index = 10;
        p->alliance = 5;
        p->info->bit5 = 0;
    }

    g_game->tail = 0;
    g_game->head = 0;
}


void __cdecl operator delete(void* p);

// FUNCTION: 0x464700
void __stdcall InitPlayerSlot(Player* p)
{
    p->updateTime = (unsigned int)g_game->ticks;
    p->winLoseTime = (unsigned int)g_game->ticks;
    goto stamp;
back:
    p->totalEnergyProducedLo = 0;
    p->totalMetalProducedLo = 0;
    p->totalEnergyConsumedLo = 0;
    p->totalMetalConsumedLo = 0;
    p->energyWastedLo = 0;
    p->metalWastedLo = 0;
    p->energy = 0;
    p->energyIncome = 0;
    p->energyUsage = 0;
    p->metal = 0;
    p->metalIncome = 0;
    p->metalUsage = 0;
    p->energyCapacity = 0;
    p->metalCapacity = 0;
    p->totalEnergyProducedHi = 0;
    p->totalMetalProducedHi = 0;
    p->totalEnergyConsumedHi = 0;
    p->totalMetalConsumedHi = 0;
    p->shareEnergy = 0;
    p->shareMetal = 0;
    p->energyWastedHi = 0;
    p->metalWastedHi = 0;
    goto guard;
stamp:
    p->displayTimer = (unsigned int)g_game->ticks;
    goto back;
guard:
    if (!p->econ) {
        p->econ = new (kSim) UnitResources;
    }
    g_seams.economy->Reset(p->econ, p->index);
    p->flags &= 0xfffe;
    p->kills = 0;
    p->losses = 0;
    p->commanderKills = 0;
    p->commanderLosses = 0;
    p->unused_102 = p->unused_100 = -1;
    int w, h;
    h = g_game->height / 2;
    w = g_game->width / 2;
    p->exploredWidth = w;
    p->exploredHeight = h;
    operator delete(p->explored);
    int& sz = p->exploredSize;
    sz = (h * w + 7) & ~7;
    p->explored = sz ? (unsigned char*)SimAlloc(sz) : 0;
    Ref<unsigned char>& bref = p->explored;
    memset(bref, 0, sz);
    g_seams.ai->CreateSquads(p);
    if (!p->active || p->type != 3) {
        p->ai = new (kSim) SquadManager(p);
        g_seams.ai->CreatePlayerAI(p->index);
    }
}

#include "../util/commands.h"
#include "../graphics/object_pool.h"

// FUNCTION: 0x4648e0
void LoadDefaultAIScript()
{
    int size;
    char* name = g_seams.map->GetNameSlot(g_game->net, 7);
    char* text = (char*)HAPI_LoadFile(name, &size);
    if (text == 0) {
        text = (char*)HAPI_LoadFile("ai\\default.txt", &size);
    }
    if (text != 0) {
        CommandArgs vars;
        vars.InitArgs();
        ExecuteCommandText(text, size, &vars, -1);
        GameFreeThunk(text);
    }
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->active && p->type == 2) {
            g_seams.ai->ParseDownloadableAiWeightScripts(i);
            g_seams.ai->ReparseAiWeightScriptsIfLimitNotSticky(i);
        }
    }
}

// FUNCTION: 0x464990
void InitPlayers()
{
    for (char i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->type) {
            InitPlayerSlot(p);
        }
    }
    LoadDefaultAIScript();
}

// FUNCTION: 0x4649d0
void RebuildAIFeatureCells()
{
    for (char i = 0; i < 10; i++) {
        if (g_game->players[i].ai) {
            g_seams.ai->RebuildFeatureCells(i);
        }
    }
}

// FUNCTION: 0x464a00
void FreePlayers()
{
    for (Player* p = g_game->players; p <= g_game->players + 10; p++) {
        int* q = (int*)((char*)p + 0x84);
        q[-1] = 0;
        q[0] = 0;
        delete[] (void*)((Ref<unsigned char>*)q)[-2].get();
        q[1] = 0;
        ((Ref<unsigned char>*)q)[-2] = (unsigned char*)0;
        g_seams.ai->FreeSquads(p);
        if (((Ref<SquadManager>*)q)[-4]) {
            g_seams.ai->DestroyPlayerAI(((unsigned char*)q)[0xc2]);
            if (SquadManager* u = ((Ref<SquadManager>*)q)[-4].get()) {
                g_seams.ai->SquadManagerDeleteTimers(u, 0);
                delete u;
            }
            ((Ref<SquadManager>*)q)[-4] = (SquadManager*)0;
        }
        if (((Ref<UnitResources>*)q)[0x1a]) {
            delete[] ((Ref<UnitResources>*)q)[0x1a].get();
            ((Ref<UnitResources>*)q)[0x1a] = (UnitResources*)0;
        }
    }
}

// FUNCTION: 0x464ab0
float __stdcall GetEnergyIncome(void* param_1)
{
    return *(float*)((char*)param_1 + 0x90);
}

void RegisterUnitOrders(void);

// FUNCTION: 0x464ac0
float __stdcall GetEnergyUsage(Player* param_1)
{
    return param_1->energyUsage;
}

// FUNCTION: 0x464ad0
float __stdcall GetNetEnergy(void* param_1)
{
    return *(float*)((char*)param_1 + 0x90) - *(float*)((char*)param_1 + 0x94);
}

// FUNCTION: 0x464af0
float __stdcall GetMetalIncome(void* param_1)
{
    return *(float*)((char*)param_1 + 0x9c);
}

// FUNCTION: 0x464b00
float __stdcall GetMetalUsage(void* param_1)
{
    return *(float*)((char*)param_1 + 0xa0);
}

struct Class_464b10 {
    char unknown_0[0x9c];
    float metalIncome;
    float metalUsage;
};

// FUNCTION: 0x464b10
float __stdcall GetNetMetal(Class_464b10* param_1)
{
    return param_1->metalIncome - param_1->metalUsage;
}

#include "../network/net_game.h"

// FUNCTION: 0x464b30
void __stdcall TransferMetal(unsigned char from, unsigned char to, float amount, int flag)
{
    if (from == 10)
        return;
    if (to == 10)
        return;
    if (flag) {
        float cap = g_game->players[from].energy;
        if (amount > cap)
            amount = cap;
    }
    if (amount == 0.0f)
        return;
    Player* player;
    player = flag ? (g_seams.economy->SpendEnergy(g_game->players[from].econ, amount),
                     g_game->players[to].econ->player)
                  : g_game->players[to].econ->player;
    if (player->active != 0 && player->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            g_game->players[to].econ->energyMake
                = g_game->players[to].econ->energyMake - amount * -0.5;
            break;
        case 1:
            g_game->players[to].econ->energyMake
                = g_game->players[to].econ->energyMake - amount * -0.7;
            break;
        default: {
            float m = g_game->players[to].econ->energyMake;
            m += amount;
            g_game->players[to].econ->energyMake = m;
            break;
        }
        }
    } else {
        float m = g_game->players[to].econ->energyMake;
        m += amount;
        g_game->players[to].econ->energyMake = m;
    }
    if (flag)
        SendShareMetal(from, to, *(int*)&amount);
}

// FUNCTION: 0x464c60
void __stdcall TransferEnergy(unsigned char from, unsigned char to, float amount, int flag)
{
    if (from == 10)
        return;
    if (to == 10)
        return;
    if (flag) {
        float cap = g_game->players[from].metal;
        if (amount > cap)
            amount = cap;
    }
    if (amount == 0.0f)
        return;
    Player* player;
    player = flag ? (g_seams.economy->SpendMetal(g_game->players[from].econ, amount), g_game->players[to].econ->player)
                  : g_game->players[to].econ->player;
    if (player->active != 0 && player->type == 2) {
        switch (g_game->difficulty) {
        case 0:
            g_game->players[to].econ->metalMake += amount * 0.5;
            break;
        case 1:
            g_game->players[to].econ->metalMake += amount * 0.7;
            break;
        default:
            g_game->players[to].econ->metalMake = amount + g_game->players[to].econ->metalMake;
            break;
        }
    } else {
        g_game->players[to].econ->metalMake = amount + g_game->players[to].econ->metalMake;
    }
    if (flag)
        SendShareEnergy(from, to, *(int*)&amount);
}

// FUNCTION: 0x464da0
void __stdcall RefreshOwnedUnitsLineOfSight(Player* range)
{
    for (Unit* u = range->unitsBegin; u <= range->unitsEnd; u++) {
        if (u->flags & 0x10000000) {
            g_seams.los_radar->UpdateUnitLineOfSight(u);
        }
    }
}

#include "../network/unit_sync.h"
void __stdcall PlaySoundByName(char* str, int flag);
int __stdcall IsGadgetNamed(int param1, int param2, char* name);
#include "../gui/dialogs.h"

// FUNCTION: 0x464de0
void __stdcall ContinueWatchingCallback(Gadget* gadget)
{
    int screen = (int)gadget->screen->form;
    if (gadget->selected == -1)
        return;
    PlaySoundByName("BigButton", 0);
    if (IsGadgetNamed(screen, gadget->selected, "CHOICE1")) {
        BroadcastPlayerInfo();
        g_game->bits.flag4 = 0;
        ReportGameEvent(4);
        return;
    }
    if (!IsGadgetNamed(screen, gadget->selected, "CHOICE2")) {
        ClearSelectedGadget(gadget);
        return;
    }
    g_game->bits.flag2 = 1;
    g_game->bits.flag4 = 0;
}


Screen* __stdcall LoadGuiLayer(Menu* menu, char* name, int value);
void __stdcall SetKeyboardInput(Menu* menu, int flag);
void __stdcall SetTranslatedTextByName(Menu* menu, char* name, char* text, int value);
#include "../gui/gui.h"

// FUNCTION: 0x464e70
void ShowContinueWatchingDialog()
{
    Screen* screen = LoadGuiLayer(&g_game->gui_519, g_yesNoGuiName, 0x900);
    if (screen) {
        Form* form;
        SetKeyboardInput(&g_game->gui_519, 1);
        form = screen->form;
        SetTranslatedTextByName(&g_game->gui_519, g_choice1GadgetName, g_yesText, 0);
        SetTranslatedTextByName(&g_game->gui_519, g_choice2GadgetName, g_noText, 0);
        SetTranslatedTextByName(&g_game->gui_519, g_titleGadgetName, g_continueWatchingTitle, 0);
        strcpy(form->choice1, g_choice1GadgetName);
        strcpy(form->choice2, g_choice2GadgetName);
        screen->callback = ContinueWatchingCallback;
        RenderLayer((Gui *)&g_game->gui_519, 0x40);
    }
}


unsigned char __stdcall FindHostSlot();
#include "../util/math.h"
Unit* __stdcall CreateUnit(unsigned char player, unsigned short typeId,
                           Vec3 pos, int a, int b, int c);
#include "main_loop.h"
#include "../ingame/selection.h"
void __stdcall SendPlayerEconomy(Player* player, int a, int b);

static int loopCond_00464f80(unsigned char i)
{
    if (i >= 0xa)
        return 0;
    return 1;
}

static int more_00464f80(unsigned char i)
{
    if (i < 0xa)
        return 1;
    return 0;
}

static const double kNegSeven = -0.7;
static const double kNegHalf = -0.5;
static const float kHundred = 100.0f;

void __stdcall UpdatePlayers()
{
    g_seams.players->UpdatePlayersOnStream(RAND_STREAM_MAIN);
}

// FUNCTION: 0x464f80
void __stdcall UpdatePlayersOnStream(RandStream stream)
{
    g_seams.pathfinding->RunSearches(g_game->pathfinder);
    unsigned char bl = 0;
    for (;;) {
        if (!loopCond_00464f80(bl))
            goto next_bl;
        Player* pi;
        if (g_game->players[bl].active == 0)
            goto next_bl;
        pi = &g_game->players[bl];

        {
            unsigned char t = pi->type;
            if (t != 1 && t != 2 && t != 3)
                goto next_bl;
        }
        if (pi->index == 0xa)
            goto next_bl;
        {
            Player* pi2 = &g_game->players[bl];
            if (pi2->active == 0)
                goto next_bl;
            unsigned char t2 = pi2->type;
            if (t2 != 1 && t2 != 2 && t2 != 3)
                goto next_bl;
            if (pi2->index == 0xa)
                goto next_bl;
        }

        if (pi->ai != 0)
            g_seams.ai->SquadManagerTickIfActive(pi->ai, 0);

        g_seams.ai->UpdatePlayerAI(bl);

        {
            Unit* u = pi->unitsBegin;
            while (u <= pi->unitsEnd) {
                if (u->flags & 0x10000000)
                    g_seams.los_radar->UpdateUnitLineOfSight(u);
                u = (Unit*)((char*)u + 0x118);
            }
        }

        if (bl == g_game->playerIndex)
            g_seams.los_radar->DrawRadarUnits();

        if ((unsigned int)pi->updateTime > (unsigned int)g_game->ticks)
            goto next_bl;
        pi->updateTime += 0x1e;

        if (bl == g_game->localPlayer) {
            if (g_seams.map->GetGameType(g_game->net) == 1) {
                if (g_game->list_391ed->CheckVictory() == 0) {
                    if (g_game->list_391ed->CheckDefeat(stream) != 0) {
                        if (g_game->endGameCountdown < 0) {
                            g_game->endGameCountdown = 4;
                        } else {
                            g_game->endGameCountdown--;
                            if (g_game->endGameCountdown < 0) {
                                g_game->endGameFlags |= 4;
                                g_game->endGameFlags &= 0xffef;
                                g_game->bits.flag6 = 1;
                            }
                        }
                    }
                } else {
                    if (g_game->endGameCountdown < 0) {
                        g_game->endGameCountdown = 4;
                    } else {
                        g_game->endGameCountdown--;
                        if (g_game->endGameCountdown < 0) {
                            g_game->endGameFlags |= 4;
                            g_game->bits.flag4 = 1;
                            g_game->bits.flag5 = 1;
                        }
                    }
                    goto skip508;
                }
            } else if ((pi->active == 0 ||
                        (pi->info->flags_9b & 0x40) == 0) &&
                       g_game->list_391ed->CheckDefeat(stream) != 0) {
                if (g_game->endGameCountdown < 0) {
                    g_game->endGameCountdown = 4;
                } else {
                    g_game->endGameCountdown--;
                    if (g_game->endGameCountdown < 0) {
                        if (g_game->commanderDeath == 2) {
                            PlayerInfo* self =
                                g_game->players[FindHostSlot()].info;
                            unsigned int typeId;
                            typeId = g_seams.units->FindUnitTypeId(
                                &g_game->startPos[0x232 *
                                    g_game->players[g_game->localPlayer].info->side]) & 0xffff;
                            int bound = 9999;
                            int typeOff = typeId * 0x249;
                            Vec3 pos;
                            do {
                                int cx = g_game->baseX / 10;
                                int cy = g_game->baseY / 10;
                                pos.x = (RandomInt(g_game->baseX - 2 * cx) + cx) << 16;
                                pos.y = 0;
                                pos.z = (RandomInt(g_game->baseY - 2 * cy) + cy) << 16;
                                int hh = g_game->height << 16;
                                int hits = 0;
                                unsigned int zacc =
                                    (unsigned int)pos.z - (unsigned int)hh;
                                int outer = 3;
                                int hw = g_game->width << 16;
                                do {
                                    unsigned int xacc =
                                        (unsigned int)pos.x - (unsigned int)hw;
                                    int inner = 3;
                                    Point16 cell;
                                    cell.y = zacc >> 20;
                                    do {
                                        cell.x = xacc >> 20;
                                        if (g_seams.map->CanPlaceUnitFootprint(
                                                (UnitDef*)((char*)g_game->defs.get() + typeOff),
                                                0, cell, 1) != 0)
                                            hits++;
                                        xacc += hw;
                                    } while (--inner != 0);
                                    zacc += hh;
                                } while (--outer != 0);
                                if (hits >= 9 && (short)g_seams.weapons->FindFeatureAtPos(&pos, 0, 0) == -1) {
                                    if (g_game->net->lavaWorld == 0)
                                        break;
                                    if (g_seams.map->GetCellMeanHeight(&pos) >
                                        (int)g_game->seaLevel)
                                        break;
                                }
                            } while (--bound > 0);

                            {
                                Unit* unit = g_seams.units->CreateUnit(
                                    g_game->localPlayer, typeId, pos, 1, 1, 0);
                                SetStartingStorageBonus(pi,
                                             self->metal * 100,
                                             self->energy * 100);
                                {
                                    float* slot = &unit->energy;
                                    float f = (float)self->energy * kHundred;
                                    if (unit->owner->active != 0 &&
                                        unit->owner->type == 2) {
                                        switch (g_game->difficulty) {
                                        case 0: f = *slot - f * kNegHalf; break;
                                        case 1: f = *slot - f * kNegSeven; break;
                                        default: f = *slot + f; break;
                                        }
                                    } else {
                                        f = *slot + f;
                                    }
                                    *slot = f;
                                }
                                {
                                    float* slot = &unit->metal;
                                    float f = (float)self->metal * kHundred;
                                    if (unit->owner->active != 0 &&
                                        unit->owner->type == 2) {
                                        switch (g_game->difficulty) {
                                        case 0: f = *slot - f * kNegHalf; break;
                                        case 1: f = *slot - f * kNegSeven; break;
                                        default: f = *slot + f; break;
                                        }
                                    } else {
                                        f = *slot + f;
                                    }
                                    *slot = f;
                                }
                                g_seams.los_radar->RecalculateLineOfSight(1);
                                FocusCommander(1);
                            }
                        } else {
                            goto watch_check;
                        }
                    }
                }
            } else {
                goto check230;
            }
        }

    skip508:
        if (pi->active != 0) {
            unsigned char t = pi->type;
            if ((t == 1 || t == 2 || t == 3) && pi->index != 0xa) {
                if ((pi->unitCount != 0 || pi->unitsCreated == 0) &&
                    (t == 1 || t == 2)) {
                    if ((g_game->endGameFlags & 4) == 0 &&
                        g_game->endGameCountdown < 0) {
                        g_seams.economy->UpdatePlayerEconomy(pi);
                    }
                }
            }
        }

        if (bl == g_game->playerIndex) {
            g_seams.los_radar->UpdateSensorRadarAndCloak();
            g_seams.los_radar->UpdateRadarMapped();
            if (g_seams.map->GetGameType(g_game->net) == 3) {
                g_economyPacketTick++;
                if ((g_economyPacketTick & 3) == 0)
                    SendPlayerEconomy(pi, 0, 0);
            }
        }
        goto next_bl;

    watch_check:
        if (g_seams.map->GetGameType(g_game->net) == 3 &&
            pi->rejectReason == 0) {
            if ((g_game->players[FindHostSlot()].info->flags_9b & 0x80) != 0 ||
                CountActiveAIPlayers() > 0) {
                pi->info->bit6 = 1;
                if (bl == g_game->localPlayer) {
                    g_game->mapFlags &= 0xfffe;
                    g_game->mapFlags &= 0xfffd;
                    g_seams.los_radar->RecalculateLineOfSight(1);
                    BroadcastPlayerInfo();
                    if (CountActiveAIPlayers() == 0) {
                        Screen* dlg = LoadGuiLayer(&g_game->gui_519, "YESORNO.GUI", 0x900);
                        if (dlg != 0) {
                            SetKeyboardInput(&g_game->gui_519, 1);
                            Form* w = dlg->form;
                            SetTranslatedTextByName(&g_game->gui_519, "CHOICE1", "Yes", 0);
                            SetTranslatedTextByName(&g_game->gui_519, "CHOICE2", "No", 0);
                            SetTranslatedTextByName(&g_game->gui_519, "TITLE",
                                         "You're out!  Continue Watching?", 0);
                            strcpy(w->choice1, "CHOICE1");
                            strcpy(w->choice2, "CHOICE2");
                            dlg->callback = ContinueWatchingCallback;
                            RenderLayer((Gui *)&g_game->gui_519, 0x40);
                        }
                        goto skip508;
                    }
                    if (CountCombatPlayers() <= 0)
                        goto skip508;
                    OpenMessageBox((Gui *)&g_game->gui_519,
                                 (char *)Translate("You are placed in watch mode because you are hosting AI players which are still alive.  If you exit, they will be terminated."),
                                 500, 1, 1);
                    g_game->endGameFlags &= 0xffef;
                    goto skip508;
                }
                goto skip508;
            }
        }

    flags82e:
        g_game->endGameFlags |= 4;
        g_game->endGameFlags &= 0xffef;
        if (pi->rejectReason == 0)
            g_game->bits.flag6 = 1;
        goto skip508;

    check230:
        if (g_game->list_391ed->CheckVictory() != 0)
            goto countdown_extra;
        goto skip508;

    countdown_extra:
        if (g_game->endGameCountdown < 0) {
            g_game->endGameCountdown = 4;
        } else {
            g_game->endGameCountdown--;
            if (g_game->endGameCountdown < 0) {
                g_game->endGameFlags |= 4;
                g_game->bits.flag4 = 1;
                g_game->bits.flag5 = 1;
            }
        }
        goto skip508;

    next_bl:
        if (!more_00464f80(++bl))
            break;
    }

    if (g_seams.map->GetGameType(g_game->net) == 3 &&
        g_game->commanderDeath != 2 &&
        CountCombatPlayers() == 0) {
        if (g_game->endGameCountdown < 0) {
            g_game->endGameCountdown = 4;
            return;
        }
        g_game->endGameCountdown--;
        if (g_game->endGameCountdown < 0) {
            g_game->endGameFlags |= 4;
            g_game->endGameFlags &= 0xffef;
            g_game->bits.flag6 = 1;
        }
    }
}

static inline int IsVisible(Player* player, Vec3Words* pos)
{
    return (g_game->mapFlags & 2) == 2 ? IsExplored(player, pos) : g_seams.ai->IsPointVisible(player, (Vec3*)pos);
}

static inline int IsVisibleByMask(Player* player, Vec3Words* pos)
{
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(player, pos);
    return IsSeen(player, pos);
}

// FUNCTION: 0x4658e0
int __stdcall IsFootprintVisible(Player* player, int x, int y, int dx, int dy, short size)
{
    unsigned int vis;
    Vec3Words pos;
    memset(&pos, 0, sizeof(pos));
    pos.xWhole = (short)(x << 4);
    pos.yWhole = size;
    pos.zWhole = (short)(y << 4);
    if ((g_game->mapFlags & 2) == 2)
        vis = IsExplored(player, &pos);
    else
        vis = IsSeen(player, &pos);
    if ((int)vis)
        return 1;
    pos.xWhole = (short)(pos.xWhole + (dx << 4));
    pos.zWhole = (short)(pos.zWhole + (dy << 4));
    if ((g_game->mapFlags & 2) == 2)
        return IsExplored(player, &pos);
    return IsSeen(player, &pos);
}

// FUNCTION: 0x465ac0
int __stdcall IsUnitVisibleToPlayer(Player* player, Unit* u)
{
    if (u->player.get() == player)
        return 1;
    if (u->activateFlags & 4)
        return 0;
    union {
        Vec3 vec;
        Vec3Words words;
    } p;
    p.vec.x = u->def->modelMinX + u->pos.x;
    p.vec.y = u->def->modelMaxY + u->pos.y;
    p.vec.z = u->def->modelMinZ + u->pos.z;
    if (!(u->flags & 0x200)) {
        if (p.vec.y < (g_game->seaLevel << 16))
            return 0;
    }
    if (IsVisible(player, &p.words))
        return 1;
    p.vec.x += u->def->modelSizeX;
    if (IsVisible(player, &p.words))
        return 1;
    p.vec.z += u->def->modelSizeZ;
    p.vec.y -= u->def->modelSizeY;
    if (IsVisible(player, &p.words))
        return 1;
    p.vec.x -= u->def->modelSizeX;
    if (IsVisibleByMask(player, &p.words))
        return 1;
    return 0;
}

// FUNCTION: 0x465e30
void InitPlayerResources()
{
    for (int i = 0; i < 10; i++) {
        Player* player = &g_game->players[i];
        if (g_game->pendingSaveStore == 0) {
            switch (g_seams.map->GetGameType(g_game->net)) {
            case 1:
                SetStartingStorageBonus(player, (int)g_game->net->startMetal[i],
                             (int)g_game->net->startEnergy[i]);
                player->energy = g_game->net->startEnergy[i];
                player->metal = g_game->net->startMetal[i];
                break;
            case 2:
                player->energy = (float)((SkirmishPlayerSlot*)g_game->options)[i].energy;
                player->metal = (float)((SkirmishPlayerSlot*)g_game->options)[i].metal;
                break;
            case 3: {
                int index = FindHostSlot();
                if (index == 10)
                    index = i;
                Player* other = &g_game->players[index];
                player->energy = other->info->energy * 100;
                player->metal = other->info->metal * 100;
                break;
            }
            }
        }
    }
}

// FUNCTION: 0x465fb0
void __stdcall LoadPlayerControllers(HapiBank* file)
{
    char name[16];
    for (int i = 0; i < 10; i++) {
        Player* player = &g_game->players[i];
        int* slot = &((SkirmishPlayerSlot*)g_game->options)[i].controller;
        sprintf(name, "Player%i", i);
        if (file->OpenAccount(name)) {
            *slot = file->GetIntegerItem("Controller", 0);
            player->type = *slot;
        } else {
            *slot = 0;
            player->type = 0;
        }
    }
}

// FUNCTION: 0x466050
void __stdcall LoadPlayers(HapiBank* file)
{
    char name[16];
    file->OpenAccount("Players");
    g_game->localPlayer = file->GetIntegerItem("Human Player", 10);
    g_game->playerIndex = g_game->localPlayer;
    file->OpenNamedBox("GameTime");
    if (file->ReadBox((char*)&g_game->lastTick, 0x1c) != 0x1c)
        return;
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        sprintf(name, "Player%i", i);
        if (file->OpenAccount(name)) {
            p->energy = file->GetDoubleItem("Energy", 0);
            p->metal = file->GetDoubleItem("Metal", 0);
            p->totalEnergyProduced = file->GetDoubleItem("TotalEnergyProduced", 0);
            p->totalMetalProduced = file->GetDoubleItem("TotalMetalProduced", 0);
            p->totalEnergyConsumed = file->GetDoubleItem("TotalEnergyConsumed", 0);
            p->totalMetalConsumed = file->GetDoubleItem("TotalMetalConsumed", 0);
            p->energyWasted = file->GetDoubleItem("EnergyWasted", 0);
            p->metalWasted = file->GetDoubleItem("MetalWasted", 0);
            p->energyStorageBonus = file->GetDoubleItem("PlayerEnergyStorage", 0);
            p->metalStorageBonus = file->GetDoubleItem("PlayerMetalStorage", 0);
            p->bits_149.storage = file->GetIntegerItem("AddPlayerStorage", 0);
            p->kills = file->GetIntegerItem("Kills", 0);
            p->losses = file->GetIntegerItem("Losses", 0);
            p->updateTime = file->GetIntegerItem("UpdateTime", 0);
            p->winLoseTime = file->GetIntegerItem("WinLoseTime", 0);
            p->displayTimer = file->GetIntegerItem("DisplayTimer", 0);
            p->info->color = file->GetIntegerItem("Logo", 0);
            p->info->side = file->GetIntegerItem("Side", 0);
            file->OpenNamedBox("Alliances");
            if (file->GetBoxSize() == 11)
                file->ReadBox(p->allied, 11);
            p->allied[i] = 1;
        } else {
            p->type = 0;
        }
    }
}

// FUNCTION: 0x4662f0
void __stdcall SavePlayers(HapiBank* file)
{
    char name[16];
    file->OpenAccount("Players");
    file->SetIntegerItem("Human Player", g_game->localPlayer);
    file->OpenNamedBox("GameTime");
    file->WriteBox((char*)&g_game->lastTick, 0x1c);
    for (int i = 0; i < 10; i++) {
        Player* p = &g_game->players[i];
        if (p->type) {
            sprintf(name, "Player%i", i);
            file->OpenAccount(name);
            file->SetDoubleItem("Energy", p->energy);
            file->SetDoubleItem("Metal", p->metal);
            file->SetDoubleItem("TotalEnergyProduced", p->totalEnergyProduced);
            file->SetDoubleItem("TotalMetalProduced", p->totalMetalProduced);
            file->SetDoubleItem("TotalEnergyConsumed", p->totalEnergyConsumed);
            file->SetDoubleItem("TotalMetalConsumed", p->totalMetalConsumed);
            file->SetDoubleItem("EnergyWasted", p->energyWasted);
            file->SetDoubleItem("MetalWasted", p->metalWasted);
            file->SetDoubleItem("PlayerEnergyStorage", p->energyStorageBonus);
            file->SetDoubleItem("PlayerMetalStorage", p->metalStorageBonus);
            file->SetIntegerItem("AddPlayerStorage", p->bits_149.storage);
            file->SetIntegerItem("Kills", p->kills);
            file->SetIntegerItem("Losses", p->losses);
            file->SetIntegerItem("UpdateTime", p->updateTime);
            file->SetIntegerItem("WinLoseTime", p->winLoseTime);
            file->SetIntegerItem("DisplayTimer", p->displayTimer);
            file->SetIntegerItem("Controller", p->type);
            file->SetIntegerItem("Logo", p->info->color);
            file->SetIntegerItem("Side", p->info->side);
            file->OpenNamedBox("Alliances");
            file->WriteBox(p->allied, 11);
        }
    }
}

// FUNCTION: 0x466580
void UpdateBlink()
{
    if (g_game->blinkTimer > 0) {
        g_game->blinkTimer--;
        return;
    }
    g_game->blinkTimer = 7;
    g_game->bits_142f1.blinkOn = !g_game->bits_142f1.blinkOn;
}
