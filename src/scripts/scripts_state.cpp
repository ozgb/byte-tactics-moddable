#include <stddef.h>
#include <stdio.h>

#pragma pack(push, 1)
#include "../units/unit.h"
#pragma pack(pop)

#include "../../interfaces/ref.h"
#include "../../interfaces/state_writer.h"
#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "cob_script.h"
#include "cob_types.h"
#include "scripts_seam.h"

#include "../../link/globals.h"

enum FieldKind { F_INT, F_BYTES, F_HEAP, F_UNIT };

struct Field {
    const char* name;
    unsigned short offset;
    unsigned char size;
    unsigned char kind;
    unsigned int mask;
};

static const Field kScriptState[] = {
    {"scale", 4, 4, F_INT, 0x0},
    {"table", 8, 4, F_HEAP, 0x0},
    {"checksum", 12, 4, F_INT, 0x0},
    {"changed", 24, 4, F_INT, 0x0},
    {"activeCount", 1340, 4, F_INT, 0x0},
};

static const Field kScriptChannel[] = {
    {"channels[%u].state", 0, 4, F_INT, 0x0},
    {"channels[%u].pc", 4, 4, F_INT, 0x0},
    {"channels[%u].sp", 8, 4, F_INT, 0x0},
    {"channels[%u].signal", 28, 4, F_INT, 0x0},
    {"channels[%u].callback", 32, 4, F_HEAP, 0x0},
};

static const Field kScriptChannelPiece[] = {
    {"channels[%u].piece", 16, 4, F_INT, 0x0},
    {"channels[%u].axis", 20, 4, F_INT, 0x0},
};

static const Field kScriptChannelDelay[] = {
    {"channels[%u].delay", 12, 4, F_INT, 0x0},
};

static const Field kScriptChannelWait[] = {
    {"channels[%u].waiting", 24, 4, F_INT, 0x0},
};

static const Field kScriptPiece[] = {
    {"pieces[%u].active", 0, 4, F_INT, 0x0},
    {"pieces[%u].move", 4, 12, F_BYTES, 0x0},
    {"pieces[%u].moveSpeed", 16, 12, F_BYTES, 0x0},
    {"pieces[%u].turn", 28, 12, F_BYTES, 0x0},
    {"pieces[%u].turnSpeed", 40, 12, F_BYTES, 0x0},
    {"pieces[%u].spin", 52, 12, F_BYTES, 0x0},
    {"pieces[%u].acceleration", 64, 12, F_BYTES, 0x0},
};

static unsigned int ReadWord(const unsigned char* p, unsigned int size)
{
    unsigned int v = 0;
    for (unsigned int b = size; b > 0; b--)
        v = (v << 8) | p[b - 1];
    return v;
}

static void WriteFields(const StateWriter* w, const unsigned char* base, const Field* fields, unsigned int count, int index)
{
    for (unsigned int k = 0; k < count; k++) {
        const Field* f = &fields[k];
        const unsigned char* p = base + f->offset;
        switch (f->kind) {
        case F_INT: {
            unsigned int v = ReadWord(p, f->size);
            w->Int(f->name, index, f->size, f->mask != 0 ? v & f->mask : v);
            break;
        }
        case F_BYTES:
            w->Bytes(f->name, index, p, f->size);
            break;
        case F_HEAP:
            w->Handle(f->name, index, STATE_HANDLE_HEAP, ReadWord(p, 4));
            break;
        case F_UNIT:
            w->Handle(f->name, index, STATE_HANDLE_UNIT, ReadWord(p, 4));
            break;
        }
    }
}

#define FIELDS(table) table, sizeof(table) / sizeof(table[0])

static void WriteChannel(const StateWriter* w, const Channel* channel, int index)
{
    const unsigned char* base = (const unsigned char*)channel;
    if (channel->state == 0) {
        w->Int("channels[%u].state", index, 4, 0);
        return;
    }
    WriteFields(w, base, FIELDS(kScriptChannel), index);
    unsigned int live = channel->sp < -1 ? 0 : (unsigned int)(channel->sp + 1);
    if (live > 32)
        live = 32;
    w->Bytes("channels[%u].stack", index, channel->stack, (int)(live * 4));
    if ((channel->state & 0xff000000) == 0x2000000) {
        switch (channel->state & 0xf00000) {
        case 0x100000:
        case 0x200000:
            WriteFields(w, base, FIELDS(kScriptChannelPiece), index);
            break;
        case 0x400000:
            WriteFields(w, base, FIELDS(kScriptChannelDelay), index);
            break;
        case 0x800000:
            WriteFields(w, base, FIELDS(kScriptChannelWait), index);
            break;
        }
    }
}

static void WriteScript(const StateWriter* w, CobScript* script)
{
    WriteFields(w, (const unsigned char*)script, FIELDS(kScriptState), 0);
    for (int c = 0; c < 8; c++)
        WriteChannel(w, &script->channels[c], c);
    ScriptTable* table = script->table;
    int* statics = script->statics.get();
    unsigned int staticCount = table == nullptr ? 0 : (unsigned int)table->staticCount;
    if (staticCount > 4096 || statics == nullptr)
        staticCount = 0;
    w->Int("statics.count", 0, 4, staticCount);
    w->Bytes("statics", 0, statics, (int)(staticCount * 4));
    const unsigned char* pieces = (const unsigned char*)script->pieces.get();
    unsigned int pieceCount = table == nullptr ? 0 : (unsigned int)table->pieceCount;
    if (pieceCount > 512 || pieces == nullptr)
        pieceCount = 0;
    for (unsigned int p = 0; p < pieceCount; p++)
        WriteFields(w, pieces + p * 0x4c, FIELDS(kScriptPiece), (int)p);
}

void __stdcall ScriptsWriteState(const StateWriter* w)
{
    Unit* units = g_game->units.get();
    char name[64];
    for (unsigned int i = 0; units != nullptr && i < g_game->poolCount; i++) {
        Unit* u = &units[i];
        if (u->unitDefIndex == 0 || u->script.offset == 0)
            continue;
        sprintf(name, "scriptsState.units[%u]", i);
        w->Begin(55, name);
        WriteScript(w, (CobScript*)u->script.get());
    }
}
