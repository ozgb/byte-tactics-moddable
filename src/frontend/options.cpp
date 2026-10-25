#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/tdf.h"

extern "C" __declspec(dllimport) int __cdecl wsprintfA(char* out, const char* format, ...);

#include "../sound/sound.h"

#include "../map/mission.h"

#include "../graphics/memory_cache.h"

class Class_004a1080;
class Object_004a0570;
class Object_004a1450;
struct Dialog;
struct Surface;
struct Mode_0045e4c0;
struct Link_0045ead0;
struct GadgetOwner_004605c0;
struct Screen_0045e100;
struct Gui;

#pragma pack(push, 1)

struct ModeList_0045b800;
struct Menu_0045b800;

struct Gadget {
    union {
        unsigned char type;
        char state;
    };
    char unknown_1;
    char name[0x11];
    short x;
    short y;
    short width;
    short height;
    int attribs;
    char unknown_1f[0x29 - 0x1f];
    unsigned char active;
    char unknown_2a[0xb6 - 0x2a];
    union {
        short count;
        char text[0x80];
        struct {
            char unknown_b6[0xba - 0xb6];
            short selected;
            char unknown_bc[0xbe - 0xbc];
            void* frames;
            char unknown_c2[0xc6 - 0xc2];
            unsigned short frame;
            char unknown_c8[0xd2 - 0xc8];
            void* records;
        };
        struct {
            char unknown_b6b[0xbc - 0xb6];
            Surface* surface;
            char unknown_c0[0xc4 - 0xc0];
            int background;
        };
    };
    union {
        short range;
        struct {
            unsigned char stages;
            unsigned char stageIndex;
        };
    };
    char unknown_138[0x13c - 0x138];
    int max;
    short knobPos;
    short knobSize;
    void (__stdcall* sliderCallback)(Menu_0045b800* obj, int value);
    char unknown_148[0x14a - 0x148];
    union {
        void* sliderUser;
        ModeList_0045b800* list;
    };
    char unknown_14e[0x15b - 0x14e];
};

struct Holder_0045b800 {
    int unknown_0;
    Gadget* entries;
};

struct Mode_0045b800 {
    int width;
    int height;
    char unknown_8[4];
};

struct ModeList_0045b800 {
    int count;
    Mode_0045b800* modes;
};

union Bits_0045cf60 {
    unsigned char byte;
    unsigned short word;
    struct {
        unsigned short bit0 : 1;
        unsigned short bit1 : 1;
        unsigned short bit2 : 1;
        unsigned short rest : 13;
    } bits;
    struct {
        unsigned short pad : 2;
        unsigned short flag : 1;
        unsigned short rest : 13;
    } flag;
    struct {
        unsigned char b0 : 1;
        unsigned char b1 : 1;
        unsigned char prefs : 1;
        unsigned char rest : 5;
    } prefsByte;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short prefs : 1;
        unsigned short rest : 13;
    } prefsWord;
};

union Flags37ebe_0045cf60 {
    unsigned char byte;
    unsigned short word;
    struct {
        unsigned short loaded : 1;
        unsigned short rest : 15;
    } loadedBits;
    struct {
        unsigned short b0 : 1;
        unsigned short rest : 15;
    } bits;
};

union Flags37f06_0045cf60 {
    unsigned char byte;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short b2 : 1;
        unsigned short b3 : 1;
        unsigned short b4 : 1;
        unsigned short b5 : 1;
        unsigned short b6 : 1;
        unsigned short rest : 9;
    } bits;
};

union SoundFlags_0045cf60 {
    unsigned short word;
    unsigned char byte;
    struct {
        unsigned short mode : 3;
        unsigned short b3 : 1;
        unsigned short b4 : 1;
        unsigned short b5 : 1;
        unsigned short speech : 1;
        unsigned short b7 : 1;
        unsigned short rest : 8;
    } bits;
};

union Los_0045cf60 {
    unsigned short losFlags;
    struct {
        unsigned short l0 : 1, l1 : 1, l2 : 1, l3 : 1, l4 : 1, l5 : 1, l6 : 1, l7 : 1,
                       l8 : 1, l9 : 1, l10 : 1, l11 : 1, l12 : 1, l13 : 1, l14 : 1, l15 : 1;
    } lb;
    struct {
        unsigned short bit0 : 1, bit1 : 1, bit2 : 1, rest : 13;
    } bits;
};

#include "../gui/gui.h"

struct Opts_0045f1d0 {
    char unknown_0[0x9b];
    union {
        unsigned short value;
        struct {
            unsigned short unknown_9b_0 : 6;
            unsigned short flag_9b_6 : 1;
            unsigned short unknown_9b_7 : 9;
        };
        struct {
            unsigned short b0 : 1, b1 : 1, b2 : 1, b3 : 1, b4 : 1, b5 : 1, b6 : 1,
                           b7 : 1, b8 : 1, b9 : 1, b10 : 1, b11 : 1, b12 : 1, b13 : 1,
                           b14 : 1, b15 : 1;
        } b;
    } u;
    char unknown_9d[0xa1 - 0x9d];
    unsigned short startEnergy;
    unsigned short startMetal;
    char unknown_a5[0x14b - 0xa5];
};

#include "../network/player.h"

struct RuleSet_0045f1d0 {
    char unknown_0[0xc];
    int startMetal;
    int startEnergy;
    char unknown_14[0x118 - 0x14];
    int startType;
};

struct Flags_00460680 {
    unsigned short unknown_bit0 : 4;
    unsigned short flag4 : 1;
    unsigned short unknown_rest : 11;
};

#include "../game/game.h"

struct Entry_0045c010 {
    char* format;
    int min;
};

struct Settings_45cde0 {
    char block[0x53];
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short rest : 14;
};

struct Vtable_0045d280 {
    char unknown_0[8];
    void (__stdcall* FUN_8)(void* obj);
};

struct Holder_0045d280 {
    Vtable_0045d280* next;
    Gadget* entries;
};

struct Object_0045d280 {
    char unknown_0[0x18];
    Holder_0045d280* holder;
    char unknown_1c[0x60 - 0x1c];
    int hotGadgetIndex;
};

struct Holder_0045d7c0 {
    int unknown_0;
    Gadget* gadgets;
};

struct Object_0045d7c0 {
    char unknown_0[4];
    Gadget* gadgets;
    void (__stdcall* callback8)(int value);
    char unknown_c[0x1c - 0xc];
    void (__stdcall* callback1c)();
};

struct Vtable_0045da90 {
    char unknown_0[8];
    void (__stdcall* FUN_8)(void* obj);
};

struct Holder_0045da90 {
    Vtable_0045da90* next;
    Gadget* entries;
};

struct Object_0045da90 {
    char unknown_0[0x18];
    Holder_0045da90* holder;
    char unknown_1c[0x60 - 0x1c];
    int hotGadgetIndex;
};

struct Object_0045de30 {
    char unknown_0[4];
    Gadget* entries;
    void (__stdcall *fn)(void* obj, int arg);
};

struct Menu_0045de30 {
    char unknown_0[0x18];
    void* holder;
};

struct List_0045e5e0 {
    int count;
    Mode_0045e4c0* modes;
    char unknown_8[0xc];
    char* buffer;
};

struct Holder_0045e5e0 {
    char unknown_0[4];
    Gadget* entries;
};

struct Menu_0045e5e0 {
    char unknown_0[0x18];
    Holder_0045e5e0* holder;
};

struct Object_0045ed50 {
    char unknown_0[4];
    Gadget* entries;
    void (__stdcall* fn)(void* obj, int arg);
};

struct Rule_0045f1d0 {
    char unknown_0[0xc];
    int startMetal;
    int startEnergy;
    char unknown_18[0x18 - 0x14];
};

struct Sub_0045f800 {
    char unknown_0[0x10];
};

struct Sub_0045fb30 {
    char unknown_0[0x10];
};

struct Menu_00460160 {
    char unknown_0[0x18];
};

struct Sub_00460680 {
    char unknown_0[0x10];
};

#pragma pack(pop)

struct Sub_0045cf60 {
    char unknown_0[0xcca];
    int changed;
};

struct Mode_0045e4c0 {
    int width;
    int height;
    int refreshRate;
};

struct ModeList_0045e4c0 {
    int count;
    Mode_0045e4c0* modes;
};

struct Page_0045f8c0 {
    char blank[2];
    char blank2[2];
    int first;
    int last;
};

struct Surface_0045fbc0 {
    int data[12];
};

struct Point_45ffb0 {
    int x;
    int y;
};

struct Quad_45ffb0 {
    Point_45ffb0 p[4];
};

struct Entry_45ffb0 {
    unsigned short w;
    unsigned short h;
};


struct Link_0045ead0 {
    void* obj;
    char* data;
    void (__stdcall* reselect)(void* gadget);
};

struct GadgetOwner_004605c0 {
    int unknown_0;
    int gadgets;
};

#include "../gui/layer.h"

#include "../graphics/surface.h"

#include "../graphics/gaf_frame.h"
#include "../state/sim_globals.h"

struct Unit;

typedef int (__stdcall* Handler_0045e100)(Gui*);

struct Screen_0045e100 {
    Screen_0045e100* next;
    Gadget* entries;
    Handler_0045e100 handler;
    void* userData;
    unsigned int flags;
    int active;
};

struct FreeObj_0045e100 {
    char unknown_0[4];
    void* field_4;
    char unknown_8[0x14 - 0x8];
    void* field_14;
};

struct Menu_004604a0 {
    int unknown_0;
    int unknown_4;
    int field_8;
    int field_c;
    int unknown_10;
    int language;
    char unknown_18[0x1c - 0x18];
};

// GLOBAL: 0x511de8
extern Game* g_game;

extern int g_optionsShellClosing;
extern char g_optionsSoundName[];
extern char g_bgmSoundName[];
extern char g_notrakGadgetName[];
extern char g_fxVolGadgetName[];
extern char g_trackTypeGadgetName[];
extern char g_cdNextGadgetName[];
extern char g_cdPlayGadgetName[];
extern char g_cdStopGadgetName[];
extern char g_cdPrevGadgetName[];
extern char g_trackModeGadgetName[];
extern char g_restoreGadgetName[];
extern char g_undoGadgetName[];
extern char g_speechGadgetName[];
extern char g_testGadgetName[];
extern char g_volTextGadgetName[];
extern char g_modeGadgetName[];
extern char g_explodeSoundFile[];
extern int DAT_00512c80;
extern int g_cmdlineHostMode;
extern char g_cmdlineHostGameName[];
extern char g_cmdlineTcpJoinAddress[];
extern int g_helpDialogBaseGadgetCount;
extern Entry_45ffb0 g_optionsFlipFrame;
extern int g_optionsLightbarY;
extern int g_optionsLightbarMaxX;
extern int g_optionsBackupInterfaceType;
extern int g_optionsBackupVisualFlags;
extern int g_optionsBackupGamma;
extern int g_optionsBackupFxVolume;
extern int g_optionsBackupMusicVolume;
extern union OptionsBackupMusicMode {
    int i;
    unsigned short s;
} g_optionsBackupMusicMode;
extern char g_optionsBackupCdMode;
extern unsigned char g_optionsBackupUnitChat;
extern char g_optionsBackupUnitChatText;
extern unsigned int g_optionsBackupSoundFlags;
extern int g_optionsBackupDisplayWidth;
extern int g_optionsBackupDisplayHeight;
extern int g_optionsBackupTextScroll;
extern int g_optionsBackupTextLines;
extern union OptionsBackupGameSpeed {
    unsigned short s;
    int i;
} g_optionsBackupGameSpeed;
extern union OptionsBackupEdgeScroll {
    unsigned char b;
    int i;
} g_optionsBackupEdgeScroll;
extern char g_optionsBackupTrackTypes[];
extern int g_optionsBackupLockedTrack;
extern int g_musicUiSelectedTrack;
extern int g_optionsShellActive;
extern Surface* g_optionsFlipSurface;
extern int g_optionsLightbarX;
extern int g_optionsLightbarAnim;
extern Surface* g_optionsBackupSurface;
extern int g_battleQuitIntent;
extern Settings_45cde0 g_optionsPrefsSnapshot;

void __stdcall SetGadgetActiveByName(Menu_0045b800* menu, char* name, int value);
char* __stdcall FindGadgetChecked_C(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked_D(Gadget* entries, char* name);
void __stdcall SetTranslatedTextByName(Menu_0045b800* obj, char* name, char* text, int param_4);
void __stdcall SetGrayedOut(Menu_0045b800* menu, int index, int value);
void __stdcall SetGrayedOutByName(Menu_0045b800* obj, char* name, int value);
void __stdcall SetGadgetGrayedOutByName(Menu_0045b800* obj, char* name, int param_3);
int __stdcall SetButtonStageByName(Menu_0045b800* obj, char* name, int value);
void __stdcall SetBrightness(float value);
#include "../game/game_state.h"
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
void __stdcall SetGadgetStatus(Menu_0045b800* menu, int index, short value);
void __stdcall GetGadgetName(Gadget* entries, char* name, int index);
Gadget* __stdcall FindGadgetOrNull(Gadget* entries, char* name);



void __stdcall BlitMenuLayers(Sub_0045cf60* sub, unsigned int* a, int* b);
void __stdcall BlitMenuLayers(char* menu, int a, int b);

void __stdcall SetOffscreenSurface(void* p);
void __stdcall SetOffscreenSurface(int param_1);
void FlipScreen();

int __stdcall LoadGuiLayer(void* obj, char* buf, int size);
int __stdcall LoadGuiLayer(Menu_0045de30* obj, char* name, int size);
Layer* __stdcall LoadGuiLayer(Menu_0045e5e0* menu, char* name, int flags);
Layer* __stdcall LoadGuiLayer(Layer* menu, const char* name, int flags);
Layer* __stdcall LoadGuiLayer(Sub_0045f800* sub, const char* name, int flags);
Layer* __stdcall LoadGuiLayer(Sub_0045fb30* sub, const char* name, int flags);
Layer* __stdcall LoadGuiLayer(Menu_004604a0* menu, const char* name, int flags);
Layer* __stdcall LoadGuiLayer(Sub_00460680* sub, const char* name, int flags);
Layer* __stdcall LoadGuiLayer(Gui* sub, const char* name, int flags);

void __stdcall SetGrayedOutByName(void* obj, char* name, int value);
void __stdcall SetGrayedOutByName(Dialog* obj, char* name, int value);
void __stdcall SetGrayedOut(Gui* sub, int index, int value);
int __stdcall GetGadgetText(void* settings, const char* key, char* out);

void __stdcall SetKeyboardInput(void* obj, int value);
void __stdcall SetKeyboardInput(Layer* menu, int flag);
void __stdcall SetKeyboardInput(Menu_00460160* menu, int value);
void __stdcall SetKeyboardInput(Menu_004604a0* menu, int value);
void __stdcall SetKeyboardInput(Sub_00460680* sub, int value);
void __stdcall SetKeyboardInput(Gui* sub, int value);

int __stdcall SetButtonStageByName(void* obj, char* name, int value);
int __stdcall SetButtonStageByName(Class_004a1080* obj, char* name, int value);
void __stdcall SetGadgetActiveByName(void* obj, char* name, int value);
void __stdcall SetGadgetActiveByName(Object_004a0570* obj, char* name, int value);
void __stdcall SetGadgetActiveByName(Menu_0045e5e0* menu, char* name, int value);
void __stdcall SetGadgetActiveByName(Gui* sub, char* name, int value);
void __stdcall SetGadgetGrayedOutByName(void* obj, char* name, int value);
void __stdcall SetGadgetGrayedOutByName(Object_004a1450* obj, char* name, int value);
void __stdcall SetTranslatedTextByName(void* obj, char* name, char* text, int value);
void __stdcall SetTranslatedTextByName(Menu_004604a0* menu, const char* name, int value, int count);
void __stdcall SetTranslatedTextByName(Sub_00460680* sub, const char* name, const char* text, int param_4);
void __stdcall SetTranslatedTextByName(Gui* sub, const char* name, const char* value, int flags);


int __stdcall GetButtonStageByName(void* obj, char* name);
int __stdcall GetButtonStageByName(Gui* gui, char* name);

void __stdcall PlaySoundByName(char* name, int value);
#include "../gui/dialogs.h"

void __stdcall SetGadgetStatusByName(void* obj, char* name, int value);
void __stdcall SetGadgetStatusByName(Menu_0045e5e0* menu, char* name, int value);
void __stdcall SetGadgetStatus(Gui* gui, int id, int flag);
void __stdcall SetButtonStage(Menu_0045e5e0* menu, int index, int value);
#include "../graphics/palette.h"
int __stdcall LockScreen(Surface_0045fbc0* out);
int __stdcall UnlockScreen(Surface_0045fbc0* s);

int __stdcall FindGadgetIndex(void* list, char* name, int flag);
int __stdcall FindGadgetIndex(Gadget* gadgets, const char* name, int flag);
int __stdcall FindGadgetIndex(void* entries, const char* name, int type);

void __stdcall LoadPictureCached(char* name, int a, int b, int c);
void __stdcall LoadPictureCached(const char* name, int a, int b, int c);
#include "../game/data_files.h"
void __stdcall AddTextGadget(Layer* layer, char* type, char* text, int x, int y,
                             int width, int attr);
#include "../util/registry.h"
char* __stdcall Translate(char* text);
const char* __stdcall Translate(const char* text);
#include "campaign_menu.h"
void __stdcall FreeBlinkWords(int param_1);
#include "../gui/file_list.h"

#include "options.h"
void __stdcall HandleVideoModeSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleGammaSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleMusicOptionsClick(int value);
void __stdcall HandleMusicVolumeSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleEffectsVolumeSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleSoundOptionsClick(void* obj, int arg);
void __stdcall HandleSpeedOptionsClick(void* obj, int arg);
void __stdcall HandleGameSpeedSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleScreenSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleMaxLinesSlider(Menu_0045b800* obj, int unused);
void __stdcall HandleTextScrollSlider(Menu_0045b800* obj, int unused);
#include "../game/savegame.h"

#include "../graphics/object_pool.h"
#include "../debug/debug_lib.h"
void __stdcall SortDisplayModes(List_0045e5e0* list);
int __stdcall GetDisplayModes(List_0045e5e0* list);
int FindHostSlot();
char* __cdecl _itoa(int value, char* buf, int radix);
int __stdcall SetCursorMode(int value);
#include "../game/cd_check.h"
int __stdcall IsGadgetNamed(int param1, int param2, char* name);
#include "../game/game_state.h"
#include "frontend.h"
void __stdcall FrameFromSurface(GafFrame* dst, Surface* src);
Surface* __stdcall AllocSurface(char* name, int width, int height);
void __stdcall DrawSurface(int a, void* surface, int b, int c);
void __stdcall DrawSurface(Surface* surface, int a, int b, int c);
void* __stdcall FindGafEntry(void* gaf, const char* name);
void* __stdcall GetGafFrame(void* a, int b);
void __stdcall DrawFrame(int a, void* b, int c, int d);
void __stdcall DrawFrameQuad(void* surf, void* entry, Quad_45ffb0* dst, Quad_45ffb0* src);

#include "options.h"
#include "frontend.h"
#include "../game/settings.h"

static inline int SliderValue(Gadget* e)
{
    if (e->range <= 1)
        return 0;
    return (int)((float)e->knobPos / (e->range - 1) * e->max);
}

static inline void ApplySound()
{
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
}

// FUNCTION: 0x45b800
void __stdcall SetDirectConnectAddress(char* param_1)
{
    g_cmdlineTcpJoinAddress[0] = 0;
    strncat(g_cmdlineTcpJoinAddress, param_1, 0x3f);
}

// FUNCTION: 0x45b820
void __stdcall SetHostGameName(int flag, char* text)
{
    g_cmdlineHostMode = flag != 0;
    if (text) {
        g_cmdlineHostGameName[0] = 0;
        strncat(g_cmdlineHostGameName, text, 0x3f);
    }
}

// FUNCTION: 0x45b860
void __stdcall SetDirectPlayProvider(int param_1)
{
    if (param_1 == 1 || param_1 == 2 || param_1 == 3 || param_1 == 4) {
        DAT_00512c80 = param_1;
    }
}

// FUNCTION: 0x45b880
void __stdcall SetGadgetsDisabledByPrefix(char* name, int value)
{
    for (int i = 0; i <= g_game->menu_519_1c.holder->entries->count; i++) {
        if (strncmp(g_game->menu_519_1c.holder->entries[i].name, name, strlen(name)) == 0 &&
            g_game->menu_519_1c.holder->entries[i].type == 1) {
            SetGrayedOut(&g_game->menu_519_1c, i, value);
        }
    }
    MarkChanged((Dialog *)&g_game->menu_519_1c);
}

// FUNCTION: 0x45b920
void __stdcall DeactivateGadgetsByPrefix(char* prefix)
{
    for (int i = 0; i <= g_game->menu_519_1c.holder->entries[0].count; i++) {
        if (strncmp(g_game->menu_519_1c.holder->entries[i].name, prefix, strlen(prefix)) == 0) {
            SetGadgetActiveByName(&g_game->menu_519_1c, g_game->menu_519_1c.holder->entries[i].name, 0);
        }
    }
}

// FUNCTION: 0x45b9b0
void __stdcall SetSliderFromValue(Gadget* param_1, int value)
{
    int max = param_1->max;
    if (value > max)
        value = max;
    float f = (float)value / (float)max * (param_1->range - 1);
    if (f - (int)f != 0.0f)
        f += 1.0;
    param_1->knobPos = (short)f;
}

// FUNCTION: 0x45ba20
int __stdcall ReadSliderValue(Gadget* param_1)
{
    if (param_1->range <= 1)
        return 0;
    return (int)((float)param_1->knobPos / (param_1->range - 1) * param_1->max);
}

// FUNCTION: 0x45ba60
void __stdcall SetGadgetStatusAndText(int index, int state, char* offText, char* onText)
{
    char name[128];
    Gadget* entries = g_game->menu_519_1c.holder->entries;
    SetGadgetStatus(&g_game->menu_519_1c, index, state);
    GetGadgetName(entries, name, index);
    Gadget* e = FindGadgetOrNull(entries, name);
    if (!state) {
        strcpy(e->text, offText);
    } else {
        strcpy(e->text, onText);
    }
}

// FUNCTION: 0x45baf0
void __stdcall ClearGadgetText(void* param_1, int param_2)
{
    *(char*)((char*)param_1 + 0xb6) = 0;
}

// FUNCTION: 0x45bb00
void __stdcall UpdateVideoModeLabel(Menu_0045b800* param_1, Gadget* param_2)
{
    int i = 0;
    int count = param_2->list->count;
    if (count > 0) {
        Game* g = g_game;
        int w = g->width_37f1b;
        Mode_0045b800* m = param_2->list->modes;
        do {
            if (w != m->width)
                goto next;
            if (g->height_37f1f != m->height)
                goto next;
            {
                int max = param_2->max;
                int n = i;
                if (n > max)
                    n = max;
                float f = (float)n / (float)max * (param_2->range - 1);
                if (f - (int)f != 0.0f)
                    f += 1.0;
                param_2->knobPos = (short)f;

                Gadget* e = (Gadget*)FindGadgetChecked_C(param_1->holder->entries, "VIDVAL");
                if (e)
                    sprintf(e->text, "%d X %d", m->width, m->height);
                break;
            }
next:
            i++;
            m++;
        } while (i < count);    }
}

// FUNCTION: 0x45bbf0
void __stdcall HandleVideoModeSlider(Menu_0045b800* obj, int unused)
{
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "VIDSLDR");
    ModeList_0045b800* list = e->list;
    if (e != 0) {
        Mode_0045b800* r = &list->modes[SliderValue(e)];
        sprintf(FindGadgetChecked_C(obj->holder->entries, "VIDVAL") + 0xb6, "%d X %d", r->width, r->height);
        g_game->width_37f1b = r->width;
        g_game->height_37f1f = r->height;
    }
    MarkChanged((Dialog *)&g_game->menu_519_1c);
}

// FUNCTION: 0x45bcc0
void ApplyBrightnessAndVolume()
{
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
}

// FUNCTION: 0x45bd20
void __stdcall HandleGammaSlider(Menu_0045b800* obj, int unused)
{
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "GAMMA");
    if (e != 0) {
        g_game->brightness = SliderValue(e);
        ApplySound();
    }
}

// FUNCTION: 0x45bde0
void __stdcall HandleEffectsVolumeSlider(Menu_0045b800* obj, int unused)
{
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "FXVOL");
    if (e != 0) {
        g_game->volume1 = SliderValue(e);
        ApplySound();
    }
}

// FUNCTION: 0x45bea0
void __stdcall HandleMusicVolumeSlider(Menu_0045b800* obj, int unused)
{
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "MUSICVOL");
    if (e != 0) {
        g_game->volume2 = SliderValue(e);
        ApplySound();
    }
}

// FUNCTION: 0x45bf60
void __stdcall SetGameSpeedLabel(char* name, char* label, int speed, int normal)
{
    char buf[200];
    char* text;
    if (speed == normal) {
        text = "Normal";
    } else if (speed < normal / 4) {
        text = "Slow";
    } else if (speed < normal / 2) {
        text = "Slower";
    } else if (speed > normal * 3 / 4) {
        text = "Fast";
    } else {
        text = "Faster";
    }
    sprintf(buf, "%s (%s)", label, text);
    SetTranslatedTextByName(&g_game->menu_519_1c, name, label, 0);
}

// FUNCTION: 0x45c010
void __stdcall SetGadgetTextFromValueTable(Entry_0045c010* table, char* name, int value)
{
    char buf[100];
    if (table->format == 0)
        return;
    while (table->format != 0) {
        if (value <= table->min) {
            sprintf(buf, table->format, value);
            SetTranslatedTextByName(&g_game->menu_519_1c, name, buf, 0);
            return;
        }
        table++;
    }
}

// FUNCTION: 0x45c070
void __stdcall HandleGameSpeedSlider(Menu_0045b800* obj, int unused)
{
    Player* player = &g_game->players[g_game->localPlayer];
    if (player->active == 0 || !((Opts_0045f1d0*)player->info.get())->u.flag_9b_6) {
        Gadget* e = FindGadgetChecked_D(obj->holder->entries, "GAME");
        if (e != 0) {
            int value = SliderValue(e);
            g_game->gameSpeed = (unsigned short)(value < 1 ? 1 : SliderValue(e));
            SetGameSpeed(g_game->gameSpeed, 1);
            MarkChanged((Dialog *)obj);
        }
    }
}

// FUNCTION: 0x45c170
void __stdcall HandleScreenSlider(Menu_0045b800* obj, int unused)
{
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "SCREEN");
    if (e != 0) {
        g_game->scrollSpeed = SliderValue(e) > 1 ? SliderValue(e) : 1;
        MarkChanged((Dialog *)obj);
    }
}

// FUNCTION: 0x45c220
void __stdcall HandleMaxLinesSlider(Menu_0045b800* obj, int unused)
{
    char text[20];
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "MAXLINES");
    if (e != 0) {
        int v = SliderValue(e);
        g_game->textLines = v < 0 ? 0 : SliderValue(e);
        MarkChanged((Dialog *)obj);
    }
    if (g_game->textLines != 0)
        sprintf(text, "%d", g_game->textLines);
    else
        strcpy(text, "None");
    SetTranslatedTextByName(obj, "MAXLINESTEXT", text, 0);
}

// FUNCTION: 0x45c330
void __stdcall HandleTextScrollSlider(Menu_0045b800* obj, int unused)
{
    char text[20];
    Gadget* e = FindGadgetChecked_D(obj->holder->entries, "TXTSCROL");
    if (e != 0) {
        g_game->textScroll = SliderValue(e);
        sprintf(text, "%d secs", g_game->textScroll);
        SetTranslatedTextByName(obj, "TEXTSCROLLTEXT", text, 0);
        MarkChanged((Dialog *)obj);
    }
}

// FUNCTION: 0x45c3d0
void __stdcall SetLockedCdTrack(int value)
{
    ((Sound *)g_game->sound)->SetLockedTrack(value);
}

// FUNCTION: 0x45c3f0
void UpdateTrackGadgets()
{
    char buf[12];
    Menu_0045b800* menu = &g_game->menu_519_1c;

    if (FindGadgetIndex(menu->holder->entries, "TRACKTYPE", 1) != -1) {
        int disc = g_musicUiSelectedTrack;
        SetGrayedOutByName(menu, "TRACKTYPE",
                     (((char)g_game->musicMode & 1) && g_game->cdMode == 4) ? 0 : 1);
        SetButtonStageByName(menu, "TRACKTYPE", ((Sound *)g_game->sound)->GetCategoryOfTrack(disc));
        if (disc == 0)
            strcpy(buf, "NO DISC");
        else
            sprintf(buf, "%d", disc);
        SetTranslatedTextByName(menu, "TRACKNUM", buf, 0);
    }
    SetGadgetGrayedOutByName(menu, "TRACKNUM", (char)(~(char)g_game->musicMode) & 1);
    if (g_game->cdMode == 3) {
        int track = g_musicUiSelectedTrack;
        ((Sound *)g_game->sound)->SetLockedTrack(track);
    }
}

// FUNCTION: 0x45c510
void ApplyTrackType()
{
    Gadget* gadgets = g_game->menu_519_1c.holder->entries;
    if (g_game->cdMode == 4) {
        int index = FindGadgetIndex(gadgets, "TRACKTYPE", 1);
        ((Sound *)g_game->sound)->SetCategoryOfTrack(g_musicUiSelectedTrack, gadgets[index].stageIndex);
    }
}

// FUNCTION: 0x45c570
void ApplyDefaultSoundOptions()
{
    g_game->volume1 = 0x1b;
    (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.b4 = 1;
    (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.b5 = 1;
    (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.speech = 1;
    ((Sound *)g_game->sound)->Disable3D();
    (*(SoundFlags_0045cf60*)&g_game->soundFlags).word = ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 0xfff9) | 1;
    g_game->unitChat = 10;
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
}

// FUNCTION: 0x45c630
void ApplyDefaultMusicOptions()
{
    g_game->volume2 = 0x20;
    g_game->cdMode = 4;
    if (!(g_game->flags_37f14 & 1)) {
        g_game->flags_37f14 |= 1;
        ((Sound *)g_game->sound)->PlayNextTrack();
    }
    ApplyBrightnessAndVolume();
}

// FUNCTION: 0x45c6d0
void ApplyDefaultUiOptions()
{
    g_game->textScroll = 10;
    g_game->textLines = 10;
    g_game->gameSpeed = 10;
    g_game->effectiveGameSpeed = 10;
    g_game->scrollSpeed = 0x20;
    g_game->interfaceType = 0;
    g_game->unitChat = 10;
    g_game->unitChatText = 5;
}

// FUNCTION: 0x45c740
void ApplyDefaultVisualOptions()
{
    g_game->bits_37f06.bit1 = 1;
    g_game->bits_37f06.bit2 = 1;
    g_game->bits_37f06.bit3 = 1;
    g_game->bits_37f06.bit4 = 1;
    g_game->bits_37f06.bit5 = 1;
    g_game->brightness = 12;
    if (!g_game->bits_2a44_1.bit2) {
        g_game->width_37f1b = 640;
        g_game->height_37f1f = 480;
        g_game->bits_37f06.bit6 = 0;
    }
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
}

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

// FUNCTION: 0x45c950
void LoadSavedAudioSettings()
{
    g_game->volume2 = g_optionsBackupMusicVolume;
    ((Sound *)g_game->sound)->CopyTrackTypeTable(&g_optionsBackupTrackTypes);
    g_game->cdMode = g_optionsBackupCdMode;
    ((Sound *)g_game->sound)->SetPlaybackOrder(g_game->cdMode);
    if (((unsigned char)g_game->flags_37f14 ^ (unsigned char)g_optionsBackupMusicMode.i) & 1) {
        ((Sound *)g_game->sound)->PlayNextTrack();
    }
    unsigned short f = g_game->flags_37f14;
    f = f ^ ((f ^ g_optionsBackupMusicMode.i) & 1);
    g_game->flags_37f14 = f;
    ((Sound *)g_game->sound)->SetLockedTrack(g_optionsBackupLockedTrack);
    ApplyBrightnessAndVolume();
}

// FUNCTION: 0x45ca50
void RestoreUiOptions()
{
    g_game->textScroll = g_optionsBackupTextScroll;
    g_game->gameSpeed = g_optionsBackupGameSpeed.i;
    g_game->effectiveGameSpeed = g_optionsBackupGameSpeed.i;
    g_game->scrollSpeed = g_optionsBackupEdgeScroll.i;
    g_game->interfaceType = g_optionsBackupInterfaceType;
    g_game->unitChat = g_optionsBackupUnitChat;
    g_game->unitChatText = g_optionsBackupUnitChatText;
    g_game->textLines = g_optionsBackupTextLines;
}

// FUNCTION: 0x45cae0
void RestoreVisualOptions()
{
    unsigned short v = g_game->visualFlags;
    g_game->visualFlags = v ^ ((v ^ g_optionsBackupVisualFlags) & 2);
    v = g_game->visualFlags;
    g_game->visualFlags = v ^ ((v ^ g_optionsBackupVisualFlags) & 4);
    v = g_game->visualFlags;
    g_game->visualFlags = v ^ ((v ^ g_optionsBackupVisualFlags) & 8);
    v = g_game->visualFlags;
    g_game->visualFlags = v ^ ((v ^ g_optionsBackupVisualFlags) & 0x10);
    v = g_game->visualFlags;
    g_game->visualFlags = v ^ ((v ^ g_optionsBackupVisualFlags) & 0x20);
    v = g_game->visualFlags;
    g_game->visualFlags = v ^ ((v ^ g_optionsBackupVisualFlags) & 0x40);

    g_game->brightness = g_optionsBackupGamma;
    if (!(g_game->netGameFlags & 4)) {
        g_game->width_37f1b = g_optionsBackupDisplayWidth;
        g_game->height_37f1f = g_optionsBackupDisplayHeight;
    }

    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
}

// FUNCTION: 0x45cc50
void LoadSavedSettings()
{
    RestoreSoundOptions();
    g_game->volume2 = g_optionsBackupMusicVolume;
    ((Sound *)g_game->sound)->CopyTrackTypeTable(&g_optionsBackupTrackTypes);
    g_game->cdMode = g_optionsBackupCdMode;
    ((Sound *)g_game->sound)->SetPlaybackOrder(g_game->cdMode);
    if (((unsigned char)g_game->flags_37f14 ^ (unsigned char)g_optionsBackupMusicMode.i) & 1) {
        ((Sound *)g_game->sound)->PlayNextTrack();
    }
    unsigned short f = g_game->flags_37f14;
    g_game->flags_37f14 = f ^ ((f ^ g_optionsBackupMusicMode.i) & 1);
    ((Sound *)g_game->sound)->SetLockedTrack(g_optionsBackupLockedTrack);
    SetBrightness(0.5 - g_game->brightness * -0.041666668f);
    ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
    ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
    g_game->textScroll = g_optionsBackupTextScroll;
    g_game->gameSpeed = g_optionsBackupGameSpeed.i;
    g_game->effectiveGameSpeed = g_optionsBackupGameSpeed.i;
    g_game->scrollSpeed = g_optionsBackupEdgeScroll.i;
    g_game->interfaceType = g_optionsBackupInterfaceType;
    g_game->unitChat = g_optionsBackupUnitChat;
    g_game->unitChatText = g_optionsBackupUnitChatText;
    g_game->textLines = g_optionsBackupTextLines;
    RestoreVisualOptions();
}

// FUNCTION: 0x45cde0
void SaveGameSettings()
{
    memcpy(g_optionsPrefsSnapshot.block, (char*)&g_game->block[0], 0x53);
    g_optionsPrefsSnapshot.bit0 = (*(Los_0045cf60*)&g_game->mapFlags_14281).bits.bit1;
    g_optionsPrefsSnapshot.bit1 = (*(Los_0045cf60*)&g_game->mapFlags_14281).bits.bit2;
    g_optionsBackupGameSpeed.i = g_game->gameSpeed;
    g_optionsBackupEdgeScroll.i = g_game->scrollSpeed;
    g_optionsBackupLockedTrack = ((Sound *)g_game->sound)->GetLockedTrack();
    for (int i = 0; i < 100; i++) {
        g_optionsBackupTrackTypes[i] = ((Sound *)g_game->sound)->GetCategoryOfTrack(i);
    }
}

// FUNCTION: 0x45ce80
void EnsureOptionsPanelGadget()
{
    Gadget* entries = g_game->menu_519_1c.holder->entries;
    int index = FindGadgetIndex(entries, "PANEL", 0xe);
    if (g_game->ordersPanelFlags & 1) {
        entries->width += 0x96;
        if (index == -1) {
            short c = entries->count;
            int i = c;
            i++;
            c++;
            entries->count = c;
            memset(&entries[i], 0, sizeof(Gadget));
            entries[i].type = 0xb;
            entries[i].x = 0x80;
            entries[i].width = entries->width;
            entries[i].y = 0;
            entries[i].width -= entries[i].x;
            entries[i].height = entries->height;
            strcpy(entries[i].name, "PANEL");
            entries[i].active = 1;
            entries[i].background = entries->background;
        }
    }
}

// FUNCTION: 0x45cf60
void BlitOptionsPanel()
{
    if ((*(Bits_0045cf60*)&g_game->netGameFlags_2a44).bits.bit2) {
        return;
    }
    RenderLayer((Gui *)((Sub_0045cf60*)&g_game->gui), 0x40);
    MarkChanged((Dialog *)((Sub_0045cf60*)&g_game->gui));
    BlitMenuLayers((Sub_0045cf60*)&g_game->gui, 0, 0);
    SetOffscreenSurface((void *)g_game->screen);
    FlipScreen();
}

#pragma auto_inline(off)
// FUNCTION: 0x45cfc0
int __cdecl OpenOptionsLayout()
{
    char buf[256];
    if ((*(Bits_0045cf60*)&g_game->netGameFlags_2a44).prefsWord.prefs) {
        strcpy(buf, "PREFS.GUI");
        (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).loadedBits.loaded |= 1;
    } else {
        strcpy(buf, "STARTOPT.GUI");
    }
    int result = LoadGuiLayer((void*)&g_game->gui, buf, 0x80);
    SetGrayedOutByName(&g_game->gui, "MUSIC", *(int*)(Sound *)g_game->sound == 0);
    EnableKeyCommands((Dialog *)&g_game->gui);
    if ((*(Bits_0045cf60*)&g_game->netGameFlags_2a44).prefsWord.prefs && g_game->net->GetGameType() != 3) {
        g_game->pauseFlags |= 1;
    }
    return result;
}
#pragma auto_inline(on)

// FUNCTION: 0x45d0c0
void TickMusicOptions()
{
    char value[20];
    GetGadgetText(&g_game->gui, "TRACKNUM", value);
    int track = atoi(value);
    if (track != ((Sound *)g_game->sound)->GetCurrentTrack()) {
        g_musicUiSelectedTrack = ((Sound *)g_game->sound)->GetCurrentTrack();
        UpdateTrackGadgets();
        MarkChanged((Dialog*)&g_game->gui);
    }
}

// FUNCTION: 0x45d130
void UpdateMusicGadgets()
{
    SetButtonStageByName((Class_004a1080*)&g_game->gui, "NOTRAK", g_game->flags_37f14_37f14.notrak & 1);
    SetButtonStageByName((Class_004a1080*)&g_game->gui, "TRACKMODE", g_game->cdMode - 1);
    SetGadgetGrayedOutByName((Object_004a1450*)&g_game->gui, "MUSICVOL", (char)(~g_game->flags_37f14_37f14.notrak & 1));
    SetGrayedOutByName((Dialog*)&g_game->gui, "CDPREV", (char)(~g_game->flags_37f14_37f14.notrak & 1));
    SetGrayedOutByName((Dialog*)&g_game->gui, "CDSTOP", (char)(~g_game->flags_37f14_37f14.notrak & 1));
    SetGrayedOutByName((Dialog*)&g_game->gui, "CDPLAY", (char)(~g_game->flags_37f14_37f14.notrak & 1));
    SetGrayedOutByName((Dialog*)&g_game->gui, "CDNEXT", (char)(~g_game->flags_37f14_37f14.notrak & 1));
    SetGrayedOutByName((Dialog*)&g_game->gui, "TRACKMODE", (char)(~g_game->flags_37f14_37f14.notrak & 1));
    SetGrayedOutByName((Dialog*)&g_game->gui, "TRACKTYPE", !((g_game->flags_37f14_37f14.notrak & 1) && g_game->cdMode == 4));
}

// FUNCTION: 0x45d280
void __stdcall HandleMusicOptionsClick(Object_0045d280* obj)
{
    Gadget* entries = obj->holder->entries;
    if (obj->hotGadgetIndex == -1) {
        if (!(*(Bits_0045cf60*)&g_game->netGameFlags_2a44).prefsByte.prefs) {
            ((Sound *)g_game->sound)->StopCdAudio();
            (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).loadedBits.loaded = 0;
            return;
        }
        ((Sound *)g_game->sound)->PlayNextTrack();
        (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).loadedBits.loaded = 0;
        return;
    }
    MarkChanged((Dialog *)obj);
    int notrak = IsCurrentGadgetNamed((Dialog *)obj, g_notrakGadgetName);
    if (notrak != 0) {
        PlaySoundByName(g_optionsSoundName, 0);
        int v = GetButtonStageByName(obj, g_notrakGadgetName);
        unsigned short f = g_game->flags_37f14_37f14.word;
        g_game->flags_37f14_37f14.word = f ^ ((f ^ v) & 1);
        ((Sound *)g_game->sound)->EnableCdAudio(g_game->flags_37f14_37f14.word & 1);
        ClearSelectedGadget(obj);
        UpdateMusicGadgets();
    } else if (IsCurrentGadgetNamed((Dialog *)obj, g_trackModeGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        g_game->cdMode = GetButtonStageByName(obj, g_trackModeGadgetName) + 1;
        ((Sound *)g_game->sound)->SetPlaybackOrder(g_game->cdMode);
        if (g_game->cdMode == 3) {
            g_musicUiSelectedTrack = ((Sound *)g_game->sound)->GetLockedTrack();
            ClearSelectedGadget(obj);
            UpdateTrackGadgets();
            return;
        }
        if (g_game->cdMode == 4) {
            SetButtonStageByName(obj, g_trackTypeGadgetName, (unsigned char)((Sound *)g_game->sound)->GetCategoryOfTrack(g_musicUiSelectedTrack));
            ApplyTrackType();
        }
        ClearSelectedGadget(obj);
        UpdateTrackGadgets();
        return;
    } else if (IsCurrentGadgetNamed((Dialog *)obj, g_trackTypeGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        int i = obj->hotGadgetIndex;
        ((Sound *)g_game->sound)->SetCategoryOfTrack(g_musicUiSelectedTrack, entries[i].stageIndex);
        UpdateTrackGadgets();
        ClearSelectedGadget(obj);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)obj, g_cdPlayGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        ((Sound *)g_game->sound)->PlayCdTrack(g_musicUiSelectedTrack, 1);
        ClearSelectedGadget(obj);
        return;
    } else if (IsCurrentGadgetNamed((Dialog *)obj, g_cdNextGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        g_musicUiSelectedTrack = g_musicUiSelectedTrack + 1;
        int n = ((Sound *)g_game->sound)->GetTrackCount();
        if (g_musicUiSelectedTrack > n)
            g_musicUiSelectedTrack = 1;
        g_musicUiSelectedTrack = ((Sound *)g_game->sound)->SelectTrack(g_musicUiSelectedTrack);
        UpdateTrackGadgets();
        ClearSelectedGadget(obj);
        return;
    } else if (IsCurrentGadgetNamed((Dialog *)obj, g_cdPrevGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        g_musicUiSelectedTrack = g_musicUiSelectedTrack - 1;
        if (g_musicUiSelectedTrack < 1)
            g_musicUiSelectedTrack = ((Sound *)g_game->sound)->GetTrackCount();
        g_musicUiSelectedTrack = ((Sound *)g_game->sound)->SelectTrack(g_musicUiSelectedTrack);
        UpdateTrackGadgets();
        ClearSelectedGadget(obj);
        return;
    } else if (IsCurrentGadgetNamed((Dialog *)obj, g_cdStopGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        ((Sound *)g_game->sound)->StopCdAudio();
        g_musicUiSelectedTrack = ((Sound *)g_game->sound)->SelectTrack(1);
        UpdateTrackGadgets();
        ClearSelectedGadget(obj);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)obj, g_undoGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        LoadSavedAudioSettings();
        CloseTopScreen((Gui *)obj);
        OpenMusicOptions();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)obj, g_restoreGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        ApplyDefaultMusicOptions();
        CloseTopScreen((Gui *)obj);
        OpenMusicOptions();
        return;
    }
    int save = obj->hotGadgetIndex;
    if (obj->hotGadgetIndex != -1) {
        if (entries[obj->hotGadgetIndex].state != 1) {
            ClearSelectedGadget(obj);
            return;
        }
        Vtable_0045d280* p = obj->holder->next;
        CloseTopScreen((Gui *)obj);
        obj->hotGadgetIndex = save;
        p->FUN_8(obj);
    }
}

// FUNCTION: 0x45d7c0
void OpenMusicOptions()
{
    Object_0045d7c0* obj = (Object_0045d7c0*)OpenOptionsLayout();
    RenderLayer(&g_game->gui, 2);
    EnsureOptionsPanelGadget();
    if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).byte & 1) {
        LoadGuiLayer(&g_game->gui, "MUSICRT.GUI", 0x280);
    } else {
        LoadGuiLayer(&g_game->gui, "MUSIC", 0x200);
        LoadPictureCached("optmusic4x", 0, 0, 0);
    }
    obj->callback8 = HandleMusicOptionsClick;
    EnableKeyCommands((Dialog *)&g_game->gui);
    obj->callback1c = TickMusicOptions;
    SetGadgetStatusByName(&g_game->gui, "MUSIC", 1);
    if (FindGadgetIndex(obj->gadgets, "MUSICVOL", 0xe) != -1) {
        Gadget* e = FindGadgetChecked_D(obj->gadgets, "MUSICVOL");
        e->max = 0x40;
        e->sliderCallback = HandleMusicVolumeSlider;
        e->knobPos = g_game->volume2Word;
        int value = e->knobPos;
        if (value > 0x40) {
            value = 0x40;
        }
        float f = (float)value * (float)(e->range - 1) * 0.015625f;
        if (f - (int)f != 0.0f) {
            f += 1.0;
        }
        e->knobPos = (short)f;
    }
    UpdateMusicGadgets();
    if (g_game->cdMode == 3) {
        g_musicUiSelectedTrack = ((Sound *)g_game->sound)->GetLockedTrack();
    }
    UpdateTrackGadgets();
    Gadget* gadgets = ((Holder_0045d7c0*)g_game->gui.layer)->gadgets;
    if (g_game->cdMode == 4) {
        int index = FindGadgetIndex(gadgets, "TRACKTYPE", 1);
        ((Sound *)g_game->sound)->SetCategoryOfTrack(g_musicUiSelectedTrack, gadgets[index].stageIndex);
    }
    MarkChanged((Dialog *)&g_game->gui);
    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x45d9d0
void UpdateSoundGadgets()
{
    SetButtonStageByName((Class_004a1080*)&g_game->gui, g_modeGadgetName, (*(SoundFlags_0045cf60*)&g_game->soundFlags).byte & 7);
    SetGadgetActiveByName((Object_004a0570*)&g_game->gui, g_volTextGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).byte & 7) != 0);
    SetGadgetGrayedOutByName((Object_004a1450*)&g_game->gui, g_fxVolGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).byte & 7) == 0);
    SetGadgetGrayedOutByName((Object_004a1450*)&g_game->gui, g_testGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).byte & 7) == 0);
    SetGadgetGrayedOutByName((Object_004a1450*)&g_game->gui, g_speechGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).byte & 7) == 0);
}

// FUNCTION: 0x45da90
void __stdcall HandleSoundOptionsClick(Object_0045da90* obj)
{
    Gadget* entries = obj->holder->entries;
    if (obj->hotGadgetIndex == -1) {
        (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).loadedBits.loaded = 0;
        return;
    }
    MarkChanged((Dialog *)obj);
    int mode = obj->hotGadgetIndex;
    if (IsCurrentGadgetNamed((Dialog *)obj, g_speechGadgetName)) {
        PlaySoundByName(g_optionsSoundName, 0);
        (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.speech = entries[obj->hotGadgetIndex].stageIndex != 0;
        g_game->unitChat = entries[obj->hotGadgetIndex].stageIndex * 5;
        ClearSelectedGadget(obj);
    } else if (IsCurrentGadgetNamed((Dialog *)obj, g_modeGadgetName)) {
        int v = GetButtonStageByName(obj, g_modeGadgetName);
        unsigned short f = (*(SoundFlags_0045cf60*)&g_game->soundFlags).word;
        (*(SoundFlags_0045cf60*)&g_game->soundFlags).word = f ^ ((f ^ v) & 7);
        if (((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0)
            StopAllSounds();
        if (((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 2)
            ((Sound *)g_game->sound)->Enable3D();
        else
            ((Sound *)g_game->sound)->Disable3D();
        if (((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 1 && !(*(Bits_0045cf60*)&g_game->netGameFlags_2a44).prefsWord.prefs)
            PlayLoopingSoundByName(g_bgmSoundName, 0);
        SetButtonStageByName(&g_game->gui, g_modeGadgetName, (*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7);
        SetGadgetActiveByName(&g_game->gui, g_volTextGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) != 0);
        SetGadgetGrayedOutByName(&g_game->gui, g_fxVolGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0);
        SetGadgetGrayedOutByName(&g_game->gui, g_testGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0);
        SetGadgetGrayedOutByName(&g_game->gui, g_speechGadgetName, ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0);
        ClearSelectedGadget(obj);
        PlaySoundByName(g_optionsSoundName, 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)obj, g_undoGadgetName)) {
        RestoreSoundOptions();
        CloseTopScreen((Gui *)obj);
        OpenSoundOptions();
        PlaySoundByName(g_optionsSoundName, 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)obj, g_restoreGadgetName)) {
        g_game->volume1 = 0x1b;
        (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.b4 = 1;
        (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.b5 = 1;
        (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.speech = 1;
        ((Sound *)g_game->sound)->Disable3D();
        (*(SoundFlags_0045cf60*)&g_game->soundFlags).word = ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 0xfff9) | 1;
        g_game->unitChat = 10;
        SetBrightness(0.5 - g_game->brightness * -0.041666668f);
        ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
        ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
        CloseTopScreen((Gui *)obj);
        OpenSoundOptions();
        PlaySoundByName(g_optionsSoundName, 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)obj, g_testGadgetName)) {
        PlaySoundFile(g_explodeSoundFile);
        ClearSelectedGadget(obj);
        return;
    }
    if (obj->hotGadgetIndex != -1) {
        if (entries[mode].type != 1) {
            ClearSelectedGadget(obj);
            return;
        }
        Vtable_0045da90* p = obj->holder->next;
        CloseTopScreen((Gui *)obj);
        obj->hotGadgetIndex = mode;
        p->FUN_8(obj);
    }
}

// FUNCTION: 0x45de30
void OpenSoundOptions()
{
    Object_0045de30* obj = (Object_0045de30*)OpenOptionsLayout();
    RenderLayer(&g_game->gui, 2);
    EnsureOptionsPanelGadget();
    if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).byte & 1) {
        LoadGuiLayer(&g_game->gui, "SOUNDSRT.GUI", 0x200);
    } else {
        LoadGuiLayer(&g_game->gui, "SOUNDS", 0x200);
        LoadPictureCached("optsound4x", 0, 0, 0);
    }
    Gadget* entries = obj->entries;
    obj->fn = HandleSoundOptionsClick;
    EnableKeyCommands((Dialog *)&g_game->gui);
    SetGadgetStatusByName(&g_game->gui, "SOUND", 1);
    int found = FindGadgetIndex(obj->entries, "FXVOL", 0xe);
    if (found != -1) {
        Gadget* e = FindGadgetChecked_D(entries, "FXVOL");
        e->max = 0x40;
        e->sliderCallback = HandleEffectsVolumeSlider;
        e->knobPos = g_game->volume1Word;
        int value = e->knobPos;
        if (value > 0x40) {
            value = 0x40;
        }
        float f = (float)value * (float)(e->range - 1) * 0.015625f;
        if (f - (int)f != 0.0f) {
            f += 1.0;
        }
        e->knobPos = (short)f;
    }
    for (int i = 1; i <= obj->entries->count; i++) {
        if (obj->entries[i].type == 4)
            obj->entries[i].sliderCallback((Menu_0045b800*)&g_game->gui, 0);
    }
    SetButtonStageByName(&g_game->gui, "SPEECH", (*(SoundFlags_0045cf60*)&g_game->soundFlags).bits.speech ? g_game->unitChat / 5 : 0);
    SetButtonStageByName(&g_game->gui, "MODE", (*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7);
    SetGadgetActiveByName(&g_game->gui, "VOLTEXT", ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) != 0);
    SetGadgetGrayedOutByName(&g_game->gui, "FXVOL", ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0);
    SetGadgetGrayedOutByName(&g_game->gui, "TEST", ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0);
    SetGadgetGrayedOutByName(&g_game->gui, "SPEECH", ((*(SoundFlags_0045cf60*)&g_game->soundFlags).word & 7) == 0);
    MarkChanged((Dialog *)&g_game->gui);
    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x45e100
void __stdcall HandleVisualOptionsClick(Gui* gui)
{
    int save = gui->hotGadgetIndex;
    Screen_0045e100* top = (Screen_0045e100*)gui->layer;
    Gadget* entries = top->entries;
    FreeObj_0045e100* obj = (FreeObj_0045e100*)top->userData;

    if (gui->hotGadgetIndex == -1) {
        if (obj) {
            if (!(*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).bits.b0) {
                GameFreeThunk(obj->field_14);
                GameFreeThunk(obj->field_4);
            }
            GameFreeThunk(obj);
            gui->layer->data = 0;
        }
        (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).bits.b0 = 0;
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)gui, "ANTI")) {
        PlaySoundByName("Options", 0);
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b1 = GetButtonStageByName(gui, "ANTI") & 1;
        if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).bits.b0)
            ((CMemoryCache *)g_game->unitTable)->FlushCache();
        MarkChanged((Dialog *)gui);
        ClearSelectedGadget(gui);
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)gui, "BSHADOWS")) {
        PlaySoundByName("Options", 0);
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b4 = GetButtonStageByName(gui, "BSHADOWS") & 1;
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b3 = (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b4;
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b2 = (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b3;
        if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).bits.b0)
            ((CMemoryCache *)g_game->unitTable)->FlushCache();
        MarkChanged((Dialog *)gui);
        ClearSelectedGadget(gui);
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)gui, "SHADING")) {
        PlaySoundByName("Options", 0);
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b5 = GetButtonStageByName(gui, "SHADING") & 1;
        if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).bits.b0)
            ((CMemoryCache *)g_game->unitTable)->FlushCache();
        MarkChanged((Dialog *)gui);
        ClearSelectedGadget(gui);
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)gui, "UNDO")) {
        PlaySoundByName("Options", 0);
        RestoreVisualOptions();
        CloseTopScreen(gui);
        OpenVisualOptions(0);
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)gui, "RESTORE")) {
        PlaySoundByName("Options", 0);
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b1 = 1;
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b2 = 1;
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b3 = 1;
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b4 = 1;
        (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b5 = 1;
        g_game->brightness = 12;
        if (!(*(Bits_0045cf60*)&g_game->netGameFlags_2a44).flag.flag) {
            g_game->width_37f1b = 640;
            g_game->height_37f1f = 480;
            (*(Flags37f06_0045cf60*)&g_game->flags_37f06).bits.b6 = 0;
        }
        SetBrightness(0.5 - g_game->brightness * -0.041666668f);
        ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
        ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
        CloseTopScreen(gui);
        OpenVisualOptions(0);
        return;
    }

    if (IsCurrentGadgetNamed((Dialog *)gui, "OK") && IsScreenNamed(gui, "selvmode.gui")) {
        PlaySoundByName("Options", 0);
        return;
    }

    if (gui->hotGadgetIndex != -1) {
        if (entries[gui->hotGadgetIndex].state != 1) {
            ClearSelectedGadget(gui);
            return;
        }
        Screen_0045e100* next = (Screen_0045e100*)gui->layer->next;
        CloseTopScreen(gui);
        gui->hotGadgetIndex = save;
        next->handler(gui);
    }
}

// FUNCTION: 0x45e4c0
void __stdcall SortDisplayModes(ModeList_0045e4c0* list)
{
    int j, i;
    for (i = 0; i < list->count; i++) {
        for (j = list->count - 1; j > i; j--) {
            if (list->modes[j].width < list->modes[i].width
                || (list->modes[j].width == list->modes[i].width
                    && list->modes[j].height < list->modes[i].height)) {
                Mode_0045e4c0 tmp = list->modes[i];
                list->modes[i] = list->modes[j];
                list->modes[j] = tmp;
            }
        }
    }
    for (i = 0; i < list->count; i++) {
        if (list->modes[i].width < 640 || list->modes[i].height < 480) {
            if (i < list->count - 1) {
                for (j = i; j < list->count - 1; j++)
                    list->modes[j] = list->modes[j + 1];
            }
            i--;
            list->count--;
        }
    }
}

// FUNCTION: 0x45e5e0
void __stdcall OpenVisualOptions(int param_1)
{
    int i = 0;
    Layer* layer;
    Menu_0045e5e0* menu;

    if (param_1 != i) {
        layer = LoadGuiLayer((Menu_0045e5e0*)&g_game->gui, "SELVMODE.GUI", 0x800);
    } else {
        layer = (Layer*)OpenOptionsLayout();
        RenderLayer(&g_game->gui, 2);
        EnsureOptionsPanelGadget();
        if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).byte & 1) {
            LoadGuiLayer(&g_game->gui, "VISUALRT.GUI", 0x200);
        } else {
            LoadGuiLayer(&g_game->gui, "VISUALS.GUI", 0x200);
            LoadPictureCached("optvisual4x", i, i, i);
        }
    }
    EnableKeyCommands((Dialog *)&g_game->gui);
    layer->handler = (void (__stdcall *)(void *))HandleVisualOptionsClick;

    if (!((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).byte & 1)) {
        List_0045e5e0* list = (List_0045e5e0*)GameAllocIgnoreTag("SELECT VIDEO MODE", 0x20);
        layer->data = list;
        list->modes = (Mode_0045e4c0*)GameAllocIgnoreTag("DISPLAY MODES", 0x4b0);
        if (GetDisplayModes(list) != 0) {
            SortDisplayModes(list);
            list->buffer = (char*)GameAllocIgnoreTag("AVAILABLE MODES", list->count << 8);
            list->buffer[0] = 0;
            if (FindGadgetIndex(layer->entries, "VIDSLDR", 0xe) != -1) {
                Gadget* e = FindGadgetChecked_D(layer->entries, "VIDSLDR");
                e->max = list->count - 1;
                e->sliderCallback = HandleVideoModeSlider;
                e->sliderUser = list;
                menu = (Menu_0045e5e0*)&g_game->gui;
                for (int j = 0; j < list->count; j++) {
                    int w = g_game->width_37f1b;
                    Mode_0045e4c0* mode = &list->modes[j];
                    if (w == mode->width && g_game->height_37f1f == mode->height) {
                        int max = e->max;
                        int value = j;
                        if (value > max)
                            value = max;
                        float f = (float)value / (float)max * (float)(e->range - 1);
                        if (f - (int)f != 0.0f)
                            f += 1.0;
                        e->knobPos = (short)f;
                        char* p = FindGadgetChecked_C(menu->holder->entries, "VIDVAL");
                        if (p != 0) {
                            sprintf(p + 0xb6, "%d X %d", mode->width, mode->height);
                        }
                        break;
                    }
                }
            }
        }
    } else {
        layer->data = (void*)i;
        if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).byte & 1) {
            for (i = 0; i <= ((Holder_0045e5e0*)g_game->gui.layer)->entries->count; i++) {
                if (strncmp(((Holder_0045e5e0*)g_game->gui.layer)->entries[i].name, "MAP", strlen("MAP")) == 0) {
                    SetGadgetActiveByName(&g_game->gui, ((Holder_0045e5e0*)g_game->gui.layer)->entries[i].name, 0);
                }
            }
            for (i = 0; i <= ((Holder_0045e5e0*)g_game->gui.layer)->entries->count; i++) {
                if (strncmp(((Holder_0045e5e0*)g_game->gui.layer)->entries[i].name, "VID", strlen("VID")) == 0) {
                    SetGadgetActiveByName(&g_game->gui, ((Holder_0045e5e0*)g_game->gui.layer)->entries[i].name, 0);
                }
            }
        }
    }

    if (param_1 == 0) {
        SetGadgetStatusByName(&g_game->gui, "VISUALS", 1);
        int found = FindGadgetIndex(layer->entries, "ANTI", 1);
        if (found != -1) {
            SetButtonStage((Menu_0045e5e0*)&g_game->gui, found, ((*(Flags37f06_0045cf60*)&g_game->flags_37f06).byte >> 1) & 1);
        }
        found = FindGadgetIndex(layer->entries, "BSHADOWS", 1);
        if (found != -1) {
            SetButtonStage((Menu_0045e5e0*)&g_game->gui, found, ((*(Flags37f06_0045cf60*)&g_game->flags_37f06).byte >> 4) & 1);
        }
        found = FindGadgetIndex(layer->entries, "SHADING", 1);
        if (found != -1) {
            SetButtonStage((Menu_0045e5e0*)&g_game->gui, found, ((*(Flags37f06_0045cf60*)&g_game->flags_37f06).byte >> 5) & 1);
        }
        found = FindGadgetIndex(layer->entries, "GAMMA", 0xe);
        if (found != -1) {
            Gadget* e = FindGadgetChecked_D(layer->entries, "GAMMA");
            e->max = 0x14;
            e->sliderCallback = HandleGammaSlider;
            int value = g_game->brightness;
            if (value > 0x14)
                value = 0x14;
            float f = (float)value * (float)(e->range - 1) * 0.05f;
            if (f - (int)f != 0.0f)
                f += 1.0;
            e->knobPos = (short)f;
        }
    }

    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x45ead0
void __stdcall HandleSpeedOptionsClick(Gui* gui)
{
    char* data = (char*)gui->layer->entries;
    if (gui->hotGadgetIndex == -1) {
        (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).word &= 0xfffe;
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)&g_game->gui, "LEFTCLICK")) {
        PlaySoundByName("Options", 0);
        g_game->interfaceType = GetButtonStageByName(&g_game->gui, "LEFTCLICK");
        ClearSelectedGadget(gui);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)&g_game->gui, "UNITCHAT")) {
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(gui);
        g_game->unitChatText = (unsigned char)(GetButtonStageByName(&g_game->gui, "UNITCHAT") * 5);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "UNDO")) {
        PlaySoundByName("Options", 0);
        g_game->textScroll = g_optionsBackupTextScroll;
        g_game->gameSpeed = g_optionsBackupGameSpeed.s;
        g_game->effectiveGameSpeed = g_optionsBackupGameSpeed.s;
        g_game->scrollSpeed = g_optionsBackupEdgeScroll.b;
        g_game->interfaceType = g_optionsBackupInterfaceType;
        g_game->unitChat = g_optionsBackupUnitChat;
        g_game->unitChatText = g_optionsBackupUnitChatText;
        g_game->textLines = g_optionsBackupTextLines;
        CloseTopScreen(gui);
        OpenSpeedOptions();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "RESTORE")) {
        PlaySoundByName("Options", 0);
        g_game->textScroll = 10;
        g_game->textLines = 10;
        g_game->gameSpeed = 10;
        g_game->effectiveGameSpeed = 10;
        g_game->scrollSpeed = 0x20;
        g_game->interfaceType = 0;
        g_game->unitChat = 10;
        g_game->unitChatText = 5;
        CloseTopScreen(gui);
        OpenSpeedOptions();
        return;
    }
    int i = gui->hotGadgetIndex;
    if (data[gui->hotGadgetIndex * 347] != 1) {
        ClearSelectedGadget(gui);
        return;
    }
    if (i != -1) {
        Link_0045ead0* link = (Link_0045ead0*)gui->layer;
        void* obj = link->obj;
        CloseTopScreen(gui);
        gui->hotGadgetIndex = i;
        ((Link_0045ead0*)obj)->reselect(gui);
    }
}

// FUNCTION: 0x45ed50
void OpenSpeedOptions()
{
    Object_0045ed50* obj = (Object_0045ed50*)OpenOptionsLayout();
    RenderLayer(&g_game->gui, 2);
    EnsureOptionsPanelGadget();
    if ((*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).byte & 1) {
        LoadGuiLayer(&g_game->gui, "SPEEDSRT.GUI", 0x200);
    } else {
        LoadGuiLayer(&g_game->gui, "SPEEDS.GUI", 0x200);
        LoadPictureCached("optinterface4x", 0, 0, 0);
    }
    obj->fn = HandleSpeedOptionsClick;
    EnableKeyCommands((Dialog *)&g_game->gui);
    int found = FindGadgetIndex(obj->entries, "GAME", 0xe);
    SetGadgetStatusByName(&g_game->gui, "SPEEDS", 1);
    if (found != -1) {
        Gadget* e = FindGadgetChecked_D(obj->entries, "GAME");
        e->max = 0x15;
        e->sliderCallback = HandleGameSpeedSlider;
        int value = g_game->gameSpeed;
        if (value > 0x15) {
            value = 0x15;
        }
        float f = (float)value * (float)(e->range - 1) * 0.0476190485060215f;
        if (f - (int)f != 0.0f) {
            f += 1.0;
        }
        e->knobPos = (short)f;
    }
    if (FindGadgetIndex(obj->entries, "SCREEN", 0xe) != -1) {
        Gadget* e = FindGadgetChecked_D(obj->entries, "SCREEN");
        e->max = 0x41;
        int value = g_game->scrollSpeed;
        if (value > 0x41) {
            value = 0x41;
        }
        float f = (float)value * (float)(e->range - 1) * 0.015384615398943424f;
        if (f - (int)f != 0.0f) {
            f += 1.0;
        }
        e->knobPos = (short)f;
        e->sliderCallback = HandleScreenSlider;
    }
    SetButtonStageByName(&g_game->gui, "UNITCHAT", g_game->unitChatText / 5);
    SetButtonStageByName(&g_game->gui, "LEFTCLICK", g_game->interfaceType);
    char text[20];
    sprintf(text, g_game->textLines ? "%d" : "None", g_game->textLines);
    SetTranslatedTextByName(&g_game->gui, "MAXLINESTEXT", text, 0);
    if (FindGadgetIndex(obj->entries, "MAXLINES", 4) != -1) {
        Gadget* e = FindGadgetChecked_D(obj->entries, "MAXLINES");
        e->max = 0x1e;
        int value = g_game->textLines;
        if (value > 0x1e) {
            value = 0x1e;
        }
        float f = (float)value * (float)(e->range - 1) * 0.03333333507180214f;
        if (f - (int)f != 0.0f) {
            f += 1.0;
        }
        e->knobPos = (short)f;
        e->sliderCallback = HandleMaxLinesSlider;
    }
    if (FindGadgetIndex(obj->entries, "TXTSCROL", 0xe) != -1) {
        Gadget* e = FindGadgetChecked_D(obj->entries, "TXTSCROL");
        e->max = 0x14;
        int value = g_game->textScroll;
        if (value > 0x14) {
            value = 0x14;
        }
        float f = (float)value * (float)(e->range - 1) * 0.05000000074505806f;
        if (f - (int)f != 0.0f) {
            f += 1.0;
        }
        e->knobPos = (short)f;
        e->sliderCallback = HandleTextScrollSlider;
    }
    for (int i = 1; i <= obj->entries->count; i++) {
        if (obj->entries[i].type == 4)
            obj->entries[i].sliderCallback((Menu_0045b800*)&g_game->gui, 0);
    }
    SetKeyboardInput(&g_game->gui, 1);
    OrLabelAttribs();
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x45f190
void __stdcall HandleGameSettingsDialogClick(Gui* gui)
{
    if (gui->hotGadgetIndex != -1) {
        if (IsCurrentGadgetNamed((Dialog *)gui, "OK")) {
            PlaySoundByName("Options", 0);
            return;
        }
        ClearSelectedGadget(gui);
    }
}

// FUNCTION: 0x45f1d0
void ShowGameSettingsDialog()
{


    Layer* layer = LoadGuiLayer((Layer*)&g_game->gui, "GAMEOPTIONS.GUI", 0x1881);
    Gadget* entries = layer->entries;
    layer->handler = (void (__stdcall *)(void *))HandleGameSettingsDialogClick;
    LoadPictureCached("GameSettings", 0, 0, 0);
    int count = layer->entries->count;
    unsigned int index = FindHostSlot();
    index &= 0xff;
    char num[0x40];
    Opts_0045f1d0* opts = (Opts_0045f1d0*)g_game->players[index].info.get();
    Rule_0045f1d0* rule = &((Rule_0045f1d0*)(RuleSet_0045f1d0 *)g_game->options)[g_game->localPlayer];
    char* deathStrs[3] = { "Game Continues", "Game Ends", "Deathmatch" };
    char* locStrs[2] = { "Random", "Fixed" };
    char* mapStrs[2] = { "Mapped", "Unmapped" };
    char* losStrs[3] = { "True", "Circular", "Permanent" };
    char* cheatStrs[2] = { "Disallowed", "Allowed" };
    char* watchStrs[2] = { "Disallowed", "Allowed" };
    char* diffStrs[3] = { "Easy", "Medium", "Hard" };
    AddTextGadget(layer, "TEXT", Translate((char*)"Commander Death:"), 0x12, 0x5a, 0x6e, 2);
    AddTextGadget(layer, "TEXT", Translate(deathStrs[g_game->commanderDeath]), 0x8c,
                 0x5a, 0x78, 2);
    AddTextGadget(layer, "TEXT", Translate((char*)"Starting Locations:"), 0x12, 0x6c, 0x6e, 2);
    if (g_game->net->GetGameType() == 2) {
        AddTextGadget(layer, "TEXT", Translate(locStrs[((RuleSet_0045f1d0 *)g_game->options)->startType]), 0x8c,
                     0x6c, 0x78, 2);
    } else {
        AddTextGadget(layer, "TEXT", Translate(locStrs[opts->u.b.b14]), 0x8c, 0x6c,
                     0x78, 2);
    }
    AddTextGadget(layer, "TEXT", Translate((char*)"Mapping Mode:"), 0x12, 0x7e, 0x6e, 2);
    AddTextGadget(layer, "TEXT", Translate(mapStrs[(*(Los_0045cf60*)&g_game->mapFlags_14281).lb.l0]), 0x8c, 0x7e, 0x78, 2);
    AddTextGadget(layer, "TEXT", Translate((char*)"Line of Sight:"), 0x12, 0x90, 0x6e, 2);
    unsigned short losFlags = (*(Los_0045cf60*)&g_game->mapFlags_14281).losFlags;
    int losIdx = !(losFlags & 2) ? 2 : (int)(((unsigned char)~losFlags >> 2) & 1);
    AddTextGadget(layer, "TEXT", Translate(losStrs[losIdx]), 0x8c, 0x90, 0x78, 2);
    int y;
    if (g_game->net->GetGameType() == 3) {
        AddTextGadget(layer, "TEXT", Translate((char*)"Cheat Codes:"), 0x12, 0xa2, 0x6e, 2);
        AddTextGadget(layer, "TEXT", Translate(cheatStrs[opts->u.b.b13]), 0x8c,
                     0xa2, 0x78, 2);
        AddTextGadget(layer, "TEXT", Translate((char*)"Watching:"), 0x12, 0xb4, 0x6e, 2);
        AddTextGadget(layer, "TEXT", Translate(watchStrs[opts->u.b.b7]), 0x8c, 0xb4,
                     0x78, 2);
        y = 0xc6;
    } else {
        AddTextGadget(layer, "TEXT", Translate((char*)"Difficulty:"), 0x12, 0xa2, 0x6e, 2);
        AddTextGadget(layer, "TEXT", Translate(diffStrs[g_game->difficulty]), 0x8c, 0xa2,
                     0x78, 2);
        y = 0xb4;
    }
    AddTextGadget(layer, "TEXT", Translate((char*)"Map:"), 0x12, y, 0x6e, 2);
    AddTextGadget(layer, "TEXT",
                 Translate(g_game->net->GetMissionName()), 0x8c, y,
                 0x78, 2);
    y += 0x12;
    AddTextGadget(layer, "TEXT", Translate((char*)"Starting Metal:"), 0x12, y, 0x6e, 2);
    if (g_game->net->GetGameType() == 3) {
        AddTextGadget(layer, "TEXT", _itoa(opts->startMetal * 100, num, 10), 0x8c, y, 0x78,
                     2);
    } else {
        AddTextGadget(layer, "TEXT", _itoa(rule->startMetal, num, 10), 0x8c, y, 0x78, 2);
    }
    y += 0x12;
    AddTextGadget(layer, "TEXT", Translate((char*)"Starting Energy:"), 0x12, y, 0x6e, 2);
    if (g_game->net->GetGameType() == 3) {
        AddTextGadget(layer, "TEXT", _itoa(opts->startEnergy * 100, num, 10), 0x8c, y,
                     0x78, 2);
    } else {
        AddTextGadget(layer, "TEXT", _itoa(rule->startEnergy, num, 10), 0x8c, y, 0x78, 2);
    }
    y += 0x12;
    AddTextGadget(layer, "TEXT", Translate((char*)"Max Units:"), 0x12, y, 0x6e, 2);
    AddTextGadget(layer, "TEXT", _itoa(g_game->maxUnitsSetting, num, 10), 0x8c, y, 0x78, 2);
    int i;
    for (i = count + 1; i <= layer->entries->count; i++)
        entries[i].attribs = 1;
    SetKeyboardInput((Layer*)&g_game->gui, 1);
    RenderLayer((Gui *)((Layer*)&g_game->gui), 0x40);
}

// FUNCTION: 0x45f770
void __stdcall HandleBriefingClick(Gui* gui)
{
    if (gui->hotGadgetIndex == -1) {
        FreeBlinkWords((int)&g_game->gui);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "OK")) {
        PlaySoundByName("Options", 0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "TextRegion") || IsCurrentGadgetNamed((Dialog *)gui, "MOREBAR")) {
        PlaySoundByName("Options", 0);
        DrawHelpPage();
        MarkChanged((Dialog *)gui);
        ClearSelectedGadget(gui);
    }
    if (gui->hotGadgetIndex != -1)
        ClearSelectedGadget(gui);
}

// FUNCTION: 0x45f800
void OpenBriefingDialog()
{
    Layer* g = LoadGuiLayer((Sub_0045f800*)&g_game->gui, "BRIEFING.GUI", 0);
    Gadget* gadgets = g->entries;
    g->handler = (void (__stdcall *)(void *))HandleBriefingClick;
    int i = FindGadgetIndex(gadgets, "MOREBAR", 0xe);
    gadgets[i].attribs &= ~0x10;
    i = FindGadgetIndex(gadgets, "TextRegion", 0xe);
    gadgets[i].attribs &= ~0x10;
    LoadPictureCached("igmbrief", 0, 0, 0);
    AllocBlinkWords((Dialog *)((Sub_0045f800*)&g_game->gui), 0xf);
    InitBriefingText();
    RenderLayer((Gui *)((Sub_0045f800*)&g_game->gui), 0x40);
}

static inline void AddLine(Page_0045f8c0* page, Layer* layer, char* value, int y)
{
    if (value[0] == '|') {
        strcpy(value, page->blank);
    } else {
        char* p = value + 1;
        do {
        } while (*p++ != '|');
        p[-1] = 0;
    }
    AddTextGadget(layer, "TEXT", Translate(SkipTextLines(value, 0)), 0x28, y, 0x4e, 2);
    layer->entries[layer->entries->count].attribs = 1;
    AddTextGadget(layer, "TEXT", Translate(SkipTextLines(value, 1)), 0x7d, y, 0x12c, 2);
    layer->entries[layer->entries->count].attribs = 1;
}

// FUNCTION: 0x45f8c0
void __stdcall FillHelpPage(Gui* sub, int page, int lineCount)
{
    Layer* layer = sub->layer;
    layer->entries->count = g_helpDialogBaseGadgetCount;
    TdfFile parser;
    char path[256];
    char key[12];
    char value[0x80];
    BuildDataPath(path, "gamedata", "help", "TDF");
    if ((&parser)->LoadFile(path)) {
        int y = 0x32;
        if ((&parser)->SelectRecord("Help")) {
            Page_0045f8c0 lines;
            Page_0045f8c0* pp = &lines;
            int p2 = (page ? page : page);
            int n = (lineCount ? lineCount : lineCount);
            int first = (n ? n : n) * (p2 ? p2 : p2);
            int last = first + n;
            pp->blank[0] = ' ';
            pp->blank[1] = 0;
            pp->blank2[0] = ' ';
            pp->blank2[1] = 0;
            pp->first = first;
            pp->last = last;
            if (pp->first < pp->last) {
                do {
                    wsprintfA(key, "Line%d", pp->first);
                    if (parser.current->GetFieldString(value, key, 0x80, DAT_005119b8)) {
                        AddLine(pp, layer, value, y);
                        y += 0x12;
                    }
                } while (++pp->first < pp->last);
            }
        }
    }
    MarkChanged((Dialog *)sub);
}

// FUNCTION: 0x45fac0
void __stdcall HandleHelpClick(Gui* gui)
{
    if (gui->hotGadgetIndex != -1) {
        if (IsCurrentGadgetNamed((Dialog *)gui, "OK")) {
            PlaySoundByName("Options", 0);
            return;
        }
        if (IsCurrentGadgetNamed((Dialog *)gui, "Page")) {
            PlaySoundByName("Options", 0);
            FillHelpPage(gui, GetButtonStageByName(gui, "Page"), 0x11);
        }
        ClearSelectedGadget(gui);
    }
}

// FUNCTION: 0x45fb30
void OpenHelpDialog()
{
    Layer* g = LoadGuiLayer((Sub_0045fb30*)&g_game->gui, "HELP.GUI", 0x1881);
    g->handler = (void (__stdcall *)(void *))HandleHelpClick;
    LoadPictureCached("dhelp", 0, 0, 0);
    g_helpDialogBaseGadgetCount = g->entries->count;
    FillHelpPage((Gui *)((Sub_0045fb30*)&g_game->gui), 0, 0x11);
    SetKeyboardInput((Sub_0045fb30*)&g_game->gui, 1);
    RenderLayer((Gui *)((Sub_0045fb30*)&g_game->gui), 0x40);
}

// FUNCTION: 0x45fbc0
void ClearScreenWithHudPalette()
{
    Surface_0045fbc0 screen;
    unsigned char palette[0x400];

    memset(palette, 0, sizeof(palette));
    SetPaletteColors(palette, 0, 0x100);
    SetOffscreenSurface((int)(void *)g_game->screen);
    if (LockScreen(&screen)) {
        FillSurface((Surface *)&screen, g_game->gui.colours[0]);
        UnlockScreen(&screen);
        FlipScreen();
    }
}

// FUNCTION: 0x45fc40
void CloseOptionsPanel()
{
    (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).word &= 0xfffe;
    g_game->frontendNextSubState = 3;
}

// FUNCTION: 0x45fc60
void __stdcall HandleOptionsPanelClick(Gui* gui)
{
    if (gui->hotGadgetIndex == -1)
        goto cleanup;
    {
        SetGadgetStatus(gui, gui->hotGadgetIndex, 1);
        if ((*(Bits_0045cf60*)&g_game->netGameFlags_2a44).bits.bit2) {
        } else {
            RenderLayer((Gui *)((char*)&g_game->gui), 0x40);
            MarkChanged((Dialog *)((char*)&g_game->gui));
            BlitMenuLayers((char*)&g_game->gui, 0, 0);
            SetOffscreenSurface((int)(void *)g_game->screen);
            FlipScreen();
        }
        if (IsCurrentGadgetNamed((Dialog *)gui, "SPEEDS")) {
            PlaySoundByName("Options", 0);
            OpenSpeedOptions();
        } else if (IsCurrentGadgetNamed((Dialog *)gui, "VISUALS")) {
            PlaySoundByName("Options", 0);
            OpenVisualOptions(0);
        } else if (IsCurrentGadgetNamed((Dialog *)gui, "MUSIC")) {
            PlaySoundByName("Options", 0);
            OpenMusicOptions();
        } else if (IsCurrentGadgetNamed((Dialog *)gui, "PREV")) {
            PlaySoundByName("Options", 0);
            SaveSettings();
            g_optionsShellClosing = 1;
            return;
        } else if (IsCurrentGadgetNamed((Dialog *)gui, "CANCEL")) {
            PlaySoundByName("Previous", 0);
            RestoreSoundOptions();
            g_game->volume2 = g_optionsBackupMusicVolume;
            ((Sound *)g_game->sound)->CopyTrackTypeTable(&g_optionsBackupTrackTypes);
            g_game->cdMode = g_optionsBackupCdMode;
            ((Sound *)g_game->sound)->SetPlaybackOrder(g_game->cdMode);
            if (((unsigned char)g_game->flags_37f14_37f14.word ^ (unsigned char)g_optionsBackupMusicMode.s) & 1) {
                ((Sound *)g_game->sound)->PlayNextTrack();
            }
            unsigned short f = g_game->flags_37f14_37f14.word;
            g_game->flags_37f14_37f14.word = f ^ ((f ^ g_optionsBackupMusicMode.s) & 1);
            ((Sound *)g_game->sound)->SetLockedTrack(g_optionsBackupLockedTrack);
            SetBrightness(0.5 - g_game->brightness * -0.041666668f);
            ((Sound *)g_game->sound)->SetWaveVolume(g_game->volume1 << 10);
            ((Sound *)g_game->sound)->SetAuxVolume(g_game->volume2 << 10, 0);
            g_game->textScroll = g_optionsBackupTextScroll;
            g_game->gameSpeed = g_optionsBackupGameSpeed.i;
            g_game->effectiveGameSpeed = g_optionsBackupGameSpeed.i;
            g_game->scrollSpeed = g_optionsBackupEdgeScroll.b;
            g_game->interfaceType = g_optionsBackupInterfaceType;
            g_game->unitChat = g_optionsBackupUnitChat;
            g_game->unitChatText = g_optionsBackupUnitChatText;
            g_game->textLines = g_optionsBackupTextLines;
            RestoreVisualOptions();
            g_optionsShellClosing = 1;
            return;
        } else if (IsCurrentGadgetNamed((Dialog *)gui, "SOUND")) {
            PlaySoundByName("Options", 0);
            OpenSoundOptions();
        } else {
            if (gui->hotGadgetIndex != -1)
                ClearSelectedGadget(gui);
            return;
        }
        g_optionsShellClosing = 0;
cleanup:
        if (g_optionsBackupSurface) {
            DrawSurface(0, g_optionsBackupSurface, 0, 0);
            FreeSurface(g_optionsBackupSurface);
            g_optionsBackupSurface = 0;
        }
        if (g_optionsFlipSurface) {
            FreeSurface(g_optionsFlipSurface);
            g_optionsFlipSurface = 0;
        }
        g_optionsShellActive = 0;
        return;
    }
}

// FUNCTION: 0x45ffb0
void __stdcall DrawOptionsScrollBar(void* surf)
{
    if (g_optionsShellActive) {
        if (g_optionsLightbarX < 0x115) {
            int old = g_optionsLightbarX;
            g_optionsLightbarX += 0x15;
            if (g_optionsLightbarX >= 0x115) {
                PlaySoundByName("Options", 0);
                g_optionsLightbarX = 0x115;
            }
            if (g_optionsLightbarX > g_optionsLightbarMaxX && old < g_optionsLightbarMaxX) {
                void* snd = FindGafEntry(g_game->gui.gaf, "LIGHTBAR");
                GafFrame* s = (GafFrame*)GetGafFrame(snd, 2);
                DrawFrame((int)g_optionsFlipSurface, s, s->xOffset, s->yOffset);
            }
        }
        if (g_optionsLightbarX < g_optionsLightbarMaxX) {
            g_optionsLightbarAnim += 6;
        } else {
            g_optionsLightbarAnim -= 6;
            if (g_optionsLightbarAnim < 0)
                g_optionsLightbarAnim = 0;
        }
        Quad_45ffb0 src;
        src.p[0].x = 1;
        src.p[0].y = 1;
        src.p[3].x = 1;
        src.p[1].y = 1;
        Quad_45ffb0 dst;
        if (g_optionsLightbarX > g_optionsLightbarMaxX) {
            if (g_optionsLightbarX < 0x115)
                g_optionsLightbarX++;
            dst.p[1].x = g_optionsLightbarX;
            dst.p[2].x = g_optionsLightbarX;
            dst.p[3].x = g_optionsLightbarMaxX;
            dst.p[0].x = g_optionsLightbarMaxX;
            dst.p[0].y = g_optionsLightbarY;
            dst.p[1].y = g_optionsLightbarY - g_optionsLightbarAnim;
        } else {
            dst.p[3].x = g_optionsLightbarX;
            dst.p[0].x = g_optionsLightbarX;
            dst.p[1].x = 127;
            dst.p[2].x = 127;
            dst.p[0].y = g_optionsLightbarY - g_optionsLightbarAnim;
            dst.p[1].y = g_optionsLightbarY;
        }
        dst.p[2].y = 479;
        dst.p[3].y = 479;
        src.p[1].x = g_optionsFlipFrame.w - 1;
        src.p[2].x = g_optionsFlipFrame.w - 1;
        src.p[2].y = g_optionsFlipFrame.h - 1;
        src.p[3].y = g_optionsFlipFrame.h - 1;
        DrawFrameQuad(surf, &g_optionsFlipFrame, &dst, &src);
        MarkChanged((Dialog *)((char*)&g_game->gui));
        g_game->flags_142f1 |= 2;
        g_game->field_37e98 = 1;
    }
}

// FUNCTION: 0x460160
void OpenOptionsPanel()
{
    Layer* layer = (Layer*)g_game->gui.layer;
    if (!(*(Bits_0045cf60*)&g_game->netGameFlags_2a44).bits.bit2) {
        BlankScreen();
    }
    g_optionsFlipSurface = AllocSurface("FLIPSURFACE", layer->entries->width, layer->entries->height);
    memcpy(g_optionsFlipSurface->pixels, layer->entries->surface->pixels,
           layer->entries->width * layer->entries->height);
    FrameFromSurface((GafFrame*)&g_optionsFlipFrame, g_optionsFlipSurface);
    g_optionsLightbarX = 0;
    g_optionsLightbarMaxX = layer->entries->width - 1;
    g_optionsLightbarY = layer->entries->y;
    g_optionsShellActive = 1;
    g_optionsBackupSurface = AllocSurface("BKUPSURFACE", 300, 480);
    DrawSurface(g_optionsBackupSurface, 0, 0, 0);
    g_optionsLightbarAnim = 0;
    Layer* panel = (Layer*)OpenOptionsLayout();
    if (!(*(Bits_0045cf60*)&g_game->netGameFlags_2a44).bits.bit2) {
        LoadPictureCached("options4x", 0, 0, 0);
    }
    panel->handler = (void (__stdcall *)(void *))HandleOptionsPanelClick;
    memcpy(g_optionsPrefsSnapshot.block, (char*)&g_game->block[0], 0x53);
    g_optionsPrefsSnapshot.bit0 = (*(Los_0045cf60*)&g_game->mapFlags_14281).bits.bit1;
    g_optionsPrefsSnapshot.bit1 = (*(Los_0045cf60*)&g_game->mapFlags_14281).bits.bit2;
    g_optionsBackupGameSpeed.i = g_game->gameSpeed;
    g_optionsBackupEdgeScroll.i = g_game->scrollSpeed;
    g_optionsBackupLockedTrack = ((Sound *)g_game->sound)->GetLockedTrack();
    for (int i = 0; i < 100; i++) {
        g_optionsBackupTrackTypes[i] = ((Sound *)g_game->sound)->GetCategoryOfTrack(i);
    }
    SetKeyboardInput((Menu_00460160*)&g_game->gui, 1);
    RenderLayer((Gui *)((Menu_00460160*)&g_game->gui), 0xc0);
    if ((*(Bits_0045cf60*)&g_game->netGameFlags_2a44).bits.bit2) {
        PlaySoundByName("Panel", 0);
    }
}

// FUNCTION: 0x460340
void __stdcall HandleRestartDialogClick(Gui* gui)
{
    if (gui->hotGadgetIndex == -1)
        return;
    PlaySoundByName("Options", 0);
    if (IsCurrentGadgetNamed((Dialog *)gui, "RESTART")) {
        int ok = 0;
        int mode = g_game->net->GetGameType();
        switch (mode) {
        case 1:
            ok = true;
            break;
        case 2:
            ok = true;
            break;
        }
        if (!ok)
            return;
        RegisterDataArchives();
        g_game->difficulty = GetButtonStageByName(gui, "Difficulty");
        g_game->restartMissionRequest = 1;
    } else if (IsCurrentGadgetNamed((Dialog *)gui, "Difficulty")) {
        PlaySoundByName("Options", 0);
        ClearSelectedGadget(gui);
    } else if (!IsCurrentGadgetNamed((Dialog *)gui, "CANCEL") && gui->hotGadgetIndex != -1) {
        ClearSelectedGadget(gui);
    }
}

// FUNCTION: 0x4604a0
void OpenRestartDialog()
{
    Menu_004604a0* menu = (Menu_004604a0*)&g_game->gui;
    Layer* dialog = LoadGuiLayer(menu, "RESTART.GUI", 0x1000);
    Gadget* gadgets = dialog->entries;
    dialog->handler = (void (__stdcall *)(void *))HandleRestartDialogClick;
    LoadPictureCached("drestart", 0, 0, 0);
    int index = FindGadgetIndex(gadgets, "MISSIONNAME", 5);
    menu->language = menu->field_c;
    char* text = WordWrapText((Gui *)((Menu_004604a0*)&g_game->gui),
                              g_game->net->GetMissionName(),
                              gadgets[index].width, -1);
    menu->language = menu->field_8;
    char* first = strtok(text, "\n");
    SetTranslatedTextByName((Menu_004604a0*)&g_game->gui, "MISSIONNAME", (int)first, 0x80);
    char* second = strtok(0, "\n");
    if (second) {
        SetTranslatedTextByName((Menu_004604a0*)&g_game->gui, "MISSIONNAME1", (int)second, 0x80);
    }
    ApplyDifficultyButtons();
    SetKeyboardInput((Menu_004604a0*)&g_game->gui, 1);
    RenderLayer((Gui *)((Menu_004604a0*)&g_game->gui), 0x40);
    SetCursorMode(0x13);
}

// FUNCTION: 0x4605c0
void __stdcall HandleSurrenderChoice(Gui* gui)
{
    int owner = (int)gui->layer->entries;
    if (gui->hotGadgetIndex == -1)
        return;
    PlaySoundByName("Exit", 0);
    if (IsGadgetNamed(owner, gui->hotGadgetIndex, "CHOICE1")) {
        ((Sound *)g_game->sound)->SetTrackCategory(4);
        switch (g_battleQuitIntent) {
        case 0:
        case 1:
            ShutdownIngameSystems();
            PopUntilNamedLayout(1);
            CloseTopScreen(&g_game->gui);
            BlankScreen();
            SetGameMode(1);
            return;
        case 2:
            g_game->flagsByte_3923b |= 4;
            ShutdownIngameAndQuit();
            return;
        }
    } else if (!IsGadgetNamed(owner, gui->hotGadgetIndex, "CHOICE2")) {
        ClearSelectedGadget(gui);
    }
}

// FUNCTION: 0x460680
void OpenSurrenderDialog()
{
    Layer* layer = LoadGuiLayer((Sub_00460680*)&g_game->gui, "YESORNO.GUI", 0x1000);
    if (layer == 0) {
        return;
    }
    SetKeyboardInput((Sub_00460680*)&g_game->gui, 1);
    Gadget* entries = layer->entries;
    FindGadgetIndex(entries, "CHOICE1", 1);
    FindGadgetIndex(entries, "CHOICE2", 1);
    strcpy(entries->text + 0x16, "CHOICE2");
    strcpy(entries->text + 0x26, "CHOICE2");
    SetTranslatedTextByName((Sub_00460680*)&g_game->gui, "CHOICE1", "Yes", 0);
    SetTranslatedTextByName((Sub_00460680*)&g_game->gui, "CHOICE2", "No", 0);
    if (g_battleQuitIntent == 0) {
        SetTranslatedTextByName((Sub_00460680*)&g_game->gui, "TITLE", "Surrender this battle and return to main menu?", 0);
    } else if (g_battleQuitIntent == 2) {
        const char* title = (*(Flags_00460680*)&g_game->lobbyUiDirtyFlags_2bee).flag4 ? "Exit the Battle"
                                                : "Surrender this battle and exit to Windows?";
        SetTranslatedTextByName((Sub_00460680*)&g_game->gui, "TITLE", title, 0);
    }
    SelectGadgetByName((Gui *)((Sub_00460680*)&g_game->gui), "CHOICE2");
    layer->handler = (void (__stdcall *)(void *))HandleSurrenderChoice;
    RenderLayer((Gui *)((Sub_00460680*)&g_game->gui), 0x40);
}

// FUNCTION: 0x460800
void __stdcall HandleExitMenuClick(Gui* gui)
{
    if (gui->hotGadgetIndex != -1) {
        PlaySoundByName("Options", 0);
        if (IsCurrentGadgetNamed((Dialog *)gui, "MAINMENU")) {
            g_battleQuitIntent = 0;
            CloseTopScreen(gui);
            OpenSurrenderDialog();
            return;
        }
        if (IsCurrentGadgetNamed((Dialog *)gui, "EXITGAME")) {
            g_battleQuitIntent = 2;
            CloseTopScreen(gui);
            OpenSurrenderDialog();
            return;
        }
        if (!IsCurrentGadgetNamed((Dialog *)gui, "CANCEL")) {
            if (IsCurrentGadgetNamed((Dialog *)gui, "RESTART")) {
                CloseTopScreen(gui);
                OpenRestartDialog();
                return;
            }
            if (gui->hotGadgetIndex != -1)
                ClearSelectedGadget(gui);
        }
    }
}

// FUNCTION: 0x4608b0
void OpenExitMenu()
{
    Layer* dialog = LoadGuiLayer(&g_game->gui, "EXITMENU.GUI", 0x1800);
    dialog->handler = (void (__stdcall *)(void *))HandleExitMenuClick;
    FindGadgetIndex(dialog->entries, "RESTART", 1);
    if (g_game->net->GetGameType() == 1) {
        SetGadgetActiveByName(&g_game->gui, "RESTART", 1);
        SetTranslatedTextByName(&g_game->gui, "RESTART", Translate((char*)"Restart"), 0x80);
        goto tail;
    }
    if (g_game->net->GetGameType() == 2) {
        SetGadgetActiveByName(&g_game->gui, "RESTART", 1);
        SetTranslatedTextByName(&g_game->gui, "RESTART", Translate((char*)"Restart"), 0x80);
        goto tail;
    }
    if ((*(Flags_00460680*)&g_game->lobbyUiDirtyFlags_2bee).flag4) {
        SetGadgetActiveByName(&g_game->gui, "MAINMENU", 0);
    }
tail:
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
}

// FUNCTION: 0x4609a0
void __cdecl HandleBattleQuitPrompt(int)
{
    g_battleQuitIntent = 2;
    OpenSurrenderDialog();
}

// FUNCTION: 0x4609b0
void __stdcall HandleInGameOptionsClick(Gui* gui)
{
    if (gui->hotGadgetIndex == -1) {
        DisableKeyCommands((Dialog *)&g_game->gui);
        g_optionsShellActive = 0;
        if (g_optionsFlipSurface) {
            FreeSurface(g_optionsFlipSurface);
            g_optionsFlipSurface = 0;
        }
        if (g_optionsBackupSurface) {
            FreeSurface(g_optionsBackupSurface);
            g_optionsBackupSurface = 0;
        }
        if ((*(Bits_0045cf60*)&g_game->netGameFlags_2a44).byte & 4) {
            if (g_game->net->GetGameType() != 3)
                g_game->otherFlags &= 0xfffe;
        }
        (*(Flags37ebe_0045cf60*)&g_game->ordersPanelFlags_37ebe).word &= 0xfffe;
        ((Sound *)g_game->sound)->PauseCdAudio(0);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "LOADGAME")) {
        PlaySoundByName("Options", 0);
        ShowLoadGameScreen();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "SAVEGAME")) {
        PlaySoundByName("Options", 0);
        ShowSaveGameScreen();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "PREFS")) {
        PlaySoundByName("Options", 0);
        OpenOptionsPanel();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "HELP")) {
        PlaySoundByName("Options", 0);
        Layer* g = LoadGuiLayer(&g_game->gui, "HELP.GUI", 0x1881);
        g->handler = (void (__stdcall *)(void *))HandleHelpClick;
        LoadPictureCached("dhelp", 0, 0, 0);
        g_helpDialogBaseGadgetCount = g->entries->count;
        FillHelpPage(&g_game->gui, 0, 0x11);
        SetKeyboardInput(&g_game->gui, 1);
        RenderLayer(&g_game->gui, 0x40);
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "MISSION")) {
        PlaySoundByName("Options", 0);
        if (g_game->net->GetGameType() == 1) {
            Layer* g = LoadGuiLayer(&g_game->gui, "BRIEFING.GUI", 0);
            Gadget* gadgets = g->entries;
            g->handler = (void (__stdcall *)(void *))HandleBriefingClick;
            int i = FindGadgetIndex(gadgets, "MOREBAR", 0xe);
            gadgets[i].attribs &= ~0x10;
            i = FindGadgetIndex(gadgets, "TextRegion", 0xe);
            gadgets[i].attribs &= ~0x10;
            LoadPictureCached("igmbrief", 0, 0, 0);
            AllocBlinkWords((Dialog *)&g_game->gui, 0xf);
            InitBriefingText();
            RenderLayer(&g_game->gui, 0x40);
            return;
        }
        ShowGameSettingsDialog();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "EXIT")) {
        PlaySoundByName("Options", 0);
        OpenExitMenu();
        return;
    }
    if (IsCurrentGadgetNamed((Dialog *)gui, "OK")) {
        PlaySoundByName("Options", 0);
        return;
    }
    if (gui->hotGadgetIndex != -1)
        ClearSelectedGadget(gui);
}

// FUNCTION: 0x460cc0
void OpenInGameOptions()
{
    Layer* layer = LoadGuiLayer(&g_game->gui, "ARMOPT.GUI", 0x800);
    layer->handler = (void (__stdcall *)(void *))HandleInGameOptionsClick;
    SetGrayedOut(&g_game->gui, FindGadgetIndex(layer->entries, "SAVEGAME", 1),
                 g_game->net->GetGameType() == 3);
    SetGrayedOut(&g_game->gui, FindGadgetIndex(layer->entries, "LOADGAME", 1),
                 g_game->net->GetGameType() == 3);
    if (g_game->net->GetGameType() == 3 || g_game->net->GetGameType() == 2) {
        SetTranslatedTextByName(&g_game->gui, "MISSION", Translate((char*)"Settings"), 0x80);
    }
    EnableKeyCommands((Dialog *)&g_game->gui);
    SetKeyboardInput(&g_game->gui, 1);
    RenderLayer(&g_game->gui, 0x40);
    if (g_game->net->GetGameType() != 3) {
        g_game->pauseFlags |= 1;
    }
    ((Sound *)g_game->sound)->PauseCdAudio(1);
}
