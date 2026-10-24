#include "options_45c820.h"

class Sound {
public:
    char unknown_0[4];
    int use3D;

    void Enable3D();
    void Disable3D();
    int SetWaveVolume(int level);
    int SetAuxVolume(int level, int flag);
};

#pragma pack(push, 1)
#include "../game/game.h"
#pragma pack(pop)

#include "../../link/globals.h"
extern int g_optionsBackupFxVolume;
extern unsigned char g_optionsBackupUnitChat;
extern unsigned int g_optionsBackupSoundFlags;

void __stdcall SetBrightness(float value);

// FUNCTION: 0x45c820
void RestoreSoundOptions()
{
    g_game->volume1 = g_optionsBackupFxVolume;
    g_game->soundFlags_37f19_2 = (g_game->soundFlags_37f19_2 & ~0x10) | (g_optionsBackupSoundFlags & 0x10);
    g_game->soundFlags_37f19_2 = (g_game->soundFlags_37f19_2 & ~0x20) | (g_optionsBackupSoundFlags & 0x20);
    g_game->soundFlags_37f19_2 = (g_game->soundFlags_37f19_2 & ~0x40) | ((g_optionsBackupSoundFlags & 0x20) << 1);
    if ((((unsigned char)g_optionsBackupSoundFlags) & 7) == 2)
        ((Sound *)g_game->sound)->Enable3D();
    else
        ((Sound *)g_game->sound)->Disable3D();
    g_game->soundFlags_37f19_2 = (g_game->soundFlags_37f19_2 & ~7) | (g_optionsBackupSoundFlags & 7);
    g_game->unitChat = g_optionsBackupUnitChat;
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
}
