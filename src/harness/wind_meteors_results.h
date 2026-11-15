#ifndef HARNESS_WIND_METEORS_RESULTS_H
#define HARNESS_WIND_METEORS_RESULTS_H

#include "../../interfaces/wind_meteors.h"

enum { WIND_METEORS_RESULT_SECTION = 37, WIND_METEORS_RESULT_WORDS = 13 };

struct WindMeteorsResult {
    int kind;
    unsigned int data[WIND_METEORS_RESULT_WORDS - 1];
    unsigned int size;
};

struct WindMeteorsResults {
    WindMeteorsResult* items;
    unsigned int count;
    unsigned int capacity;
};

enum WindMeteorsResultsAdded { WIND_METEORS_RESULT_TOO_LARGE = -1, WIND_METEORS_RESULT_NO_MEMORY = 0, WIND_METEORS_RESULT_ADDED = 1 };

int WindMeteorsResultsAdd(WindMeteorsResults* results, const void* record, unsigned int size);
void WindMeteorsResultsClear(WindMeteorsResults* results);

#endif
