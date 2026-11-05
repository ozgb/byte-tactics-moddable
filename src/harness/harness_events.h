#ifndef HARNESS_EVENTS_H
#define HARNESS_EVENTS_H


#define HEV_UNIT_SPEECH 1
#define HEV_SOUND 2
#define HEV_NANO_PARTICLES 3
#define HEV_REVERSE_NANO_PARTICLES 4
#define HEV_TELEPORT_PARTICLES 5
#define HEV_THRUST_PARTICLES 6
#define HEV_WAKE_PARTICLES 7
#define HEV_BUBBLES 8
#define HEV_WEAPON_SMOKE 9
#define HEV_TIMED_SUB_PARTICLES 10
#define HEV_SCREEN_SHAKE 11
#define HEV_CD_ACTIVITY 12
#define HEV_FORCES_DESTROYED 13
#define HEV_SCORE_FLASH 14
#define HEV_SELECTION_ORDERS_DIRTY 15
#define HEV_BUILD_MENU_UPDATE 16
#define HEV_DESELECT_IF_INELIGIBLE 17
#define HEV_GUI_CHANGED_IF_SCREEN 18
#define HEV_FOCUS_COMMANDER 19
#define HEV_POP_TO_GAME_LAYOUT 20
#define HEV_PLAYER_TOOK_LEAD 21
#define HEV_PLAYER_LEFT 22
#define HEV_BUILD_QUEUE_SOUND 23
#define HEV_WATCH_PROMPT 25
#define HEV_WATCH_MODE_NOTICE 26
#define HEV_SMOKE 27
#define HEV_WHITE_SMOKE 28
#define HEV_BLACK_SMOKE 29
#define HEV_SOUND_AT 30
#define HEV_SOUND_AT_NAME 31
#define HEV_REMOVE_UNIT_SPEECH 32
#define HEV_MESSAGE 33
#define HEV_RADAR_UNITS 34
#define HEV_RADAR_MAPPED 35
#define HEV_SPEECH_IF_NOT_VISIBLE 1u

void __cdecl HarnessEventWords(int kind, unsigned flags, int count, ...);
void __cdecl HarnessEventPtrs(int kind, const int* a, int na, const int* b, int nb, int ntail, ...);
void __stdcall HarnessEventUnit(int kind, const void* unit);
void __stdcall HarnessEventSpeech(const void* unit, int slot, const char* text, unsigned flags);
void __stdcall HarnessEventSoundAt(int kind, int sound, const int* pos);
void __stdcall HarnessEventSuppress(int on);
void __stdcall HarnessEventRadar(int kind, const unsigned char* bytes, int size);
unsigned char* __stdcall HarnessEventBuffer(void);
void __stdcall HarnessRecordPhase(int phase);

#endif

