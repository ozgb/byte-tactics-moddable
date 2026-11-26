#ifndef VIEW_FILL_H
#define VIEW_FILL_H

#include "view_block.h"

#define VIEW_REPORT_PROJECTILES 300
#define VIEW_REPORT_EXPLODED 100
#define VIEW_REPORT_PIECES 300
#define VIEW_REPORT_DEBRIS 300
#define VIEW_REPORT_EVENT_ARGS 12

struct ViewReportProjectile {
    int startX;
    int startY;
    int startZ;
    short bank;
    short heading;
    short pitch;
    short propellerSpin;
    short radius;
    short active;
    unsigned spawnTick;
    unsigned time;
};

struct ViewReportExploded {
    unsigned used;
    unsigned unit;
    int index;
    unsigned flags;
    short angle10;
    short angle12;
    short angle14;
    int x;
    int y;
    int z;
};

struct ViewReportDebris {
    unsigned used;
    unsigned obj;
    unsigned short frame;
    unsigned short shadowFrame;
    unsigned spriteSource;
    unsigned shadowSource;
    int x;
    int y;
    int z;
    short angleX;
    short angleY;
    short angleZ;
};

struct ViewReportPiece {
    unsigned used;
    int firstFace;
    int faceCount;
};

struct ViewWeaponsReport {
    ViewReportProjectile projectiles[VIEW_REPORT_PROJECTILES];
    unsigned projectileUsed[VIEW_REPORT_PROJECTILES];
    ViewReportExploded exploded[VIEW_REPORT_EXPLODED];
    ViewReportDebris debris[VIEW_REPORT_DEBRIS];
    ViewReportPiece pieces[VIEW_REPORT_PIECES];
};

struct ViewRawEvent {
    unsigned short kind;
    unsigned short flags;
    unsigned count;
    unsigned args[VIEW_REPORT_EVENT_ARGS];
};

const unsigned char* ViewHostGame(void);
const unsigned char* ViewHostRef(unsigned stored);
void ViewHostUnitAccount(const unsigned char* unit, float out[4]);
int ViewHostOrderDrawMask(unsigned kind);
const ViewWeaponsReport* ViewHostWeaponsReport(void);
unsigned ViewHostTakeEvents(const ViewRawEvent** events);
void ViewHostFail(const char* message);

int ViewIsMessageKind(unsigned kind);

void ViewFill(PlayerView* v);

#endif
