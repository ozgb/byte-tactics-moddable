#include "win_types.h"
#include <stdlib.h>
#include <string.h>
#include "../../interfaces/host.h"
#include <stdio.h>
#include <float.h>
#include <io.h>


extern HWND g_cdPlayerWindow;


extern int g_cdFadeVolume;
extern int g_cdCategorySavedTrack[];
extern int g_cdFadeTimer;
extern int g_cdNextTrackTimer;

#include "../graphics/display.h"
extern int __stdcall AddTimer(int delay, int id, void (__stdcall* callback)(void*));

struct App_004b6220 {
    char unknown_0[0x40];
    HWND hwnd;
};

extern App_004b6220* GetDisplay();

struct FileHandle;

void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);

void __cdecl GameFreeThunk(void* p);

extern int g_playBufferLooping;

struct Pos_004cf570 {
    int x, y, z;
};

#include "../files/files_api.h"

#include "samples.h"
int __stdcall AddTimer(int delay, int param, void (__stdcall* callback)(int));

extern int g_delayedSampleVolume;
extern char g_delayedSampleName[];

#include "sound.h"

static inline int ClampVolume(int v)
{
    if (v < 0) {
        v = 0;
    }
    if (v > 0xffff) {
        v = 0xffff;
    }
    return v;
}

int UpdatePlacementGhostValidity();
void RegisterUnitOrders();

struct WaveFormat {
    unsigned short wFormatTag;
    unsigned short nChannels;
    int nSamplesPerSec;
    int nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short wBitsPerSample;
};


static inline unsigned int FindChunk(FileHandle* file, const char* tag) {
    unsigned int limit, id, offset, size;
    HAPI_SeekFile(file, 4);
    HAPI_readfromfile(file, &limit, 4);
    limit += 8;
    HAPI_SeekFile(file, 12);
    HAPI_readfromfile(file, &id, 4);
    HAPI_readfromfile(file, &size, 4);
    offset = 20;
    for (;;) {
        if (strncmp((char*)&id, tag, 4) == 0)
            return size;
        HAPI_SeekFile(file, size + offset);
        offset += size;
        if (offset >= limit)
            return 0;
        HAPI_readfromfile(file, &id, 4);
        HAPI_readfromfile(file, &size, 4);
        offset += 8;
    }
}

extern Sound* g_cdPlayer;

extern HANDLE g_squashThreadLockEvent;
extern int g_squashThreadLockTicket;

struct Entry_004d0a10 {
    short a;
    short b;
    short c;
};

struct Struct_00526ff0 {
    Entry_004d0a10 entries[0x1000];
    short field_6000;
    short field_6002;
    short field_6004;
};

extern Struct_00526ff0* g_squashDictTree;

extern void* g_squashWindow;

// FUNCTION: 0x4cee50
Sound::Sound()
{
    open = 0;
    use3D = 0;
    noDriver = 0;
    minDistance = 1.0f;
    maxDistance = 1e20f;
    directSound = 0;
    primary = 0;
    for (int j = 0; j < 8; j++) {
        sets[j] = 0;
    }
    InitMixerVolumes();
    maxBuffers = 8;
    count = 0;
    sequence = 1;
    for (int i = 0; i < 32; i++) {
        buffers[i] = 0;
        priority[i] = 0;
        flags[i] = 0;
    }
    stream = 0;
    cdVolume = QueryAuxVolume();
    streamTimer = -1;
}

// FUNCTION: 0x4ceee0
void Sound::ReleaseDirectSound()
{
    int i;
    for (i = 0; i < 8; i++) {
        if (sets[i] != 0) {
            ReleaseSampleSet(sets[i]);
            sets[i] = 0;
        }
    }
    if (streamTimer != -1) {
        RemoveTimer(streamTimer);
        streamTimer = -1;
    }
    if (stream != 0) {
        host_sound_buffer_stop(stream);
        host_sound_buffer_free(stream);
        stream = 0;
        HAPI_CloseFile(streamFile);
    }
    CloseCdAudio();
    host_sound_close();
    primary = 0;
    directSound = 0;
}

// FUNCTION: 0x4cef90
int Sound::InitDirectSound(int rate, int bits, int channels, HWND handle)
{
    int err;
    directSound = host_sound_open(handle, rate, bits, channels, &err);
    if (directSound == 0) {
        if (err == HOST_SOUND_NO_DRIVER)
            noDriver = 1;
        ReleaseDirectSound();
        return 0;
    }
    sampleChannels = channels;
    sampleRate = rate;
    sampleBits = bits;
    return 1;
}

// FUNCTION: 0x4cf0b0
void Sound::ReapFinishedBuffers()
{
    int i;
    for (i = 0; i < 8; i++) {
        if (sets[i] != 0) {
            if (host_sound_buffer_status(sets[i][0]) <= 0) {
                ReleaseSampleSet(sets[i]);
                sets[i] = 0;
            }
        }
    }
    for (i = 0; i < 0x20; i++) {
        if (buffers[i] != 0) {
            if (host_sound_buffer_status(buffers[i]) <= 0) {
                buffers[i] = 0;
                count--;
            }
        }
    }
    if (stream != 0)
        UpdateStream();
}

// FUNCTION: 0x4cf150
void Sound::StopAllBuffers()
{
    void** slot = buffers;
    for (int i = 0x20; i != 0; i--) {
        void* obj = *slot;
        if (obj != 0) {
            host_sound_buffer_stop(obj);
            *slot = 0;
            count--;
        }
        slot++;
    }
}

// FUNCTION: 0x4cf180
void Sound::StopOldestBuffer()
{
    int i;
    for (i = 0; i < 0x20; i++) {
        if (buffers[i] != 0 && (flags[i] & 1) == 0)
            break;
    }
    for (int j = i + 1; j < 0x20; j++) {
        if (buffers[j] != 0 && (flags[j] & 1) == 0 && priority[j] < priority[i])
            i = j;
    }
    host_sound_buffer_stop(buffers[i]);
    buffers[i] = 0;
    count--;
}

// FUNCTION: 0x4cf210
void Sound::SetMaxBuffers(int val)
{
    maxBuffers = val;
}

// FUNCTION: 0x4cf220
int Sound::GetMaxBuffers()
{
    return maxBuffers;
}

// FUNCTION: 0x4cf230
void** Sound::CreateSampleFromMemory(void* src, unsigned long bytes,
                                                  int sampleRate, int bits, int channels)
{
    void* buf = 0;

    buf = host_sound_buffer_create(bytes, sampleRate, bits, channels, HOST_SOUND_3D);
    if (buf == 0)
        goto error;

    {
        void* ptr;
        unsigned int size;
        if (host_sound_buffer_lock(buf, 0, bytes, &ptr, &size) == 0)
            goto error;
        memcpy(ptr, src, size);
        if (host_sound_buffer_unlock(buf, ptr, size) == 0)
            goto error;
    }

    {
        void** set = (void**)GameAllocIgnoreTag("Digital Audio Sample", 0x10);
        for (int i = 0; i < 4; i++)
            set[i] = 0;
        set[0] = buf;
        return set;
    }

error:
    if (buf != 0)
        host_sound_buffer_free(buf);
    return 0;
}

// FUNCTION: 0x4cf370
void** Sound::CreateSampleFromFile(FileHandle* file, unsigned long bytes,
                                                  int sampleRate, int bits, int channels)
{
    void* buf = 0;

    unsigned int oldcw = _controlfp(0, 0);
    _controlfp(0x18, 0x18);
    buf = host_sound_buffer_create(bytes, sampleRate, bits, channels, HOST_SOUND_3D);
    _controlfp(oldcw, 0x18);
    if (buf == 0)
        goto error;

    {
        void* ptr;
        unsigned int size;
        if (host_sound_buffer_lock(buf, 0, bytes, &ptr, &size) == 0)
            goto error;
        unsigned copied = (unsigned)HAPI_readfromfile(file, ptr, size);
        if (host_sound_buffer_unlock(buf, ptr, size) == 0)
            goto error;
        if (copied < size)
            goto error;
    }

    {
        void** set = (void**)GameAllocIgnoreTag("Digital Audio Sample", 0x10);
        for (int i = 0; i < 4; i++)
            set[i] = 0;
        set[0] = buf;
        return set;
    }

error:
    if (buf != 0)
        host_sound_buffer_free(buf);
    return 0;
}

// FUNCTION: 0x4cf4d0
void Sound::ReleaseSampleSet(void** set)
{
    if (set == 0)
        return;
    for (int i = 0; i < 4; i++) {
        if (set[i] != 0) {
            host_sound_buffer_stop(set[i]);
            host_sound_buffer_free(set[i]);
            for (int j = 0; j < 0x20; j++) {
                if (buffers[j] == set[i]) {
                    buffers[j] = 0;
                    flags[j] = 0;
                    count--;
                    break;
                }
            }
        }
    }
    GameFreeThunk(set);
}

// FUNCTION: 0x4cf540
void Sound::PlayLooping(void** set, LONG volume)
{
    g_playBufferLooping = 1;
    PlaySampleSet(set, volume, 0);
    g_playBufferLooping = 0;
}

// FUNCTION: 0x4cf570
int Sound::PlaySampleSet(void** set, LONG volume, Pos_004cf570* pos)
{
    void* unit = 0;
    int slot = 0;
    if (g_playBufferLooping != 0) {
        for (int i = 0; i < 0x20; i++) {
            if (buffers[i] != 0 && flags[i] == 1)
                return 0;
        }
    }
    while (count >= maxBuffers)
        StopOldestBuffer();
    unsigned int best = 0;
    if (set == 0)
        return 0;
    int bestidx = 0;
    for (int i = 0; i < 4; i++) {
        if (set[i] != 0) {
            int status = host_sound_buffer_status(set[i]);
            if (status < 0)
                return 0;
            if (status == 0) {
                unit = set[i];
                break;
            }
            unsigned int play, write;
            host_sound_buffer_position(set[i], &play, &write);
            if (play > best) {
                best = play;
                bestidx = i;
            }
        } else {
            slot = i;
        }
    }
    if (unit == 0) {
        if (slot > 0) {
            unit = host_sound_buffer_duplicate(set[0]);
            if (unit == 0)
                return 0;
            set[slot] = unit;
        } else {
            unit = set[bestidx];
            host_sound_buffer_set_position(unit, 0);
        }
    }
    if (use3D == 0 || pos == 0) {
        host_sound_buffer_place(unit, 0, 0, 0);
    } else {
        float xyz[3] = {(float)pos->x, (float)pos->y, (float)pos->z};
        host_sound_buffer_place(unit, xyz, minDistance, maxDistance);
    }
    if (host_sound_buffer_set_position(unit, 0) == 0)
        return 0;
    if (host_sound_buffer_set_volume(unit, volume) == 0)
        return 0;
    if (host_sound_buffer_play(unit, g_playBufferLooping != 0) == 0)
        return 0;
    for (int j = 0; j < 0x20; j++) {
        if (buffers[j] == 0) {
            buffers[j] = unit;
            priority[j] = ++sequence;
            flags[j] = g_playBufferLooping != 0;
            count++;
            break;
        }
    }
    return 1;
}

// FUNCTION: 0x4cf800
int Sound::PlayMemorySample(void* src, unsigned long bytes, int sampleRate, int bits, int channels, LONG volume, Pos_004cf570* pos)
{
    ReapFinishedBuffers();
    for (int i = 0; i < 8; i++) {
        if (sets[i] == 0) {
            sets[i] = CreateSampleFromMemory(src, bytes, sampleRate, bits, channels);
            if (sets[i] == 0)
                return 0;
            if (PlaySampleSet(sets[i], volume, pos) == 0) {
                ReleaseSampleSet(sets[i]);
                sets[i] = 0;
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x4cf8a0
int Sound::PlayFileSample(FileHandle* file, unsigned long bytes, int sampleRate, int bits, int channels, LONG volume, Pos_004cf570* pos)
{
    ReapFinishedBuffers();
    for (int i = 0; i < 8; i++) {
        if (sets[i] == 0) {
            sets[i] = CreateSampleFromFile(file, bytes, sampleRate, bits, channels);
            if (sets[i] == 0)
                return 0;
            if (PlaySampleSet(sets[i], volume, pos) == 0) {
                ReleaseSampleSet(sets[i]);
                sets[i] = 0;
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x4cf940
void Sound::StartStream(FileHandle* file, int sampleRate, int bits,
                                  int channels, LONG volume)
{
    int n;

    if (streamTimer != -1) {
        RemoveTimer(streamTimer);
        streamTimer = -1;
    }
    if (stream != 0) {
        host_sound_buffer_stop(stream);
        host_sound_buffer_free(stream);
        stream = 0;
        HAPI_CloseFile(streamFile);
    }
    if (streamTimer != -1) {
        RemoveTimer(streamTimer);
        streamTimer = -1;
    }

    n = channels * sampleRate * (bits / 8) * 2;
    streamSize = n / 2;
    stream = host_sound_buffer_create(n, sampleRate, bits, channels, 0);
    if (stream == 0)
        return;

    streamBits = bits;
    streamPos = 0;
    streamOffset = -1;
    streamFile = file;
    FillStreamHalf();
    if (host_sound_buffer_set_volume(stream, volume) == 0) {
        if (streamTimer != -1) {
            RemoveTimer(streamTimer);
            streamTimer = -1;
        }
        if (stream != 0) {
            host_sound_buffer_stop(stream);
            host_sound_buffer_free(stream);
            stream = 0;
            HAPI_CloseFile(streamFile);
        }
        return;
    }
    if (host_sound_buffer_play(stream, 1) == 0) {
        if (streamTimer != -1) {
            RemoveTimer(streamTimer);
            streamTimer = -1;
        }
        if (stream != 0) {
            host_sound_buffer_stop(stream);
            host_sound_buffer_free(stream);
            stream = 0;
            HAPI_CloseFile(streamFile);
        }
    }
}

// FUNCTION: 0x4cfb40
void Sound::StopStream()
{
    if (streamTimer != -1) {
        RemoveTimer(streamTimer);
        streamTimer = -1;
    }
    if (stream != 0) {
        host_sound_buffer_stop(stream);
        host_sound_buffer_free(stream);
        stream = 0;
        HAPI_CloseFile(streamFile);
    }
}

// FUNCTION: 0x4cfba0
int Sound::IsStreamActive(void)
{
    if (stream == 0 && streamTimer == -1) {
        return 0;
    }
    return 1;
}

// FUNCTION: 0x4cfbc0
void Sound::UpdateStream()
{
    unsigned int play;
    unsigned int write;
    host_sound_buffer_position(stream, &play, &write);
    if (streamOffset >= 0) {
        if (streamPos == 0 && streamOffset < streamSize && play >= streamSize) {
            if (streamTimer != -1) {
                RemoveTimer(streamTimer);
                streamTimer = -1;
            }
            if (stream != 0) {
                host_sound_buffer_stop(stream);
                host_sound_buffer_free(stream);
                stream = 0;
                HAPI_CloseFile(streamFile);
            }
            return;
        }
        if (streamPos != 0 && streamOffset >= streamSize && play < streamSize) {
            if (streamTimer != -1) {
                RemoveTimer(streamTimer);
                streamTimer = -1;
            }
            if (stream != 0) {
                host_sound_buffer_stop(stream);
                host_sound_buffer_free(stream);
                stream = 0;
                HAPI_CloseFile(streamFile);
            }
            return;
        }
    }
    if (streamPos >= streamSize) {
        if (play >= streamSize) {
            return;
        }
        FillStreamHalf();
    } else {
        if (play < streamSize) {
            return;
        }
        FillStreamHalf();
    }
}

// FUNCTION: 0x4cfca0
void Sound::FillStreamHalf()
{
    unsigned int flags;
    char* buffer;
    if (host_sound_buffer_lock(stream, streamPos, streamSize, (void**)&buffer, &flags) == 0) {
        if (streamTimer != -1) {
            RemoveTimer(streamTimer);
            streamTimer = -1;
        }
        if (stream != 0) {
            host_sound_buffer_stop(stream);
            host_sound_buffer_free(stream);
            stream = 0;
            HAPI_CloseFile(streamFile);
        }
        return;
    }
    if (streamOffset >= 0) {
        int b = streamBits;
        unsigned n = streamSize;
        if (b != 8) {
            if (b == 16) {
                memset(buffer, 0, n);
            }
        } else {
            memset(buffer, 0x80, n);
        }
    } else {
        long seek = HAPI_TellFile(streamFile);
        long avail = HAPI_FileLength(streamFile) - seek;
        unsigned n = (unsigned)avail < (unsigned)streamSize ? (unsigned)avail : (unsigned)streamSize;
        HAPI_readfromfile(streamFile, buffer, n);
        if (n < (unsigned)streamSize) {
            streamOffset = n + streamPos;
            unsigned len = streamSize - n;
            char* p = buffer + n;
            int b = streamBits;
            if (b != 8) {
                if (b == 16) {
                    memset(p, 0, len);
                }
            } else {
                memset(p, 0x80, len);
            }
        }
    }
    host_sound_buffer_unlock(stream, buffer, streamSize);
    if (streamPos == 0) {
        streamPos = streamSize;
    } else {
        streamPos = 0;
    }
}

// FUNCTION: 0x4cfe40
void Sound::FillSilence(void* dest, unsigned int size)
{
    if (streamBits != 8) {
        if (streamBits != 0x10) {
            return;
        }
        memset(dest, 0, size);
        return;
    }
    memset(dest, 0x80, size);
}

// FUNCTION: 0x4cfe80
void Sound::Enable3D()
{
    use3D = 1;
}

// FUNCTION: 0x4cfe90
void Sound::Disable3D()
{
    use3D = 0;
}

// FUNCTION: 0x4cfea0
int Sound::Is3DEnabled()
{
    return use3D;
}

// FUNCTION: 0x4cfeb0
void Sound::Set3DDistances(float minimum, float maximum)
{
    minDistance = minimum;
    maxDistance = maximum;
}

// FUNCTION: 0x4cfed0
int NullStubRet0(void)
{
    return 0;
}

// FUNCTION: 0x4cfee0
void NopRet_B(void)
{
}

// FUNCTION: 0x4cfef0
int NullStubRet0_B(void)
{
    return 0;
}

// FUNCTION: 0x4cff00
void NopRet_C(void)
{
}

// FUNCTION: 0x4cff10
int NullStubRet0_C(void)
{
    return 0;
}

// FUNCTION: 0x4cff20
int Sound::HasNoDriver()
{
    return noDriver;
}

static inline int GetWaveOutVolume(Sound* sound)
{
    unsigned int volume;
    for (int i = 0; i < sound->waveDevices; i++) {
        if (host_mixer_get(HOST_MIXER_WAVE, i, &volume))
            return volume & 0xffff;
    }
    return -1;
}

static inline int GetAuxOutVolume(Sound* sound)
{
    unsigned int volume;
    if (sound->auxDevice >= 0 && host_mixer_get(HOST_MIXER_CD, sound->auxDevice, &volume))
        return volume & 0xffff;
    return -1;
}

#pragma auto_inline(off)
// FUNCTION: 0x4cff30
void Sound::InitMixerVolumes()
{
    waveDevices = host_mixer_wave_count();
    auxDevice = host_mixer_cd_device();

    waveVolume = GetWaveOutVolume(this);
    auxVolume = GetAuxOutVolume(this);
}
#pragma auto_inline(on)

// FUNCTION: 0x4cfff0
int Sound::QueryWaveVolume()
{
    unsigned int volume;
    for (int i = 0; i < waveDevices; i++) {
        if (host_mixer_get(HOST_MIXER_WAVE, i, &volume))
            return volume & 0xffff;
    }
    return -1;
}

#pragma auto_inline(off)
// FUNCTION: 0x4d0040
int Sound::QueryAuxVolume()
{
    unsigned int volume;
    if (auxDevice >= 0 && host_mixer_get(HOST_MIXER_CD, auxDevice, &volume)) {
        return volume & 0xffff;
    }
    return -1;
}
#pragma auto_inline(on)

// FUNCTION: 0x4d0070
int Sound::SetWaveVolume(int volume)
{
    int v = ClampVolume(volume);
    int failed = 0;
    for (int i = 0; i < waveDevices; i++) {
        if (!host_mixer_set(HOST_MIXER_WAVE, i, (v << 16) | v)) {
            failed = 1;
        }
    }
    return failed;
}

// FUNCTION: 0x4d00d0
int Sound::SetAuxVolume(int volume, int temporary)
{
    if (step != 0 && temporary == 0) {
        return 1;
    }
    int v = ClampVolume(volume);
    if (temporary == 0) {
        cdVolume = v;
    }
    return host_mixer_set(HOST_MIXER_CD, auxDevice, (v << 16) | v);
}

// FUNCTION: 0x4d0130
void Sound::RestoreMixerVolumes()
{
    if (waveVolume >= 0) {
        int v = ClampVolume(waveVolume);
        for (int i = 0; i < waveDevices; i++) {
            host_mixer_set(HOST_MIXER_WAVE, i, (v << 16) | v);
        }
    }
    if (auxVolume >= 0 && step == 0) {
        int v = ClampVolume(auxVolume);
        cdVolume = v;
        host_mixer_set(HOST_MIXER_CD, auxDevice, (v << 16) | v);
    }
}

// FUNCTION: 0x4d01b0
int Sound::DetectSampleFormat(void* file)
{
    char tag[4];
    HAPI_SeekFile(file, 0);
    HAPI_readfromfile(file, tag, 4);
    if (strncmp(tag, "DIGI", 4) == 0) {
        HAPI_SeekFile(file, 8);
        HAPI_readfromfile(file, tag, 4);
        if (strncmp(tag, "HSHD", 4) == 0) {
            HAPI_SeekFile(file, 0x20);
            HAPI_readfromfile(file, tag, 4);
            if (strncmp(tag, "SDAT", 4) == 0) {
                return 1;
            }
        }
    }
    if (strncmp(tag, "RIFF", 4) == 0) {
        HAPI_SeekFile(file, 8);
        HAPI_readfromfile(file, tag, 4);
        if (strncmp(tag, "WAVE", 4) == 0) {
            return 2;
        }
    }
    return 0;
}

// FUNCTION: 0x4d02a0
int Sound::OpenSample(char* path, int mode, int p3, int p4) {
    int result = 0;
    FileHandle* file = HAPI_OpenFileRead(path);
    if (file == 0)
        return result;
    int kind = DetectSampleFormat(file);
    int size = HAPI_FileLength(file);
    switch (kind) {
    case 0:
        HAPI_SeekFile(file, 0);
        switch (mode) {
        case 0:
            result = (int)this->CreateSampleFromFile(file, size, 0x2b11, 8, 1);
            break;
        case 1:
            result = this->PlayFileSample(file, size, 0x2b11, 8, 1, p3, (Pos_004cf570*)p4);
            break;
        case 2:
            this->StartStream(file, 0x2b11, 8, 1, p3);
            return 1;
        }
        break;
    case 1: {
        unsigned int x;
        HAPI_SeekFile(file, 0x16);
        HAPI_readfromfile(file, &x, 4);
        if (x == 0x2af8)
            x = 0x2b11;
        HAPI_SeekFile(file, 0x28);
        switch (mode) {
        case 0:
            result = (int)this->CreateSampleFromFile(file, size - 0x28, x, 8, 1);
            break;
        case 1:
            result = this->PlayFileSample(file, size - 0x28, x, 8, 1, p3, (Pos_004cf570*)p4);
            break;
        case 2:
            this->StartStream(file, x, 8, 1, p3);
            return 1;
        }
        break;
    }
    case 2: {
        unsigned int len = FindChunk(file, "fmt ");
        if (len < 0x10)
            goto end;
        WaveFormat wfx;
        HAPI_readfromfile(file, &wfx, 0x10);
        int bits = wfx.wBitsPerSample;
        int chans = wfx.nChannels;
        len = FindChunk(file, "data");
        if ((int)len <= 0)
            goto end;
        switch (mode) {
        case 0:
            result = (int)CreateSampleFromFile(file, len, wfx.nSamplesPerSec, bits, chans);
            break;
        case 1:
            result = PlayFileSample(file, len, wfx.nSamplesPerSec, bits, chans, p3, (Pos_004cf570*)p4);
            break;
        case 2:
            StartStream(file, wfx.nSamplesPerSec, bits, chans, p3);
            return 1;
        }
        break;
    }
    }
end:
    HAPI_CloseFile(file);
    return result;
}

// FUNCTION: 0x4d0620
void Sound::LoadSample(char* path)
{
    OpenSample(path, 0, 0, 0);
}

// FUNCTION: 0x4d0640
void Sound::PlaySample(const char* param1, int param2, int param3)
{
    OpenSample((char*)param1, 1, param2, param3);
}

// FUNCTION: 0x4d0660
void Sound::StreamSample(char* a, int b)
{
    OpenSample(a, 2, b, 0);
}

// FUNCTION: 0x4d0680
void __stdcall OnStreamTimer(int unused1)
{
    RemoveTimer(g_cdPlayer->streamTimer);
    g_cdPlayer->streamTimer = -1;
    g_cdPlayer->OpenSample(g_delayedSampleName, 2, g_delayedSampleVolume, 0);
}

// FUNCTION: 0x4d06c0
int Sound::StreamSampleDelayed(char* name, int value, int delay)
{
    strcpy(g_delayedSampleName, name);
    g_delayedSampleVolume = value;
    streamTimer = AddTimer(delay, 0, OnStreamTimer);
    return 1;
}

// FUNCTION: 0x4d0720
int Sound::FindChunkSize(void* file, char* target)
{
    char name[4];
    unsigned int total;
    unsigned int pos;
    unsigned int off;

    HAPI_SeekFile(file, 4);
    HAPI_readfromfile(file, &total, 4);
    total += 8;
    HAPI_SeekFile(file, 0xc);
    HAPI_readfromfile(file, name, 4);
    HAPI_readfromfile(file, &off, 4);
    pos = 0x14;
    for (;;) {
        if (strncmp(name, target, 4) == 0)
            return off;
        HAPI_SeekFile(file, pos + off);
        pos += off;
        if (pos >= total)
            return 0;
        HAPI_readfromfile(file, name, 4);
        HAPI_readfromfile(file, &off, 4);
        pos += 8;
    }
}

// FUNCTION: 0x4d07f0
int Sound::ReadWaveFormat(void* file, int* sampleRate, int* bitsPerSample, int* channels)
{
    unsigned int total;
    char tag[4];
    unsigned int pos;
    unsigned int len;
    unsigned int n;
    char fmt[0x10];

    HAPI_SeekFile(file, 4);
    HAPI_readfromfile(file, &total, 4);
    total = total + 8;
    HAPI_SeekFile(file, 0xc);
    HAPI_readfromfile(file, tag, 4);
    HAPI_readfromfile(file, &len, 4);
    pos = 0x14;
    for (;;) {
        if (strncmp(tag, "fmt ", 4) == 0) {
            n = len;
            break;
        }
        HAPI_SeekFile(file, pos + len);
        pos += len;
        if (pos >= total) {
            n = 0;
            break;
        }
        HAPI_readfromfile(file, tag, 4);
        HAPI_readfromfile(file, &len, 4);
        pos += 8;
    }
    if (n < 0x10)
        return 0;
    HAPI_readfromfile(file, fmt, 0x10);
    *sampleRate = *(int*)(fmt + 4);
    *bitsPerSample = *(unsigned short*)(fmt + 14);
    *channels = *(unsigned short*)(fmt + 2);
    return 1;
}

// FUNCTION: 0x4d0910
int Sound::FindDataChunkSize(void* file)
{
    char tag[4];
    unsigned int size;
    unsigned int pos;
    unsigned int len;

    HAPI_SeekFile(file, 4);
    HAPI_readfromfile(file, &size, 4);
    size += 8;
    HAPI_SeekFile(file, 0xc);
    HAPI_readfromfile(file, tag, 4);
    HAPI_readfromfile(file, &len, 4);
    pos = 0x14;
    for (;;) {
        if (strncmp(tag, "data", 4) == 0)
            return len;
        HAPI_SeekFile(file, pos + len);
        pos += len;
        if (pos >= size)
            return 0;
        HAPI_readfromfile(file, tag, 4);
        HAPI_readfromfile(file, &len, 4);
        pos += 8;
    }
}

// FUNCTION: 0x4d09e0
void InitLzssLock()
{
    g_squashThreadLockTicket = 0;
    if (g_squashThreadLockEvent != 0) {
        host_event_reset(g_squashThreadLockEvent);
        return;
    }
    g_squashThreadLockEvent = host_event_create(0, 0);
}

// FUNCTION: 0x4d0a10
void __stdcall LzssInitTree(int index)
{
    g_squashDictTree->field_6000 = 0;
    g_squashDictTree->field_6004 = (short)index;
    g_squashDictTree->field_6002 = 0;
    g_squashDictTree->entries[index].a = 0x1000;
    g_squashDictTree->entries[index].c = 0;
    g_squashDictTree->entries[index].b = 0;
}

// FUNCTION: 0x4d0a70
int __cdecl LzssAllocWindow(void)
{
    void* eax = calloc(1, 0x1011);
    g_squashWindow = eax;
    if (eax != 0) return 0;
    return -1;
}

// FUNCTION: 0x4d0a90
int LzssAllocTree()
{
    void* p = calloc(0x1001, 6);
    g_squashDictTree = (Struct_00526ff0*)p;
    return p ? 0 : -1;
}
