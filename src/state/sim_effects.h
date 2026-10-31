#ifndef STATE_SIM_EFFECTS_H
#define STATE_SIM_EFFECTS_H

#include "../units/unit.h"
#include "../orders/box.h"
#include "../util/vec3.h"
#include "sim_events.h"

inline void SimEventPut(unsigned int* args, unsigned int at, const Vec3& v)
{
    args[at] = (unsigned int)v.x;
    args[at + 1] = (unsigned int)v.y;
    args[at + 2] = (unsigned int)v.z;
}

inline void RecordSound(SimSoundName name, int flag)
{
    unsigned int args[2] = {name, (unsigned int)flag};
    RecordSimEvent(SIM_EVENT_SOUND, 0, args, 2);
}

inline void RecordNanoParticles(const Vec3& origin, const Vec3& low, const Vec3& high, short list)
{
    unsigned int args[10];
    SimEventPut(args, 0, origin);
    SimEventPut(args, 3, low);
    SimEventPut(args, 6, high);
    args[9] = (unsigned int)list;
    RecordSimEvent(SIM_EVENT_NANO_PARTICLES, 0, args, 10);
}

inline void RecordNanoParticles(const Vec3& origin, const Box& target, short list)
{
    RecordNanoParticles(origin, target.lo, target.hi, list);
}

inline void RecordNanoParticles(const Vec3& origin, const Vec3 (&target)[2], short list)
{
    RecordNanoParticles(origin, target[0], target[1], list);
}

inline void RecordReverseNanoParticles(const Vec3& low, const Vec3& high, const Vec3& origin, short list)
{
    unsigned int args[10];
    SimEventPut(args, 0, low);
    SimEventPut(args, 3, high);
    SimEventPut(args, 6, origin);
    args[9] = (unsigned int)list;
    RecordSimEvent(SIM_EVENT_REVERSE_NANO_PARTICLES, 0, args, 10);
}

inline void RecordReverseNanoParticles(const Box& target, const Vec3& origin, short list)
{
    RecordReverseNanoParticles(target.lo, target.hi, origin, list);
}

inline void RecordReverseNanoParticles(const Vec3 (&target)[2], const Vec3& origin, short list)
{
    RecordReverseNanoParticles(target[0], target[1], origin, list);
}

inline void RecordTeleportParticles(const Vec3& from, const Vec3& to, int lifetime, short list)
{
    unsigned int args[8];
    SimEventPut(args, 0, from);
    SimEventPut(args, 3, to);
    args[6] = (unsigned int)lifetime;
    args[7] = (unsigned int)list;
    RecordSimEvent(SIM_EVENT_TELEPORT_PARTICLES, 0, args, 8);
}

inline void RecordThrustParticles(const Vec3& from, const Vec3& to, int period, int lifetime, short list)
{
    unsigned int args[9];
    SimEventPut(args, 0, from);
    SimEventPut(args, 3, to);
    args[6] = (unsigned int)period;
    args[7] = (unsigned int)lifetime;
    args[8] = (unsigned int)list;
    RecordSimEvent(SIM_EVENT_THRUST_PARTICLES, 0, args, 9);
}

inline void RecordWakeEvent(SimEventKind kind, const Vec3& from, const Vec3& to, int period, short list)
{
    unsigned int args[8];
    SimEventPut(args, 0, from);
    SimEventPut(args, 3, to);
    args[6] = (unsigned int)period;
    args[7] = (unsigned int)list;
    RecordSimEvent(kind, 0, args, 8);
}

inline void RecordWakeParticles(const Vec3& from, const Vec3& to, int period, short list)
{
    RecordWakeEvent(SIM_EVENT_WAKE_PARTICLES, from, to, period, list);
}

inline void RecordBubbles(const Vec3& from, const Vec3& to, int period, short list)
{
    RecordWakeEvent(SIM_EVENT_BUBBLES, from, to, period, list);
}

inline void RecordPointParticles(SimEventKind kind, const Vec3& at, short list)
{
    unsigned int args[4];
    SimEventPut(args, 0, at);
    args[3] = (unsigned int)list;
    RecordSimEvent(kind, 0, args, 4);
}

inline void RecordWeaponSmoke(const Vec3& at, short list)
{
    RecordPointParticles(SIM_EVENT_WEAPON_SMOKE, at, list);
}

inline void RecordTimedSubParticles(const Vec3& at, short list)
{
    RecordPointParticles(SIM_EVENT_TIMED_SUB_PARTICLES, at, list);
}

inline void RecordScreenShake(int dx, int dy, int value)
{
    unsigned int args[3] = {(unsigned int)dx, (unsigned int)dy, (unsigned int)value};
    RecordSimEvent(SIM_EVENT_SCREEN_SHAKE, 0, args, 3);
}

inline void RecordCdActivity(int amount, int playerA, int playerB)
{
    unsigned int args[3] = {(unsigned int)amount, (unsigned int)playerA, (unsigned int)playerB};
    RecordSimEvent(SIM_EVENT_CD_ACTIVITY, 0, args, 3);
}

inline void RecordForcesDestroyed(int player)
{
    unsigned int args[1] = {(unsigned int)player};
    RecordSimEvent(SIM_EVENT_FORCES_DESTROYED, 0, args, 1);
}

inline void RecordPlayerTookLead(int player, int kills)
{
    unsigned int args[2] = {(unsigned int)player, (unsigned int)kills};
    RecordSimEvent(SIM_EVENT_PLAYER_TOOK_LEAD, 0, args, 2);
}

inline void RecordScoreFlash(int killer, int loser)
{
    unsigned int args[2] = {(unsigned int)killer, (unsigned int)loser};
    RecordSimEvent(SIM_EVENT_SCORE_FLASH, 0, args, 2);
}

inline void RecordUnitEvent(SimEventKind kind, const Unit* unit)
{
    unsigned int args[1] = {unit->id};
    RecordSimEvent(kind, 0, args, 1);
}

inline void RecordSelectionOrdersDirty(const Unit* unit)
{
    RecordUnitEvent(SIM_EVENT_SELECTION_ORDERS_DIRTY, unit);
}

inline void RecordBuildMenuUpdate(const Unit* unit)
{
    RecordUnitEvent(SIM_EVENT_BUILD_MENU_UPDATE, unit);
}

inline void RecordDeselectIfIneligible(const Unit* unit)
{
    RecordUnitEvent(SIM_EVENT_DESELECT_IF_INELIGIBLE, unit);
}

inline void RecordGuiChangedIfScreen(SimScreenName screen)
{
    unsigned int args[1] = {screen};
    RecordSimEvent(SIM_EVENT_GUI_CHANGED_IF_SCREEN, 0, args, 1);
}

inline void RecordFocusCommander(int force)
{
    unsigned int args[1] = {(unsigned int)force};
    RecordSimEvent(SIM_EVENT_FOCUS_COMMANDER, 0, args, 1);
}

inline void RecordPopToGameLayout(int force)
{
    unsigned int args[1] = {(unsigned int)force};
    RecordSimEvent(SIM_EVENT_POP_TO_GAME_LAYOUT, 0, args, 1);
}

#endif
