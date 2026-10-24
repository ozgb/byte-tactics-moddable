#include "archive_entry.h"
#include "../state/sim_globals.h"

struct ArchiveDirectory {
    int count;
    ArchiveEntry* entries;
};

struct Node_004be320 {
    char unknown_0[0x10];
    ArchiveDirectory* list;
};

struct Item_004be320 {
    char unknown_0[8];
    Node_004be320* node;
};

struct State_004be320 {
    char unknown_0[0x618];
    Item_004be320** items;
    int itemCount;
};


State_004be320* GetDisplay(void);
#include "hpi.h"

// FUNCTION: 0x4be320
void HAPI_ResolveShadowedFiles(void)
{
    State_004be320* state = GetDisplay();
    if (state->itemCount > 0) {
        for (int i = 0; i < state->itemCount; i++) {
            ArchiveDirectory* list = state->items[i]->node->list;
            for (int j = list->count - 1; j >= 0; j--) {
                list->entries[j].flags &= ~2;
                if (list->entries[j].flags & 1)
                    HAPI_ClearShadowFlags(list->entries[j].dir);
            }
        }
        HAPI_MarkShadowedFiles(DAT_005119b8, -1, 1);
    }
}
