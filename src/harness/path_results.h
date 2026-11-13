#ifndef HARNESS_PATH_RESULTS_H
#define HARNESS_PATH_RESULTS_H

#include "../../interfaces/pathfinding.h"

enum { PATH_RESULT_SECTION = 32, PATH_RESULT_POINTS = 64 };

struct PathResult {
    int kind;
    int status;
    PathUnit unitRef;
    unsigned int unitTag;
    unsigned int unitId;
    unsigned int tick;
    int count;
    int delivered;
    PathPoint points[PATH_RESULT_POINTS];
};

struct PathResults {
    PathResult* items;
    unsigned int count;
    unsigned int capacity;
};

int PathResultsAdd(PathResults* results, const PathEvent* event, const PathPoint* points, int capacity);
void PathResultsClear(PathResults* results);

#endif
