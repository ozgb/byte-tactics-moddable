#ifndef HARNESS_ECONOMY_RESULTS_H
#define HARNESS_ECONOMY_RESULTS_H

#include "../../interfaces/economy.h"

enum { ECONOMY_RESULT_SECTION = 33, ECONOMY_RESULT_ACCOUNT_WORDS = 12 };

struct EconomyResult {
    int kind;
    int player;
    EconUnit unitRef;
    unsigned int unitTag;
    unsigned int unitId;
    unsigned int resources;
    float energy;
    float metal;
    int value;
    int outcome;
    unsigned int account[ECONOMY_RESULT_ACCOUNT_WORDS];
};

struct EconomyResults {
    EconomyResult* items;
    unsigned int count;
    unsigned int capacity;
};

int EconomyResultsAdd(EconomyResults* results, const EconomyEvent* event, int outcome);
void EconomyResultsClear(EconomyResults* results);

#endif
