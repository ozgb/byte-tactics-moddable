#ifndef SCRIPTS_SCRIPTS_SEAM_H
#define SCRIPTS_SCRIPTS_SEAM_H

struct ScriptsEventHeader;

int __stdcall ScriptsApply(const ScriptsEventHeader* event, unsigned int size);

#endif
