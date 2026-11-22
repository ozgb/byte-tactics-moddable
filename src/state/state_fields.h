#ifndef STATE_STATE_FIELDS_H
#define STATE_STATE_FIELDS_H

#include "../../interfaces/state_writer.h"

enum StateFieldKind { F_INT, F_BYTES, F_HEAP, F_UNIT };

struct StateField {
    const char* name;
    unsigned short offset;
    unsigned char size;
    unsigned char kind;
    unsigned int mask;
};

inline unsigned int ReadStateWord(const unsigned char* p, unsigned int size)
{
    unsigned int v = 0;
    for (unsigned int b = size; b > 0; b--)
        v = (v << 8) | p[b - 1];
    return v;
}

inline void WriteStateFields(const StateWriter* w, const unsigned char* base, const StateField* fields, unsigned int count, int index)
{
    for (unsigned int k = 0; k < count; k++) {
        const StateField* f = &fields[k];
        const unsigned char* p = base + f->offset;
        switch (f->kind) {
        case F_INT: {
            unsigned int v = ReadStateWord(p, f->size);
            w->Int(f->name, index, f->size, f->mask != 0 ? v & f->mask : v);
            break;
        }
        case F_BYTES:
            w->Bytes(f->name, index, p, f->size);
            break;
        case F_HEAP:
            w->Handle(f->name, index, STATE_HANDLE_HEAP, ReadStateWord(p, 4));
            break;
        case F_UNIT:
            w->Handle(f->name, index, STATE_HANDLE_UNIT, ReadStateWord(p, 4));
            break;
        }
    }
}

#define STATE_FIELDS(table) table, sizeof(table) / sizeof(table[0])

#endif
