#ifndef ARCHIVE_ENTRY_H
#define ARCHIVE_ENTRY_H

struct Info;
struct ArchiveDirectory;

#pragma pack(push, 1)
struct ArchiveEntry {
    union {
        int name;
        char* text;
    };
    union {
        int data;
        Info* info;
        ArchiveDirectory* dir;
    };
    unsigned char flags;
};
#pragma pack(pop)

#endif
