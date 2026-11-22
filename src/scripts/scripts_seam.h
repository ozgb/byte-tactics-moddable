#ifndef SCRIPTS_SCRIPTS_SEAM_H
#define SCRIPTS_SCRIPTS_SEAM_H

struct ScriptsEventHeader;
struct StateWriter;

int __stdcall ScriptsApply(const ScriptsEventHeader* event, unsigned int size);
void __stdcall ScriptsWriteState(const StateWriter* w);

#endif
