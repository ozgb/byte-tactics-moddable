#include "win_types.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../units/squad.h"

class Sound {
public:
    int InitDirectSound(int rate, int bits, int channels, int handle);
    int HasNoDriver();
    void ReleaseDirectSound();
    void* LoadSample(char* path);
    void ReleaseSampleSet(void** set);
    int StreamSampleDelayed(char* name, int value, int delay);
    int PlayLooping(int sample, int volume);
    int PlaySampleSet(int a, int b, int c);
    int PlaySampleSet(int a, int b, void* c);
    int PlaySample(const char* name, int volume, int pos);
    int Is3DEnabled();
    void Set3DDistances(float a, float b);
    void StopAllBuffers();
    void OpenCdAudio();
    void RestoreMixerVolumes();
    void SetMaxBuffers(int count);
};

struct DisplayHwndView {
    char unknown_0[0x40];
    int hwnd;
};

union SoundWorldPos {
    struct {
        short xFrac;
        short x;
        short yFrac;
        short y;
        short zFrac;
        short z;
    };
    struct {
        int xVal;
        int yVal;
        int zVal;
    };
};

#pragma pack(push, 1)
struct SoundAtPacket {
    unsigned char type;
    unsigned char flag;
    int index;
    SoundWorldPos pos;
};

#include "../units/unit_def.h"

#include "../units/unit.h"

struct SoundCategorySlotView {
    int count;
    char* table1;
    char* table2;
};

struct SoundCategoryEntryView {
    char unknown_0[0x40];
    SoundCategorySlotView slots[24];
};

#include "../network/player.h"

#include "../game/game.h"

struct SpeechEntry {
    int kind;
    int frame;
    Unit* unit;
    char* data;
    unsigned char priority;
};

#include "../graphics/object_pool.h"

class SpeechQueue {
public:
    SpeechEntry entries[9];
    int count;
    unsigned int lastFrame;
    unsigned int interval;
    int field_a5;

    void Remove(int i)
    {
        if (entries[i].data) {
            GameFreeThunk(entries[i].data);
            entries[i].data = 0;
        }
        for (int j = i; j < count; j++)
            entries[j] = entries[j + 1];
        count--;
    }

    SpeechQueue(int param_1, int param_2);
    void ClearSpeechList(void);
    void ClearSpeechEntries(void);
    void EnqueueSpeech(Unit* unit, int kind, char* text);
    void PlayNextSpeechEntry();
    void PlaySpeech(int index, int param_2, int param_3);
    void RemoveSpeechAt(int index);
    void RemoveSpeechOfId(Unit* unit);
};

struct SpeechTypeInfo {
    int priority;
    int cooldown;
    int unknown_8;
    char* text;
    unsigned int minFrame;
    int unknown_14;
};
#pragma pack(pop)

#include "../../link/globals.h"
extern SpeechTypeInfo g_speechTypes[24];


#include "../game/main.h"
#include "play_sound.h"
#include "../network/play_sound_packet.h"
#include "../graphics/display.h"
void* __cdecl operator new(size_t size);
void __cdecl operator delete(void* p);
#include "../debug/debug_lib.h"
#include "../game/data_files.h"
#include "../files/files_api.h"
void __stdcall AddMessage(char* text, unsigned char key, unsigned short value, char last);
int __stdcall PlayWavMemory(int sound);
int __stdcall PlayLoopingWavMemory(int sound);
#include "../network/net_game.h"
int __stdcall BroadcastPacket(int player, void* data, int size);
struct Cell;
#include "../map/line_of_sight.h"
char* __stdcall Translate(char* text);
#include "../ingame/selection.h"

// FUNCTION: 0x47f960
SpeechQueue::SpeechQueue(int param_1, int param_2)
{
    count = 0;
    lastFrame = 0;
    interval = param_1;
    field_a5 = param_2;
}

// FUNCTION: 0x47ed40
void InitSound(void)
{
    if (GetPreferenceInt(g_noDirectSoundKey, 0))
        g_noDirectSound = 1;
    if (GetPreferenceInt(g_useWindowsSoundKey, 0))
        g_useWindowsSound = 1;
    if (g_useWindowsSound) {
        if (!IsWindowsSoundAvailable())
            g_useWindowsSound = 0;
        g_noDirectSound = 1;
    }
    if (!g_noDirectSound) {
        int hr = ((Sound *)g_game->sound)->InitDirectSound(0x2b11, 0x10, 2, ((DisplayHwndView *)g_game->displayContext)->hwnd);
        if (hr == 0) {
            if (((Sound *)g_game->sound)->HasNoDriver())
                g_noDirectSound = 1;
            else
                FatalError(g_soundInitError);
        }
    }
    ((Sound *)g_game->sound)->OpenCdAudio();
    g_speechQueue = new SpeechQueue(0x1e, 0x96);
}

// FUNCTION: 0x47ee30
void ResetSpeech()
{
    SpeechQueue* list = g_speechQueue;
    if (list->count > 0) {
        int* count = &list->count;
        do {
            int i = *count - 1;
            SpeechEntry* last = &list->entries[i];
            if (last->data) {
                GameFreeThunk(last->data);
                last->data = 0;
            }
            for (int j = i; j < *count; j++) {
                count = &list->count;
                list->entries[j] = list->entries[j + 1];
            }
            (*count)--;
        } while (*count > 0);
    }
    list->lastFrame = 0;
    for (int k = 0; k < 24; k++)
        g_speechTypes[k].minFrame = 0;
}

// FUNCTION: 0x47eee0
void ShutdownSound()
{
    SpeechQueue* list = g_speechQueue;
    if (list) {
        if (list->count > 0) {
            int* count = &list->count;
            do {
                int i = *count - 1;
                SpeechEntry* last = &list->entries[i];
                if (last->data) {
                    GameFreeThunk(last->data);
                    last->data = 0;
                }
                for (int j = i; j < *count; j++) {
                    count = &list->count;
                    list->entries[j] = list->entries[j + 1];
                }
                (*count)--;
            } while (*count > 0);
        }
        list->lastFrame = 0;
        operator delete(list);
        g_speechQueue = 0;
    }
    ((Sound *)g_game->sound)->RestoreMixerVolumes();
    Sound* sound = (Sound *)g_game->sound;
    if (sound) {
        sound->ReleaseDirectSound();
        operator delete(sound);
    }
    (*(Sound **)&g_game->sound) = 0;
}

// FUNCTION: 0x47efc0
void SetNoDirectSound()
{
    g_noDirectSound = 1;
}

// FUNCTION: 0x47efd0
void SetUseWindowsSound()
{
    g_useWindowsSound = 1;
    g_noDirectSound = 1;
}

// FUNCTION: 0x47efe0
void* __stdcall LoadSoundFile(const char* name)
{
    char path[256];
    if (g_noDirectSound != 0 && g_useWindowsSound == 0) {
        return 0;
    }
    BuildDataPath(path, "sounds", name, "WAV");
    if (g_useWindowsSound != 0) {
        return HAPI_LoadFile(path, 0);
    }
    return ((Sound *)g_game->sound)->LoadSample(path);
}

// FUNCTION: 0x47f060
void __stdcall FreeSoundSet(void** set)
{
    if (g_useWindowsSound != 0) {
        GameFreeThunk(set);
        return;
    }
    ((Sound *)g_game->sound)->ReleaseSampleSet(set);
}

// FUNCTION: 0x47f090
void __stdcall StreamSoundDelayed(char* param_1, int param_2, int param_3)
{
    if (g_noDirectSound == 0) {
        ((Sound *)g_game->sound)->StreamSampleDelayed(param_1, param_2, param_3);
    }
}

// FUNCTION: 0x47f0c0
int __stdcall PlaySoundByIndex(int index, int param_2)
{
    if (index != 0xffff) {
        int sound = g_game->sounds[index];
        if (g_useWindowsSound) {
            if (g_playLooping)
                return PlayLoopingWavMemory(sound);
            return PlayWavMemory(sound);
        }
        if (g_game->volume1 != 0 && (g_game->soundFlags_37f19 & 7) != 0
            && g_noDirectSound == 0) {
            if (param_2) {
                PlaySoundPacket packet;
                packet.type = 0x13;
                packet.flag = 1;
                packet.index = index;
                BroadcastPacket(GetLocalDpid(), &packet, sizeof(packet));
            }
            if (g_playLooping)
                return ((Sound *)g_game->sound)->PlayLooping(sound, -585);
            return ((Sound *)g_game->sound)->PlaySampleSet(sound, -585, 0);
        }
    }
    return 0;
}

static inline int FindSound(char* name)
{
    for (int i = 0; i < g_game->soundCount; i++) {
        if (g_game->soundNames_33e13_4000[i][0] && _strcmpi(g_game->soundNames_33e13_4000[i], name) == 0)
            return i;
    }
    return 0xffff;
}

// FUNCTION: 0x47f1a0
void __stdcall PlaySoundByName(char* name, int param_2)
{
    PlaySoundByIndex(FindSound(name), param_2);
}

// FUNCTION: 0x47f210
void __stdcall PlayLoopingSoundByName(char* name, int param_2)
{
    g_playLooping = 1;
    PlaySoundByIndex(FindSound(name), param_2);
    g_playLooping = 0;
}

// FUNCTION: 0x47f290
int __stdcall PlaySoundFile(char* name)
{
    if (g_useWindowsSound)
        return PlayWavFromDisk(name);
    if (name && strlen(name) && g_game->volume1 && (g_game->soundFlags_37f19 & 7) && !g_noDirectSound)
        return ((Sound *)g_game->sound)->PlaySample(name, -0x249, 0);
    return 0;
}

static inline int MapContains(unsigned int w, unsigned int h, int tx, int ty)
{
    return tx < w && ty < h;
}

// FUNCTION: 0x47f300
int __stdcall PlaySoundAt(int index, SoundWorldPos* pos, int param_3)
{
    if (g_useWindowsSound)
        return PlaySoundByIndex(index, param_3);
    if (index == 0xffff)
        return 0;
    if (g_game->volume1 == 0)
        return 0;
    if ((g_game->soundFlags_37f19 & 7) == 0)
        return 0;
    if (g_noDirectSound != 0)
        return 0;

    int sound = g_game->sounds[index];

    if (param_3) {
        SoundAtPacket packet;
        packet.type = 0x13;
        packet.flag = 1;
        packet.index = index;
        packet.pos = *pos;
        BroadcastPacket(GetLocalDpid(), &packet, 0x12);
    }

    if (GetMapCell(pos->xVal / (1 << 20), pos->zVal / (1 << 20)) == 0)
        return 0;

    int pi = g_game->playerIndex;
    Player* player = g_game->Current();
    Player* player2 = g_game->Current();
    int vis;
    if ((g_game->mapFlags & 2) == 2) {
        int tx = pos->x >> 5;
        int ty = (pos->z - (pos->y >> 1)) >> 5;
        if (tx < (unsigned int)player->exploredWidth && ty < (unsigned int)player->exploredHeight
            && player->explored[player2->exploredWidth * ty + tx] != 0)
            vis = 1;
        else
            vis = 0;
    } else {
        int tx = pos->x >> 5;
        int ty = (pos->z - (pos->y >> 1)) >> 5;
        if (!MapContains(player->exploredWidth, player->exploredHeight, tx, ty))
            vis = 0;
        else
            vis = (g_game->visibilityMask[player2->exploredWidth * ty + tx]
                & (1 << pi)) ? 1 : 0;
    }
    if (vis != 0) {
        if (((Sound *)g_game->sound)->Is3DEnabled()) {
            Vec3 p;
            p.x = pos->x - g_game->scrollX - (g_game->screenTilesX / 2) * 16;
            p.z = g_game->scrollY + (g_game->screenTilesY / 2) * 16
                + (pos->y >> 1) - pos->z;
            p.y = 0;
            ((Sound *)g_game->sound)->Set3DDistances(
                (float)(((g_game->screenTilesX + g_game->screenTilesY) / 2) * 16),
                (float)((g_game->width + g_game->height) * 16));
            return ((Sound *)g_game->sound)->PlaySampleSet(sound, -585, &p);
        } else {
            if (g_game->scrollX > pos->x || g_game->scrollY > pos->z
                || g_game->scrollX + g_game->screenTilesX * 16 < pos->x
                || g_game->scrollY + g_game->screenTilesY * 16 < pos->z)
                return ((Sound *)g_game->sound)->PlaySampleSet(sound, -1585, 0);
            return ((Sound *)g_game->sound)->PlaySampleSet(sound, -585, 0);
        }
    }
    return 0;
}

// FUNCTION: 0x47f610
void __stdcall PlaySoundAtByName(char* name, int param_2, int param_3)
{
    PlaySoundAt(FindSound(name), (SoundWorldPos*)param_2, param_3);
}

// FUNCTION: 0x47f680
void PlayNextSpeech()
{
    SpeechQueue* driver = g_speechQueue;
    int& count = g_speechQueue->count;
    if (count) {
        if ((unsigned int)g_game->ticks >= g_speechQueue->lastFrame + g_speechQueue->interval) {
            g_speechQueue->PlaySpeech(0, 1, 1);
            driver->lastFrame = (unsigned int)g_game->ticks;
        } else {
            g_speechQueue->PlaySpeech(0, 0, 1);
        }
        if (driver->entries[0].data) {
            GameFreeThunk(driver->entries[0].data);
            driver->entries[0].data = 0;
        }
        for (int i = 0; i < count; i++)
            driver->entries[i] = driver->entries[i + 1];
        count--;
    }
    if (g_useWindowsSound)
        ResumeLoopingWav();
}

// FUNCTION: 0x47f750
void StopAllSounds()
{
    if (g_noDirectSound == 0) {
        ((Sound *)g_game->sound)->StopAllBuffers();
    }
    if (g_useWindowsSound != 0) {
        StopWindowsSound();
    }
}

// FUNCTION: 0x47f780
void __stdcall QueueUnitSpeech(Unit* unit, int kind, char* text)
{
    if (unit->playerIndex == g_game->playerIndex && (unit->flags & 0x10000000) && !(unit->flags & 0x4000)) {
        if (text == 0) {
            text = g_speechTypes[kind].text;
        }
        g_speechQueue->EnqueueSpeech(unit, kind, Translate(text));
    }
}

// FUNCTION: 0x47f7e0
void __stdcall QueueUnitSpeechIfVisible(Unit* unit, int kind, char* text)
{
    if (IsUnitVisible((char *)unit) != 0) {
        QueueUnitSpeech(unit, kind, text);
    }
}

// FUNCTION: 0x47f850
void __stdcall QueueUnitSpeechIfNotVisible(Unit* unit, int kind, char* text)
{
    if (IsUnitVisible((char *)unit) == 0) {
        QueueUnitSpeech(unit, kind, text);
    }
}

// FUNCTION: 0x47f8c0
void __stdcall RemoveSpeechOfUnit(int id)
{
    int* count = &g_speechQueue->count;
    SpeechEntry* entries = g_speechQueue->entries;
    int i = 0;
    while (i < *count) {
        if (entries[i].unit == (Unit*)id) {
            if (entries[i].data) {
                GameFreeThunk(entries[i].data);
                entries[i].data = 0;
            }
            for (int j = i; j < *count; j++)
                entries[j] = entries[j + 1];
            (*count)--;
        } else {
            i++;
        }
    }
}

// FUNCTION: 0x47f990
void SpeechQueue::ClearSpeechList(void)
{
    while (count > 0)
        Remove(count - 1);
    lastFrame = 0;
}

// FUNCTION: 0x47fa30
void SpeechQueue::ClearSpeechEntries(void)
{
    while (count > 0)
        Remove(count - 1);
    lastFrame = 0;
}

// FUNCTION: 0x47fad0
void SpeechQueue::EnqueueSpeech(Unit* unit, int kind, char* text)
{
    if ((unsigned int)g_game->ticks < g_speechTypes[kind].minFrame)
        return;

    for (int i = 0; i < count; i++)
        if (entries[i].kind == kind)
            return;

    if (count == 8) {
        PlaySpeech(7, 0, 1);
        if (entries[7].data) {
            GameFreeThunk(entries[7].data);
            entries[7].data = 0;
        }
        for (int i = 7; i < count; i++)
            entries[i] = entries[i + 1];
        count--;
    }

    int j;
    if (count != 0) {
        for (j = 0; j < count; j++)
            if (g_speechTypes[entries[j].kind].priority < g_speechTypes[kind].priority)
                break;

        for (int i = count; i > j; i--)
            entries[i] = entries[i - 1];
    } else {
        j = 0;
    }

    entries[j].kind = kind;
    entries[j].frame = (unsigned int)g_game->ticks;
    entries[j].unit = unit;
    entries[j].priority = g_speechTypes[kind].priority;
    if (text) {
        entries[j].data = (char*)GameAllocIgnoreTag("Speech Text", strlen(text) + 1);
        strcpy(entries[j].data, text);
    } else {
        entries[j].data = 0;
    }
    count++;
}

// FUNCTION: 0x47fca0
void SpeechQueue::PlayNextSpeechEntry()
{
    if (count == 0) {
        return;
    }
    if ((unsigned int)g_game->ticks >= interval + lastFrame) {
        this->PlaySpeech(0, 1, 1);
        lastFrame = (unsigned int)g_game->ticks;
    } else {
        this->PlaySpeech(0, 0, 1);
    }
    if (entries[0].data) {
        GameFreeThunk(entries[0].data);
        entries[0].data = 0;
    }
    for (int i = 0; i < count; i++) {
        entries[i] = entries[i + 1];
    }
    count--;
}

#include "../harness/harness.h"

// FUNCTION: 0x47fd70
void SpeechQueue::PlaySpeech(int index, int param_2, int param_3)
{
    SpeechEntry* e = &entries[index];
    int slot = entries[index].kind;
    unsigned int catIndex = 0;
    catIndex = (unsigned short)entries[index].unit->def->soundcategory;
    SoundCategoryEntryView* cat = &((SoundCategoryEntryView *)g_game->entries_37e13)[catIndex];
    int count = cat->slots[slot].count;
    int idx = (int)((__int64)HarnessViewRand() * count / 0x8000);

    if ((int)e->priority > 10 - g_game->unitChat && count > 0 && param_2 != 0
        && (g_game->soundFlags_37f19 & 0x40)) {
        char* name;
        if (g_cmdSingEnabled)
            name = (char*)((((unsigned int)g_game->ticks / 30) & 7) ? "sing" : "honk");
        else
            name = cat->slots[slot].table1 + idx * 0x40;

        char path[256];
        BuildDataPath(path, "sounds", name, "WAV");
        if (g_useWindowsSound) {
            PlayWavFromDisk(path);
        } else if (path && strlen(path) && g_game->volume1
                   && (g_game->soundFlags_37f19 & 7) && g_noDirectSound == 0) {
            ((Sound *)g_game->sound)->PlaySample(path, -0x249, 0);
        }
        g_speechTypes[slot].minFrame =
            (unsigned int)g_game->ticks + g_speechTypes[slot].cooldown * 0x1e;
    }

    if (param_3 != 0 && (int)e->priority > 10 - g_game->unitChatText) {
        char* text = e->data;
        if (text == 0) {
            if (idx != -1)
                text = cat->slots[slot].table2 + idx * 0x40;
            if (text == 0)
                return;
        }
        if (*text == 0)
            return;
        if (e->unit->flags & 0x10000000) {
            char msg[100];
            sprintf(msg, "%s: %s", e->unit->def->name, text);
            AddMessage(msg, 1, e->unit->id, '\n');
        }
    }
}

// FUNCTION: 0x47ffa0
void SpeechQueue::RemoveSpeechAt(int index)
{
    SpeechEntry* e = &entries[index];
    if (e->data != 0) {
        GameFreeThunk(e->data);
        e->data = 0;
    }
    for (int i = index; i < count; i++) {
        entries[i] = entries[i + 1];
    }
    count--;
}

// FUNCTION: 0x480020
void SpeechQueue::RemoveSpeechOfId(Unit* unit)
{
    int i = 0;
    while (i < count) {
        if (entries[i].unit == unit) {
            if (entries[i].data) {
                GameFreeThunk(entries[i].data);
                entries[i].data = 0;
            }
            for (int j = i; j < count; j++)
                entries[j] = entries[j + 1];
            count--;
        } else {
            i++;
        }
    }
}

#include <vector>
#include <algorithm>

// FUNCTION: 0x4800c0
UnitSwapEraseVector::UnitVector::iterator UnitSwapEraseVector::EraseSwapBack(UnitVector::iterator where)
{
    UnitVector::iterator last = units.end() - 1;
    *where = *last;
    units.erase(last);
    return where;
}

// FUNCTION: 0x480100
int IntDynArray::EraseByValue(int value)
{
    std::vector<int>::iterator it = std::find(items.begin(), items.end(), value);
    if (it == items.end()) {
        return 0;
    }
    std::vector<int>::iterator last = items.end() - 1;
    *it = *last;
    items.erase(last);
    return 1;
}

// FUNCTION: 0x480160
Squad::Squad(Player* a, int b)
    : owner(a), index(b), field_8(0), field_c(0)
{
}
