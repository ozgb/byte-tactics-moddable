#ifndef QUIRKS_QUIRKS_H
#define QUIRKS_QUIRKS_H

#include <stddef.h>

#include "../../interfaces/quirks.h"

enum QuirkIndex {
#define QUIRK(member, flagName, scope, openAnnihilationId) QUIRK_INDEX_##member,
#include "quirks.def"
#undef QUIRK
    QUIRK_COUNT
};

#define QUIRK(member, flagName, scope, openAnnihilationId) \
    static_assert(offsetof(Quirks, member) == QUIRK_INDEX_##member, "Quirks::" #member " is not at its place in quirks.def");
#include "quirks.def"
#undef QUIRK
static_assert(sizeof(Quirks) == QUIRK_COUNT, "Quirks and quirks.def differ in size");

bool QuirksParse(const char* spec, Quirks* out, char* err, int errLen);

bool QuirksNormalise(const char* spec, char* out, int outLen);

unsigned QuirksHash(const Quirks& q);

bool QuirksAllOff(const Quirks& q);

bool QuirksFlipFirstSim(Quirks* q);

#endif
