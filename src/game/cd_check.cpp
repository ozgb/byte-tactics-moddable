#include <stdio.h>
#include <string.h>
#include "win_types.h"
#include "../../interfaces/host.h"
#include "cd_check.h"

class TdfRecord;

#include "../util/tdf.h"
#include "../state/sim_globals.h"


struct ArchiveFindData {
    char unknown_0[0x14];
    char name[260];
};

#include "../../link/globals.h"

#include "../files/files_api.h"
#include "main.h"
#include "../debug/debug_lib.h"

// FUNCTION: 0x41d4c0
void RegisterDataArchives()
{
    ArchiveFindData fd;
    char pattern[64];
    if (DAT_0050289c != 0) {
        HAPI_DropMissingArchives();
        ChdirToExeDirectory();
        sprintf(pattern, "rev%s.GP3", "31");
        int h = HAPI_FindFirst(pattern, &fd, -1, 1);
        if (h >= 0) {
            do {
                HAPI_AddArchive(fd.name, 1);
            } while (HAPI_FindNext(h, &fd) == 0);
            HAPI_FindClose(h);
        }
        h = HAPI_FindFirst("*.CCX", &fd, -1, 1);
        if (h >= 0) {
            do {
                HAPI_AddArchive(fd.name, 1);
            } while (HAPI_FindNext(h, &fd) == 0);
            HAPI_FindClose(h);
        }
        h = HAPI_FindFirst("*.UFO", &fd, -1, 1);
        if (h >= 0) {
            do {
                HAPI_AddArchive(fd.name, 0);
            } while (HAPI_FindNext(h, &fd) == 0);
            HAPI_FindClose(h);
        }
        int hpi = HAPI_FindFirst("*.HPI", &fd, -1, 1);
        int left = 10;
        if (hpi >= 0) {
            do {
                if (HAPI_AddArchive(fd.name, 0))
                    left--;
            } while (HAPI_FindNext(hpi, &fd) == 0 && left != 0);
            HAPI_FindClose(hpi);
        }
        HAPI_ResolveShadowedFiles();
    }
}

// FUNCTION: 0x41d8a0
int UsesDataArchives(void)
{
    return DAT_0050289c;
}

// FUNCTION: 0x41d8c0
void* __stdcall AllocZeroedWithTickOffset(unsigned int size)
{
    unsigned int pad = host_time_ms() % 1000 * 7;
    unsigned int total = pad + size;
    char* p = (char*)operator new(total);
    memset(p, 0, total);
    AllocNotifyNop((int)p);
    return p + pad;
}
