#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../map/mission.h"
#include "../../interfaces/seams.h"

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)
#include "../../link/globals.h"
#include "ai_profile.h"

extern int __cdecl _strcmpi(const char*, const char*);

#include "../debug/debug_lib.h"

class AIProfileParser {
public:
    char token[0x80];
    char* bufferStart;
    char* bufferEnd;
    char* cursor;
    int errorReported;
    int tokenPending;
    int tokenType;

    AIProfileParser();
    void SetBuffer(char* buffer, int size);
    int NextToken();
    int ReadInt();
    bool TokenEquals(char* text);
    int ScanToken();
    void ReportParseError(char* text);
};

// FUNCTION: 0x428c60
AIProfileParser::AIProfileParser()
{
    bufferStart = 0;
    bufferEnd = 0;
    cursor = 0;
    errorReported = 0;
    tokenPending = 0;
    tokenType = 0x102;
    token[0] = 0;
}

// FUNCTION: 0x428c90
void AIProfileParser::SetBuffer(char* buffer, int size)
{
    bufferStart = buffer;
    bufferEnd = buffer + size;
    cursor = buffer;
    errorReported = 0;
    tokenPending = 0;
    tokenType = 0x102;
    token[0] = 0;
}

// FUNCTION: 0x428cd0
int AIProfileParser::NextToken()
{
    if (errorReported != 0)
        return 0x102;
    if (tokenPending != 0) {
        tokenPending = 0;
        return tokenType;
    }
    tokenType = ScanToken();
    return tokenType;
}

// FUNCTION: 0x428d10
int AIProfileParser::ScanToken()
{
    if (errorReported != 0)
        return 0x102;

    while (isspace(*cursor) && cursor != bufferEnd)
        cursor++;

    if (cursor == bufferEnd || *cursor == '\0')
        return 0;

    if (ispunct(*cursor))
        return *cursor++;

    int len = 0;
    if (!isdigit(*cursor) && *cursor != '-' && *cursor != '.') {
        while (isalnum(*cursor) && cursor != bufferEnd && len < 0x7f) {
            token[len] = *cursor;
            len++;
            cursor++;
        }
        token[len] = 0;
        if (len != 0 && len != 0x7f)
            return 0x100;
        return 0x102;
    }

    token[0] = *cursor;
    int i = 1;
    for (;;) {
        char* q = cursor;
        cursor = q + 1;
        if (!(isdigit(*cursor) || *cursor == '.'))
            break;
        token[i++] = *cursor;
    }
    token[i] = 0;
    return 0x101;
}

// FUNCTION: 0x428e90
int AIProfileParser::ReadInt()
{
    char buffer[256];
    int token;
    if (errorReported != 0) {
        token = 0x102;
    } else if (tokenPending != 0) {
        token = tokenType;
        tokenPending = 0;
    } else {
        token = ScanToken();
        tokenType = token;
    }
    if (token != 0x101) {
        if (errorReported == 0) {
            sprintf(buffer, "parse error reading AI profile %s\n%s\nlast string =",
                    (char*)g_seams.map->GetNameSlot(g_game->net, 7),
                    "expecting int", this);
        }
        errorReported = 1;
        return 0;
    }
    return atoi((char*)this);
}

// FUNCTION: 0x428f40
bool AIProfileParser::TokenEquals(char* text)
{
    return _strcmpi(text, (const char*)this) == 0;
}

// FUNCTION: 0x428f60
void AIProfileParser::ReportParseError(char* text)
{
    char buffer[256];
    if (!errorReported) {
        sprintf(buffer, "parse error reading AI profile %s\n%s\nlast string =",
                (char*)g_seams.map->GetNameSlot(g_game->net, 7), text, this);
    }
    errorReported = 1;
}

// FUNCTION: 0x428fc0
void ProtectUnitDefsReadOnly()
{
    ProtectBlockReadOnly((void *)g_game->defs);
}

// FUNCTION: 0x428fe0
void ProtectUnitDefsReadWrite()
{
    ProtectBlockReadWrite((void *)g_game->defs);
}
