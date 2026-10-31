#include "win_types.h"
#include "../state/block.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>

#include "../map/mission.h"
#include "../../interfaces/seams.h"

class StringRef;

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../network/packet_manager.h"

#include "../files/files_api.h"
#include "../gui/dialogs.h"

#include "../graphics/object_pool.h"

#include "../game/main.h"
#include "../graphics/palette.h"

#include "../util/registry.h"

#include "map_enum.h"

#include "../game/data_files.h"
#include "../util/tdf.h"
void* __cdecl GameReallocTagged(void* p, const char* name, unsigned int size);
void __stdcall SetCursorMode(int n);
#include "../network/net_game.h"

#include "../graphics/display.h"

#include "../../interfaces/services.h"
#include "../state/sim_globals.h"

// FUNCTION: 0x434b90
void FreeOtaEnumCache()
{
    if (g_otaEnumFileList) {
        GameFreeThunk(g_otaEnumFileList);
        g_otaEnumFileList = 0;
    }
    g_otaEnumCacheComplete = 0;
    g_otaEnumFileListBytes = 0;
    g_otaEnumFileCount = 0;
}

// FUNCTION: 0x434bf0
int __stdcall LoadMapList(void** out, int param_2, int param_3)
{
    if (g_otaEnumFileList != 0) {
        if (out != 0) {
            if (param_3 != 0 && g_otaEnumCacheComplete == 0) {
                *out = g_otaEnumFileList;
                g_otaEnumFileList = 0;
            } else {
                char* p = (char*)GameAllocIgnoreTag("MULTI MAPS", g_otaEnumFileListBytes);
                *out = p;
                memcpy(p, g_otaEnumFileList, g_otaEnumFileListBytes);
            }
        }
        return g_otaEnumFileCount;
    }

    SetCursorMode(0x14);
    struct S { int count; int bFlag; int i; } s;
    
    if (param_2 == 0) {
        s.bFlag = 1;
        if (g_game->net->type != 3)
            s.bFlag = 0;
    } else {
        s.bFlag = 0;
    }
    int offset = 0;
    g_otaEnumFileListBytes = 1;
    g_otaEnumFileList = (char*)GameAllocIgnoreTag("MULTI MAPS", 1);
    *(char*)g_otaEnumFileList = 0;

    std::vector<StringRef> files;
    ListDirectory("Maps\\*.ota", 0, &files);
    g_otaEnumFileCount = 0;

    s.count = files.size();
    for (s.i = 0; s.i < s.count; s.i++) {
        struct A { char name[256]; char lower[256]; char path[256]; } a;
        BuildDataPath(a.path, "Maps", files[s.i].data, "OTA");
        TdfFile parser;
        if (parser.LoadFile(a.path) != 0
            && g_game->net->SelectSchema(3, &parser, 0) != 0) {
            strcpy(a.name, files[s.i].data);
            StripExtension(a.name);
            strcpy(a.lower, a.name);
            _strlwr(a.lower);
            char* src = Translate(a.lower);
            if (_strcmpi(src, a.lower) == 0)
                src = a.name;
            int len = strlen(src) + 1;
            g_otaEnumFileList = (char*)GameReallocTagged(g_otaEnumFileList, "MULTI MAPS",
                                               len + g_otaEnumFileListBytes);
            strcpy(g_otaEnumFileList + offset, src);
            g_otaEnumFileList[offset + len] = 0;
            offset += len;
            g_otaEnumFileListBytes += len;
            g_otaEnumFileCount++;
            if (param_2 != 0)
                break;
        }
        if (s.bFlag != 0) {
            HandleNetPackets();
            if (g_usePacketManager != 0)
                g_packetManager.SendAllQueued(0);
        }
    }
    SetCursorMode(0x13);
    if (g_otaEnumCacheComplete == 0)
        g_otaEnumCacheComplete = (param_2 == 0);
    int result = LoadMapList(out, param_2, param_3);
    return result;
}
