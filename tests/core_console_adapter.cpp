#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>

#include "../src/state/block.h"
#include "../interfaces/seams.h"
#include "../src/util/engine_services.h"
#include "../src/state/sim_globals.h"
#include "../src/units/unit_def.h"
#include "../src/util/vec3.h"
#include "../src/util/tdf.h"
#pragma pack(push, 1)
#include "../src/map/cell.h"
#include "../src/map/spatial_bucket.h"
#include "../src/game/unit_resources.h"
#include "../src/network/player.h"
#include "../src/network/player_info.h"
#include "../src/units/unit.h"
#include "../src/game/game.h"
#pragma pack(pop)
#include "../src/util/commands.h"

Game* g_game;
Seams g_seams;
SimGlobals* g_simGlobals = 0;
void (*g_outOfMemoryHandler)() = 0;
int g_nonCampaignGame;
char g_commandLineBuf[80];
int g_defaultCommandHandler;
int g_defaultCommandMask;

extern ConsoleCommand g_consoleCommands[];
extern ConsoleCommand g_cheatCommands[];
extern ConsoleCommand g_debugCommands[];
void __stdcall ExecuteCommandLine(char* commandLine, int mask);

struct Event {
    std::vector<unsigned char> bytes;
};

static std::vector<Event> s_events;

void* GetAllocLock()
{
    static void* lock;
    if (lock == 0)
        lock = host_mutex_create();
    return lock;
}

int __stdcall HarnessTestMode()
{
    return 0;
}

extern "C" void HOST_CALL host_fatal(void*, const char* title, const char* text)
{
    fprintf(stderr, "host_fatal: %s: %s\n", title, text);
    exit(3);
}

void* __cdecl GameAllocIgnoreTag(const char*, unsigned int size)
{
    return malloc(size);
}

void __cdecl GameFreeThunk(void* p)
{
    free(p);
}

void __stdcall FatalError(const char* text)
{
    fprintf(stderr, "FatalError: %s\n", text);
    exit(3);
}

SimGlobals& __stdcall SimCreate()
{
    static char storage[sizeof(SimGlobals)];
    return *(SimGlobals*)storage;
}

void __stdcall SchedPublish(int outbox, const void* event, unsigned int size)
{
    if (outbox != SCHED_ENGINE)
        return;
    Event e;
    e.bytes.assign((const unsigned char*)event, (const unsigned char*)event + size);
    s_events.push_back(e);
}

static std::vector<char*> Fields(char* line)
{
    std::vector<char*> out;
    char* p = line;
    for (;;) {
        out.push_back(p);
        char* t = strchr(p, '\t');
        if (!t)
            break;
        *t = 0;
        p = t + 1;
    }
    return out;
}

static int HexDigit(char c)
{
    return c >= 'a' ? c - 'a' + 10 : c >= 'A' ? c - 'A' + 10 : c - '0';
}


int main(int argc, char** argv)
{
    if (argc != 4 || strcmp(argv[1], "console") != 0) {
        fprintf(stderr, "usage: core_console_adapter console CALLS OUT\n");
        return 2;
    }
    FILE* in = fopen(argv[2], "rb");
    FILE* out = fopen(argv[3], "wb");
    if (!in || !out)
        return 2;

    SimBlockInit();
    g_game = (Game*)SimAlloc(sizeof(Game));
    memset(g_game, 0, sizeof(Game));
    PlayerInfo* infos[10];
    for (int i = 0; i < 10; i++) {
        infos[i] = (PlayerInfo*)SimAlloc(sizeof(PlayerInfo));
        memset(infos[i], 0, sizeof(PlayerInfo));
        g_game->players[i].info = infos[i];
    }
    RegisterCommands(g_consoleCommands);
    RegisterCommands(g_cheatCommands);
    RegisterCommands(g_debugCommands);

    int gameType = 1;
    static char buf[1 << 16];
    while (fgets(buf, sizeof buf, in)) {
        size_t n = strlen(buf);
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
            buf[--n] = 0;
        if (n == 0)
            continue;
        std::vector<char*> f = Fields(buf);
        const char* name = f[0];
        if (strcmp(name, "match") == 0 && f.size() == 5) {
            gameType = atoi(f[1]);
            g_game->localPlayer = (unsigned char)atoi(f[2]);
            g_game->playerIndex = (unsigned char)atoi(f[3]);
            g_game->uiOptionFlags = (unsigned short)(atoi(f[4]) ? 2 : 0);
            g_nonCampaignGame = gameType == 2;
            fprintf(out, "match\tnull\n");
        } else if (strcmp(name, "slot") == 0 && f.size() == 6) {
            int s = atoi(f[1]);
            if (s < 0 || s > 9)
                return 2;
            g_game->players[s].active = atoi(f[2]);
            g_game->players[s].type = (unsigned char)atoi(f[3]);
            infos[s]->side = (unsigned char)atoi(f[4]);
            g_game->players[s].index = (unsigned char)atoi(f[5]);
            fprintf(out, "slot\tnull\n");
        } else if (strcmp(name, "line") == 0 && f.size() == 2) {
            static char text[1 << 12];
            size_t hex = strlen(f[1]);
            size_t len = hex / 2;
            if (len >= sizeof text)
                return 2;
            for (size_t i = 0; i < len; i++)
                text[i] = (char)(HexDigit(f[1][2 * i]) * 16 + HexDigit(f[1][2 * i + 1]));
            text[len] = 0;
            int mask = 1;
            if (mask & (unsigned char)(g_game->uiOptionFlags >> 1))
                mask = 7;
            if (g_nonCampaignGame)
                mask |= 2;
            s_events.clear();
            ExecuteCommandLine(text, mask);
            fprintf(out, "line\t{\"events\": [");
            for (size_t i = 0; i < s_events.size(); i++) {
                const std::vector<unsigned char>& b = s_events[i].bytes;
                unsigned int kind = 0;
                for (size_t k = 0; k < 4 && k < b.size(); k++)
                    kind |= (unsigned int)b[k] << (8 * k);
                fprintf(out, "%s[%u, \"", i ? ", " : "", kind);
                for (size_t k = 0; k < b.size(); k++)
                    fprintf(out, "%02x", b[k]);
                fprintf(out, "\"]");
            }
            fprintf(out, "], \"code\": %d}\n", (g_game->uiOptionFlags & 2) ? 1 : 0);
        } else {
            return 2;
        }
    }
    fclose(out);
    return 0;
}
