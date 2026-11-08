#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "match_setup.h"
#include "../../interfaces/host.h"
#include "../network/player_info.h"
#include "../state/block.h"

namespace {

MatchSetup s_requested;
bool s_commandLineRead;
unsigned int s_blockBytes;
unsigned int s_matchBlockBytes = kSimBlockBytes;

void Fatal(const char* text)
{
    host_log(text);
    host_log("\n");
    host_fatal(nullptr, "Byte Tactics", text);
}

bool SwitchValue(const char* line, const char* name, unsigned int* value)
{
    size_t length = strlen(name);
    for (const char* p = line; *p != '\0'; p++) {
        if ((p[0] == '-' || p[0] == '/') && strncmp(p + 1, name, length) == 0 && (p == line || p[-1] == ' ')
            && (p[1 + length] == ' ' || p[1 + length] == '=')) {
            char* end;
            unsigned long number = strtoul(p + 2 + length, &end, 0);
            if (end == p + 2 + length)
                Fatal("a match setup switch needs a number");
            *value = (unsigned int)number;
            return true;
        }
    }
    return false;
}

void ReadCommandLine()
{
    if (s_commandLineRead)
        return;
    s_commandLineRead = true;
    const char* line = host_command_line();
    SwitchValue(line, "blockbytes", &s_requested.blockBytes);
    SwitchValue(line, "searchsteps", &s_requested.searchSteps);
    const char* why;
    if (!MatchSetupBlockBytesValid(s_requested.blockBytes, &why)) {
        char text[200];
        sprintf(text, "-blockbytes %u: %s", s_requested.blockBytes, why);
        Fatal(text);
    }
}

}

const char* MatchSetupBlockBytesKey()
{
    return "BlockBytes";
}

const char* MatchSetupSearchStepsKey()
{
    return "SearchSteps";
}

bool MatchSetupBlockBytesValid(unsigned int blockBytes, const char** why)
{
    if (blockBytes == 0)
        return true;
    if (blockBytes % kSimBlockPage != 0)
        *why = "must be a multiple of 65536";
    else if (blockBytes < kSimBlockBytes)
        *why = "must be at least 33554432";
    else if (blockBytes > kSimBlockCeilingBytes)
        *why = "must be at most 268435456";
    else
        return true;
    return false;
}

void MatchSetupRequest(const MatchSetup& setup)
{
    s_commandLineRead = true;
    s_requested = setup;
}

MatchSetup MatchSetupRequested()
{
    ReadCommandLine();
    return s_requested;
}

MatchSetup MatchSetupOfInfo(const PlayerInfo* info)
{
    unsigned short pages;
    MatchSetup setup;
    memcpy(&pages, info->matchSetup, sizeof pages);
    memcpy(&setup.searchSteps, info->matchSetup + sizeof pages, sizeof setup.searchSteps);
    setup.blockBytes = (unsigned int)pages * kSimBlockPage;
    return setup;
}

void MatchSetupToInfo(PlayerInfo* info)
{
    MatchSetup setup = MatchSetupRequested();
    unsigned short pages = (unsigned short)(setup.blockBytes / kSimBlockPage);
    memcpy(info->matchSetup, &pages, sizeof pages);
    memcpy(info->matchSetup + sizeof pages, &setup.searchSteps, sizeof setup.searchSteps);
}

void MatchSetupBegin(const MatchSetup& setup)
{
    const char* why;
    if (!MatchSetupBlockBytesValid(setup.blockBytes, &why)) {
        char text[200];
        sprintf(text, "match setup: block size %u %s", setup.blockBytes, why);
        Fatal(text);
    }
    s_blockBytes = setup.blockBytes;
    SimSetSearchSteps(setup.searchSteps);
}

void MatchSetupSizeBlock(unsigned int extraBytes)
{
    unsigned int needed = kSimBlockBytes + extraBytes;
    needed = (needed + kSimBlockPage - 1) & ~(kSimBlockPage - 1);
    if (needed < kSimBlockBytes || needed > kSimBlockCeilingBytes) {
        char text[200];
        sprintf(text, "match setup: the loaded modules need a block of %u bytes, over the ceiling of %u", needed, kSimBlockCeilingBytes);
        Fatal(text);
    }
    if (s_blockBytes != 0 && s_blockBytes < needed) {
        char text[200];
        sprintf(text, "match setup: the block of %u bytes is below the %u the loaded modules need", s_blockBytes, needed);
        Fatal(text);
    }
    s_matchBlockBytes = s_blockBytes != 0 ? s_blockBytes : needed;
    s_blockBytes = 0;
    SimBlockGrow(s_matchBlockBytes);
}

MatchSetup MatchSetupCurrent()
{
    MatchSetup setup;
    setup.blockBytes = s_matchBlockBytes;
    setup.searchSteps = SimSearchSteps();
    return setup;
}
