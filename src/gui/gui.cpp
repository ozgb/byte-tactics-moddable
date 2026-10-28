#include "win_types.h"
#include "../../interfaces/host.h"

#include "../graphics/display.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>

struct GafEntry;

struct ConfigureListBoxByNameList {
    char unknown_0[0xc];
    union {
        GafEntry* glyphs;
    };
};

struct Gadget;

struct ConfigureListBoxByNameHolder {
    int unknown_0;
    Gadget* entries;
};

struct ConfigureListBoxByNameRoot {
    union { int current; int group; int fontId; };
    char unknown_04[0x14 - 0x04];
    union {
        ConfigureListBoxByNameList* language;
        ConfigureListBoxByNameList* font;
        ConfigureListBoxByNameList* list;
    };
    ConfigureListBoxByNameHolder* holder;
};

extern ConfigureListBoxByNameRoot* g_guiContext;

#include "../graphics/rect.h"
#include "../state/sim_globals.h"
#include "../files/files_api.h"
#include "layer.h"
#include "gui.h"

void ShowSoftwareCursor();
char* __stdcall Translate(char* text);
void __stdcall SetFont(int id);
int __cdecl tolower(int c);
int __stdcall GetTextWidth(int font, char* text);
char* __stdcall GetGafFrame(void* glyphs, int c);
void __stdcall SetTextColors(int colour, int font);
void __stdcall DrawFrame(void* surface, void* glyph, int x, int y);
void __stdcall DrawString(void* surface, char* text, int x, int y, int maxw);
unsigned int __cdecl GetTicks();

struct GafEntry;
struct GafFrame;
struct Gui;

#pragma pack(push, 1)
struct Gadget {
    unsigned char type;
    unsigned char team;
    char name[0x11];
    short x;
    short y;
    short width;
    short height;
    int attribs;
    int colours;
    int image;
    char textureNumber;
    signed char tab;
    unsigned char active;
    char unknown_2a;
    void* archive;
    GafEntry* gaf;
    char helpKey[0xb4 - 0x33];
    unsigned char resourceFlags;
    char unknown_b5;
    union {
        short count;
        char text[0x80];
        struct {
            char unknown_b6[2];
            void* saveUnder;
            void* surface;
            void* archive;
            GafEntry* background;
        } assets;
        struct {
            char unknown_b6[2];
            GafFrame* glyph;
            unsigned short flag;
        } frame;
        struct {
            int sortKey;
            short selected;
            short firstRow;
            short maxFirstRow;
            short rowCount;
            char* field_c2;
            void* rows;
            GafEntry* gaf;
            void (__stdcall* callback)(Gui*, Gadget*);
            char unknown_d2[4];
            union {
                int language;
                void* filebuf;
            };
            short scroll;
        } list;
        struct {
            char unknown_b6[0x16];
            char choice[0x10];
            char choice2[0x10];
        } names;
        struct {
            void (__stdcall* callback)(Gui*, Gadget*);
            char unknown_ba[4];
            GafEntry* gaf;
            GafFrame* frame;
            short index;
            unsigned char flags;
        } hotspot;
        struct {
            int total;
            int value;
            int max;
            int interval;
            int next;
            float step;
            int active;
            int showText;
        } anim;
    } u;
    union {
        short range;
        struct {
            unsigned char stages;
            unsigned char stageIndex;
        };
    };
    short status;
    union {
        struct {
            unsigned char quickKey;
            unsigned char frameBase;
            union {
                unsigned char grayedout;
                unsigned short flag;
                struct {
                    unsigned short flag_bit : 1;
                    unsigned short unknown_13c_1 : 15;
                };
                int max;
            };
        };
        GafEntry* inputGaf;
        struct {
            char unknown_13a_b[0x14e - 0x13a];
            GafEntry* sliderGaf;
            unsigned char sliderStyle;
        };
        struct {
            char unknown_13a_c[0x140 - 0x13a];
            short knobPos;
            short knobSize;
            void (__stdcall* sliderCallback)(Gui*, int);
            short unknown_148;
            int sliderUser;
        };
        struct {
            char unknown_13a_d[0x147 - 0x13a];
            unsigned char hotkey;
            unsigned int flag_148;
        };
    };
    char unknown_153[0x157 - 0x153];
    int sliderLocked;
};
#pragma pack(pop)

class SquadManager;


#pragma pack(push, 1)

struct Vec6_49fd20 {
    int x;
    int y;
    char pad[16];
};

struct Obj1_49fd20 {
    char unknown_0[0x3c];
    Vec6_49fd20 data;
};

struct Obj2_49fd20 {
    char unknown_0[0x13];
    short val_13;
    short val_15;
};

#pragma pack(pop)

void __stdcall UpdateMenu(Dialog* menu);
void __stdcall BlitLayers(void* param_1, unsigned int* param_2, int* param_3);
void FlipScreen();
void __stdcall DrawButton(Dialog* param_1, int param_2);
void __stdcall SelectAdjacentGadget(Dialog* menu, int value);


static inline int FindEntry(Gadget* entries, char* name)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0)
            return i;
    }
    return -1;
}

// FUNCTION: 0x49f8c0
int __stdcall HasGadgetNamed(Dialog* obj, char* name, int unused)
{
    Gadget* entries = obj->holder->entries;
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x49f930
void __stdcall SetGadgetName(Dialog* obj, char* name, char* text)
{
    if (obj->holder) {
        Gadget* entries = obj->holder->entries;
        int index = FindEntry(entries, name);
        if (index != -1)
        {
            int i = 0;
            while (i < 0x10 && text[i]) {
                entries[index].name[i] = text[i];
                i++;
            }
            entries[index].name[i] = 0;
        }
    }
}

// FUNCTION: 0x49f9c0
void __stdcall RunWhileScreenNamed(Dialog* menu, char* name)
{
    while (1) {
        if (!menu->holder || _strnicmp(menu->holder->entries->name, name, 16) != 0)
            break;
        host_pump_events(0);
        DispatchHostEvents();
        UpdateMenu(menu);
        BlitLayers(menu->holder, 0, 0);
        ShowSoftwareCursor();
        FlipScreen();
    }
}

// FUNCTION: 0x49fa50
void __stdcall EnableKeyCommands(Dialog* p)
{
    *(int*)((char*)p + 0xa2) = 1;
}

// FUNCTION: 0x49fa70
void __stdcall DisableKeyCommands(Dialog* obj)
{
    obj->keyCommands = 0;
}

// FUNCTION: 0x49fa90
void __stdcall MarkChanged(Dialog* obj)
{
    obj->changed = 1;
}

// FUNCTION: 0x49fab0
void __stdcall ClearChanged(void* param_1)
{
    *(int*)((char*)param_1 + 0xcca) = 0;
}

// FUNCTION: 0x49fad0
void __stdcall MarkLayerChanged(Dialog* obj)
{
    if (obj->holder != 0) {
        obj->holder->dirty = 1;
    }
}

// FUNCTION: 0x49faf0
void __stdcall ClearLayerChanged(Dialog* param)
{
    param->holder->dirty = 0;
}

// FUNCTION: 0x49fb10
void __stdcall SetKeyboardInput(int param_1, int param_2)
{
    int eax = *(int*)(param_1 + 0x18);
    if (eax != 0) {
        *(int*)(eax + 0x18) = param_2;
    }
}

// FUNCTION: 0x49fb30
int __stdcall GetKeyboardInput(Dialog* obj)
{
    if (obj->holder != 0) {
        return obj->holder->keyboardInput;
    }
    return 0;
}

// FUNCTION: 0x49fb50
void __stdcall SetFontsPath(Dialog* obj, const char* dir)
{
    strncpy(obj->path_bb6, dir, 0x100);
    strcat(obj->path_bb6, "\\");
}

// FUNCTION: 0x49fba0
void __stdcall SetGuiPath(Dialog* obj, const char* dir)
{
    strncpy(obj->path_9b6, dir, 0x100);
    strcat(obj->path_9b6, "\\");
}

// FUNCTION: 0x49fbf0
void __stdcall SetAnimsPath(Dialog* obj, const char* dir)
{
    strncpy(obj->path_ab6, dir, 0x100);
    strcat(obj->path_ab6, "\\");
}

// FUNCTION: 0x49fc40
void __stdcall ClearFocus(Dialog* param_1)
{
    param_1->focus = 0xffffffff;
}

// FUNCTION: 0x49fc50
int __stdcall TrySetFocus(Dialog* obj, int index)
{
    if (obj->focus != -1 && obj->holder->entries[obj->focus].type == 3) {
        obj->focus = -1;
    }
    if (obj->focus != -1 && obj->focus != index) {
        return 0;
    }
    obj->focus = index;
    if (index != -1 && obj->holder->entries[index].type == 3) {
        obj->length = strlen(obj->holder->entries[index].u.text);
    }
    return 1;
}

// FUNCTION: 0x49fcf0
int __stdcall IsFocused(int param_1, int param_2)
{
    int result = 0;
    result = *(int*)(param_1 + 0x64) == param_2;
    return result;
}

// FUNCTION: 0x49fd10
bool __stdcall HasFocus(int param_1)
{
    return *(int*)(param_1 + 0x64) != -1;
}

// FUNCTION: 0x49fd20
void __stdcall FUN_0049fd20(Obj1_49fd20* param_1, Obj2_49fd20* param_2, Vec6_49fd20* param_3)
{
    *param_3 = param_1->data;
    param_3->x -= param_2->val_13;
    param_3->y -= param_2->val_15;
}

// FUNCTION: 0x49fd60
int __stdcall IsCurrentGadgetNamed(Dialog* gui, char* name)
{
    if (!gui->holder)
        return 0;
    if (gui->current == -1)
        return 0;
    return strcmp(gui->holder->entries[gui->current].name, name) == 0;
}

// FUNCTION: 0x49fdf0
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: 0x49fe60
int __stdcall FindGadgetIndexBySubstring(Gadget* entries, char* name)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strstr(entries[i].name, name)) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: 0x49fed0
void __stdcall GetGadgetName(Gadget* entries, char* name, int index)
{
    strncpy(name, entries[index].name, 0x10);
    name[0x10] = 0;
}

// FUNCTION: 0x49ff10
Gadget* __stdcall FindGadgetOrNull(Gadget* entries, char* name)
{
    int i = FindEntry(entries, name);
    if (i != -1) {
        return &entries[i];
    }
    return 0;
}

// FUNCTION: 0x49ff90
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name)
{
    int i = FindEntry(entries, name);
    if (i != -1) {
        return &entries[i];
    }
    FatalError("Error in GUI layout");
    return 0;
}

// FUNCTION: 0x4a0010
Gadget* __stdcall FindGadgetChecked_B(Gadget* entries, char* name)
{
    int i = FindEntry(entries, name);
    if (i != -1) {
        return &entries[i];
    }
    FatalError("Error in GUI layout");
    return 0;
}

// FUNCTION: 0x4a0090
void __stdcall UpdateHelpText(Dialog* obj)
{
    char* text = DAT_005119b8;
    if (obj->index != -1) {
        obj->used = obj->index;
        text = obj->holder->entries[obj->index].helpKey;
    }
    int found = FindEntry(obj->holder->entries, "HELPTEXT");
    if (found != -1) {
        strcpy((char*)&obj->holder->entries[found].u.count, Translate(text));
        obj->changed = 1;
    }
}

// FUNCTION: 0x4a0180
Gadget* __stdcall FindGadgetChecked_C(Gadget* entries, char* name)
{
    int i = FindEntry(entries, name);
    if (i != -1) {
        return &entries[i];
    }
    FatalError("Error in GUI layout");
    return 0;
}

// FUNCTION: 0x4a0200
Gadget* __stdcall FindGadgetChecked_D(Gadget* entries, char* name)
{
    int i = FindEntry(entries, name);
    if (i != -1) {
        return &entries[i];
    }
    FatalError("Error in GUI layout");
    return 0;
}

// FUNCTION: 0x4a0280
Gadget* __stdcall FindGadgetChecked_E(Gadget* entries, char* name)
{
    int i = FindEntry(entries, name);
    if (i != -1) {
        return &entries[i];
    }
    FatalError("Error in GUI layout");
    return 0;
}

// FUNCTION: 0x4a0300
int __stdcall IsGadgetNamed(Gadget* entries, int i, char* name)
{
    if (i == -1) {
        return 0;
    }
    return strncmp(entries[i].name, name, 0x10) == 0;
}

// FUNCTION: 0x4a0340
void __stdcall ClearPeerStatus(Dialog* param_1, int index)
{
    Gadget* entries = param_1->holder->entries;
    Gadget* me = &entries[index];
    if (me->team != 0) {
        Gadget* e = &entries[1];
        for (int i = 1; i < entries->u.count + 1; i++, e++) {
            if (e->type == 1 && i != index && e->team == me->team
                && e->status != 0) {
                e->status = 0;
                DrawButton(param_1, i);
                if (param_1->holder != 0) {
                    param_1->holder->dirty = 1;
                }
            }
        }
    }
}

// FUNCTION: 0x4a03f0
void __stdcall SetGadgetActive(Dialog* menu, int index, int value)
{
    Gadget* entries = menu->holder->entries;
    int i;

    entries[index].active = value;

    if (entries[index].type == 4) {
        for (i = 0; i <= entries[0].u.count; i++) {
            if (entries[i].type == 1 && entries[i].team == entries[index].team) {
                entries[i].active = value;
            }
        }
    } else if (entries[index].type == 2) {
        if (value == 0) {
            unsigned char team = entries[index].team;
            int found = 0;
            i = 1;
            for (;;) {
                if (i >= entries[0].u.count + 1) {
                    found = 0;
                    break;
                }
                if (entries[i].type == 4 && entries[i].team == team) {
                    found = i;
                    break;
                }
                i++;
            }
            if (found != -1) {
                SetGadgetActive(menu, found, 0);
            }
        }
    }

    if (value == 0 && index == menu->holder->current) {
        SelectAdjacentGadget(menu, 1);
    }
    menu->changed = 1;
}

struct Pad_p2_0 { int field; };


#pragma pack(push, 1)

int __cdecl CountHumanSlots();

struct GadgetAccessHolder {
    GadgetAccessHolder* unknown_0;
    Gadget* entries;
};

struct GadgetAccessDialog {
    int group;
    char unknown_04[0x18 - 0x04];
    GadgetAccessHolder* holder;
    char unknown_1c[0x64 - 0x1c];
    int current;
    char unknown_68[0x74 - 0x68];
    int length;
    char unknown_78[0xcca - 0x78];
    int changed;
};

#pragma pack(pop)

void __stdcall SetGadgetActive(GadgetAccessDialog* menu, int index, int value);
void __stdcall ClearPeerStatus(GadgetAccessDialog* obj, int index);
void __stdcall DrawLitRectangle(int surface, Rect* rect, int level);


static inline int FindGadgetAccessEntry(Gadget* entries, char* name)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0) {
            return i;
        }
    }
    return -1;
}

static inline Gadget* GetEntry(GadgetAccessDialog* obj, int index)
{
    return obj->holder->entries + index;
}

static inline Gadget* GetEntries(GadgetAccessDialog* obj)
{
    return obj->holder->entries;
}

static inline char* GetData(GadgetAccessDialog* obj)
{
    return (char*)obj->holder->entries;
}

// FUNCTION: 0x4a04f0
char __stdcall GetGadgetActiveByName(GadgetAccessDialog* obj, char* name)
{
    Gadget* entries = obj->holder->entries;
    int i = FindGadgetAccessEntry(entries, name);
    if (i == -1) {
        return -1;
    }
    return entries[i].active;
}

// FUNCTION: 0x4a0570
void __stdcall SetGadgetActiveByName(GadgetAccessDialog* obj, char* name, int param_3)
{
    if (obj->holder) {
        int index = FindGadgetAccessEntry(obj->holder->entries, name);
        if (index != -1)
            SetGadgetActive(obj, index, param_3);
    }
}

// FUNCTION: 0x4a05e0
void __stdcall AssignQuickKey(GadgetAccessDialog* obj, int index)
{
    Gadget* entries;
    Gadget* scan;
    char* text;
    int length;
    Gadget* entry;
    int i;
    int j;

    if (index == -1)
        return;

    entries = obj->holder->entries;
    entry = entries + index;
    if (entry->type == 1) {
        if ((entry->attribs & 0x10000) != 0)
            return;
    }
    if (entry->type == 5 && strlen((char*)&entry->stages) == 0)
        return;
    if (entry->type != 5 && entry->type != 1)
        return;

    if (entry->type == 1 && entry->stages != 0) {
        entry->quickKey = 0;
        return;
    }
    if (entry->type == 1) {
        if (strlen(entry->u.text) == 0)
            return;
        entry->quickKey = 0;
        text = entry->u.text;
    } else if (entry->type == 5) {
        text = entry->u.text;
        entry->hotkey = 0;
    }

    length = strlen(text);
    if (length == 0)
        return;

    for (i = 0; i < length; i++) {
        if (text[i] != ' ') {
            for (j = 0; j <= entries->u.count; j++) {
                scan = entries + j;
                if (scan->type == 1) {
                    if (tolower((signed char)scan->quickKey) == tolower((signed char)text[i]))
                        break;
                } else if (scan->type == 5) {
                    if (tolower((signed char)scan->hotkey) == tolower((signed char)text[i]))
                        break;
                }
            }
            if (j > entries->u.count) {
                if (entry->type == 1) {
                    entry->quickKey = text[i];
                    return;
                }
                if (entry->type == 5) {
                    entry->hotkey = text[i];
                    return;
                }
                return;
            }
        }
    }
}

// FUNCTION: 0x4a07d0
void __stdcall SetGadgetTextByName(GadgetAccessDialog* obj, char* name, char* text)
{
    if (obj->holder) {
        Gadget* entries = obj->holder->entries;
        int index = FindGadgetAccessEntry(entries, name);
        if (index != -1) {
            strncpy(entries[index].u.text, text, 0x80);
            obj->changed = 1;
            AssignQuickKey(obj, index);
        }
    }
}

// FUNCTION: 0x4a0880
void __stdcall SetGadgetText(Dialog* obj, int index, char* text)
{
    strcpy(obj->holder->entries[index].u.text, text);
    if (obj->focus == index) {
        obj->length = strlen(text);
    }
}

// FUNCTION: 0x4a08f0
void __stdcall TranslateGadgetItems(GadgetAccessDialog* obj, int index)
{
    Gadget* entry = GetEntry(obj, index);
    char temp[0x80];
    char* dst = temp;
    char* src = entry->u.text;
    int i = 0;
    while (i < entry->stages) {
        strcpy(dst, Translate(src));
        dst += strlen(dst) + 1;
        src += strlen(src) + 1;
        i++;
    }
    memcpy(entry->u.text, temp, 0x80);
}

// FUNCTION: 0x4a09c0
void __stdcall SetTranslatedText(GadgetAccessDialog* context, int index, char* source, int value)
{
    if (index == -1 || context->holder == 0)
        return;
    Gadget* entries = context->holder->entries;
    char* text = Translate(source);

    switch (entries[index].type) {
    case 5:
        strncpy(entries[index].u.text, text, 0x80);
        if (entries[index].stages != 0)
            AssignQuickKey(context, index);
        break;
    case 3:
        if (text != 0) {
            strcpy(context->holder->entries[index].u.text, text);
            if (context->current == index)
                context->length = strlen(text);
        }
        if (value != 0)
            entries[index].status = (short)value;
        break;
    case 1:
        strncpy(entries[index].u.text, text, 0x80);
        AssignQuickKey(context, index);
        if (entries[index].stages != 0) {
            char* p = entries[index].u.text;
            while (*p != 0) {
                if (*p == '|')
                    *p = 0;
                p++;
            }
            Gadget* entry = &context->holder->entries[index];
            char temp[0x80];
            char* dst = temp;
            char* src = entry->u.text;
            int i = 0;
            while (i < entry->stages) {
                strcpy(dst, Translate(src));
                dst += strlen(dst) + 1;
                src += strlen(src) + 1;
                i++;
            }
            memcpy(entry->u.text, temp, 0x80);
        }
        break;
    }
    context->changed = 1;
}

// FUNCTION: 0x4a0bf0
void __stdcall SetTranslatedTextByName(GadgetAccessDialog* obj, char* name, char* param_3, int param_4)
{
    if (obj->holder != 0) {
        int index = FindGadgetAccessEntry(obj->holder->entries, name);
        if (index != -1) {
            SetTranslatedText(obj, index, param_3, param_4);
        }
    }
}

// FUNCTION: 0x4a0c70
void __stdcall SetGadgetColorByName(GadgetAccessDialog* obj, char* name, int value)
{
    if (obj->holder) {
        Gadget* entries = obj->holder->entries;
        int index = FindGadgetAccessEntry(entries, name);
        if (index != -1) {
            entries[index].colours = value;
            obj->changed = 1;
        }
    }
}

// FUNCTION: 0x4a0d00
char* __stdcall GetGadgetText(GadgetAccessDialog* obj, char* name, char* buf)
{
    char* desc = 0;
    if (obj->holder == 0) {
        FatalError("Internal error");
    }
    Gadget* entries = obj->holder->entries;
    int index = FindGadgetAccessEntry(entries, name);
    if (index != -1) {
        switch (entries[index].type) {
        case 1:
        case 3:
        case 5:
            desc = entries[index].u.text;
            break;
        }
        if (desc != 0 && buf != 0) {
            strcpy(buf, desc);
        }
    }
    return desc;
}

// FUNCTION: 0x4a0e00
void __stdcall SetControlTextByName(GadgetAccessDialog* obj, char* name, char* text)
{
    Gadget* entries = obj->holder->unknown_0->entries;
    if (obj->holder->unknown_0 == 0)
        return;
    int index = FindGadgetAccessEntry(entries, name);
    if (index == -1)
        return;
    switch (entries[index].type) {
    case 3:
        strcpy(obj->holder->entries[index].u.text, text);
        if (obj->current == index)
            obj->length = strlen(text);
        break;
    case 1:
        strncpy(entries[index].u.text, text, 0x80);
        break;
    case 5:
        strncpy(entries[index].u.text, text, 0x80);
        break;
    }
    obj->changed = 1;
}

// FUNCTION: 0x4a0f30
int __stdcall GetGadgetStatus(GadgetAccessDialog* obj, int index)
{
    return GetEntries(obj)[index].status;
}

// FUNCTION: 0x4a0f60
int __stdcall GetButtonStageByName(GadgetAccessDialog* obj, char* name)
{
    Gadget* entries = obj->holder->entries;
    int i = FindGadgetAccessEntry(entries, name);
    if (i != -1 && entries[i].type == 1) {
        return entries[i].stageIndex;
    }
    return -1;
}

// FUNCTION: 0x4a0ff0
int __stdcall GetButtonStage(GadgetAccessDialog* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    if (entries[index].type == 1) {
        return entries[index].stageIndex;
    }
    return -1;
}

// FUNCTION: 0x4a1030
int __stdcall SetButtonStage(GadgetAccessDialog* obj, int index, char value)
{
    Gadget* entries = obj->holder->entries;
    if (entries[index].type == 1) {
        entries[index].stageIndex = value;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4a1080
int __stdcall SetButtonStageByName(GadgetAccessDialog* obj, char* name, char value)
{
    Gadget* entries = obj->holder->entries;
    int i = FindGadgetAccessEntry(entries, name);
    if (i != -1) {
        entries[i].stageIndex = value;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4a1110
int __stdcall SetGadgetStatusByName(GadgetAccessDialog* obj, char* name, int value)
{
    Gadget* entries = obj->holder->entries;
    int i = FindGadgetAccessEntry(entries, name);
    if (i != -1) {
        entries[i].status = value;
        obj->changed = 1;
        if (value) {
            ClearPeerStatus(obj, i);
        }
        return 1;
    }
    return 0;
}


// FUNCTION: 0x4a11c0
void __stdcall SetGadgetStatus(GadgetAccessDialog* param_1, int param_2, short param_3)
{
    *(short*)(GetData(param_1) + param_2 * 347 + 0x138) = param_3;
    ClearPeerStatus(param_1, param_2);
}

// FUNCTION: 0x4a1200
void __stdcall SetGrayedOut(GadgetAccessDialog* obj, int index, int value)
{
    GetEntries(obj)[index].flag_bit = value;
    obj->changed = 1;
}

// FUNCTION: 0x4a1250
void __stdcall SetGrayedOutByName(GadgetAccessDialog* obj, char* name, int value)
{
    Gadget* entries = obj->holder->entries;
    int i = FindGadgetAccessEntry(entries, name);
    if (i != -1) {
        entries[i].flag_bit = value;
    }
}

// FUNCTION: 0x4a12e0
void __stdcall SetGadgetGrayedOut(GadgetAccessDialog* obj, int index, int value)
{
    Gadget* entries = obj->holder->entries;
    if (index == -1) {
        return;
    }
    Gadget* e = &entries[index];
    switch (e->type) {
    case 4: {
        e->sliderLocked = value;
        unsigned char group = e->team;
        for (int i = 1; i < entries->u.count + 1; i++) {
            if (entries[i].type == 1 && entries[i].team == group && (entries[i].attribs & 0x1800)) {
                entries[i].flag = (entries[i].flag & 0xfffe) | (value & 1);
            }
        }
        break;
    }
    case 1:
        e->flag = (e->flag & 0xfffe) | (value & 1);
        break;
    case 2:
        if (value) {
            e->attribs |= 0x100;
        } else {
            e->attribs &= 0xfffffeff;
        }
        break;
    case 5:
        e->flag_148 = (e->flag_148 & 0xfffffffe) | (value & 1);
        break;
    case 12:
        e->u.frame.flag = (e->u.frame.flag & 0xfffe) | (value & 1);
        break;
    default:
        break;
    }
}

// FUNCTION: 0x4a1450
void __stdcall SetGadgetGrayedOutByName(GadgetAccessDialog* obj, char* name, int param_3)
{
    int index = FindGadgetAccessEntry(obj->holder->entries, name);
    if (index != -1)
        SetGadgetGrayedOut(obj, index, param_3);
}

// FUNCTION: 0x4a14c0
void __stdcall SetGadgetGrayedOutByNameTwin(GadgetAccessDialog* obj, char* name, int param_3)
{
    int index = FindGadgetAccessEntry(obj->holder->entries, name);
    if (index != -1)
        SetGadgetGrayedOut(obj, index, param_3);
}

// FUNCTION: 0x4a1530
void __stdcall SetQuickKeyByName(GadgetAccessDialog* obj, char* name, char value)
{
    int i = FindGadgetAccessEntry(obj->holder->entries, name);
    obj->holder->entries[i].quickKey = value;
    obj->changed = 1;
}

// FUNCTION: 0x4a15c0
void __stdcall GetGadgetRectByIndex(char* param_1, int param_2, Rect* param_3)
{
    char* e = param_1 + param_2 * 0x15b;
    if (*e == 0) {
        param_3->left = 0;
        param_3->top = 0;
    } else {
        param_3->left = *(short*)(e + 0x13);
        param_3->top = *(short*)(e + 0x15);
    }
    param_3->right = *(short*)(e + 0x17) - 1 + param_3->left;
    param_3->bottom = *(short*)(e + 0x19) - 1 + param_3->top;
}

// FUNCTION: 0x4a1630
void __stdcall GetGadgetRect(Gadget* entry, Rect* rect)
{
    if (entry->type == 0) {
        rect->left = 0;
        rect->top = 0;
    } else {
        rect->left = entry->x;
        rect->top = entry->y;
    }
    rect->right = entry->width + rect->left - 1;
    rect->bottom = entry->height + rect->top - 1;
}

// FUNCTION: 0x4a1680
void __stdcall GetGadgetScreenRect(char* param_1, int param_2, Rect* param_3)
{
    char* e = param_1 + param_2 * 0x15b;
    param_3->left = *(short*)(e + 0x13);
    param_3->top = *(short*)(e + 0x15);
    if (*e != 0) {
        param_3->left += *(short*)(param_1 + 0x13);
        param_3->top += *(short*)(param_1 + 0x15);
    }
    param_3->right = *(short*)(e + 0x17) - 1 + param_3->left;
    param_3->bottom = *(short*)(e + 0x19) - 1 + param_3->top;
}

// FUNCTION: 0x4a16f0
void __stdcall DrawGadgetSelectionBox(GadgetAccessDialog* obj, int index, int param_3)
{
    Gadget* entries = obj->holder->entries;
    int i;
    Rect rect;
    int level;
    int step;
    obj->changed = 1;
    for (i = 1; i <= entries->u.count; i++) {
        if (entries[i].type == 3) {
            entries[i].colours = 0;
        }
    }
    if (entries[index].type == 3) {
        entries[index].colours = 0x1e;
        return;
    }
    if (entries[index].type == 5) {
        return;
    }
    if (entries[index].type == 2) {
        return;
    }
    {
        Gadget* e = &entries[index];
        if (e->type == 0) {
            rect.left = 0;
            rect.top = 0;
        } else {
            rect.left = e->x;
            rect.top = e->y;
        }
        rect.right = e->width + rect.left - 1;
        rect.bottom = e->height + rect.top - 1;
        level = 0x1f;
        for (step = 0; step < 6; step++) {
            rect.left--;
            rect.top--;
            rect.right++;
            rect.bottom++;
            DrawLitRectangle((int)obj->holder->entries->u.assets.surface, &rect, level);
            level += -3 - step;
        }
    }
}

// FUNCTION: 0x4a1810
int __stdcall SelectFontForEntry(Gadget* entries, int index)
{
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == entries[index].tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->group);
        i = -1;
    }
    return i;
}

// FUNCTION: 0x4a18c0
int __stdcall FindBackgroundCell(Gadget* entries, int index)
{
    int n = 0;
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 8) {
            if (n == entries[index].textureNumber) {
                return 0;
            }
            n++;
        }
    }
    return 0;
}

// FUNCTION: 0x4a1920
int __stdcall IsPointInRect(Rect* r, int px, int py)
{
    if (px >= r->left && px <= r->right && py >= r->top && py <= r->bottom) {
        return 1;
    }
    return 0;
}

struct IsAboveRectTopRect {
    char unknown_0[4];
    int top;
};

// FUNCTION: 0x4a1950
int __stdcall IsAboveRectTop(IsAboveRectTopRect* param_1, int param_2) {
    return param_2 < param_1->top;
}

struct IsBelowRectBottomRect {
    char unknown_0[0xc];
    int bottom;
};

// FUNCTION: 0x4a1970
int __stdcall IsBelowRectBottom(IsBelowRectBottomRect* obj, int value)
{
    return value > obj->bottom;
}

int FindFreeColor();

// FUNCTION: 0x4a1990
int __stdcall FindAssocListBox(Gadget* entries, int index)
{
    unsigned char id = entries[index].team;
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 2 && entries[i].team == id) {
            return i;
        }
    }
    return 0;
}


// FUNCTION: 0x4a19f0
int __stdcall FindAssocSlider(Gadget* entries, int index)
{
    unsigned char id = entries[index].team;
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 4 && entries[i].team == id) {
            return i;
        }
    }
    return 0;
}


// FUNCTION: 0x4a1a50
int __stdcall FindAssocTextInput(Gadget* entries, int index)
{
    unsigned char id = entries[index].team;
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 3 && entries[i].team == id) {
            return i;
        }
    }
    return 0;
}

struct RedrawGadgetRectSurface;

#include "../graphics/draw.h"

struct RedrawGadgetRectHolder {
    char unknown_0[4];
    Gadget* entries;
};

struct RedrawGadgetRectDialog {
    char unknown_0[0x18];
    RedrawGadgetRectHolder* holder;
};


// FUNCTION: 0x4a1ab0
void __stdcall RedrawGadgetRect(RedrawGadgetRectDialog* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    Gadget* e = &entries[index];
    Rect rect;
    if (e->type == 0) {
        rect.left = 0;
        rect.top = 0;
    } else {
        rect.left = e->x;
        rect.top = e->y;
    }
    rect.right = e->width + rect.left - 1;
    rect.bottom = e->height + rect.top - 1;
    GrayRectangle((Surface *)(RedrawGadgetRectSurface*)entries->u.assets.surface, &rect);
    FadeRectangle((Surface *)(RedrawGadgetRectSurface*)entries->u.assets.surface, &rect, -0x14);
}

#pragma pack(push, 1)


struct DrawListBoxHolder {
    char unknown_00[4];
    Gadget* entries;
    char unknown_08[0x10 - 0x08];
    int flags;
    int dirty;
    char unknown_18[0x20 - 0x18];
    int current;
    void* surface;
};

struct DrawListBoxDialog {
    char unknown_00[0x18];
    DrawListBoxHolder* holder;
    char unknown_1c[0x8b2 - 0x1c];
    unsigned char colour[0x8be - 0x8b2];
    unsigned char colour_8be;
    char unknown_8bf[0xcd2 - 0x8bf];
    void* fallback;
};

#include "../graphics/gaf_frame.h"

struct DrawListBoxLanguage {
    char unknown_0[0xc];
    unsigned short* glyphs;
};

struct DrawListBoxLanguageRoot {
    int current;
    char unknown_04[0x14 - 0x04];
    DrawListBoxLanguage* language;
};

struct DrawListBoxPoint { int x; int y; };
struct DrawListBoxQuad { DrawListBoxPoint points[4]; };


struct DrawListBoxItem {
    char unknown_0[0x28];
    GafFrame* cell;
};

#pragma pack(pop)

#include "../graphics/surface.h"


void __stdcall DrawListboxFrame(DrawListBoxDialog* obj, int index, void* bmp);
#include "../util/registry.h"
#include "gui.h"
int __stdcall DrawWrappedText(void* surface, char* text, int x, int y, int maxw,
                            int rem, int style);
void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2,
                            int colour);
void __stdcall DrawFrameQuad(void* surface, void* bitmap, DrawListBoxQuad* dst,
                            DrawListBoxQuad* src);

static inline int MeasureDrawListBoxText(char* text)
{
    int width = 0;
    if (0 == text)
        return 0;
    if (!g_guiContext->language)
        return GetTextWidth(GetFont(), text);
    char* q = text;
    while (*q) {
        char ch = *q;
        GafFrame* glyph = (GafFrame*)GetGafFrame(
            g_guiContext->language->glyphs, (unsigned char)ch);
        if (0 != glyph)
            width += glyph->width;
        q++;
    }
    return width;
}

static inline int GetDrawListBoxLineHeight()
{
    if (0 == g_guiContext->language)
        return GetFontHeight();
    return ((GafFrame*)GetGafFrame(g_guiContext->language->glyphs, 0x49))->height + 2;
}

extern int Pad_a1b40_e0;
extern int Pad_a1b40_e1;
extern int Pad_a1b40_e2;
extern int Pad_a1b40_e3;


// FUNCTION: 0x4a1b40
void __stdcall DrawListBox(DrawListBoxDialog* obj, int index)
{
    unsigned char font;
    int yoff;
    int xx;
    Rect bounds;
    int xw;
    int flag = 0;
    if (0 != obj->holder)
        obj->holder->dirty = 1;
    DrawListBoxHolder* holder = obj->holder;
    Gadget* entries;
    entries = obj->holder->entries;
    Gadget* me = &entries[index];
    int h = me->height;
    void* surface;
    GetGadgetRect(&entries[index], (Rect*)&bounds);
    surface = holder->surface;
    if (surface == 0)
        surface = obj->fallback;
    if (surface == 0 && !(holder->flags & 0x80))
        DrawListboxFrame(obj, index, surface);
    else if (surface != 0)
        CopySurfaceRect((Surface *)entries->u.assets.surface, (Surface *)surface, &bounds, (Point *)&bounds);
    int lh = GetDrawListBoxLineHeight();
    int step;
    if (me->u.list.scroll == 0)
        step = lh + 1;
    else
        step = me->u.list.scroll;
    unsigned int flags;
    flags = me->attribs;
    if ((flags & 0x10) && me->u.list.field_c2 && 0 != me->u.list.rowCount) {
        Rect rowRect;
        int i;
        int t = 0;
        for (i = 1; i < entries->u.count + 1; i++) {
            if (7 == entries[i].type) {
                if (t == me->tab) {
                    SetFont((int)entries[i].u.list.language);
                    break;
                }
                t++;
            }
        }
        if (i == entries->u.count + 1) { SetFont(g_guiContext->current); }
        GetFont();
        font = GetTextKeyColor();
        char* q = SkipTextLines(me->u.list.field_c2, me->u.list.firstRow);
        int line = 0;
        int y = me->u.list.firstRow;
        yoff = 0;
        while (1) {
            rowRect.left = 2 + bounds.left;
            rowRect.right = me->width + rowRect.left - 2;
            rowRect.top = bounds.top + yoff + 2;
            rowRect.bottom = rowRect.top + step;
            int w = MeasureDrawListBoxText(q);
            int col = obj->colour[me->colours];
            if (me->u.list.filebuf == 0 || ((char*)me->u.list.filebuf)[y] != 1) {
                if (*q == '&') {
                    if (q[1] == 'G')
                        flag = 1;
                    q += 2;
                }
            } else {
                flag = 1;
            }
            int ty = rowRect.top;
            int f = me->attribs;
            if ((f & 1) != 0) {
                xx = rowRect.left;
                xw = rowRect.right - rowRect.left + 1;
            } else if ((4 & f) != 0) {
                xw = w;
                xx = rowRect.right - w;
            } else if (2 & f) {
                xx = (rowRect.left + rowRect.right - w) / 2;
                if (xx < rowRect.left)
                    xx = rowRect.left;
                xw = rowRect.right - xx + 1;
            }
            if (me->u.list.scroll > 6 + GetDrawListBoxLineHeight())
                DrawWrappedText(entries->u.assets.surface, q, xx, ty, xw, bounds.bottom - bounds.top, 0);
            else
                DrawTextClipped(entries->u.assets.surface, q, xx, ty, xw, 0);
            q = SkipTextLines(q, 1);
            if (flag) {
                flag = 0;
                FadeRectangle((Surface *)entries->u.assets.surface, &rowRect, -0x13);
                FadeRectangle((Surface *)entries->u.assets.surface, &rowRect, -0x14);
                FadeRectangle((Surface *)entries->u.assets.surface, &rowRect, -0x15);
                FadeRectangle((Surface *)entries->u.assets.surface, &rowRect, -0x16);
            } else if (!(me->attribs & 0x100) && me->u.list.selected == line + me->u.list.firstRow && me->u.list.rowCount) {
                if (obj->holder->current == index)
                    FadeRectangle((Surface *)entries->u.assets.surface, &rowRect, 0x1e);
                else
                    FadeRectangle((Surface *)entries->u.assets.surface, &rowRect, 0x1e);
            } else {
                SetTextColors(col, font);
            }
            line += 1;
            yoff += step;
            ++y;
            h -= step;
            if (h >= lh) {
                if (line + me->u.list.firstRow >= me->u.list.rowCount)
                    return;
            } else {
                break;
            }
        }
    } else if (flags & 0xa0) {
        DrawListBoxItem** colPtr;
        GafFrame* cellPtr;
        Rect cellRect;
        Rect clip;
        unsigned int bp = (flags >> 7) & 1;
        void* surf = entries->u.assets.surface;
        ((Surface*)surf)->GetClipRect(&clip);
        ((Surface*)surf)->SetClipRect(bounds);
        int k = me->u.list.firstRow;
        if (!bp) {
            colPtr = &((DrawListBoxItem**)me->u.list.rows)[k];
        } else {
            colPtr = 0;
            cellPtr = &((GafFrame*)me->u.list.rows)[k];
        }
        int yy = bounds.top + 2;
        bounds.left += 2;
        int y = step + yy;
        for (;;) {
            GafFrame* cell;
            if (0 != bp) {
                cell = cellPtr;
                cellPtr++;
            } else {
                cell = (*colPtr)->cell;
            }
            if (cell != 0 && cell->pixelsOrLayers != 0) {
                DrawListBoxQuad dst;
                DrawListBoxQuad src;
                dst.points[3].x = bounds.left;
                src.points[0].x = 1;
                src.points[0].y = 1;
                src.points[3].x = 1;
                src.points[1].y = 1;
                dst.points[0].x = bounds.left;
                dst.points[1].x = bounds.right;
                dst.points[2].x = bounds.right;
                dst.points[3].y = y - 1;
                dst.points[2].y = y - 1;
                src.points[1].x = cell->width - 1;
                src.points[2].x = cell->width - 1;
                dst.points[1].y = yy;
                dst.points[0].y = yy;
                src.points[2].y = cell->height - 1;
                src.points[3].y = cell->height - 1;
                DrawFrameQuad(surf, cell, &dst, &src);
                cellRect.left = dst.points[0].x;
                cellRect.right = dst.points[1].x;
                cellRect.top = dst.points[0].y;
                cellRect.bottom = dst.points[2].y;
                unsigned char v = ((char*)me->u.list.filebuf)[k];
                if (1 & v) {
                    FadeRectangle((Surface *)surf, &cellRect, -0x14);
                } else if ((2 & v) != 0) {
                    DrawLine(surf, cellRect.left + 1, cellRect.bottom - 1, cellRect.right - 2, 1 + cellRect.top, obj->colour_8be);
                    DrawLine(surf, 2 + cellRect.left, cellRect.bottom - 1, cellRect.right - 1, cellRect.top + 1, obj->colour_8be);
                    DrawLine(surf, 1 + cellRect.left, cellRect.top + 2, cellRect.right - 1, cellRect.bottom - 2, obj->colour_8be);
                    DrawLine(surf, cellRect.left + 2, cellRect.top + 2, cellRect.right - 2, cellRect.bottom - 2, obj->colour_8be);
                }
            }
            if (!(me->attribs & 0x100) && me->u.list.selected == k) {
                Rect hl;
                hl.left = bounds.left;
                hl.top = yy;
                hl.right = bounds.left + cell->width - 1;
                hl.bottom = yy + cell->height - 1;
                FadeRectangle((Surface *)surf, &hl, 0x14);
            }
            k++;
            if (!bp)
                colPtr++;
            yy += step;
            y += step;
            if (yy >= bounds.bottom || k >= me->u.list.rowCount)
                break;
        }
        ((Surface*)surf)->SetClipRect(clip);
    }
}


// FUNCTION: 0x4a23b0
void __stdcall GetSliderRects(Gadget* base, int index, int* r1, int* r2)
{
    Gadget* e = base + index;
    r1[0] = e->x;
    r1[1] = e->y;
    r1[2] = r1[0] + e->width;
    r1[3] = r1[1] + e->height;
    if (e->attribs & 1) {
        int left = r1[0] + e->knobPos + 1;
        r2[0] = left;
        r2[1] = r1[1] + 1;
        r2[2] = left + e->knobSize;
        r2[3] = r2[1] + e->height - 2;
    } else {
        r2[0] = r1[0] + 1;
        r2[1] = r1[1] + e->knobPos + 2;
        r2[2] = r2[0] + e->width - 2;
        r2[3] = r2[1] + e->knobSize;
    }
}


void RegisterUnitOrders();

void __stdcall DrawFrame(void* surface, void* image, int x, int y);

struct Pad_a2480_0 { int field; };
extern int Pad_a2480_e0;
extern int Pad_a2480_e1;
extern int Pad_a2480_e2;
extern int Pad_a2480_e3;
extern int Pad_a2480_e4;

struct FixedPosView;

// FUNCTION: 0x4a2480
void __stdcall DrawGafBar(GadgetAccessDialog* param_1, int index)
{
    Gadget* base = param_1->holder->entries;
    void* surface = base->u.assets.surface;
    Gadget* e = &base[index];
    GafEntry* glyphs = e->inputGaf;
    GafFrame* glyph = (GafFrame*)GetGafFrame(glyphs, 0);
    int x = e->x;
    int y = e->y + e->height / 2;
    if (glyph != 0)
        y -= glyph->height / 2;
    else
        y = e->y;
    int limit = e->x + e->width;
    if (glyph != 0)
        DrawFrame(surface, glyph, x, y);
    x += glyph->width;
    GafFrame* mid = (GafFrame*)GetGafFrame(glyphs, 1);
    if (x + mid->width < limit) {
        do {
            DrawFrame(surface, mid, x, y);
            x += mid->width;
        } while (x + mid->width < limit);
    }
    GafFrame* last = (GafFrame*)GetGafFrame(glyphs, 2);
    DrawFrame(surface, last, limit - last->width, y);
}
int __stdcall BroadcastPacket(int id, unsigned char* packet, int size);


struct DrawSliderBarHolder {
    char unknown_0[4];
    Gadget* entries;
};

struct DrawSliderBarObject {
    char unknown_0[0x18];
    DrawSliderBarHolder* holder;
    char unknown_1c[0x8b2 - 0x1c];
    unsigned char field_8b2;
    char unknown_8b3[0x8c1 - 0x8b3];
    unsigned char field_8c1;
    char unknown_8c2[0x8c3 - 0x8c2];
    unsigned char field_8c3;
    char unknown_8c4[0x8c6 - 0x8c4];
    unsigned char field_8c6;
};

struct DrawSliderBarFont {
    char unknown_0[0xc];
    void* glyphs;
};

struct DrawSliderBarDialog {
    int group;
    char unknown_04[0x14 - 0x04];
    DrawSliderBarFont* font;
};

void __stdcall DrawRaisedBox(void* surface, int* r, int a, int b, int c);
#include "listbox.h"
void __stdcall SetTextColors(int a, int b);
void __stdcall GetTextWidth(DrawSliderBarFont* font, char* text);

static inline void* GetSliderBarSurface(DrawSliderBarObject* o)
{
    return o->holder->entries->u.assets.surface;
}

#include <setjmp.h>

struct UnitDef;
struct Unit;

// FUNCTION: 0x4a2580
void __stdcall DrawSliderBar(DrawSliderBarObject* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    Gadget* e = &entries[index];
    void* surface = GetSliderBarSurface(obj);
    GafFrame* g;

    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == e->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1)
        SetFont(g_guiContext->group);

    int r1[4];
    int r2[4];
    GetSliderRects(entries, index, r1, r2);

    GafEntry* gl = e->sliderGaf;
    if (gl == 0) {
        DrawRaisedBox(surface, r1, obj->field_8b2, obj->field_8c3, obj->field_8c6);
        DrawSunkenBox(surface, (Rect *)r2, obj->field_8b2, obj->field_8c3, obj->field_8c6);
    } else {
        short w = e->width;
        short h = e->height;
        if (w < h) {
            void* surf = GetSliderBarSurface(obj);
            int y = e->y;
            int x = e->x;
            int limit = y + e->height - 1;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle);
            if (g != 0)
                DrawFrame(surf, g, x, y);
            y += g->height;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 1);
            while (y + g->height <= limit) {
                DrawFrame(surf, g, x, y);
                y += g->height;
            }
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 2);
            DrawFrame(surf, g, x, limit - g->height + 1);
            x += g->width / 2;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 3);
            x -= g->width / 2;
            int ybase = e->knobPos + e->y + 3;
            int lc = min(e->height - 6, e->knobSize);
            limit = lc + ybase - 1;
            int t = e->height + e->y - 4;
            limit = min(limit, t);
            if (ybase > limit - lc + 1)
                ybase = limit - lc + 1;
            DrawFrame(surf, g, x, ybase);
            lc -= g->height;
            ybase += g->height;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 4);
            while (ybase <= limit - g->height) {
                DrawFrame(surf, g, x, ybase);
                ybase += g->height;
                lc -= g->height;
            }
            DrawFrame(surf, g, x, limit - g->height);
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 5);
            DrawFrame(surf, g, x, limit - g->height + 1);
        } else {
            void* surf = GetSliderBarSurface(obj);
            int x = e->x;
            int y = e->y;
            int limit = x + e->width - 1;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle);
            if (g != 0)
                DrawFrame(surf, g, x, y);
            x += g->width;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 1);
            while (x + g->width <= limit) {
                DrawFrame(surf, g, x, y);
                x += g->width;
            }
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 2);
            DrawFrame(surf, g, limit - g->width + 1, y);
            y += g->height / 2;
            g = (GafFrame*)GetGafFrame(e->sliderGaf, e->sliderStyle + 3);
            y -= g->height / 2;
            int a = e->knobPos + e->x + 3;
            a = min(a, limit - g->width - 2);
            DrawFrame(surf, g, a, y);
        }
    }

    if (e->attribs & 4) {
        int cur = GetTextKeyColor();
        SetTextColors(obj->field_8c1, cur);
        char buf[0x10];
        if (e->u.text[0] != 0) {
            strcpy(buf, e->u.text);
        } else if (e->max != 0) {
#ifdef __clang__
            int num = (int)((double)e->knobPos * e->max / (e->width - e->knobSize));
#else
            int num = (int)((float)e->knobPos * e->max / (e->width - e->knobSize));
#endif
            _itoa(num, buf, 10);
        } else if (e->attribs & 8) {
            _itoa(e->knobPos + 1, buf, 10);
        } else {
            _itoa(e->knobPos, buf, 10);
        }
        char* text = buf;
        int total = 0;
        if (text != 0) {
            if (g_guiContext->font == 0) {
                GetTextWidth((DrawSliderBarFont*)GetFont(), text);
            } else {
                char* p = text;
                for (; *p != 0; p++) {
                    unsigned char c = *p;
                    g = (GafFrame*)GetGafFrame((unsigned short*)g_guiContext->font->glyphs, c);
                    if (g != 0)
                        total += g->width;
                }
            }
        }
        if (g_guiContext->font == 0)
            GetFontHeight();
        else
            GetGafFrame((unsigned short*)g_guiContext->font->glyphs, 0x49);
        DrawString(surface, buf, e->x + e->width + 2, e->y + 4, -1);
    }

    if ((e->attribs & 0x10) || e->sliderLocked != 0) {
        int rect[4];
        if (e->type == 0) {
            rect[0] = 0;
            rect[1] = 0;
        } else {
            rect[0] = e->x;
            rect[1] = e->y;
        }
        rect[2] = e->width + rect[0] - 1;
        rect[3] = e->height + rect[1] - 1;
        GrayRectangle((Surface *)entries->u.assets.surface, (Rect *)rect);
        FadeRectangle((Surface *)entries->u.assets.surface, (Rect *)rect, -0x14);
    }
}
#pragma pack(push, 1)
int WriteScreenshot(int, int, int, int, int, int);
#pragma pack(pop)

struct SyncAssocGadgetsHolder {
    int current;
    Gadget* entries;
    char unknown_08[0x14 - 0x08];
    void* list;
};

struct SyncAssocGadgetsDialog {
    char unknown_0[0x18];
    SyncAssocGadgetsHolder* holder;
};

void __stdcall DrawSliderBar(SyncAssocGadgetsDialog* param_1, int param_2);
void __stdcall DrawTextInput(SyncAssocGadgetsDialog* param_1, int param_2);

// FUNCTION: 0x4a2be0
void __stdcall SyncAssocGadgets(SyncAssocGadgetsDialog* param_1, int param_2)
{
    Gadget* entries = param_1->holder->entries;
    int i = 1;
    char* me = (char*)entries + param_2 * 0x15b;
    int type = *(unsigned char*)me;
    int attribs = *(int*)(me + 0x1b);
    for (; i < (short)entries->u.count + 1; i++) {
        char* entry = (char*)entries + i * 0x15b + 0x140;
        if (i != param_2) {
            if (entry[-0x13f] == me[1]) {
                switch ((unsigned char)entry[-0x140]) {
                case 2:
                    if (type == 2) {
                        *(short*)(entry - 0x84) = *(short*)(me + 0xbc);
                        *(short*)(entry - 0x86) = *(short*)(me + 0xba);
                        DrawListBox((DrawListBoxDialog *)param_1, i);
                    } else if (type == 4) {
                        int esi_val;
                        if (*(unsigned char*)(entry - 0x125) & 0x20) {
                            esi_val = *(short*)(me + 0x136) / (*(short*)(entry - 0x82) + 1);
                        } else {
                            esi_val = 0;
                        }
                        short rows = *(short*)(entry - 0x66);
                        if (rows != 0) {
                            int edx_val = *(short*)(entry - 0x80) -
                                *(short*)(entry - 0x127) / rows;
                            int eax_val = *(short*)(me + 0x140) + esi_val;
#ifdef __clang__
                            int result = (int)((double)edx_val * eax_val /
                                (*(short*)(me + 0x136) - 1));
#else
                            int result = (int)((float)edx_val * eax_val /
                                (*(short*)(me + 0x136) - 1));
#endif
                            *(short*)(entry - 0x84) = result;
                        }
                        DrawListBox((DrawListBoxDialog *)param_1, i);
                    }
                    break;
                case 3:
                    if (type == 2 && attribs & 8) {
                        char* line = SkipTextLines(*(char**)(me + 0xc2), *(short*)(me + 0xba));
                        strcpy(entry - 0x8a, line);
                        DrawTextInput(param_1, i);
                    }
                    break;
                case 4:
                    if (type == 2) {
                        if (*(short*)(me + 0xc0) > 1) {
                            int result;
                            if (*(short*)(me + 0xbe) != 0) {
                                short scale = *(short*)(me + 0xbc);
                                short height = *(short*)(entry - 0xa);
                                short count = *(short*)(me + 0xbe);
#ifdef __clang__
                                double ratio = (double)scale * height;
                                result = (int)(ratio / count);
#else
                                float ratio = (float)scale * height;
                                result = (int)(ratio / count);
#endif
                            } else {
                                result = 0;
                            }
                            if (*(short*)entry != result) {
                                *(short*)entry = result;
                                DrawSliderBar(param_1, i);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

#pragma pack(push, 1)
#include "dialogs.h"
#pragma pack(pop)

struct SetListBoxScrollByNameList {
    char unknown_0[0x0c];
    unsigned short* glyphs;
};

struct SetListBoxScrollByNameHolder {
    int current;
    Gadget* entries;
    char unknown_08[0x14 - 0x08];
    SetListBoxScrollByNameList* list;
};

#pragma pack(push, 1)
struct SetListBoxScrollByNameDialog {
    char unknown_00[0x18];
    SetListBoxScrollByNameHolder* holder;
    char unknown_1c[0xcca - 0x1c];
    int changed;
};
#pragma pack(pop)


static inline int FindSetListBoxScrollEntry(Gadget* entries, char* name) {
    int i;
    for (i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0)
            return i;
    }
    return -1;
}

static inline int FindSetListBoxScrollPeerByTeam(Gadget* entries, unsigned char kind) {
    int i;
    for (i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 4 && entries[i].team == kind)
            return i;
    }
    return 0;
}

struct Pad_a2e40_0 { int field; };
struct Pad_a2e40_1 { int field; };
struct Pad_a2e40_2 { int field; };
struct Pad_a2e40_3 { int field; };
struct Pad_a2e40_4 { int field; };
extern int Pad_a2e40_e0;
extern int Pad_a2e40_e1;
extern int Pad_a2e40_e2;

// FUNCTION: 0x4a2e40
void __stdcall SetListBoxScrollByName(SetListBoxScrollByNameDialog* param_1, char* param_2, int param_3) {
    Gadget* entries = param_1->holder->entries;
    int found = FindSetListBoxScrollEntry(entries, param_2);
    if (found == -1)
        return;

    Gadget* me = &entries[found];
    me->u.list.selected = param_3;

    int n = 0;
    int i;
    for (i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == me->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1)
        SetFont(g_guiContext->current);

    int size;
    if (g_guiContext->list == 0)
        size = GetFontHeight();
    else
        size = *(unsigned short*)(GetGafFrame(g_guiContext->list->glyphs, 0x49) + 2) + 2;
    int step = (me->height - 2) / (size + 1);
    short last = me->u.list.firstRow;
    short sel = me->u.list.selected;
    if (sel > step + last - 1 || sel < last) {
        if (me->u.list.maxFirstRow != 0)
            me->u.list.firstRow = sel;
        if (me->u.list.firstRow > me->u.list.maxFirstRow)
            me->u.list.firstRow = me->u.list.maxFirstRow;
        Gadget* peer = &entries[FindSetListBoxScrollEntry(entries, param_2)];
        unsigned char pkind = peer->team;
        Gadget* e3 = &entries[FindSetListBoxScrollPeerByTeam(entries, pkind)];
#ifdef __clang__
        double q = (double)e3->range * me->u.list.firstRow / me->u.list.maxFirstRow;
        if ((double)e3->knobPos != q)
#else
        float q = (float)e3->range * me->u.list.firstRow / me->u.list.maxFirstRow;
        if ((float)e3->knobPos != q)
#endif
            e3->knobPos = (short)q;
    }
    param_1->changed = 1;
}


struct ResetListBoxFont {
    char unknown_0[0xc];
    void* glyphs;
};

struct ResetListBoxDialog {
    int group;
    char unknown_04[0x14 - 0x04];
    ResetListBoxFont* font;
};


// FUNCTION: 0x4a30c0
void __stdcall ResetListBox(GadgetAccessDialog* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    Gadget* e = &entries[index];
    e->u.list.firstRow = 0;
    e->u.list.selected = 0;
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == e->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->group);
    }
    int step;
    if (g_guiContext->font == 0) {
        step = GetFontHeight();
    } else {
        step = *(unsigned short*)((char*)GetGafFrame(g_guiContext->font->glyphs, 0x49) + 2) + 2;
    }
    int h = e->height;
    e->height = h - h % (step + 2);
    e->u.list.sortKey = GetTicks();
}
#include "../debug/debug_lib.h"
void __stdcall TruncateTextWithEllipsis(int a1, char* text, int a3, int a4, int a5);

// FUNCTION: 0x4a31c0
void __stdcall TruncateListItems(int a1, int a2, char* a3, int a4)
{
    char* items = (char*)GameAllocIgnoreTag("SCROLLITEMS", *(int*)(a3 - 0x44));
    char* out = items;
    for (int i = 0; i < a4; i++) {
        char buf[100];
        strncpy(buf, SkipTextLines(a3, i), 100);
        TruncateTextWithEllipsis(a1, buf, a2, -1, 0);
        strcpy(out, buf);
        out += strlen(buf);
        *out = 0;
        out++;
    }
    memcpy(a3, items, out - items);
}


struct ConfigureListBoxByNameDialog {
    char unknown_00[0x18];
    ConfigureListBoxByNameHolder* holder;
};

void __stdcall SetGadgetActive(ConfigureListBoxByNameRoot* menu, int index, int value);

static inline int ConfigureListBoxFontHeight() {
    if (g_guiContext->language == 0)
        return GetFontHeight();
    return (int)((GafFrame*)GetGafFrame(g_guiContext->language->glyphs, 0x49))->height + 2;
}

static inline int FindConfigureListBoxEntry(Gadget* entries, char* name) {
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0)
            return i;
    }
    return -1;
}

static inline int FindConfigureListBoxPeerByTeam(Gadget* entries, unsigned char kind) {
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 4 && entries[i].team == kind)
            return i;
    }
    return 0;
}

// FUNCTION: 0x4a32a0
void __stdcall ConfigureListBoxByName(ConfigureListBoxByNameDialog* param_1, char* name, int bitmap, int count, int flag) {
    ConfigureListBoxByNameHolder* holder = param_1->holder;
    Gadget* entries = holder->entries;
    int index = FindConfigureListBoxEntry(entries, name);
    Gadget* me;
    if (index != -1) {
        me = &entries[index];
    } else {
        FatalError("Error in GUI layout");
        me = 0;
    }
    me->u.list.rowCount = (short)count;
    me->u.list.field_c2 = (char*)bitmap;
    me->attribs |= 0x10;
    me->u.list.scroll =
        (short)((me->u.list.scroll > ConfigureListBoxFontHeight() + 1) ? (int)me->u.list.scroll : ConfigureListBoxFontHeight() + 1);
    if (flag != 0) {
        me->u.list.language = flag;
        me->attribs |= 0x800;
    }
    me->u.list.firstRow = 0;
    me->u.list.selected = 0;
    int remain = me->height;
    me->u.list.maxFirstRow = (short)(count - 1);
    int step;
    if (me->u.list.scroll == 0) {
        step = ConfigureListBoxFontHeight() + 1;
    } else {
        step = me->u.list.scroll;
    }
    for (int j = count - 1; j > -1; j--) {
        remain -= step;
        if (remain < 0)
            break;
        me->u.list.maxFirstRow = (short)j;
    }
    if (me->active == 0)
        return;
    int i2 = FindConfigureListBoxEntry(holder->entries, name);
    Gadget* list = holder->entries;
    unsigned char kind = list[i2].team;
    int found = FindConfigureListBoxPeerByTeam(list, kind);
    if (found == -1)
        return;
    char* text = (char*)&holder->entries[found].name;
    ConfigureListBoxByNameRoot* root = g_guiContext;
    if (root->holder != 0) {
        int j2 = FindConfigureListBoxEntry(root->holder->entries, text);
        if (j2 != -1) {
            SetGadgetActive(root, j2, remain < 0);
        }
    }
    if (remain < 0) {
        DrawSlider((Dialog *)g_guiContext, found);
    }
}

#pragma pack(push, 1)
struct SetGadgetRowsRow {
    char unknown_0[2];
    unsigned short height;
    char unknown_4[0x18 - 4];
};

struct SetGadgetRowsTable {
    int unknown_0;
    Gadget* entries;
};
#pragma pack(pop)

static inline int FindSetGadgetRowsEntry(Gadget* entries, char* name)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0)
            return i;
    }
    return -1;
}

// FUNCTION: 0x4a35a0
void __stdcall SetGadgetRows(SetGadgetRowsTable* table, char* name,
                            SetGadgetRowsRow* rows, int count)
{
    Gadget* entries = table->entries;
    int index = FindSetGadgetRowsEntry(entries, name);
    Gadget* e;
    if (index != -1) {
        e = &entries[index];
    } else {
        FatalError("Error in GUI layout");
        e = 0;
    }
    e->u.list.rows = rows;
    SetGadgetRowsRow* row = rows + count - 1;
    e->u.list.rowCount = (short)count;
    e->attribs |= 0x80;
    e->u.list.firstRow = 0;
    e->u.list.selected = 0;
    e->u.list.maxFirstRow = (short)(count - 1);
    int remain = e->height;
    for (int i = count - 1; i > -1; i--, row--) {
        if (e->u.list.scroll != 0) remain -= e->u.list.scroll; else remain -= row->height;
        if (remain < 0)
            break;
        e->u.list.maxFirstRow = (short)i;
    }
}

struct SetGadgetItemsTable {
    char unknown_0[4];
    Gadget* entries;
};

struct SetGadgetItemsItem {
    char unknown_0[0x28];
    int cell;
};

static inline int FindSetGadgetItemsEntry(Gadget* entries, char* name)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (strncmp(entries[i].name, name, 0x10) == 0) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: 0x4a36a0
void __stdcall SetGadgetItems(SetGadgetItemsTable* table, char* name, int* items, int count)
{
    Gadget* entries = table->entries;
    int i = FindSetGadgetItemsEntry(entries, name);
    Gadget* e;
    if (i != -1) {
        e = &entries[i];
    } else {
        FatalError("Error in GUI layout");
        e = 0;
    }
    e->u.list.rowCount = (short)count;
    e->u.list.rows = items;
    int v = e->height;
    e->u.list.firstRow = 0;
    e->u.list.selected = 0;
    e->attribs |= 0x20;
    int* p = &items[count - 1];
    e->u.list.maxFirstRow = (short)(count - 1);
    for (int j = count - 1; j > -1; ) {
        SetGadgetItemsItem* item = (SetGadgetItemsItem*)*p;
        unsigned short w = *(unsigned short*)(item->cell + 2);
        v -= w;
        if (v < 0) {
            break;
        }
        e->u.list.maxFirstRow = (short)j;
        j--;
        p--;
    }
}
#pragma pack(push, 1)
int DrawTriangleContours(int, int, int, int, int, int, int);

struct HandleListBoxInputList {
    char unknown_0[0xc];
    unsigned short* glyphs;
};

struct HandleListBoxInputHolder {
    int current;
    Gadget* entries;
    char unknown_08[0x14 - 0x08];
    HandleListBoxInputList* list;
    char unknown_18[0x20 - 0x18];
    int selected;
};

struct HandleListBoxInputRect { int x0, y0, x1, y1; };

struct HandleListBoxInputPoint {
    int x;
    int y;
    int unknown_08[4];
};

static inline int PointInListRect(const HandleListBoxInputRect& r, const HandleListBoxInputPoint& p)
{
    return p.x >= r.x0 && p.x <= r.x1 && p.y >= r.y0 && p.y <= r.y1;
}

struct HandleListBoxInputObject {
    char unknown_00[0x18];
    HandleListBoxInputHolder* holder;
    char unknown_1c[0x3c - 0x1c];
    HandleListBoxInputPoint point;
    char unknown_54[0x60 - 0x54];
    int hotGadgetIndex;
    int focus;
    char unknown_68[0xcca - 0x68];
    int changed;
};
#pragma pack(pop)

#include "../../link/globals.h"

void __stdcall TrySetFocus(HandleListBoxInputObject* obj, int index);

int __stdcall AllocAlphaTable(int param_1);

struct HandleListBoxInputItem {
    char unknown_0[0x28];
    GafFrame* row;
};


static inline int GetHandleListBoxInputLineHeight()
{
    if (0 == g_guiContext->list)
        return GetFontHeight();
    return ((GafFrame*)GetGafFrame(g_guiContext->list->glyphs, 0x49))->height + 2;
}

static inline int SelectFontForEntry_inlined(Gadget* entries, int index)
{
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == entries[index].tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->fontId);
        i = -1;
    }
    return i;
}

// FUNCTION: 0x4a3780
int __stdcall HandleListBoxInput(HandleListBoxInputObject* obj, int index, int param_3)
{
    if (obj->hotGadgetIndex != -1)
        return 0;
    Gadget* entries = obj->holder->entries;
    Gadget* me = &entries[index];
    int orig_sel = me->u.list.selected;
    if (me->u.list.rowCount == 0)
        return 0;
    HandleListBoxInputRect r;
    unsigned int flags;
    GetGadgetRect(&entries[index], (Rect*)&r);
    int n = 0;
    r.y0 += 2;
    r.y1 -= 3;
    HandleListBoxInputPoint point = obj->point;
    point.x -= entries[0].x;
    point.y -= entries[0].y;
    int i = SelectFontForEntry_inlined(entries, index);

    int size = GetHandleListBoxInputLineHeight();
    short da = me->u.list.scroll;
    int span = (da != 0) ? da : size + 1;
    int step = (me->height - 2) / span;
    char* s;

    if (IsDoubleClickMessage((Gui *)obj, 1)) {
        if (PointInListRect(r, point)) {
            if (me->u.list.rowCount == 0) goto skip0;
            if (!(me->attribs & 0x200))
                goto ret1;
            me->u.list.selected = (point.y - r.y0) / span + me->u.list.firstRow;
            if (me->u.list.selected < 0)
                goto above;
            if (me->u.list.selected - me->u.list.firstRow > step - 1)
                me->u.list.selected = me->u.list.firstRow + step - 1;
            if (me->u.list.selected >= me->u.list.rowCount - 1)
                me->u.list.selected = me->u.list.rowCount - 1;
            s = SkipTextLines(me->u.list.field_c2, me->u.list.selected);
            if (strncmp(DAT_00502a20, s, 2) != 0)
                goto ret1;
            me->u.list.selected = orig_sel;
            return 0;
skip0:;
        }
    } else if (IsMouseButtonMessage((Gui *)obj, 1)) {
        if (PointInListRect(r, point)) {
            TrySetFocus(obj, index);
            SetClickMode(obj, 1);
        }
    } else if (IsMouseButtonMessage((Gui *)obj, 2)) {
        if (PointInListRect(r, point)) {
            TrySetFocus(obj, index);
            SetClickMode(obj, 2);
        }
    }

    if (obj->focus != index)
        goto end;
    if (!HasMouseKeyFlags((Gui *)obj, 3))
        obj->focus = -1;
    if (PointInListRect(r, point)) {
        obj->holder->selected = index;
        flags = me->attribs;
        if (flags & 0x10) {
            me->u.list.selected = (point.y - r.y0) / span + me->u.list.firstRow;
            if (me->u.list.selected >= 0) {
                if (me->u.list.selected - me->u.list.firstRow > step - 1)
                    me->u.list.selected = me->u.list.firstRow + step - 1;
                if (me->u.list.selected >= me->u.list.rowCount - 1)
                    me->u.list.selected = me->u.list.rowCount - 1;
                if (me->u.list.selected < 0)
                    me->u.list.selected = 0;
                if (flags & 0x200) {
                    s = SkipTextLines(me->u.list.field_c2, me->u.list.selected);
                    if (strncmp(DAT_00502a20, s, 2) == 0)
                        me->u.list.selected = orig_sel;
                }
                for (i = 1; i <= entries[0].u.count; i++) {
                    if (entries[i].type == 2 && entries[i].team == me->team)
                        entries[i].u.list.selected = min(entries[i].u.list.rowCount - 1, me->u.list.selected);
                }
            } else {
                me->u.list.selected = orig_sel;
            }
        } else if (flags & 0x20 | 0x80) {
            int flag8 = (flags >> 7) & 1;
            GafFrame* fixed;
            HandleListBoxInputItem** ip;
            if (flag8)
                fixed = &((GafFrame*)me->u.list.rows)[me->u.list.firstRow];
            else
                ip = &((HandleListBoxInputItem**)me->u.list.rows)[me->u.list.firstRow];
            int k = me->u.list.firstRow;
            int remain = point.y - r.y0 - 2;
            int n2 = 0;
            for (;;) {
                GafFrame* row = flag8 ? fixed : (*ip)->row;
                if (me->u.list.scroll != 0)
                    remain -= span;
                else
                    remain -= row->height;
                if (remain <= 0) {
                    me->u.list.selected = n2 + me->u.list.firstRow;
                    break;
                }
                if (flag8)
                    fixed++;
                else
                    ip++;
                n2++;
                k++;
                if (k > me->u.list.rowCount - 1)
                    break;
            }
        }
        if (orig_sel != me->u.list.selected) {
            DrawListBox((DrawListBoxDialog *)obj, index);
            if (me->u.list.callback)
                me->u.list.callback((Gui*)obj, me);
        }
        if (me->attribs & 0x40) {
above:
ret1:
            return 1;
        }
        obj->changed = 1;
    } else if (point.y < r.y0) {
        if (me->u.list.firstRow > 0 && me->u.list.sortKey < (int)GetTicks()) {
            me->u.list.sortKey = GetTicks() + 2;
            if (me->u.list.selected > me->u.list.firstRow)
                me->u.list.selected = me->u.list.firstRow;
            me->u.list.firstRow--;
            me->u.list.selected--;
            short sel = me->u.list.selected;
            if (me->u.list.field_c2 != 0) {
                s = SkipTextLines(me->u.list.field_c2, sel < 0 ? 0 : sel);
                if (strncmp(DAT_00502a20, s, 2) == 0)
                    me->u.list.selected = orig_sel;
            }
            goto finish;
        }
        if (me->u.list.selected > 0) {
            me->u.list.selected = 0;
            goto finish;
        }
    } else if (point.y > r.y1) {
        if (me->u.list.firstRow < me->u.list.maxFirstRow && me->u.list.sortKey < (int)GetTicks()) {
            me->u.list.sortKey = GetTicks() + 2;
            me->u.list.firstRow++;
            me->u.list.selected = me->u.list.firstRow + step - 1;
            if (me->u.list.field_c2 != 0) {
                s = SkipTextLines(me->u.list.field_c2, me->u.list.selected);
                if (strncmp(DAT_00502a20, s, 2) == 0)
                    me->u.list.selected = orig_sel;
            }
finish:
            DrawListBox((DrawListBoxDialog *)obj, index);
            SyncAssocGadgets((SyncAssocGadgetsDialog *)obj, index);
        }
    }
end:
    return obj->hotGadgetIndex != -1;
}



struct ResetSliderGadgetTable {
    char unknown_0[4];
    Gadget* entries;
};

struct ResetSliderGadgetDialog {
    char unknown_0[0x18];
    ResetSliderGadgetTable* table;
};

static inline void ClearEntry(Gadget* e)
{
    e->knobPos = 0;
    e->sliderCallback = 0;
    e->sliderUser = 0;
}

struct Pad_a3eb0_0 { int field; };
struct Pad_a3eb0_1 { int field; };
struct Pad_a3eb0_2 { int field; };

// FUNCTION: 0x4a3eb0
void __stdcall ResetSliderGadget(ResetSliderGadgetDialog* obj, int i)
{
    ClearEntry(&obj->table->entries[i]);
}


struct DrawSliderFont {
    char unknown_0[0x28];
    GafFrame* glyph;
};

void __stdcall DrawSliderBar(Dialog* gui, int gadgetIndex);

static inline GafFrame* GetDrawSliderGlyph(unsigned char c)
{
    return (GafFrame*)GetGafFrame(g_guiContext->list->glyphs, c);
}

static inline int FindDrawSliderPeerByTeam(Gadget* entries, unsigned char kind)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 2 && entries[i].team == kind)
            return i;
    }
    return 0;
}

static inline int GetDrawSliderRowsHeight(Gadget* e)
{
    int count = e->u.list.rowCount;
    DrawSliderFont** font = (DrawSliderFont**)e->u.list.rows;
    int lines = 0;
    if (count > 0)
        lines = (*font)->glyph->height * count;
    return lines;
}

// FUNCTION: 0x4a3ef0
void __stdcall DrawSlider(Dialog* gui, int gadgetIndex)
{
    Gadget* entries = (Gadget*)gui->holder->entries;
    Gadget* me = &entries[gadgetIndex];
    unsigned char kind = me->team;
    int found = FindDrawSliderPeerByTeam(entries, kind);
    union { int full; short word; } lines;
    lines.full = 0;
    lines.word = 0;
    if (found != 0) {
        Gadget* e = &entries[found];
        if (e->type == 2) {
            if (e->attribs & 0x10) {
                int i = 1;
                int n = 0;
                for (; i < entries->u.count + 1; i++) {
                    if (entries[i].type == 7) {
                        if (n == e->tab) {
                            SetFont(entries[i].u.list.language);
                            break;
                        }
                        n++;
                    }
                }
                if (i == entries->u.count + 1) {
                    SetFont(g_guiContext->current);
                }
                int size = (g_guiContext->list == 0) ? GetFontHeight()
                    : GetDrawSliderGlyph(0x49)->height + 2;
                int numerator = e->height - 2;
                int denominator = (e->u.list.scroll > size + 1) ? e->u.list.scroll : size + 1;
                int step = numerator / denominator;
                int last = e->u.list.rowCount;
#ifdef __clang__
                int rows = (int)((double)step / last * (me->height - 3));
#else
                int rows = (int)((float)step / last * (me->height - 3));
#endif
                me->knobSize = rows;
                if (me->knobSize < 10) {
                    me->knobSize = 10;
                }
                if (last <= step) {
                    me->range = 0;
                } else {
                    me->range = me->height - me->knobSize - 3;
                }
            } else if (e->attribs & 0x20) {
                lines.full = GetDrawSliderRowsHeight(e);
                int s = e->height * me->height / lines.full;
                me->knobSize = s;
                if (me->attribs & 1) {
                    me->range = me->width - s;
                } else {
                    me->range = me->height - s;
                }
            } else if (e->attribs & 0x80) {
                if (e->u.list.scroll != 0 && e->u.list.rowCount != 0) {
                    int s = e->height / e->u.list.scroll * me->height / e->u.list.rowCount;
                    me->knobSize = s;
                    if (me->attribs & 1) {
                        me->range = me->width - s;
                    } else {
                        me->range = me->height - s;
                    }
                }
            }
        }
    }
    DrawSliderBar(gui, gadgetIndex);
}


struct HandleSliderInputHolder {
    char unknown_00[4];
    Gadget* entries;
    char unknown_08[0x14 - 0x08];
    int dirty;
};

struct HandleSliderInputPoint {
    int x;
    int y;
    int unknown_08[4];
};

struct HandleSliderInputObject {
    char unknown_00[0x18];
    HandleSliderInputHolder* holder;
    char unknown_1c[0x3c - 0x1c];
    HandleSliderInputPoint point;
    char unknown_54[0x64 - 0x54];
    int focus;
    char unknown_68[0x78 - 0x68];
    int knobDragging;
    HandleSliderInputPoint saved;
    short dragKnobPos;
};

void __stdcall TrySetFocus(HandleSliderInputObject* obj, int index);
void __stdcall DrawSliderBar(HandleSliderInputObject* obj, int index);

static inline void ClampKnobPosAndNotify(HandleSliderInputObject* obj, int index, Gadget* e, int old)
{
    if (e->knobPos > e->range - 1)
        e->knobPos = e->range - 1;
    if (e->knobPos < 0)
        e->knobPos = 0;
    if (e->knobPos == old)
        return;
    if (obj->holder)
        obj->holder->dirty = 1;
    DrawSliderBar(obj, index);
    SyncAssocGadgets((SyncAssocGadgetsDialog *)obj, index);
    if (e->sliderCallback)
        e->sliderCallback((Gui*)obj, e->sliderUser);
}

// FUNCTION: 0x4a4170
void __stdcall HandleSliderInput(HandleSliderInputObject* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    Gadget* e = &entries[index];
    if (e->attribs & 0x10)
        return;
    if (e->sliderLocked)
        return;

    HandleSliderInputPoint p = obj->point;
    p.x -= entries->x;
    p.y -= entries->y;
    int r1[4];
    int r2[4];
    GetSliderRects(entries, index, r1, r2);

    if (obj->focus == index) {
        if (!HasMouseKeyFlags((Gui *)obj, 3)) {
            obj->focus = -1;
            obj->knobDragging = 0;
        }
        if (obj->knobDragging) {
            int old = e->knobPos;
            if (e->attribs & 1)
                e->knobPos = obj->dragKnobPos - obj->saved.x + p.x;
            else
                e->knobPos = obj->dragKnobPos - obj->saved.y + p.y;
            ClampKnobPosAndNotify(obj, index, e, old);
        } else {
            int old = e->knobPos;
            if (e->attribs & 1) {
                if (p.x < r2[0])
                    e->knobPos--;
                else if (p.x > r2[2])
                    e->knobPos++;
            } else {
                if (p.y < r2[1])
                    e->knobPos--;
                else if (p.y > r2[3])
                    e->knobPos++;
            }
            ClampKnobPosAndNotify(obj, index, e, old);
        }
        return;
    }

    if (obj->knobDragging)
        return;
    if (IsMouseButtonMessage((Gui *)obj, 1)) {
        obj->knobDragging = 0;
        if (p.x < r1[0] || p.x > r1[2] || p.y < r1[1] || p.y > r1[3])
            return;
        TrySetFocus(obj, index);
        SetClickMode(obj, 1);
        if (p.x < r2[0] || p.x > r2[2] || p.y < r2[1] || p.y > r2[3])
            return;
        obj->saved = p;
        obj->knobDragging = 1;
        obj->dragKnobPos = e->knobPos;
        return;
    }
    if (IsMouseButtonMessage((Gui *)obj, 2)) {
        obj->knobDragging = 0;
        if (p.x < r1[0] || p.x > r1[2] || p.y < r1[1] || p.y > r1[3])
            return;
        TrySetFocus(obj, index);
        SetClickMode(obj, 2);
        if (p.x < r2[0] || p.x > r2[2] || p.y < r2[1] || p.y > r2[3])
            return;
        obj->saved = p;
        obj->knobDragging = 1;
        obj->dragKnobPos = e->knobPos;
    }
}
int __cdecl tolower(int);
int __cdecl toupper(int);


struct HandleLabelInputHolder {
    int unknown_0;
    Gadget* entries;
};

#pragma pack(push, 1)
struct HandleLabelInputObject {
    char unknown_0[0x18];
    HandleLabelInputHolder* holder;
    char unknown_1c[0x3c - 0x1c];
    DialogMouseEvent point;
    char unknown_54[0x64 - 0x54];
    int focus;
    char unknown_68[0xcc6 - 0x68];
    int pathsReady;
};
#pragma pack(pop)

int __stdcall TrySetFocus(HandleLabelInputObject* obj, int index);
#include "keyboard.h"

// FUNCTION: 0x4a4440
int __stdcall HandleLabelInput(HandleLabelInputObject* obj, int index, char key)
{
    Gadget* entries = obj->holder->entries;
    Gadget* entry = &entries[index];
    struct HandleLabelInputRect { int x1, y1, x2, y2; } rect;
    DialogMouseEvent point;
    int rel_x, rel_y;

    if (entry->hotkey == 0 && (entry->attribs & 0x10))
        return 0;

    if (entry->type == 0) {
        rect.x1 = 0;
        rect.y1 = 0;
    } else {
        rect.x1 = entry->x;
        rect.y1 = entry->y;
    }
    rect.x2 = entry->width + rect.x1 - 1;
    rect.y2 = entry->height + rect.y1 - 1;

    memcpy(&point, &obj->point, 24);

    rel_x = point.x - entries->x;
    rel_y = point.y - entries->y;

    if (IsMouseButtonMessage((Gui *)obj, 1)) {
        if (rel_x < rect.x1 || rel_x > rect.x2 || rel_y < rect.y1 || rel_y > rect.y2)
            goto fail;
        TrySetFocus(obj, index);
        SetClickMode(obj, 1);
    } else if (IsMouseButtonMessage((Gui *)obj, 2)) {
        if (rel_x < rect.x1 || rel_x > rect.x2 || rel_y < rect.y1 || rel_y > rect.y2)
            goto fail;
        TrySetFocus(obj, index);
        SetClickMode(obj, 2);
    }

fail:
    if (obj->focus != index)
        goto check_queue;
    if (HasMouseKeyFlags((Gui *)obj, 3))
        goto check_queue;
    obj->focus = -1;
    if (rel_x < rect.x1 || rel_x > rect.x2 || rel_y < rect.y1 || rel_y > rect.y2)
        goto check_queue;
    return 1;

check_queue:
    if (obj->focus != -1) {
        Gadget* e = &entries[obj->focus];
        if (e->type == 3 && !IsKeyDown(0xfb))
            return 0;
    }

final_check:
    if (obj->pathsReady != 1 || *(int*)&key == 0)
        return 0;
    if ((char)tolower((char)entry->hotkey) != key &&
        (char)toupper((char)entry->hotkey) != key)
        return 0;
    PopKey();
    return 1;
}


struct ScheduleGadgetEndTickTable {
    char unknown_0[4];
    Gadget* entries;
};

struct ScheduleGadgetEndTickDialog {
    char unknown_0[0x18];
    ScheduleGadgetEndTickTable* table;
};


#include <time.h>

extern int Pad_a4620_e0;

// FUNCTION: 0x4a4620
void __stdcall ScheduleGadgetEndTick(ScheduleGadgetEndTickDialog* obj, int i)
{
    Gadget* e = &obj->table->entries[i];
    e->u.anim.next = GetTicks() + e->u.anim.interval;
}

struct DrawProgressBarGadgetHolder {
    char unknown_0[4];
    Gadget *entries;
};

struct DrawProgressBarGadgetDialog {
    char unknown_0[0xc];
    void *surface;
    int unknown_10;
    void *oldSurface;
    DrawProgressBarGadgetHolder *holder;
    char unknown_1c[0x8b2 - 0x1c];
    unsigned char color1;
    char unknown_8b3[0x8c3 - 0x8b3];
    unsigned char color2;
    char unknown_8c4[2];
    unsigned char color3;
    char unknown_8c7[0xcd2 - 0x8c7];
    void *fallbackSurface;
};

struct DrawProgressBarGadgetRect { int left, top, right, bottom; };

struct DrawProgressBarGadgetLanguage { char unknown_0[0xc]; unsigned short *glyphs; };
struct DrawProgressBarGadgetLanguageRoot { char unknown_0[0x14]; DrawProgressBarGadgetLanguage *language; };

void __stdcall LockScreen(void *);
void __stdcall FillRectangle(void *, DrawProgressBarGadgetRect *, int);
int __stdcall GetTextWidth(int, char *);
void __stdcall UnlockScreen(void *);

static inline int MeasureProgressBarText(char *text)
{
    int width = 0;
    char *p = text;
    if (p == 0)
        return 0;
    if (g_guiContext->language == 0)
        return GetTextWidth(GetFont(), text);
    char *q = text;
    while (*q != 0) {
        char ch = *q;
        GafFrame *glyph = (GafFrame *)GetGafFrame(
            g_guiContext->language->glyphs, (unsigned char)ch);
        if (glyph != 0)
            width += glyph->width;
        ++q;
    }
    return width;
}

// FUNCTION: 0x4a4660
void __stdcall DrawProgressBarGadget(DrawProgressBarGadgetDialog *obj, int index)
{
    Gadget *entries = obj->holder->entries;
    Gadget *entry = (Gadget *)((char *)entries + index * 0x15b);
    obj->oldSurface = obj->surface;

    void *surface = entries->u.assets.surface;
    if (surface == 0)
        surface = *(void **)((char *)obj + 0xcd2);
    LockScreen(surface);

    DrawProgressBarGadgetRect rect;
    rect.left = entry->x;
    rect.top = entry->y;
    rect.right = entry->width + entry->x;
    rect.bottom = entry->height + entry->y;
    FillBevelBox(surface, (Rect *)&rect, obj->color1, obj->color2, obj->color3);

    rect.left += 2;
    rect.top += 2;
    rect.right -= 2;
    rect.bottom -= 2;
    FillRectangle(surface, &rect, entry->image);

#ifdef __clang__
    double scale = (double)entry->u.anim.value / entry->u.anim.total;
    rect.right = (int)(scale * (entry->width - 4)) + rect.left;
#else
    float scale = (float)entry->u.anim.value / entry->u.anim.total;
    rect.right = (int)(scale * (entry->width - 4)) + rect.left;
#endif
    FillRectangle(surface, &rect, entry->colours);

    if (entry->u.anim.showText != 0) {
        char text[20];
        _itoa(entry->u.anim.value, text, 10);
        int width = MeasureProgressBarText(text);
        int height;
        if (g_guiContext->language == 0) {
            height = GetFontHeight();
        } else {
            GafFrame *glyph = (GafFrame *)GetGafFrame(
                g_guiContext->language->glyphs, 0x49);
            height = glyph->height + 2;
        }
        DrawTextClipped(surface, text,
            (entry->width / 2 - width / 2) + entry->x,
            (entry->height / 2 - height / 2) + entry->y, -1, 0);
    }

    obj->oldSurface = *(void **)((char *)obj + 8);
    UnlockScreen(surface);
}


struct TickProgressBarTable {
    char unknown_0[4];
    Gadget* entries;
};

struct TickProgressBarDialog {
    char unknown_0[0x18];
    TickProgressBarTable* table;
};


// FUNCTION: 0x4a4890
void __stdcall TickProgressBar(TickProgressBarDialog* obj, int i)
{
    Gadget* e = &obj->table->entries[i];
    if (e->u.anim.active && e->u.anim.value < e->u.anim.max) {
        if ((int)GetTicks() > e->u.anim.next) {
            e->u.anim.value += (int)e->u.anim.step;
            if (e->u.anim.value > e->u.anim.max) {
                e->u.anim.value = e->u.anim.max;
                e->u.anim.active = 0;
            }
            e->u.anim.next = GetTicks() + e->u.anim.interval;
        }
        DrawProgressBarGadget((DrawProgressBarGadgetDialog *)obj, i);
    }
}
struct ResetProgressBarEntries {
    char unknown_0[4];
    char* base;
};

struct ResetProgressBarDialog {
    char unknown_0[0x18];
    ResetProgressBarEntries* arr;
};

// FUNCTION: 0x4a4930
void __stdcall ResetProgressBar(ResetProgressBarDialog* param_1, int index)
{
    char* e = param_1->arr->base + index * 0x15b;
    *(int*)(e + 0xb6) = 0;
    *(int*)(e + 0xbe) = 0;
    *(int*)(e + 0xc2) = 0;
    *(short*)(e + 0xc6) = 0;
}


struct DrawHotspotHolder {
    char unknown_0[4];
    Gadget* entries;
};

#pragma pack(push, 1)
struct DrawHotspotDialog {
    char unknown_0[0x18];
    DrawHotspotHolder* holder;
    char unknown_1c[0x8b9 - 0x1c];
    unsigned char colour7;
};
#pragma pack(pop)

struct DrawHotspotPoint {
    int x;
    int y;
};

struct DrawHotspotQuad {
    DrawHotspotPoint p[4];
};


void __stdcall DrawFrame(void* surface, void* frame, int x, int y);
int __stdcall FillRectangle(void* surface, Rect* rect, int color);
void __stdcall DrawFrameQuad(void* surf, void* entry, DrawHotspotQuad* dst, DrawHotspotQuad* src);

// FUNCTION: 0x4a4980
void __stdcall DrawHotspot(DrawHotspotDialog* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    Gadget* e = &entries[index];

    Rect rect;
    if (e->type == 0) {
        rect.left = 0;
        rect.top = 0;
    } else {
        rect.left = e->x;
        rect.top = e->y;
    }
    rect.right = e->width + rect.left - 1;
    rect.bottom = e->height + rect.top - 1;

    DrawHotspotQuad dst;
    dst.p[0].x = rect.left;
    dst.p[3].x = rect.left;
    dst.p[0].y = rect.top;
    dst.p[1].x = rect.right;
    dst.p[1].y = rect.top;
    dst.p[2].x = rect.right;
    dst.p[2].y = rect.bottom;
    dst.p[3].y = rect.bottom;

    DrawHotspotQuad src;
    src.p[0].x = 1;
    src.p[0].y = 1;
    src.p[3].x = 1;
    src.p[1].y = 1;

    void* field_be = e->u.hotspot.gaf;
    if (field_be != 0) {
        GafFrame* result = (GafFrame*)GetGafFrame(field_be, e->u.hotspot.index);
        if (result != 0) {
            src.p[1].x = result->width - 1;
            src.p[2].x = result->width - 1;
            src.p[2].y = result->height - 1;
            src.p[3].y = result->height - 1;
            if (result->compressed == 0) {
                DrawFrameQuad(entries->u.assets.surface, result, &dst, &src);
                return;
            }
            DrawFrame(entries->u.assets.surface, result, result->xOffset + rect.left, result->yOffset + rect.top);
            return;
        }
    } else if (e->u.hotspot.frame != 0) {
        src.p[1].x = e->u.hotspot.frame->width - 1;
        src.p[2].x = e->u.hotspot.frame->width - 1;
        src.p[2].y = e->u.hotspot.frame->height - 1;
        src.p[3].y = e->u.hotspot.frame->height - 1;
        DrawFrameQuad(entries->u.assets.surface, e->u.hotspot.frame, &dst, &src);
    } else {
        FillRectangle(entries->u.assets.surface, &rect, obj->colour7);
    }
}

struct Feature;
struct WeaponDef;
struct Explosion;

#pragma pack(push, 1)

struct HandleHotspotInputDialog;
struct HandleHotspotInputPoint { int x, y; };

int GetType();
int KeepAfterComplete();
int IsFxStyle();
void TickTowardGoal();
int HasNetUnitState();
int TryClaimRepath();

struct HandleHotspotInputTable {
    char unknown_0[4];
    Gadget* entries;
};

struct HandleHotspotInputDialog {
    char unknown_0[0x18];
    HandleHotspotInputTable* table;
    char unknown_1c[0x3c - 0x1c];
    HandleHotspotInputPoint pos;
    char unknown_44[0x64 - 0x44];
    int focus;
};
#pragma pack(pop)

extern int __stdcall TrySetFocus(HandleHotspotInputDialog* obj, int index);

struct HandleHotspotInputRect { int left, top, right, bottom; };

static inline int PointInHotspotRect(const HandleHotspotInputRect& r, const HandleHotspotInputPoint& p)
{
    return p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom;
}

// FUNCTION: 0x4a4b50
int __stdcall HandleHotspotInput(HandleHotspotInputDialog* obj, int index)
{
    Gadget* entries = obj->table->entries;
    HandleHotspotInputRect r;
    if (entries[index].type == 0) {
        r.left = 0;
        r.top = 0;
    } else {
        r.left = entries[index].x;
        r.top = entries[index].y;
    }
    r.right = entries[index].width - 1 + r.left;
    r.bottom = entries[index].height - 1 + r.top;
    if (entries[index].u.hotspot.callback)
        entries[index].u.hotspot.callback((Gui*)obj, &entries[index]);
    if (entries[index].u.hotspot.flags & 1) {
        if (IsMouseButtonMessage((Gui *)obj, 1)) {
            HandleHotspotInputPoint p = obj->pos;
            if (PointInHotspotRect(r, p)) {
                TrySetFocus(obj, index);
                SetClickMode(obj, 1);
            }
        } else if (IsMouseButtonMessage((Gui *)obj, 2)) {
            HandleHotspotInputPoint p = obj->pos;
            if (PointInHotspotRect(r, p)) {
                TrySetFocus(obj, index);
                SetClickMode(obj, 2);
            }
        }
        if (obj->focus == index && !HasMouseKeyFlags((Gui *)obj, 3)) {
            obj->focus = -1;
            HandleHotspotInputPoint* pp = &obj->pos;
            HandleHotspotInputPoint q;
            q.y = pp->y;
            q.x = pp->x;
            if (PointInHotspotRect(r, q))
                return 1;
        }
    }
    return 0;
}

int GetHostDpid();

struct DrawLineGadgetHolder {
    char unknown_0[4];
    Gadget* entries;
};

struct DrawLineGadgetDialog {
    char unknown_0[0x18];
    DrawLineGadgetHolder* holder;
    char unknown_1c[0x8b2 - 0x1c];
    unsigned char colour[0x100];
};

void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2,
                            int color);

static inline void GetLineGadgetRect(Gadget* e, Rect* r)
{
    if (e->type == 0) {
        r->left = 0;
        r->top = 0;
    } else {
        r->left = e->x;
        r->top = e->y;
    }
    r->right = e->width - 1 + r->left;
    r->bottom = e->height - 1 + r->top;
}

// FUNCTION: 0x4a4c90
void __stdcall DrawLineGadget(DrawLineGadgetDialog* obj, int index, unsigned char param_3)
{
    Gadget* entries = obj->holder->entries;
    Gadget* e = (Gadget*)((char*)entries + index * 0x15b);
    Rect rect;
    GetLineGadgetRect(e, &rect);
    if (param_3 & 1) {
        if (e->attribs & 1)
            DrawLine(entries->u.assets.surface, rect.left, rect.top, rect.right, rect.top,
                         obj->colour[e->colours]);
        else if (e->attribs & 2)
            DrawLine(entries->u.assets.surface, rect.left, rect.top, rect.left, rect.bottom,
                         obj->colour[e->colours]);
        else if (e->attribs & 4)
            DrawLine(entries->u.assets.surface, rect.left, rect.top, rect.right, rect.bottom,
                         obj->colour[e->colours]);
    }
}

void InitNetConnection();

#pragma pack(push, 1)
struct DrawTextInputList {
    char unknown_0[0x0c];
    unsigned short* glyphs;
};

struct DrawTextInputHolder {
    int current;
    Gadget* entries;
    char unknown_08[0x14 - 0x08];
    DrawTextInputList* language;
    char unknown_18[0x24 - 0x18];
    void* surface;
};

struct DrawTextInputDialog {
    char unknown_00[0x18];
    DrawTextInputHolder* holder;
    char unknown_1c[0x64 - 0x1c];
    int focus;
    char unknown_68[0x74 - 0x68];
    int cursor;
    char unknown_78[0x8b2 - 0x78];
    unsigned char colours[0xcd2 - 0x8b2];
    void* fallback;
};

struct DrawTextInputRect { int left, top, right, bottom; };

struct DrawTextInputLanguageRoot {
    int current;
    char unknown_04[0x14 - 0x04];
    DrawTextInputList* language;
};
#pragma pack(pop)

int __stdcall DrawListboxFrame(DrawTextInputDialog* obj, int index, void* bmp);
int __stdcall FillRectangle(void* surface, DrawTextInputRect* rect, int colour);
void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2,
                            int colour);

static inline GafFrame* GetDrawTextInputGlyph(unsigned char c)
{
    return (GafFrame*)GetGafFrame(g_guiContext->language->glyphs, c);
}

static inline int MeasureTextInputText(char* text)
{
    int width = 0;
    char* p = text;
    if (p == 0)
        return 0;
    if (g_guiContext->language == 0)
        return GetTextWidth(GetFont(), text);
    char* q = text;
    while (*q != 0) {
        char ch = *q;
        GafFrame* glyph = GetDrawTextInputGlyph(ch);
        if (glyph != 0)
            width += glyph->width;
        ++q;
    }
    return width;
}

static inline int SelectTextInputFont(Gadget* entries, int index)
{
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == entries[index].tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->fontId);
        i = -1;
    }
    return i;
}

// FUNCTION: 0x4a4d70
void __stdcall DrawTextInput(DrawTextInputDialog* obj, int index)
{
    Gadget* entries = obj->holder->entries;
    SelectTextInputFont(entries, index);

    Gadget* me = &entries[index];

    DrawTextInputRect rect;
    if (me->type == 0) {
        rect.left = 0;
        rect.top = 0;
    } else {
        rect.left = me->x;
        rect.top = me->y;
    }
    rect.right = me->width + rect.left - 1;
    rect.bottom = me->height + rect.top - 1;

    if (me->attribs & 1) {
        FillRectangle(entries->u.assets.surface, &rect, obj->colours[0]);
    } else {
        void* surface = obj->holder->surface;
        if (surface == 0)
            surface = obj->fallback;
        if (surface == 0) {
            DrawListboxFrame(obj, index, 0);
        } else {
            CopySurfaceRect((Surface *)entries->u.assets.surface, (Surface *)surface, (Rect *)&rect, (Point *)((int*)&rect));
        }
    }

    SetTextColors(obj->colours[me->colours], GetTextKeyColor());
    rect.top += 3;
    DrawTextClipped(entries->u.assets.surface, me->u.text, rect.left, rect.top,
                 rect.right - rect.left, entries[index].colours);

    if (index == obj->focus) {
        char* at = &me->u.text[obj->cursor];
        char save = *at;
        *at = 0;
        int w = MeasureTextInputText(me->u.text);
        *at = save;
        int height;
        if (g_guiContext->language == 0)
            height = GetFontHeight();
        else
            height = GetDrawTextInputGlyph(0x49)->height + 2;
        int x = rect.left + w;
        DrawLine(entries->u.assets.surface, x, rect.top, x, height + rect.top, obj->colours[9]);
    }
}

struct GetCharGlyphFont {
    char unknown_0[0xc];
    void* glyphs;
};

struct GetCharGlyphDialog {
    char unknown_0[0x14];
    GetCharGlyphFont* font;
};

// FUNCTION: 0x4a5010
void* __stdcall GetCharGlyph(unsigned char c)
{
    return GetGafFrame(g_guiContext->font->glyphs, c);
}

struct GetTextPixelWidthFont {
    char unknown_0[0xc];
    void* glyphs;
};

struct GetTextPixelWidthDialog {
    char unknown_0[0x14];
    GetTextPixelWidthFont* font;
};

int __stdcall GetTextWidth(int param_1, unsigned char* text);

// FUNCTION: 0x4a5030
int __stdcall GetTextPixelWidth(unsigned char* text)
{
    int width = 0;
    if (text == 0)
        return 0;
    if (g_guiContext->font == 0)
        return GetTextWidth(GetFont(), text);
    for (int i = 0; text[i]; i++) {
        unsigned char c = text[i];
        unsigned short* glyph = (unsigned short*)GetGafFrame(g_guiContext->font->glyphs, c);
        if (glyph)
            width += *glyph;
    }
    return width;
}

struct FontGlyphTable {
    char unknown_0[3];
    unsigned char first;
    unsigned short offsets[1];
    char unknown_6[6];
    void* glyphs;
};

struct GetFontLineHeightDialog {
    char unknown_0[0x14];
    FontGlyphTable* font;
};


// FUNCTION: 0x4a50b0
int GetFontLineHeight()
{
    if (g_guiContext->font == 0) {
        return GetFontHeight();
    }
    return ((GafFrame*)GetGafFrame(g_guiContext->font->glyphs, 'I'))->height + 2;
}

struct DrawTextClippedFont {
    char unknown_0[0xc];
    void* glyphs;
};

struct DrawTextClippedDialog {
    char unknown_0[0x14];
    DrawTextClippedFont* font;
};

#include "../graphics/gaf.h"
void __stdcall DrawString(void* surface, char* text, int x, int y, int maxWidth);

// FUNCTION: 0x4a50e0
void __stdcall DrawTextClipped(void* surface, char* text, int x, int y, int maxw, int style)
{
    if (g_guiContext->font == 0) {
        DrawString(surface, text, x, y, -1);
        return;
    }
    unsigned char* s = (unsigned char*)text;
    while (*s) {
        if (*s >= ' ') {
            unsigned char c = *s;
            GafFrame* g = (GafFrame*)GetGafFrame(g_guiContext->font->glyphs, c);
            if (g) {
                if (maxw != -1 && (int)g->width > maxw)
                    return;
                if (*s != ' ') {
                    if (style == 0)
                        DrawFrame(surface, g, x, y);
                    else
                        DrawFrameLit((Surface *)surface, g, x, y, style);
                }
                if (maxw != -1) {
                    maxw -= g->width;
                    if (maxw < 0)
                        return;
                }
                x += g->width;
            }
        }
        s++;
    }
}


struct DrawWrappedTextFont {
    char unknown_0[0xc];
    void* glyphs;
};

struct DrawWrappedTextDialog {
    char unknown_0[0x14];
    DrawWrappedTextFont* font;
};

int __stdcall GetTextWidth(int a, char* text);

static inline int GetDrawWrappedTextLineHeight()
{
    if (g_guiContext->font == 0)
        return GetFontHeight();
    return (int)((GafFrame*)GetGafFrame(g_guiContext->font->glyphs, 'I'))->height + 2;
}

static inline int Measure(char* word, int t)
{
    if (word == 0)
        return t;
    if (g_guiContext->font == 0)
        return GetTextWidth(GetFont(), word);
    for (char* n = word; *n; n++) {
        unsigned char ch = *n;
        unsigned short* g = (unsigned short*)GetGafFrame(g_guiContext->font->glyphs, ch);
        if (g)
            t += *g;
    }
    return t;
}

// FUNCTION: 0x4a51d0
int __stdcall DrawWrappedText(char* p2, char* text, int p4, int y, int maxw, int rem, int p7)
{
    int last = 0;
    int w;
    int k = 0;
    int i = 0;
    int len = strlen(text);
    while (text[k] != 0) {
        while (i != len && text[i] != ' ' && text[i] != '\r')
            i++;
        char saved = text[i];
        char* word = text + k;
        text[i] = 0;
        w = 0;
        w = Measure(word, w);
        if (w > maxw) {
            text[i] = saved;
            i = last;
            saved = text[i];
            text[i] = 0;
        } else if (saved != '\r' && i != len) {
            last = i;
            text[i] = saved;
            i++;
            continue;
        }
        DrawTextClipped(p2, word, p4, y, maxw, p7);
        text[i] = saved;
        y += GetDrawWrappedTextLineHeight() + 2;
        rem -= GetDrawWrappedTextLineHeight() + 2;
        if (text[i] == 0)
            goto done;
        k = i + 1;
        if (rem <= 0)
            goto done;
        last = k;
        i = k;
    }
done:
    return y;
}


#pragma pack(push, 1)

struct GafEntry {
    unsigned short count;
    unsigned short unknown_2;
};

struct Gui;

int WalkFrameChain(int, int, int, int, int, int, int, int, int, int);
int RIReport(int, int, int, int, int, int, int, int, int, int);
int OrderSquad(int, int, int, int, int, int, int, int);
int AddBlinkWord(int, int, int, int, int, int, int, int);
int WalkStack(int, int, int, int, int, int, int);
int SendScriptCallByName(int, int, int, int, int, int, int);
int SendScriptCall(int, int, int, int, int, int, int);
int IssueOrCancelOrder(int, int, int, int, int, int, int);
int IsFootprintVisible(int, int, int, int, int, int);
int DrawProgressBar(int, int, int, int, int, int);
int PickRandomReclaimableResourcesInRadius(int, int, int, int, int, int);
int CreateUnit(int, int, int, int, int, int);

struct Language {
    char unknown_0[0xc];
    GafEntry* glyphs;
};

struct MouseRecord {
    int x;
    int y;
    int unknown_08[4];
};


#pragma pack(pop)


void HideSoftwareCursor();
int __cdecl toupper(int c);

GafFrame* __stdcall GetGafFrame(GafEntry* table, int index);
GafEntry* __stdcall FindGafEntry(void* gaf, const char* name);
void* __stdcall LoadGaf(char* path);
#include "../graphics/palette.h"
char* __stdcall Translate(char* key);
char* __stdcall GetGadgetText(Gui* menu, char* name, char* buf);

void* __stdcall AllocSurface(char* name, int width, int height);
void __stdcall DrawSurface(void* dst, void* bmp, int x, int y);
int __stdcall FillRectangle(void* surface, Rect* rect, int colour);
void __stdcall DrawLine(void* surface, int x1, int y1, int x2, int y2, int colour);
void __stdcall DrawFrame(void* surface, GafFrame* glyph, int x, int y);
int __stdcall DrawWrappedText(void* surface, char* text, int x, int y, int maxw, int rem, int style);

void __stdcall AssignQuickKey(Gui* obj, int index);
void __stdcall ClearPeerStatus(Gui* obj, int index);
void __stdcall DrawGadgetSelectionBox(Gui* obj, int index, int param3);
void __stdcall DrawSliderBar(Gui* obj, int index);
void __stdcall DrawTextInput(Gui* obj, int index);
void __stdcall DrawListboxFrame(Gui* obj, int index, void* bmp);
void __stdcall DrawLineGadget(Gui* obj, int index, unsigned int param3);
int __stdcall HandleLabelInput(Gui* obj, int index, int key);
int __stdcall HandleHotspotInput(Gui* obj, int index);
int __stdcall TrySetFocus(Gui* obj, int index);
int __stdcall HandleListBoxInput(Gui* obj, int index, int key);
void __stdcall HandleSliderInput(Gui* obj, int index);
#include "../graphics/object_pool.h"
int __stdcall SelectFontForEntry(void* entries, int index);

static inline int GetTextPixelWidth(char* text)
{
    int width = 0;
    if (text == 0)
        return 0;
    if (g_guiContext->language == 0)
        return GetTextWidth(GetFont(), text);
    char* p = text;
    while (*p != 0) {
        char ch = *p;
        GafFrame* glyph = GetGafFrame(g_guiContext->language->glyphs, (unsigned char)ch);
        if (glyph != 0)
            width += glyph->width;
        ++p;
    }
    return width;
}

static inline int LineHeight()
{
    if (g_guiContext->language == 0)
        return GetFontHeight();
    GafFrame* glyph = GetGafFrame(g_guiContext->language->glyphs, 0x49);
    return glyph->height + 2;
}

static inline int LineHeightDirect()
{
    if (g_guiContext->language == 0)
        return GetFontHeight();
    return (int)GetGafFrame(g_guiContext->language->glyphs, 0x49)->height + 2;
}

static inline int SelectLabelFont(Gadget* entries, int index)
{
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == entries[index].tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->fontId);
        i = -1;
    }
    return i;
}

// FUNCTION: 0x4a53c0
void __stdcall LayoutLabelText(Gui* obj, int index)
{
    Gadget* entries = obj->layer->entries;
    Gadget* entry = &entries[index];
    SelectLabelFont(entries, index);
    int x = !entry->type ? 0 : entry->x;
    int nx = x;
    int lh;
    if (g_guiContext->language == 0)
        lh = GetFontHeight();
    else
        lh = GetGafFrame(g_guiContext->language->glyphs, 0x49)->height + 2;
    if (entry->attribs & 4) {
        nx = entry->width + x;
        nx -= GetTextPixelWidth(entry->u.text);
    } else if (entry->attribs & 2) {
        nx = entry->width / 2 + x;
        int half = GetTextPixelWidth(entry->u.text) / 2;
        nx -= half;
        entry->width = (short)(half * 2);
    } else if (entry->attribs & 1)
        entry->width = (short)GetTextPixelWidth(entry->u.text);
    entries[index].x = (short)nx;
    entry->height = (short)lh;
}

// FUNCTION: 0x4a56b0
void __stdcall DrawLabel(Gui* obj, int index)
{
    obj->language = obj->values[1];
    Gadget* entries = obj->layer->entries;

    int i = SelectFontForEntry_inlined(entries, index);


    if (entries[index].x == -1)
        entries[index].x = (short)((entries[0].width - GetTextPixelWidth(entries[index].u.text)) / 2);

    Rect rect;
    GetGadgetRect(&entries[index], &rect);

    if (entries[index].image != 0)
        FillRectangle(entries->u.assets.surface, &rect, obj->colours[entries[index].image]);
    int nx = rect.left;
    Gadget* entry = &entries[index];

    if (entry->attribs & 4) {
        nx = entry->width + rect.left;
        nx -= GetTextPixelWidth(entry->u.text);
    } else if (entry->attribs & 2) {
        nx = entry->width / 2 + rect.left;
        int half = GetTextPixelWidth(entry->u.text) / 2;
        nx -= half;
    }

    if (i != -1 && (entries[index].attribs & 8)) {
        SetTextColors(obj->colours[0], GetTextKeyColor());
        DrawString(entries->u.assets.surface, entries[index].u.text, nx + 1, rect.top + 3, -1);
    }

    SetTextColors(entries[index].colours, GetTextKeyColor());

    if (i == -1) {
        int lh = LineHeightDirect();
        if (rect.bottom - rect.top > lh * 2)
            DrawWrappedText(entries->u.assets.surface, entries[index].u.text, nx, rect.top,
                         rect.right - rect.left + 1,
                         rect.bottom - rect.top + 1, entries[index].colours);
        else
            DrawTextClipped(entries->u.assets.surface, entries[index].u.text, nx, rect.top,
                         rect.right - rect.left + 1, entries[index].colours);
    } else {
        DrawString(entries->u.assets.surface, entries[index].u.text, nx, rect.top, -1);
    }

    if (entries[index].flag_148 & 1) {
        Gadget* entries2 = obj->layer->entries;
        Rect rect2;
        if (entries2[index].type == 0) {
            rect2.left = 0;
            rect2.top = 0;
        } else {
            rect2.left = entries2[index].x;
            rect2.top = entries2[index].y;
        }
        rect2.right = entries2[index].width + rect2.left - 1;
        rect2.bottom = entries2[index].height + rect2.top - 1;
        GrayRectangle((Surface *)entries2->u.assets.surface, &rect2);
        FadeRectangle((Surface *)entries2->u.assets.surface, &rect2, -0x14);
    } else {
        unsigned char c = entries[index].hotkey;
        if (c != 0) {
            char pat[2];
            pat[0] = (char)c;
            pat[1] = 0;
            char buf[0x80];
            strcpy(buf, entries[index].u.text);
            char* p = strstr(buf, pat);
            if (p != 0) {
                int y = rect.top;
                *p = 0;
                int x0 = rect.left;
                x0 += GetTextPixelWidth(buf);
                int x1 = x0;
                x0 += GetTextPixelWidth(pat);
                int lh1 = LineHeightDirect();
                int lh2 = LineHeightDirect();
                DrawLine(obj->layer->entries->u.assets.surface, x1, lh2 + y - 1, x0 - 1,
                             lh1 + y - 1, obj->colours[2]);
            }
        }
    }

    obj->language = obj->values[0];
}

// FUNCTION: 0x4a5d30
void __stdcall SetCurrentFont(Gui* p, int index)
{
    p->language = p->values[index];
}

// FUNCTION: 0x4a5d50
int __stdcall TruncateGadgetText(Gui* menu, int index)
{
    Gadget* entries = menu->layer->entries;
    char* text = GetGadgetText(menu, entries[index].name, 0);
    if (text == 0)
        return 0;
    if (entries[index].type == 5 ||
        (entries[index].type == 1 && (entries[index].attribs & 0x8000) != 0))
        menu->language = menu->values[1];
    while (1) {
        int w = GetTextPixelWidth(text);
        if (w <= entries[index].width - 6) {
            menu->language = menu->values[0];
            return w;
        }
        if (strlen(text) == 0)
            continue;
        text[strlen(text) - 1] = 0;
    }
}

// FUNCTION: 0x4a5e50
void __stdcall DrawGadgetGlyph(Gui* obj, int index)
{
    if (obj->layer != 0)
        obj->layer->dirty = 1;
    Gadget* entries = obj->layer->entries;
    Gadget* e = &entries[index];
    GafFrame* glyph = e->u.frame.glyph;
    if (glyph == 0)
        return;
    Rect rect;
    if (e->type == 0) {
        rect.left = 0;
        rect.top = 0;
    } else {
        rect.left = e->x;
        rect.top = e->y;
    }
    rect.right = e->width + rect.left - 1;
    rect.bottom = e->height + rect.top - 1;
    int count = e->colours;
    if (count > 0) {
        DrawFrameLit((Surface *)entries->u.assets.surface, glyph, glyph->xOffset + rect.left, glyph->yOffset + rect.top, count);
    } else {
        DrawFrame(entries->u.assets.surface, glyph, glyph->xOffset + rect.left, glyph->yOffset + rect.top);
    }
    if (e->u.frame.flag & 1) {
        FadeRectangle((Surface *)entries->u.assets.surface, &rect, -0x1c);
    }
}


// FUNCTION: 0x4a5f40
void __stdcall DrawButton(Gui* menu, int index)
{
    char* p;
    char key2[2];
    void* surface;
    Gadget* me;
    Rect rect;
    int x;
    int width;
    int border;
    int flagy;
    char* text;
    int pass;
    int saved;
    char buf[0x80];
    int y, i;
    int t;
    int textw;

    border = 0;
    if (menu->layer != 0)
        menu->layer->dirty = 1;
    Gadget* entries = menu->layer->entries;
    me = &entries[index];
    if (me->type == 0) {
        rect.left = 0;
        rect.top = 0;
    } else {
        rect.left = me->x;
        rect.top = me->y;
    }
    rect.right = me->width + rect.left - 1;
    rect.bottom = rect.top + me->height - 1;
    if (me->attribs & 0x8000)
        menu->language = menu->values[1];

    i = SelectFontForEntry_inlined(entries, index);

    textw = TruncateGadgetText(menu, index);
    surface = entries->u.assets.surface;
    if (me->gaf != 0) {
        GafFrame* glyph;
        if (me->grayedout & 1) {
            if (me->attribs & 0x100) {
                glyph = GetGafFrame(me->gaf, me->gaf->count - 1);
            } else if (me->stages != 0) {
                glyph = GetGafFrame(me->gaf, me->stageIndex);
                border = 1;
            } else if (me->attribs & 0x1800) {
                glyph = GetGafFrame(me->gaf, me->frameBase);
                border = 1;
            } else {
                int val = me->gaf->count - 1;
                if (me->status + 2 < val)
                    val = me->status + 2;
                glyph = GetGafFrame(me->gaf, val + me->frameBase);
                if (!(me->attribs & 0x80))
                    border = 1;
            }
        } else {
            if (me->status != 0 && (unsigned short)me->gaf->count > (unsigned short)me->stages) {
                if (me->stages != 0)
                    glyph = GetGafFrame(me->gaf, me->gaf->count - 2);
                else
                    glyph = GetGafFrame(me->gaf, me->frameBase + me->status);
            } else if (me->stages != 0)
                glyph = GetGafFrame(me->gaf, me->stageIndex);
            else
                glyph = GetGafFrame(me->gaf, me->frameBase);
        }
        if (glyph != 0) {
            if (me->colours != 0)
                DrawFrameLit((Surface *)surface, glyph, glyph->xOffset + rect.left, glyph->yOffset + rect.top, me->colours);
            else
                DrawFrame(surface, glyph, glyph->xOffset + rect.left, glyph->yOffset + rect.top);
        }
    } else {
        if (me->grayedout & 1) {
            FillBevelBox(surface, &rect, menu->colours[0], menu->colours[0x13], menu->colours[0x13]);
        } else if (me->status != 0) {
            FillBevelBox(surface, &rect, menu->colours[0], menu->colours[0x11], menu->colours[0x14]);
        } else {
            FillBevelBoxDarkFirst(surface, &rect, menu->colours[0], menu->colours[0x11], menu->colours[0x14]);
        }
    }

    t = 0;
    flagy = 0;
    if (me->status != 0) {
        t = 1;
        flagy = 1;
    }
    text = me->u.text;
    pass = 0;
    do {
        if (me->status != 0)
            SetTextColors(menu->colours[0], GetTextKeyColor());
        else
            SetTextColors(menu->colours[me->colours], GetTextKeyColor());

        p = text;
        if (me->stages != 0) {
            for (unsigned int k = me->stageIndex; k != 0; k--) {
                while (*p != 0)
                    p++;
                p++;
            }
        }

        y = (rect.bottom - LineHeight() - rect.top) / 2 + flagy + rect.top;
        if (me->attribs & 0x8000)
            menu->language = menu->values[1];

        if (me->attribs & 1) {
            DrawTextClipped(surface, p, t + rect.left + 3, y,
                         rect.right - rect.left + 1, 0);
        } else if (me->attribs & 4) {
            x = rect.right - textw - 3;
            if (x < rect.left)
                x = rect.left;
            DrawTextClipped(surface, p, x, y, rect.right - rect.left + 1, 0);
        } else if (me->attribs & 2) {
            x = (rect.right - textw - rect.left) / 2 + t;
            x += rect.left + 1;
            if (me->quickKey == 0 || (me->grayedout & 1)) {
                DrawTextClipped(surface, p, x, y, 1 + (rect.right - rect.left), 0);
            } else {
                char key1[2];
                char* found;
                key1[0] = me->quickKey;
                width = rect.right - rect.left + 1;
                strcpy(buf, p);
                key1[1] = 0;
                found = strstr(buf, key1);
                if (found != 0) {
                    GetFont();
                    strcpy(buf, p);
                    *found = 0;
                    DrawTextClipped(surface, buf, x, y, width, 0);
                    x += GetTextPixelWidth(buf);
                    saved = x;
                    if (me->status != 0)
                        SetTextColors(menu->colours[0], GetTextKeyColor());
                    else
                        SetTextColors(menu->colours[me->colours], GetTextKeyColor());
                    DrawTextClipped(surface, key1, x, y, width, 0);
                    x += GetTextPixelWidth(key1);
                    if (me->status != 0) {
                        DrawLine(surface, saved, LineHeight() + y - 1,
                                     x - 1, LineHeight() + y - 1,
                                     menu->colours[0]);
                    } else {
                        DrawLine(surface, saved, LineHeight() + y - 1,
                                     x - 1, LineHeight() + y - 1,
                                     menu->colours[2]);
                    }
                    if (me->status != 0)
                        SetTextColors(menu->colours[0], GetTextKeyColor());
                    else
                        SetTextColors(menu->colours[me->colours], GetTextKeyColor());
                    DrawTextClipped(surface, found + 1, x, y, width, 0);
                } else {
                    DrawTextClipped(surface, p, x, y, width, 0);
                }
            }
        } else if (me->attribs & 0x20) {
            int xb;
            char* found;
            xb = (rect.right - textw - rect.left) / 2 + t;
            xb += rect.left + 1;
            int ys = flagy - LineHeight();
            ys += rect.bottom - 4;
            if (me->quickKey != 0 && (found = strchr(p, (signed char)me->quickKey)) != 0) {
                width = rect.right - rect.left + 1;
                key2[0] = me->quickKey;
                key2[1] = 0;
                GetFont();
                *found = 0;
                DrawTextClipped(surface, p, xb, ys, width, 0);
                xb += GetTextPixelWidth(text);
                SetTextColors(menu->colours[10], GetTextKeyColor());
                DrawTextClipped(surface, key2, xb, ys, width, 0);
                xb += GetTextPixelWidth(key2);
                SetTextColors(menu->colours[me->colours], GetTextKeyColor());
                DrawTextClipped(surface, found + 1, xb, ys, width, 0);
            } else {
                DrawTextClipped(surface, p, xb, ys, rect.right - rect.left + 1, 0);
            }
        }
    } while (pass--);

    menu->language = menu->values[0];
    if (border) {
        GrayRectangle((Surface *)surface, &rect);
        FadeRectangle((Surface *)surface, &rect, -0x14);
    }
}

// FUNCTION: 0x4a69d0
void __stdcall ClearAllStatus(Gui* param_1)
{
    Gadget* entries = param_1->layer->entries;
    Gadget* e = &entries[1];
    for (int i = 1; i < entries->u.count + 1; i++, e++) {
        if (e->type == 1 && e->status != 0) {
            e->status = 0;
            DrawButton(param_1, i);
            param_1->changed = 1;
        }
    }
}

// FUNCTION: 0x4a6a40
void __stdcall ClearGroupStatus(Gui* param_1, int index)
{
    Gadget* entries = param_1->layer->entries;
    Gadget* e = &entries[1];
    unsigned char team = entries[index].team;
    for (int i = 1; i < entries->u.count + 1; i++, e++) {
        if (e->type == 1 && e->team == team && e->status != 0) {
            e->status = 0;
            DrawButton(param_1, i);
            param_1->changed = 1;
        }
    }
}

static inline int PointInButtonRect(const Rect* r, int px, int py)
{
    return px >= r->left && px <= r->right && py >= r->top && py <= r->bottom;
}

static inline int FindKind(Gadget* entries, unsigned char kind)
{
    for (int i = 1; i < entries->u.count + 1; i++) {
        if (entries[i].type == 4 && entries[i].team == kind)
            return i;
    }
    return 0;
}

// FUNCTION: 0x4a6ae0
int __stdcall HandleButtonInput(Gui* obj, int index, int param_3)
{
    Gadget* entries = obj->layer->entries;
    Gadget* entry = &entries[index];
    if (entry->grayedout & 1)
        goto fail;

    Rect r;
    GetGadgetRect(entry, &r);

    MouseRecord point = *(MouseRecord*)&obj->pointX;
    point.x -= entries->x;
    point.y -= entries->y;

    if (PointInButtonRect(&r, point.x, point.y)) {
        obj->hoverGadgetIndex = index;
        if (IsMouseButtonMessage(obj, 1)) {
            obj->focus = -1;
            TrySetFocus(obj, index);
            SetClickMode(obj, 1);
            obj->clickStatusCache = entry->status;
        } else if (IsMouseButtonMessage(obj, 2)) {
            obj->focus = -1;
            TrySetFocus(obj, index);
            SetClickMode(obj, 2);
            obj->clickStatusCache = entry->status;
        }
    }

    if (obj->focus == index) {
        if (entry->attribs & 0x10) {
            if (!HasMouseKeyFlags(obj, 3))
                goto fail;
            obj->focus = -1;
            if (point.x < r.left || point.x > r.right
                || point.y < r.top || point.y > r.bottom) {
                entry->status = obj->clickStatusCache;
                DrawButton(obj, index);
                return 0;
            }
            entry->status = 1;
            ClearPeerStatus(obj, index);
            DrawButton(obj, index);
            return 1;
        }
        if (entry->attribs & 0x40) {
            if (!HasMouseKeyFlags(obj, 3)) {
                obj->focus = -1;
                if (point.x < r.left || point.x > r.right
                    || point.y < r.top || point.y > r.bottom) {
                    entry->status = obj->clickStatusCache;
                    DrawButton(obj, index);
                    return 0;
                }
                entry->status = (obj->clickStatusCache == 0);
                ClearPeerStatus(obj, index);
                DrawButton(obj, index);
                return 1;
            }
            if (point.x < r.left || point.x > r.right
                || point.y < r.top || point.y > r.bottom) {
                if (entry->status == 0)
                    goto fail;
                entry->status = 0;
                DrawButton(obj, index);
                return 0;
            }
            if (entry->status != 0)
                goto fail;
            entry->status = 1;
            DrawButton(obj, index);
            return 0;
        }
        if (entry->attribs & 8) {
            if (HasMouseKeyFlags(obj, 3))
                goto fail;
            if (!PointInButtonRect(&r, point.x, point.y))
                goto fail;
            if (entry->status == 1)
                entry->status = 0;
            else if (entry->status == 0)
                entry->status = 1;
            ClearPeerStatus(obj, index);
            DrawButton(obj, index);
            obj->focus = -1;
            return 1;
        }
        if (entry->attribs & 0x100) {
            if (!IsMouseButtonMessage(obj, 1))
                goto fail;
            if (!PointInButtonRect(&r, point.x, point.y))
                goto fail;
            GafEntry* p = entry->gaf;
            if (p != 0) {
                if (entry->status < p->count - 1)
                    entry->status += 1;
                else
                    entry->status = 0;
            }
            ClearPeerStatus(obj, index);
            DrawButton(obj, index);
            obj->focus = -1;
            return 1;
        }
        if (!HasMouseKeyFlags(obj, 3)) {
            obj->focus = -1;
            entry->status = 0;
            ClearPeerStatus(obj, index);
            if (PointInButtonRect(&r, point.x, point.y)
                && !(entry->attribs & 0x1800)) {
                if (entry->stages != 0) {
                    entry->stageIndex += 1;
                    if (entry->stageIndex >= entry->stages)
                        entry->stageIndex = 0;
                }
                DrawButton(obj, index);
                return 1;
            }
            DrawButton(obj, index);
            return 0;
        }
        if (entry->status != 0 && (entry->attribs & 0x2000)) {
            if (DAT_0051fbb0 == GetTicks())
                goto fail;
            DAT_0051fbb0 = GetTicks();
            if (DAT_0051fbac > 0) {
                DAT_0051fbac -= 1;
                return 0;
            }
        } else if (entry->status == 0
                   && PointInButtonRect(&r, point.x, point.y)) {
            entry->status = 1;
            DAT_0051fbac = 0xf;
        } else {
            if (entry->status == 0)
                goto fail;
            if (PointInButtonRect(&r, point.x, point.y))
                goto fail;
            entry->status = 0;
            DrawButton(obj, index);
            return 0;
        }
        DrawButton(obj, index);
        int flags = entry->attribs;
        if (!(flags & 0x1800))
            goto fail;
        unsigned char team = entry->team;
        int found = FindKind(entries, team);
        if (found == -1)
            goto fail;
        Gadget* f = &entries[found];
        short off = f->knobPos;
        if (flags & 0x1000) {
            if (off > 0)
                f->knobPos = off - 1;
        } else {
            if (off < f->range - 1)
                f->knobPos = off + 1;
        }
        if (obj->layer)
            obj->layer->dirty = 1;
        DrawSliderBar(obj, found);
        SyncAssocGadgets((SyncAssocGadgetsDialog *)obj, found);
        if (f->sliderCallback)
            f->sliderCallback(obj, f->sliderUser);
        return 0;
    } else {
        if (!(obj->focus != -1 && entries[obj->focus].type == 3) || IsKeyDown(0xfb)) {
            if (obj->pathsReady == 1) {
                if (param_3 != 0) {
                    if ((char)tolower((char)entry->quickKey) == (char)param_3
                        || (char)toupper((char)entry->quickKey) == (char)param_3) {
                        if (entry->attribs & 0x40) {
                            entry->status = (entry->status == 0);
                            DrawButton(obj, index);
                        } else if (entry->attribs & 0x10) {
                            if (entry->status == 0) {
                                entry->status = 1;
                                DrawButton(obj, index);
                            }
                        }
                        ClearPeerStatus(obj, index);
                        PopKey();
                        return 1;
                    }
                }
            }
        }
    }
fail:
    return 0;
}

// FUNCTION: 0x4a7190
void __stdcall BeginTextEdit(Gui* obj, int index)
{
    Gadget* entries = obj->layer->entries;
    Gadget* target = &entries[index];

    SetTextColors(obj->colours[target->colours], GetTextKeyColor());

    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == target->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->fontId);
    }

    TrySetFocus(obj, index);
    obj->layer->current = index;
    CommitTextEdit(obj, index, target->u.text, target->status, 0);
    ClearKeyQueue();
}

static inline void Activate(Gui* obj, int index)
{
    Rect rect;
    Gadget* ep = obj->layer->entries;
    GetGadgetRectByIndex((char*)ep, index, &rect);
    SetTextColors(obj->colours[ep[index].colours], GetTextKeyColor());
    SelectFontForEntry((void*)ep, index);
    TrySetFocus(obj, index);
    obj->layer->current = index;
    CommitTextEdit(obj, index, ep[index].u.text, ep[index].status, 0);
    ClearKeyQueue();
}

// FUNCTION: 0x4a7290
int __stdcall HandleTextInput(Gui* obj, int index, int key)
{
    Gadget* entries = obj->layer->entries;
    Rect rect;
    GetGadgetRectByIndex((char*)entries, index, &rect);
    SelectFontForEntry_inlined(entries, index);

    MouseRecord point = *(MouseRecord*)&obj->pointX;
    int rel_x = point.x - entries->x;
    int rel_y = point.y - entries->y;

    if (IsPointInRect(&rect, rel_x, rel_y)) {
        obj->hoverGadgetIndex = index;
        if (IsMouseButtonMessage(obj, 1)) {
            Activate(obj, index);
            SetClickMode(obj, 1);
        } else if (IsMouseButtonMessage(obj, 2)) {
            Activate(obj, index);
            SetClickMode(obj, 2);
        }
    }

    if (IsFocused((int)obj, index)) {
        SetTextColors(obj->colours[entries[index].colours],
                     obj->colours[entries[index].image]);
        int r = HandleTextEditKey(obj, index, key);
        if (r == 13) {
            ClearFocus((Dialog*)obj);
            return 1;
        }
        if (r == 27) {
            ClearFocus((Dialog*)obj);
            entries[index].u.text[0] = 0;
            return 1;
        }
        if (obj->layer)
            obj->layer->dirty = 1;
    }
    return 0;
}

// FUNCTION: 0x4a7560
int __stdcall FindNextTextInput(Gadget* list, int index)
{
    int old = index;
    for (index++; index < list[0].u.count + 1; index++) {
        if (list[index].type == 3)
            break;
    }
    if (index == list[0].u.count + 1) {
        for (index = 1; index < old; index++) {
            if (list[index].type == 3)
                break;
        }
    }
    return index;
}

// FUNCTION: 0x4a75d0
int __stdcall LoadScreenGaf(Gui* obj, char* name)
{
    char path[256];
    path[0] = 0;
    if (obj->str_ab6[0] != 0)
        strncpy(path, obj->str_ab6, 0x100);
    strcat(path, name);
    ChangeExtension(path, path, "GAF");
    if (HAPI_FileLengthByName(path)) {
        obj->layer->entries->u.assets.archive = LoadGaf(path);
        if (obj->layer->entries->u.assets.archive != 0)
            return 1;
    }
    return 0;
}

static inline void DoSelect(Gui* menu, Gadget* entries, int sel)
{
    Gadget* entry = &entries[sel];
    SetTextColors(menu->colours[entry->colours], GetTextKeyColor());
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == entry->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1)
        SetFont(g_guiContext->fontId);
    TrySetFocus(menu, sel);
    menu->layer->current = sel;
    CommitTextEdit(menu, sel, entry->u.text, entry->status, 0);
    ClearKeyQueue();
}

// FUNCTION: 0x4a76b0
void __stdcall SelectGadgetByName(Gui* menu, char* name)
{
    Gadget* entries = menu->layer->entries;
    int index = FindEntry(entries, name);
    if (index != -1) {
        menu->focus = -1;
        menu->layer->current = index;
        if (entries[menu->layer->current].type == 3) {
            DoSelect(menu, menu->layer->entries, menu->layer->current);
        }
    }
}

// FUNCTION: 0x4a7830
void __stdcall SelectGadgetByIndex(Gui* menu, int index)
{
    Gadget* first = menu->layer->entries;
    menu->focus = -1;
    menu->layer->current = index;
    if (first[menu->layer->current].type == 3) {
        int i = menu->layer->current;
        Gadget* entries = menu->layer->entries;
        Gadget* entry = &entries[i];
        int font = GetTextKeyColor();
        SetTextColors(menu->colours[entry->colours], font);

        int n = 0;
        int j = 1;
        for (; j < entries->u.count + 1; j++) {
            if (entries[j].type == 7) {
                if (n == entry->tab) {
                    SetFont(entries[j].u.list.language);
                    break;
                }
                n++;
            }
        }
        if (j == entries->u.count + 1) {
            SetFont(g_guiContext->fontId);
        }

        TrySetFocus(menu, i);
        menu->layer->current = i;
        CommitTextEdit(menu, i, entry->u.text, entry->status, 0);
        ClearKeyQueue();
    }
}

// FUNCTION: 0x4a7960
void __stdcall SelectAdjacentGadget(Gui* menu, int dir)
{
    int used[200];

    Layer* layer = menu->layer;
    int index = layer->current;
    Gadget* entries = layer->entries;
    if (index == -1)
        return;

    for (int k = 0; k < 50; k++)
        used[k] = 0;

    for (int i = 1; i < entries->u.count + 1; i++) {
        int idx = -1;
        for (int j = 1; used[j] != 0; j++) {
            int d = used[j] - entries[i].x;
            if (d < 10 && d > -10) {
                idx = j;
                break;
            }
        }
        if (idx != -1)
            used[i] = used[idx];
        else
            used[i] = entries[i].x;
    }

    int start;
    int bound;
    switch (dir) {
    case 0:
        start = entries[index].x + entries[index].y * 5000;
        bound = start - 0x17d7840;
        break;
    case 2:
        start = entries[index].y + used[index] * 5000;
        bound = start - 0x17d7840;
        break;
    case 1:
        start = entries[index].x + entries[index].y * 5000;
        bound = start + 0x17d7840;
        break;
    case 3:
        start = entries[index].y + used[index] * 5000;
        bound = start + 0x17d7840;
        break;
    }

    {
    int pos;
    for (int i = 1; i < entries->u.count + 1; i++) {
        int* up = &used[i];
        char* b = (char*)&entries[i].width;
        if (*(signed char*)(b + 0x12) != 0 && !(*(int*)(b + 4) & 0x400)
            && !(*(unsigned char*)(b - 0x17) == 1 && (*(unsigned char*)(b + 0x125) & 1))
            && !(*(unsigned char*)(b - 0x17) == 4 && *(int*)(b + 0x140) != 0)) {
            if (*(unsigned char*)(b - 0x17) == 3 || *(unsigned char*)(b - 0x17) == 4
                || *(unsigned char*)(b - 0x17) == 1
                || *(unsigned char*)(b - 0x17) == 6
                || *(unsigned char*)(b - 0x17) == 2) {
                if (!(*(unsigned char*)(b - 0x17) == 4
                      && *(short*)b < *(short*)(b + 2))) {
                    if (!(*(unsigned char*)(b - 0x17) == 2 && (*(int*)(b + 4) & 0x100))
                        && !(*(unsigned char*)(b - 0x17) == 1
                             && (*(unsigned char*)(b + 0x125) & 1))) {
                        switch (dir) {
                        case 0:
                        case 1:
                            pos = *(short*)(b - 4) + *(short*)(b - 2) * 5000;
                            break;
                        case 2:
                        case 3:
                            pos = *(short*)(b - 2) + *up * 5000;
                            break;
                        }
                        switch (dir) {
                        case 1:
                        case 3:
                            if (pos <= start)
                                pos += 0x17d7840;
                            if (pos < bound) {
                                bound = pos;
                                index = i;
                            }
                            break;
                        case 0:
                        case 2:
                            if (pos >= start)
                                pos -= 0x17d7840;
                            if (pos > bound) {
                                bound = pos;
                                index = i;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    }

    menu->focus = -1;
    layer->current = index;
    if (entries[menu->layer->current].type == 3)
        BeginTextEdit(menu, menu->layer->current);
    if (entries[menu->layer->current].type == 3)
        BeginTextEdit(menu, menu->layer->current);
}

// FUNCTION: 0x4a7ee0
void __stdcall MoveEntryToFront(Gadget* entries, int index)
{
    Gadget temp;
    if (index != -1) {
        temp = entries[index];
        for (int i = index; i > 1; i--) {
            entries[i] = entries[i - 1];
        }
        entries[1] = temp;
    }
}

// FUNCTION: 0x4a7f70
void __stdcall FindButtonGaf(Gui* button, Gadget* obj)
{
    char name[0x10];
    char str[0x20];
    int best;
    GafEntry* entry = 0;
    Gadget* holder = button->layer->entries;
    strncpy(name, obj->name, 0x10);
    name[0xf] = 0;
    obj->frameBase = 0;
    void* gaf = holder->u.assets.archive;
    if (gaf)
        entry = FindGafEntry(gaf, name);
    if (entry == 0) {
        if (button->gaf != 0) {
            entry = FindGafEntry(button->gaf, name);
            if (entry == 0) {
                if (obj->attribs & 0x80) {
                    entry = FindGafEntry(button->gaf, "CHECKBOX");
                } else if (obj->stages != 0) {
                    int n = obj->stages < 4 ? obj->stages : 4;
                    sprintf(str, "stagebuttn%d", n);
                    entry = FindGafEntry(button->gaf, str);
                    if (obj->stages == 1) {
                        obj->stages = 2;
                        obj->attribs |= 0x4000;
                    }
                } else {
                    strcpy(str, "BUTTONS0");
                    entry = FindGafEntry(button->gaf, str);
                }
                if (entry != 0) {
                    best = 1000;
                    for (int i = 0; i < entry->count; i++) {
                        GafFrame* f = GetGafFrame(entry, i);
                        if (f != 0) {
                            f->yOffset = 0;
                            f->xOffset = 0;
                        }
                    }
                    for (int j = 0; j < entry->count; j += 4) {
                        GafFrame* f = GetGafFrame(entry, j);
                        int d = abs(obj->height - f->height) + abs(obj->width - f->width);
                        if (d < best) {
                            obj->frameBase = (unsigned char)j;
                            best = d;
                        }
                    }
                }
            }
        }
    }
    obj->gaf = entry;
    if (entry != 0) {
        GafFrame* f = GetGafFrame(entry, obj->frameBase);
        if (f != 0) {
            obj->width = f->width;
            obj->height = f->height;
        }
    }
}

// FUNCTION: 0x4a8150
int __stdcall AddGadgetEntry(Gui* obj, unsigned char type)
{
    Gadget* entries = obj->layer->entries;
    entries->u.count++;
    Gadget* e = &entries[entries->u.count];
    memset(&entries[entries->u.count], 0, sizeof(Gadget));
    e->type = type;
    e->active = 1;
    return entries->u.count;
}

// FUNCTION: 0x4a81b0
void __stdcall GetAnimsPath(Gui* obj, char* out)
{
    *out = 0;
    if (obj->str_ab6[0] != 0)
        strncpy(out, obj->str_ab6, 0x100);
}

static inline void ClearGlyphOffsets(GafEntry* g)
{
    for (int f = 0; f < g->count; f++) {
        GafFrame* frame = GetGafFrame(g, f);
        frame->yOffset = 0;
        frame->xOffset = 0;
    }
}

// FUNCTION: 0x4a81e0
int __stdcall RenderLayer(Gui* menu, unsigned int flags)
{
    int savedType;
    int orientation;
    Gadget* entries;
    char* name;
    int* pf;
    int i, force;
    GafEntry* g;

    if (!menu->layer)
        return 0;
    entries = menu->layer->entries;
    if ((flags & 0x100) && (flags & 1)) {
        entries[0].y = -1;
        entries[0].x = -1;
    }
    if ((flags & 0x1000) && (flags & 1)) {
        entries[0].y = -2;
        entries[0].x = -2;
    }
    if (-1 == entries[0].x) {
        entries[0].x = (short)((GetScreenWidth() - entries[0].width) / 2);
        entries[0].y = (short)((GetScreenHeight() - entries[0].height) / 2);
    }
    if (entries[0].x == -2) {
        entries[0].x = (short)(((GetScreenWidth() - 0x80 - entries[0].width) / 2) + 0x80);
        entries[0].y = (short)((GetScreenHeight() - entries[0].height) / 2);
    }
    if (entries[0].x + entries[0].width > GetScreenWidth())
        entries[0].x = (short)((GetScreenWidth() - entries[0].width) / 2);
    if (entries[0].y + entries[0].height > GetScreenHeight())
        entries[0].y = (short)((GetScreenHeight() - entries[0].height) / 2);

    force = flags & 1;
    if (force) {
    do {
        i = PopKey();
    } while (i != 0);
    if (menu->clearQuickKeys != 0) {
        for (i = 0; i <= entries[0].u.count; i++) {
            if (entries[i].type == 1)
                entries[i].quickKey = 0;
        }
    }
    if (entries[0].width > GetScreenWidth() || entries[0].height > GetScreenHeight())
        return 0;

    entries[0].u.assets.archive = 0;
    i = 0;
    while (i < entries[0].u.count + 1) {
        char stagebuf[0x20];
        char textbuf[0x100];
        char buf1[0x100];
        char buf2[0x100];
        char buf3[0x80];
        buf2[0] = 0;
        if (menu->str_9b6[0])
            strcpy(buf2, menu->str_9b6);
        g = 0;
        if (entries[i].resourceFlags & 1) {
            entries[i].archive = 0;
            entries[i].gaf = 0;
            GetAnimsPath(menu, buf1);
            strcat(buf1, entries[i].name);
            strcat(buf1, "_gadget");
            ChangeExtension(buf1, buf1, "GAF");
            if (HAPI_FileLengthByName(buf1)) {
                entries[i].archive = LoadGaf(buf1);
                if (entries[i].archive)
                    entries[i].gaf = FindGafEntry(entries[i].archive, entries[i].name);
            }
        }
        switch (entries[i].type) {
        case 0:
        case 11: {
            if (0 > entries[0].y)
                entries[0].y += (short)GetScreenHeight();
            GetAnimsPath(menu, buf1);
            strncpy(textbuf, entries[0].name, 0x10);
            textbuf[0x10] = 0;
            strcat(buf1, textbuf);
            ChangeExtension(buf1, buf1, "GAF");
            if (!entries[0].u.assets.archive) {
                if (HAPI_FileLengthByName(buf1))
                    entries[0].u.assets.archive = LoadGaf(buf1);
            }
            strncpy(textbuf, entries[0].u.text + 0x46, 0x10);
            textbuf[0x10] = 0;
            if (entries[0].u.assets.archive)
                g = FindGafEntry(entries[0].u.assets.archive, textbuf);
            if (g == 0) {
                if (0 != menu->gaf) {
                    g = FindGafEntry(menu->gaf, textbuf);
                    if (g == 0) {
                        g = FindGafEntry(menu->gaf, "BackTile");
                        if (g != 0) {
                            ClearGlyphOffsets(g);
                        }
                    }
                }
            }
            entries[0].u.assets.background = g;
            break;
        }

        case 4: {
            entries[i].frameBase = 0;
            if (entries[0].u.assets.archive)
                g = FindGafEntry(entries[0].u.assets.archive, "SLIDERS");
            if (g == 0 && menu->gaf != 0) {
                g = FindGafEntry(menu->gaf, "SLIDERS");
                if (g != 0) {
                    ClearGlyphOffsets(g);
                    orientation = entries[i].width > entries[i].height ? 10 : 0;
                    GafFrame* frame = GetGafFrame(g, orientation);
                    if (entries[i].width < entries[i].height)
                        entries[i].width = frame->width;
                    else
                        entries[i].height = frame->height;
                    entries[i].sliderStyle = (unsigned char)orientation;
                }
            }
            entries[i].sliderGaf = g;
            if (g != 0) {
                Gadget* firstEnd = &entries[AddGadgetEntry(menu, 1)];
                firstEnd->x = entries[i].x;
                firstEnd->y = entries[i].y;
                firstEnd->gaf = g;
                firstEnd->frameBase = entries[i].sliderStyle + 6;
                firstEnd->team = entries[i].team;
                GafFrame* frame = GetGafFrame(g, entries[i].sliderStyle + 6);
                firstEnd->width = frame->width;
                firstEnd->height = frame->height;
                firstEnd->attribs = 0x3400;
                firstEnd->active = entries[i].active;
                Gadget* secondEnd = &entries[AddGadgetEntry(menu, 1)];
                secondEnd->active = entries[i].active;
                frame = GetGafFrame(g, entries[i].sliderStyle + 8);
                secondEnd->y = entries[i].y;
                secondEnd->gaf = g;
                secondEnd->frameBase = entries[i].sliderStyle + 8;
                secondEnd->team = entries[i].team;
                secondEnd->attribs = 0x2c00;
                secondEnd->width = frame->width;
                secondEnd->height = frame->height;
                frame = GetGafFrame(g, entries[i].sliderStyle + 6);
                if (entries[i].width > entries[i].height) {
                    secondEnd->x = entries[i].x - (frame->width - entries[i].width);
                    entries[i].width += (short)(frame->width * -2);
                    entries[i].x += frame->width;
                    frame = GetGafFrame(g, entries[i].sliderStyle + 5);
                    entries[i].knobSize = frame->width;
                    entries[i].range = entries[i].width - entries[i].knobSize - 4;
                } else {
                    secondEnd->y = entries[i].y - (frame->height - entries[i].height);
                    secondEnd->x = entries[i].x;
                    entries[i].height += (short)(-2 * frame->height);
                    entries[i].y += frame->height;
                }
            } else {
                entries[i].range = (entries[i].width > entries[i].height ? entries[i].width : entries[i].height) - 6;
            }
            break;
        }

        case 3: {
            GafEntry* input = menu->gaf ? FindGafEntry(menu->gaf, "TEXTINPUT") : 0;
            if (input != 0) {
                ClearGlyphOffsets(input);
            }
            entries[i].inputGaf = input;
            if (entries[i].status >= 0x80)
                entries[i].status = 0x7f;
            memset(entries[i].u.text, 0, 0x80);
            break;
        }
        case 2: {
            GafEntry* list = menu->gaf ? FindGafEntry(menu->gaf, "LISTBOX") : 0;
            if (list != 0) {
                ClearGlyphOffsets(list);
            }
            entries[i].u.list.gaf = list;
            int j = 1;
            for (; j <= entries[0].u.count; ) {
                Gadget* other = &entries[j];
                if (j != i && other->type == 2) {
                    if (other->team == entries[i].team) {
                        short scroll = other->u.list.scroll > entries[i].u.list.scroll
                                         ? other->u.list.scroll : entries[i].u.list.scroll;
                        entries[i].u.list.scroll = scroll;
                        other->u.list.scroll = scroll;
                    }
                }
                j = j + 1;
            }
            break;
        }
        case 12: {
            entries[i].colours = 0;
            strncpy(textbuf, entries[i].name, 0x10);
            textbuf[0x10] = 0;
            entries[i].u.frame.glyph = 0;
            if (entries[0].u.assets.archive != 0)
                g = FindGafEntry(entries[0].u.assets.archive, textbuf);
            if (0 == g)
                g = FindGafEntry(menu->gaf, textbuf);
            if (g != 0)
                entries[i].u.frame.glyph = GetGafFrame(g, 0);
            break;
        }

        case 1: {
            Gadget* cur = &entries[i];
            cur->colours = 0;
            pf = &cur->attribs;
            if ((cur->attribs & 0x1800) || (cur->resourceFlags & 1))
                break;
            AssignQuickKey(menu, i);
            strncpy(textbuf, entries[i].name, 0x10);
            entries[i].frameBase = 0;
            textbuf[0x10] = 0;
            if (entries[0].u.assets.archive)
                g = FindGafEntry(entries[0].u.assets.archive, textbuf);
            if (g == 0) {
                if (menu->gaf != 0) {
                    g = FindGafEntry(menu->gaf, textbuf);
                    if (0 == g) {
                        if (0x80 & entries[i].attribs) {
                            g = FindGafEntry(menu->gaf, "CHECKBOX");
                        } else if (entries[i].stages != 0) {
                            if (strcmp(entries[i].u.text, "Off|On") != 0 && entries[i].stages != 1 && 0 == (*pf & 0x4000)) {
                                int n = entries[i].stages < 4 ? entries[i].stages : 4;
                                sprintf(stagebuf, "stagebuttn%d", n);
                            } else {
                                entries[i].stages = 2;
                                strcpy(stagebuf, "stagebuttn1");
                                *pf |= 0x4000;
                            }
                            g = FindGafEntry(menu->gaf, stagebuf);
                        } else {
                            strcpy(stagebuf, "BUTTONS0");
                            g = FindGafEntry(menu->gaf, stagebuf);
                        }
                        if (g) {
                            int best = 1000;
                            for (int f = 0; f < g->count; ++f) {
                                GafFrame* frame = GetGafFrame(g, f);
                                if (frame != 0) {
                                    frame->yOffset = 0;
                                    frame->xOffset = 0;
                                }
                            }
                            for (int j = 0; j < g->count; j += 4) {
                                GafFrame* frame = GetGafFrame(g, j);
                                int distance = abs(entries[i].height - frame->height) + abs(entries[i].width - frame->width);
                                if (distance < best) {
                                    entries[i].frameBase = (unsigned char)j;
                                    best = distance;
                                }
                            }
                        }
                    }
                }
            }
            entries[i].gaf = g;
            if (g != 0) {
                GafFrame* frame = GetGafFrame(g, entries[i].frameBase);
                if (frame != 0) {
                    entries[i].width = frame->width;
                    entries[i].height = frame->height;
                }
            }
            if (0 != entries[i].stages) {
                char* p = entries[i].u.text;
                while (*p) {
                    if (*p == '|')
                        *p = 0;
                    p++;
                }
                cur = &menu->layer->entries[i];
                char* dst = buf3;
                char* src = cur->u.text;
                int k = 0;
                for (; k < cur->stages; k++) {
                    strcpy(dst, Translate(src));
                    dst += strlen(dst) + 1;
                    src += strlen(src) + 1;
                }
                memcpy(cur->u.text, buf3, sizeof(buf3));
                *pf = (*pf & 0x4000) | 1;
            }
            break;
        }

        case 7:
            strcpy(buf2, menu->str_bb6);
            strcat(buf2, entries[i].u.text);
            strcat(buf2, ".FNT");
            entries[i].u.list.filebuf = HAPI_LoadFile(buf2, 0);
            break;

        case 8:
            strcat(buf2, entries[i].u.text);
            entries[i].u.list.filebuf = HAPI_LoadFile(buf2, 0);
            break;

        case 13: {
            Gadget* en = menu->layer->entries;
            en[i].u.anim.next = GetTicks() + en[i].u.anim.interval;
            break;
        }

        case 5:
            if (strlen((char*)&entries[i].range) == 0)
                entries[i].attribs |= 0x10;
            else
                AssignQuickKey(menu, i);
            entries[i].colours = 0;
            break;

        default:
            break;
        }
        i++;
    }

    name = entries[0].name;
    if (0 == name)
        name = "GUI SURFACE";
    entries[0].u.assets.surface = AllocSurface(name, entries[0].width, entries[0].height);
    DrawSurface(entries[0].u.assets.surface, 0, -entries[0].x, -entries[0].y);
    if (!(flags & 0x20)) {
        entries[0].u.assets.saveUnder = AllocSurface("SAVE UNDER", entries[0].width, entries[0].height);
        DrawSurface(entries[0].u.assets.saveUnder, entries[0].u.assets.surface, 0, 0);
    } else {
        entries[0].u.assets.saveUnder = 0;
    }
    }

    if ((flags & 4) != 0 || force || (flags & 0x40)) {
        if (force || (flags & 0x40)) {
            if (menu->layer->surface)
                DrawSurface(entries[0].u.assets.surface, menu->layer->surface, 0, 0);
            else if ((flags & 0x80) == 0)
                DrawListboxFrame(menu, 0, entries[0].u.assets.background);
        }

        for (i = 1; i < 1 + entries[0].u.count; i++) {
            if (entries[i].active == 0)
                continue;
            switch (entries[i].type) {
            case 11:
                DrawListboxFrame(menu, i, entries[i].u.assets.background);
                break;
            case 12:
                DrawGadgetGlyph(menu, i);
                break;
            case 1:
                if (force != 0 || (flags & 0x48) != 0)
                    DrawButton(menu, i);
                break;
            case 2: {
                if (force) {
                    int fh;
                    Gadget* base = menu->layer->entries;
                    int t = 0;
                    int j;
                    base[i].u.list.firstRow = 0;
                    base[i].u.list.selected = 0;
                    for (j = 1; j < base[0].u.count + 1; j++) {
                        if (base[j].type == 7) {
                            if (t == base[i].tab) {
                                SetFont(base[j].u.list.language);
                                break;
                            }
                            t++;
                        }
                    }
                    if (j == base[0].u.count + 1)
                        SetFont(g_guiContext->fontId);
                    if (g_guiContext->language == 0)
                        fh = GetFontHeight();
                    else
                        fh = GetGafFrame(g_guiContext->language->glyphs, 0x49)->height + 2;
                    int hh = base[i].height;
                    base[i].height = (short)(hh - ((int)base[i].height) % (fh + 2));
                    base[i].u.list.sortKey = GetTicks();
                }
                if (force || (flags & 0x40))
                    DrawListBox((DrawListBoxDialog *)menu, i);
                break;
            }
            case 3:
                if (force || (flags & 0x40))
                    DrawTextInput(menu, i);
                break;
            case 4:
                if (force) {
                    Gadget* base = menu->layer->entries;
                    base[i].knobPos = 0;
                    base[i].sliderCallback = 0;
                    base[i].sliderUser = 0;
                }
                if (force || (0x40 & flags))
                    DrawSlider((Dialog *)menu, i);
                break;
            case 5:
                if (force || (flags & 0x40))
                    DrawLabel(menu, i);
                break;
            case 6:
                if (force) {
                    Gadget* base = menu->layer->entries;
                    base[i].u.hotspot.callback = 0;
                    base[i].u.hotspot.gaf = 0;
                    base[i].u.hotspot.frame = 0;
                    base[i].u.hotspot.index = 0;
                }
                if (force || (flags & 0x40))
                    DrawHotspot((DrawHotspotDialog *)menu, i);
                break;
            case 13:
                if (force) {
                    Gadget* base = menu->layer->entries;
                    base[i].u.anim.next = GetTicks() + base[i].u.anim.interval;
                }
                if (force || (flags & 0x40))
                    DrawProgressBarGadget((DrawProgressBarGadgetDialog *)menu, i);
                break;
            case 10:
                if (force || (flags & 0x40))
                    DrawLineGadget(menu, i, flags);
                break;
            default:
                break;
            }
    }
        if (menu->layer->current != -1 && menu->dirty != 0) {
            savedType = entries[menu->layer->current].type;
            DrawGadgetSelectionBox(menu, menu->layer->current, 8);
            int j = FindEntry(entries, entries[0].u.text + 0x16);
            if (j != -1 && savedType != 1 && entries[i].active != 0)
                DrawGadgetSelectionBox(menu, j, 8);
        }
    }

    if (flags & 2) {
        if (entries[0].u.assets.saveUnder != 0) {
            DrawSurface(0, entries[0].u.assets.saveUnder, entries[0].x, entries[0].y);
            FreeSurface((Surface *)entries[0].u.assets.saveUnder);
            entries[0].u.assets.saveUnder = 0;
        }
        FreeSurface((Surface *)entries[0].u.assets.surface);
        entries[0].u.assets.surface = 0;
        for (int j = 0; j < 1 + entries[0].u.count; j = j + 1) {
            if ((entries[j].resourceFlags & 1) && entries[j].archive)
                GameFreeThunk(entries[j].archive);
            switch (entries[j].type) {
            case 0:
                GameFreeThunk(entries[j].u.assets.archive);
                break;
            case 7:
                GameFreeThunk(entries[j].u.list.filebuf);
                break;
            case 8:
                GameFreeThunk(entries[j].u.list.filebuf);
                break;
            default:
                break;
            }
        }
    }

    return 1;
}

// FUNCTION: 0x4a9660
void __stdcall CloseTopScreen(Gui* gui)
{
    if (gui->layer) {
        unsigned int flags = gui->layer->flags;
        gui->hoverGadgetIndex = gui->hotGadgetIndex = gui->focus = -1;
        if (gui->layer->handler)
            gui->layer->handler(gui);
        HideSoftwareCursor();
        RenderLayer(gui, 2);
        ShowSoftwareCursor();
        Layer* old = gui->layer;
        gui->layer = old->next;
        if (gui->layer)
            gui->layer->dirty = 1;
        GameFreeThunk(old);
        if (flags & 0x800)
            RenderLayer(gui, 0x40);
    }
}

// FUNCTION: 0x4a96d0
void __stdcall DecrementKnobPos(Gui* obj, int index)
{
    Gadget* e = &obj->layer->entries[index];
    short raw = e->knobPos;
    int old = raw;
    e->knobPos = raw - 1;
    if (e->knobPos > e->range - 1) {
        e->knobPos = e->range - 1;
    }
    if (e->knobPos < 0) {
        e->knobPos = 0;
    }
    if (e->knobPos != old) {
        obj->changed = 1;
        DrawSliderBar(obj, index);
        SyncAssocGadgets((SyncAssocGadgetsDialog *)obj, index);
    }
    if (e->sliderCallback) {
        e->sliderCallback(obj, e->sliderUser);
    }
}


static inline Gadget* entry_at(Gui* obj, int index)
{
    return &obj->layer->entries[index];
}

// FUNCTION: 0x4a9780
void __stdcall IncrementKnobPos(Gui* obj, int index)
{
    Gadget* e = entry_at(obj, index);
    short raw = e->knobPos;
    int old = raw;
    e->knobPos = raw + 1;
    if (e->knobPos > e->range - 1) {
        e->knobPos = e->range - 1;
    }
    if (e->knobPos < 0) {
        e->knobPos = 0;
    }
    if (e->knobPos != old) {
        obj->changed = 1;
        DrawSliderBar(obj, index);
        SyncAssocGadgets((SyncAssocGadgetsDialog *)obj, index);
    }
    if (e->sliderCallback) {
        e->sliderCallback(obj, e->sliderUser);
    }
}


struct Pad_a9830_0 { int field; };
struct Pad_a9830_1 { int field; };
struct Pad_a9830_2 { int field; };
struct Pad_a9830_3 { int field; };
struct Pad_a9830_4 { int field; };
extern int Pad_a9830_e0;
extern int Pad_a9830_e1;
extern int Pad_a9830_e2;
extern int Pad_a9830_e3;
extern int Pad_a9830_e4;
extern int Pad_a9830_e5;

// FUNCTION: 0x4a9830
void __stdcall ListBoxSelectUp(Gui* param_1, int index)
{
    Gadget* entries = param_1->layer->entries;
    Gadget* me = &entries[index];
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == me->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->fontId);
    }
    int size;
    if (g_guiContext->language == 0) {
        size = GetFontHeight();
    } else {
        unsigned short* pg = (unsigned short*)GetGafFrame(g_guiContext->language->glyphs, 0x49);
        size = pg[1] + 2;
    }
    size++;
    int step = (me->height - 2) / size;
    short last = me->u.list.firstRow;
    short sel = me->u.list.selected;
    int isel = sel;
    if (sel < last + step && sel >= last) {
        if (me->u.list.rowCount == 0) {
            return;
        }
        if (sel == 0) {
            return;
        }
        short prev = sel - 1;
        me->u.list.selected = prev;
        if (prev < last) {
            last--;
            me->u.list.firstRow = last;
        }
        if (me->u.list.field_c2 != 0) {
            char* line = SkipTextLines(me->u.list.field_c2, prev);
            if (strncmp(DAT_00502a20, line, 2) == 0) {
                me->u.list.selected = isel;
            }
        }
        DrawListBox((DrawListBoxDialog *)param_1, index);
        SyncAssocGadgets((SyncAssocGadgetsDialog *)param_1, index);
        return;
    }
    if (me->u.list.rowCount != 0) {
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)param_1, me->name, isel);
    }
}

int __stdcall UpdateMenu(Gui* menu);

// FUNCTION: 0x4a99c0
void __stdcall ListBoxSelectDown(Gui* param_1, int index)
{
    Gadget* entries = param_1->layer->entries;
    Gadget* me = &entries[index];
    int n = 0;
    int i = 1;
    for (; i < entries->u.count + 1; i++) {
        if (entries[i].type == 7) {
            if (n == me->tab) {
                SetFont(entries[i].u.list.language);
                break;
            }
            n++;
        }
    }
    if (i == entries->u.count + 1) {
        SetFont(g_guiContext->fontId);
    }
    int size = (g_guiContext->language == 0) ? GetFontHeight()
        : (*(unsigned short*)((int)GetGafFrame(g_guiContext->language->glyphs, 0x49) + 2) + 2);
    size++;
    int step = (me->height - 2) / size;
    short last = me->u.list.firstRow;
    short sel = me->u.list.selected;
    int isel = sel;
    if (isel < last + step && sel >= last) {
        if (me->u.list.rowCount != 0) {
            if (isel == me->u.list.maxFirstRow + step - 1) {
                return;
            }
            short next = sel + 1;
            me->u.list.selected = next;
            if (next > last + step - 1) {
                me->u.list.firstRow = last + 1;
            }
            if (next >= me->u.list.rowCount - 1) {
                me->u.list.selected = me->u.list.rowCount - 1;
            }
            if (me->u.list.field_c2 != 0) {
                char* line = SkipTextLines(me->u.list.field_c2, me->u.list.selected);
                if (strncmp(DAT_00502a20, line, 2) == 0) {
                    me->u.list.selected = isel;
                }
            }
            DrawListBox((DrawListBoxDialog *)param_1, index);
            SyncAssocGadgets((SyncAssocGadgetsDialog *)param_1, index);
            return;
        }
    }
    if (me->u.list.rowCount != 0) {
        SetListBoxScrollByName((SetListBoxScrollByNameDialog *)param_1, me->name, isel);
    }
}

// FUNCTION: 0x4a9b90
int __stdcall HandleGuiCommand(Gui* obj, int cmd)
{
    int newsel = -1;
    int index = obj->layer->current;
    Gadget* entries = obj->layer->entries;
    Gadget* e = &entries[index];
    int type = e->type;

    switch (cmd) {
    case 9:
        if (IsKeyDown(0xf9))
            SelectAdjacentGadget(obj, 0);
        else
            SelectAdjacentGadget(obj, 1);
        obj->changed = 1;
        cmd = 0;
        break;
    case 0x1b:
        {
            int found = FindEntry(entries, entries[0].u.names.choice2);
            if (found == -1 || entries[found].active == 0)
                break;
            newsel = found;
        }
        cmd = 0;
        break;
    case 0xd:
        if (obj->focus != -1 && entries[obj->focus].type == 3)
            break;
        {
            int found = FindEntry(entries, entries[0].u.names.choice);
            if (found != -1 && entries[found].active != 0
                && !(entries[found].type == 1 && (entries[found].max & 1))) {
                newsel = found;
                cmd = 0;
                break;
            }
        }
    case 0x20:
        if (type == 3)
            break;
        if (type != 1 && type != 2 && type != 6)
            break;
        if (e->active == 0)
            break;
        if (type == 1 && (e->grayedout & 1))
            break;
        newsel = index;
        if (type == 1) {
            if (e->attribs & 0x10) {
                e->status = 1;
                ClearPeerStatus(obj, index);
                DrawButton(obj, index);
            }
        }
        if (type == 1 && e->stages != 0) {
            unsigned char* p = &e->stageIndex;
            if (++*p >= e->stages)
                *p = 0;
        }
        cmd = 0;
        break;
    case 0xf5:
        if (type == 2) {
            ListBoxSelectUp(obj, index);
            if (e->u.list.callback)
                e->u.list.callback(obj, e);
        } else {
            SelectAdjacentGadget(obj, 2);
        }
        obj->changed = 1;
        cmd = 0;
        break;
    case 0xf4:
        if (type == 3)
            break;
        if (type == 4 && e->width > e->height)
            DecrementKnobPos(obj, index);
        else
            SelectAdjacentGadget(obj, 0);
        obj->changed = 1;
        cmd = 0;
        break;
    case 0xf7:
        if (type == 2) {
            ListBoxSelectDown(obj, index);
            if (e->u.list.callback)
                e->u.list.callback(obj, e);
        } else {
            SelectAdjacentGadget(obj, 3);
        }
        obj->changed = 1;
        cmd = 0;
        break;
    case 0xf6:
        if (type == 3)
            break;
        if (type == 4 && e->width > e->height)
            IncrementKnobPos(obj, index);
        else
            SelectAdjacentGadget(obj, 1);
        obj->changed = 1;
        cmd = 0;
        break;
    }
    if (cmd == 0 && obj->layer->keyboardInput == 0)
        PopKey();
    if (newsel != -1) {
        obj->hotGadgetIndex = newsel;
        obj->changed = 1;
    }
    return cmd;
}

static inline void SelectCurrentByName(Gui* menu, Gadget* entries, int sel)
{
    Gadget* entry = &entries[sel];
    SetTextColors(menu->colours[entry->colours], GetTextKeyColor());
    SelectFontForEntry((void*)entries, sel);
    TrySetFocus(menu, sel);
    menu->layer->current = sel;
    CommitTextEdit(menu, sel, entry->u.text, entry->status, 0);
    ClearKeyQueue();
}

static inline void UpdateHelpText(Gui* obj)
{
    char* text = DAT_005119b8;
    if (obj->hoverGadgetIndex != -1) {
        obj->prevHoverGadget = obj->hoverGadgetIndex;
        text = obj->layer->entries[obj->hoverGadgetIndex].helpKey;
    }
    int found = FindEntry(obj->layer->entries, "HELPTEXT");
    if (found != -1) {
        strcpy(obj->layer->entries[found].u.text, Translate(text));
        obj->changed = 1;
    }
}

static inline void TickProgressBar(Gui* menu, int i)
{
    Gadget* entries = menu->layer->entries;
    Gadget* e = &entries[i];
    if (e->u.anim.active && e->u.anim.value < e->u.anim.max) {
        if ((int)GetTicks() > e->u.anim.next) {
            e->u.anim.value += (int)e->u.anim.step;
            if (e->u.anim.value > e->u.anim.max) {
                e->u.anim.value = e->u.anim.max;
                e->u.anim.active = 0;
            }
            e->u.anim.next = GetTicks() + e->u.anim.interval;
        }
        DrawProgressBarGadget((DrawProgressBarGadgetDialog *)menu, i);
    }
}

// FUNCTION: 0x4a9fd0
int __stdcall UpdateMenu(Gui* menu)
{
    int k;
    Gadget* e;
    int i;
    Gadget* entries = 0;

    if (menu->layer == 0)
        return 0;

    int now = GetTicks();
    menu->animTimer = now - menu->time;
    menu->time = now;
    UpdateCursorAndMouse(menu);

    int key;
    if (menu->layer->keyboardInput == 0) {
        key = PeekKey();
        if (key >= 0xe2 && key <= 0xeb)
            key = 0;
    } else {
        key = PopKey();
    }

    if (menu->layer->keyboardInput != 0 && key != 0 && menu->dirty != 0) {
        key = HandleGuiCommand(menu, key);
        if (key != 0) {
            for (int n = 0; n < 0xe; n++)
                menu->layer->text[n] = menu->layer->text[n + 1];
            menu->layer->lastKey = (char)toupper(key);
            if (menu->layer->textHandler != 0)
                menu->layer->textHandler(menu);
            menu->hotGadgetIndex = -1;
        }
    }

    int sel = menu->hotGadgetIndex;
    if (menu->layer == 0)
        return 1;
    if (menu->changed == 1) {
        menu->changed = 0;
        RenderLayer(menu, menu->layer->flags | 0x40);
    }

    entries = menu->layer->entries;
    if (entries == 0)
        return 1;

    MouseRecord& pt = *(MouseRecord*)&menu->pointX;
    {
        Rect box;
        box.left = entries->x;
        box.top = entries->y;
        if (entries->type != 0) {
            box.left *= 2;
            box.top *= 2;
        }
        int ptY = pt.y;
        box.right = entries->width + box.left - 1;
        box.bottom = entries->height + box.top - 1;
        SetCursorHover(menu, pt.x >= box.left && pt.x <= box.right &&
                           ptY >= box.top && ptY <= box.bottom);
    }

    int saved = menu->hoverGadgetIndex;
    menu->hoverGadgetIndex = -1;

    MouseRecord point;
    memcpy(&point, &menu->pointX, 24);
    point.x -= entries->x;
    point.y -= entries->y;

    int elapsed;
    if ((int)GetTicks() - DAT_0051fbb4 > 0) {
        elapsed = 1;
        DAT_0051fbb4 = GetTicks();
    } else {
        elapsed = 0;
    }

    i = 1;
    for (; i < entries->u.count + 1; i++) {
        e = &entries[i];
        if (e->active != 0) {
            int x, y;
            if (e->type == 0) {
                x = 0;
                y = 0;
            } else {
                x = e->x;
                y = e->y;
            }
            int right = e->width + x - 1;
            int bottom = e->height + y - 1;
            if (point.x >= x && point.x <= right && point.y >= y && point.y <= bottom)
                menu->hoverGadgetIndex = i;

            k = IsKeyDown(0xfb) == 0 ? key : 0;
            switch (e->type) {
            case 1:
                if (HandleButtonInput(menu, i, key) == 1)
                    sel = i;
                if (e->colours != 0 && elapsed) {
                    e->colours -= 2;
                    if (e->colours < 0)
                        e->colours = 0;
                    if (menu->layer != 0)
                        menu->layer->dirty = 1;
                }
                break;
            case 2:
                if (HandleListBoxInput(menu, i, 0) == 1)
                    sel = i;
                break;
            case 3:
                if (HandleTextInput(menu, i, k) == 1)
                    sel = i;
                break;
            case 4:
                HandleSliderInput(menu, i);
                break;
            case 5:
                if (HandleLabelInput(menu, i, key) != 0) {
                    sel = -1;
                    int found = FindEntry(entries, (char*)&e->range);
                    if (found != sel) {
                        Gadget* me;
                        sel = found;
                        me = &entries[found];
                        if (me->type == 1) {
                            if (me->active == 0 || (me->grayedout & 1) != 0) {
                                sel = -1;
                            } else {
                                me->stageIndex++;
                                if (me->stageIndex >= me->stages)
                                    me->stageIndex = 0;
                                DrawButton(menu, found);
                            }
                        } else {
                            if (me->active != 0) {
                                if (me->type == 4 && me->sliderLocked != 0) {
                                    sel = -1;
                                } else {
                                    Gadget* entriesNow = menu->layer->entries;
                                    menu->focus = -1;
                                    menu->layer->current = found;
                                    if (entriesNow[menu->layer->current].type == 3)
                                        SelectCurrentByName(menu, menu->layer->entries, menu->layer->current);
                                }
                            } else {
                                sel = -1;
                            }
                        }
                    } else {
                        sel = i;
                    }
                }
                break;
            case 6:
                if (HandleHotspotInput(menu, i) == 1)
                    sel = i;
                break;
            case 13:
                TickProgressBar(menu, i);
                break;
            case 12:
                if (e->colours != 0 && elapsed) {
                        e->colours--;
                        DrawGadgetGlyph(menu, i);
                        if (menu->layer != 0)
                            menu->layer->dirty = 1;
                    }
                break;
            }
        }
        if (sel != -1)
            break;
    }

    if (menu->hoverGadgetIndex != saved)
        UpdateHelpText(menu);

    if (menu->layer->cb1c != 0)
        menu->layer->cb1c();

    if (sel != -1) {
        menu->hotGadgetIndex = sel;
        SelectGadgetByIndex(menu, sel);
        if (menu->layer->handler != 0)
            menu->layer->handler(menu);
        if (menu->hotGadgetIndex != -1)
            CloseTopScreen(menu);
    }
    return 1;
}

// FUNCTION: 0x4aa850
void __stdcall SetCurrentGuiContext(Gui* ctx)
{
    g_guiContext = (ConfigureListBoxByNameRoot*)ctx;
    ctx->layer = 0;
    ctx->str_9b6[0] = 0;
    ctx->str_ab6[0] = 0;
    ctx->str_bb6[0] = 0;
    ctx->pathsReady = 1;
    ctx->time = GetTicks();
    ctx->animTimer = 0;
    ctx->inputEnabled = 1;
    ctx->gaf = 0;
    ctx->fallback = 0;
    ctx->cachedBgName[0] = 0;
    ctx->knobDragging = 0;
    ctx->hoverGadgetIndex = -1;
    memset(ctx->values, 0, 12);
    ctx->language = 0;
    ctx->dirty = 1;
}

// FUNCTION: 0x4aa8d0
int GetGuiContext(void)
{
    return (int)g_guiContext;
}

// FUNCTION: 0x4aa8e0
void __stdcall SetDefaultFont(int* param_1, int param_2)
{
    *param_1 = param_2;
}
