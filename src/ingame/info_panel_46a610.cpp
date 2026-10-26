#include "../util/vec3.h"

#include "../util/angles.h"

#include "../graphics/handle.h"

#pragma pack(push, 1)
struct Feature {
    char name[0x94];
    Point16 footprint;
    char unknown_98[0xac - 0x98];
    unsigned short* animTable;
    unsigned short* shadowTable;
    char unknown_b4[0xcc - 0xb4];
    Handle anim;
    Handle shadowAnim;
    char unknown_e4[0xfe - 0xe4];
    unsigned short drawn : 1;
    unsigned short over : 1;
    unsigned short flipAnim : 1;
    unsigned short flipShadow : 1;
    unsigned short unknown_bits : 4;
};

struct Unit;

struct ObjectState {
    char unknown_0[0xc];
    Unit* unit;
};

struct Cell {
    char unknown_0[4];
    unsigned char shade;
    char unknown_5[0x8 - 0x5];
    unsigned short feature;
    unsigned short spot;
    unsigned char flags : 1;
    unsigned char unknown_d : 7;
};

#include "feature_spot.h"

#include "../units/unit.h"

#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

int __stdcall GetGafSequenceFrame(Handle* h);
int __stdcall GetGafFrame(unsigned short* table, int index);
void __stdcall DrawFrame(void* dest, short* frame, int x, int y);
void __stdcall DrawFrameBlended(void* dest, short* frame, int x, int y);
void __stdcall DrawUnit(void* dest, Unit* unit);

static int DrawFlags()
{
    return g_game->visualFlagsByte;
}

#define DrawFlip(flip, dest, frame, x, y) \
    do { \
        if (flip) \
            DrawFrameBlended(dest, frame, x, y); \
        else \
            DrawFrame(dest, frame, x, y); \
    } while (0)

// FUNCTION: 0x46a610
void __stdcall BlitFeatureGaf(void* dest, Cell* cell, int ix, int iy)
{
    Feature* f = &g_game->features[cell->feature];
    int x = f->footprint.x * 16 / 2 + (ix + 8) * 16 - g_game->scrollX;
    int s = cell->shade;
    s += cell[1].shade;
    s += cell[g_game->width].shade;
    s += cell[g_game->width + 1].shade;
    int shade = s >> 3;
    int y = f->footprint.y * 16 / 2 - shade + (iy + 2) * 16 - g_game->scrollY;
    if (cell->flags) {
        FeatureSpot* spot = &g_game->spots[cell->spot];
        if (f->drawn) {
            if ((spot->spotFlags & 4) && (DrawFlags() & 0x10)) {
                short* frame = (short*)GetGafSequenceFrame(&spot->shadow);
                DrawFrame(dest, frame, x, y);
            }
            {
                short* frame = (short*)GetGafSequenceFrame(&spot->anim);
                DrawFrame(dest, frame, x, y);
            }
        } else {
            Unit* unit = g_game->unit;
            ObjectState* st = (ObjectState*)spot->state.get();
            unit->state = (SpotState*)st;
            st->unit = unit;
            unit->angles = spot->rot;
            unit->pos = spot->pos;
            DrawUnit(dest, unit);
        }
    } else {
        if (f->over) {
            if (f->shadowTable && (DrawFlags() & 0x10)) {
                short* frame = (short*)GetGafSequenceFrame(&f->shadowAnim);
                DrawFlip(f->flipShadow, dest, frame, x, y);
            }
            if (f->animTable) {
                short* frame = (short*)GetGafSequenceFrame(&f->anim);
                DrawFlip(f->flipAnim, dest, frame, x, y);
            }
        } else {
            if (f->shadowTable && (DrawFlags() & 0x10)) {
                short* frame = (short*)GetGafFrame(f->shadowTable, 0);
                DrawFlip(f->flipShadow, dest, frame, x, y);
            }
            if (f->animTable) {
                short* frame = (short*)GetGafFrame(f->animTable, 0);
                DrawFlip(f->flipAnim, dest, frame, x, y);
            }
        }
    }
}
