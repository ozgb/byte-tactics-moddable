#ifndef GAME_SCHEDULER_H
#define GAME_SCHEDULER_H

#include "../../interfaces/economy.h"
#include "../../interfaces/rand_stream.h"

int SchedEconomy(const EconomyEvent* event);
void SchedEconomyEffect(const EconomyEvent* effect);
void SchedUnitsPhase();
void SchedPlayersPhase(RandStream stream);

#endif
