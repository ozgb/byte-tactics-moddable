#ifndef VIEW_FILL_H
#define VIEW_FILL_H

#include "view_block.h"

#define VIEW_REPORT_PROJECTILES 300
#define VIEW_REPORT_EXPLODED 100
#define VIEW_REPORT_PIECES 300
#define VIEW_REPORT_DEBRIS 300
#define VIEW_REPORT_EVENT_ARGS 12

// What weapons' report (record 48) holds that the fill needs, by the table slot it names. A ref is as the game record
// stores it; ViewHostRef resolves it.
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

// One sim event as the event log sees it: its kind, flags and every argument.
struct ViewRawEvent {
    unsigned short kind;
    unsigned short flags;
    unsigned count;
    unsigned args[VIEW_REPORT_EVENT_ARGS];
};

// The game record, which holds the state of the match.
const unsigned char* ViewHostGame(void);
// The address a ref of the game record stands for; 0 for a null ref.
const unsigned char* ViewHostRef(unsigned stored);
// A unit's account as the unit panel shows it: metal made, metal used, energy made, energy used in the last settlement.
void ViewHostUnitAccount(const unsigned char* unit, float out[4]);
// The draw mask of the order kind in the table of order kinds; bit 1 is set for the kinds that draw a build footprint.
int ViewHostOrderDrawMask(unsigned kind);
// Weapons' report of this tick; valid until the next call.
const ViewWeaponsReport* ViewHostWeaponsReport(void);
// The sim events captured since the last call, in the order raised; the host forgets them.
unsigned ViewHostTakeEvents(const ViewRawEvent** events);
// Ends the run with the message.
void ViewHostFail(const char* message);

// Whether a sim event of the kind can reach a viewer as a message.
int ViewIsMessageKind(unsigned kind);

// Fills the block with the view of the player game.playerIndex, whose controller is game.localPlayer.
void ViewFill(PlayerView* v);

#endif
