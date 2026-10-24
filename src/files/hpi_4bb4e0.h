#ifndef HPI_4BB4E0_H
#define HPI_4BB4E0_H

struct ArchiveDirectory;
struct ArchiveEntry;

ArchiveEntry* __stdcall HAPI_FindEntry(ArchiveDirectory* list, char* path);

#endif
