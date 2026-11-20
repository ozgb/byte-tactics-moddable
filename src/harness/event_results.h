#ifndef HARNESS_EVENT_RESULTS_H
#define HARNESS_EVENT_RESULTS_H

enum { EVENT_RESULT_BYTES = 80, EVENT_RESULT_REFS = 4 };

struct EventResult {
    unsigned int size;
    int outcome;
    unsigned int tag[EVENT_RESULT_REFS];
    unsigned int id[EVENT_RESULT_REFS];
    unsigned char data[EVENT_RESULT_BYTES];
};

struct EventResults {
    EventResult* items;
    unsigned int count;
    unsigned int capacity;
};

enum EventResultsAdded { EVENT_RESULT_BAD_SIZE = -1, EVENT_RESULT_NO_MEMORY = 0, EVENT_RESULT_ADDED = 1 };

int EventResultsAdd(EventResults* results, const void* event, unsigned int size, int outcome);
void EventResultsClear(EventResults* results);

#endif
