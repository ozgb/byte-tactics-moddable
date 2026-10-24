#include <malloc.h>
#include <string.h>

#include "archive_entry.h"

struct ArchiveDirectory {
    int count;
    ArchiveEntry* entries;
};

// FUNCTION: 0x4bb4e0
ArchiveEntry* __stdcall HAPI_FindEntry(ArchiveDirectory* list, char* path)
{
    int len;
    char* name;
    int last;
    char* sep;
    for (;;) {
        sep = strchr(path, '\\');
        if (sep) {
            len = sep - path;
            char* buf = (char*)_alloca(len + 1);
            strncpy(buf, path, len);
            buf[len] = 0;
            name = buf;
            last = 0;
        } else {
            name = path;
            last = 1;
        }
        int i = list->count - 1;
        for (;;) {
            if (i < 0)
                return 0;
            if (_strcmpi(name, list->entries[i].text) == 0)
                break;
            i--;
        }
        if (last)
            return &list->entries[i];
        if (!(list->entries[i].flags & 1))
            return 0;
        path = sep + 1;
        list = list->entries[i].dir;
    }
}
