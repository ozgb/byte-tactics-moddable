#include <malloc.h>
#include <string.h>

#include "archive_entry.h"

struct ArchiveDirectory {
    int count;
    ArchiveEntry* entries;
};

// FUNCTION: 0x4bc800
ArchiveDirectory* __stdcall HAPI_FindDirectory(ArchiveDirectory* list, char* path)
{
    char* sep;
    while ((sep = strchr(path, '\\')) != 0) {
        char* name;
        int len = sep - path;
        name = (char*)_alloca(len + 1);
        strncpy(name, path, len);
        name[len] = 0;
        int i = list->count - 1;
        for (;;) {
            if (i < 0)
                return 0;
            if (_strcmpi(name, list->entries[i].text) == 0)
                break;
            i--;
        }
        if (!(list->entries[i].flags & 1))
            return 0;
        path = sep + 1;
        list = list->entries[i].dir;
    }
    return list;
}
