#ifndef SFX_OCCUPY_H
#define SFX_OCCUPY_H

struct Unit;

class SfxOccupyTracker {
public:
    void UpdateSfxOccupy(Unit* unit);
};

#endif
