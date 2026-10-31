#include "../../interfaces/host.h"

#include "../../link/globals.h"

#include "../files/files_api.h"
#include "../graphics/object_pool.h"
#include "../harness/harness.h"

// FUNCTION: 0x49f610
int IsWindowsSoundAvailable(void)
{
    return 1;
}

// FUNCTION: 0x49f620
void ResumeLoopingWav(void)
{
    if (g_loopingWav != 0) {
        host_sound_wav_play(g_loopingWav, HOST_WAV_KEEP);
    }
}

// FUNCTION: 0x49f640
void StopWindowsSound()
{
    host_sound_wav_stop();
    if (g_diskWav != 0) {
        GameFreeThunk(g_diskWav);
        g_diskWav = 0;
    }
    g_loopingWav = 0;
}

// FUNCTION: 0x49f680
void __stdcall PlayWavMemory(const char* sound)
{
    HarnessSoundEvent("wav", sound, nullptr, nullptr, nullptr);
    host_sound_wav_play(sound, 0);
}

// FUNCTION: 0x49f6a0
void __stdcall PlayLoopingWavMemory(char* sound)
{
    g_loopingWav = sound;
    HarnessSoundEvent("wav_loop", sound, nullptr, nullptr, nullptr);
    host_sound_wav_play(sound, HOST_WAV_LOOP);
}

// FUNCTION: 0x49f6c0
int __stdcall PlayWavFromDisk(char* path)
{
    int* sound = (int*)HAPI_LoadFile(path, 0);
    if (sound == 0)
        return 0;
    HarnessSoundEvent("wav_file", nullptr, path, nullptr, nullptr);
    int result = host_sound_wav_play(sound, 0);
    if (g_diskWav != 0)
        GameFreeThunk(g_diskWav);
    g_diskWav = sound;
    return result;
}
