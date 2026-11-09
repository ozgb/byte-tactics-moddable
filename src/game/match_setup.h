#ifndef GAME_MATCH_SETUP_H
#define GAME_MATCH_SETUP_H

struct PlayerInfo;

struct MatchSetup {
    unsigned int blockBytes;
    unsigned int searchSteps;
};

const char* MatchSetupBlockBytesKey();
const char* MatchSetupSearchStepsKey();

bool MatchSetupBlockBytesValid(unsigned int blockBytes, const char** why);

bool MatchSetupSearchStepsValid(unsigned int searchSteps, const char** why);

void MatchSetupRequest(const MatchSetup& setup);

MatchSetup MatchSetupRequested();

MatchSetup MatchSetupOfInfo(const PlayerInfo* info);

void MatchSetupToInfo(PlayerInfo* info);

void MatchSetupBegin(const MatchSetup& setup);

void MatchSetupSizeBlock(unsigned int extraBytes);

MatchSetup MatchSetupCurrent();

#endif
