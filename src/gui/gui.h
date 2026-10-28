#ifndef GUI_H
#define GUI_H
struct GadgetAccessDialog;
struct IsAboveRectTopRect;
struct IsBelowRectBottomRect;

struct Gadget;

struct BlinkWord;
struct Dialog;
struct DrawListBoxDialog;
struct SyncAssocGadgetsDialog;
struct SetListBoxScrollByNameDialog;
struct DrawProgressBarGadgetDialog;
struct DrawHotspotDialog;
struct Gadget;
struct Rect;

struct FileRequester;
struct FontGlyphTable;
struct Dialog;
struct RedrawGadgetRectDialog;
struct ResetSliderGadgetDialog;
struct ScheduleGadgetEndTickDialog;
struct Gadget;
struct ResetProgressBarDialog;
struct DialogPointView;
struct DialogOriginView;
struct Rect;
#include "mouse_event.h"

#pragma pack(push, 1)

struct Layer;

struct Dialog {
    union {
        struct {
            int unknown_0;
            Gadget* entries;
            void (__stdcall* handler)(void*);
            void* data;
            int flags;
            FontGlyphTable* language;
        };
        struct {
            char path[0x13];
            short field_13;
            char unknown_15[0x18 - 0x15];
        };
    };
    Layer* holder;
    void (__stdcall* field_1c)(void);
    int field_20;
    void* surface;
    char text[14];
    char field_36;
    int field_37;
    char unknown_3b[1];
    MouseEvent point;
    unsigned int flags_54;
    char unknown_58[8];
    int current;
    int focus;
    int index;
    int used;
    int field_70;
    int length;
    int field_78;
    MouseEvent saved;
    short field_94;
    char unknown_96[12];
    int keyCommands;
    BlinkWord* words;
    int count;
    int active;
    char unknown_b2[0x8b2 - 0xb2];
    unsigned char colour;
    char unknown_8b3[0x9b6 - 0x8b3];
    char path_9b6[0x100];
    char path_ab6[0x100];
    char path_bb6[0x100];
    char unknown_cb6[16];
    int field_cc6;
    int changed;

    FileRequester* OpenFileRequester(Dialog* self, char* arg2, char* arg3, char* arg4);
};

struct Gui {
    int font;
    void* gaf;
    void* values[3];
    void* language;
    Layer* layer;
    int cursorSharedFrame;
    int cursorDefaultFrame;
    int cursorHoverFrame;
    void* src_28;
    void* src_2c;
    unsigned short cursorIndex;
    unsigned short cursorValue;
    unsigned char cursorKind;
    char unknown_35[3];
    void* cursorSrc;
    int pointX;
    int pointY;
    unsigned int pointKeyFlags;
    int pointTick;
    int pointMessage;
    int pointDoubleClick;
    int mouseKeyFlags;
    int clickMode;
    unsigned int cursorFlags;
    int hotGadgetIndex;
    int focus;
    int hoverGadgetIndex;
    int prevHoverGadget;
    int clearQuickKeys;
    int cursor;
    int knobDragging;
    char unknown_7c[0x96 - 0x7c];
    int time;
    int animTimer;
    int inputEnabled;
    int dirty;
    char unknown_a6[0xb2 - 0xa6];
    unsigned int palette[0x100];
    char unknown_4b2[0x8b2 - 0x4b2];
    unsigned char colours[0x100];
    char unknown_9b2[0x9b6 - 0x9b2];
    char str_9b6[0x100];
    char str_ab6[0x100];
    char str_bb6[0x110];
    int pathsReady;
    int changed;
    int clickStatusCache;
    void* fallback;
    char cachedBgName[0x20];
};

#pragma pack(pop)

int __stdcall HasGadgetNamed(Dialog* obj, char* name, int unused);
void __stdcall ClearChanged(void*);
void __stdcall ClearLayerChanged(Dialog* param);
int __stdcall GetKeyboardInput(Dialog* obj);
void __stdcall ClearFocus(Dialog*);
int __stdcall IsFocused(int, int);
bool __stdcall HasFocus(int);
void __stdcall FUN_0049fd20(DialogPointView*, DialogOriginView*, MouseEvent*);
void __stdcall GetGadgetRect(Gadget* entry, Rect* rect);
int __stdcall FindAssocListBox(Gadget* entries, int index);
int __stdcall FindAssocSlider(Gadget* entries, int index);
int __stdcall FindAssocTextInput(Gadget* entries, int index);
void __stdcall RedrawGadgetRect(RedrawGadgetRectDialog* obj, int index);
void __stdcall GetSliderRects(Gadget* base, int index, int* r1, int* r2);
void __stdcall TruncateListItems(int a1, int a2, char* a3, int a4);
void __stdcall ResetSliderGadget(ResetSliderGadgetDialog* obj, int i);
void __stdcall ScheduleGadgetEndTick(ScheduleGadgetEndTickDialog* obj, int i);
void __stdcall ResetProgressBar(ResetProgressBarDialog*, int index);
void* __stdcall GetCharGlyph(unsigned char c);
int __stdcall GetFontLineHeight();
void __stdcall LayoutLabelText(Gui* obj, int index);
void __stdcall DrawLabel(Gui* obj, int index);
void __stdcall DrawGadgetGlyph(Gui* obj, int index);
void __stdcall ClearAllStatus(Gui*);
int __stdcall HandleButtonInput(Gui* obj, int index, int);
int __stdcall HandleTextInput(Gui* obj, int index, int key);
int __stdcall FindNextTextInput(Gadget* list, int index);
int __stdcall LoadScreenGaf(Gui* obj, char* name);
void __stdcall MoveEntryToFront(Gadget* entries, int index);
void __stdcall FindButtonGaf(Gui* button, Gadget* obj);
int __stdcall AddGadgetEntry(Gui* obj, unsigned char type);
void __stdcall GetAnimsPath(Gui* obj, char* out);
void __stdcall DecrementKnobPos(Gui* obj, int index);
void __stdcall IncrementKnobPos(Gui* obj, int index);
void __stdcall ListBoxSelectUp(Gui*, int index);
void __stdcall ListBoxSelectDown(Gui*, int index);
int __stdcall HandleGuiCommand(Gui* obj, int cmd);
int __stdcall GetGuiContext(void);

void __stdcall SetGadgetName(Dialog* obj, char* name, char* text);
void __stdcall RunWhileScreenNamed(Dialog* menu, char* name);
void __stdcall EnableKeyCommands(Dialog* p);
void __stdcall DisableKeyCommands(Dialog* obj);
void __stdcall MarkChanged(Dialog* obj);
void __stdcall MarkLayerChanged(Dialog* obj);
void __stdcall DrawSlider(Dialog* gui, int gadgetIndex);
void __stdcall SetFontsPath(Dialog* obj, const char* dir);
void __stdcall SetGuiPath(Dialog* obj, const char* dir);
void __stdcall SetAnimsPath(Dialog* obj, const char* dir);
int __stdcall IsCurrentGadgetNamed(Dialog* gui, char* name);
void __stdcall GetGadgetRectByIndex(char*, int, Rect*);
void __stdcall GetGadgetScreenRect(char*, int, Rect*);
int __stdcall FindBackgroundCell(Gadget* entries, int index);
int __stdcall IsPointInRect(Rect* r, int px, int py);
void __stdcall DrawListBox(DrawListBoxDialog* obj, int index);
void __stdcall SyncAssocGadgets(SyncAssocGadgetsDialog*, int);
void __stdcall SetListBoxScrollByName(SetListBoxScrollByNameDialog*, char*, int);
void __stdcall DrawProgressBarGadget(DrawProgressBarGadgetDialog * obj, int index);
void __stdcall DrawHotspot(DrawHotspotDialog* obj, int index);
void __stdcall DrawTextClipped(void* surface, char* text, int x, int y, int maxw, int style);
void __stdcall SetCurrentFont(Gui* p, int index);
int __stdcall TruncateGadgetText(Gui* menu, int index);
void __stdcall ClearGroupStatus(Gui*, int index);
void __stdcall BeginTextEdit(Gui* obj, int index);
void __stdcall SelectGadgetByName(Gui* menu, char* name);
void __stdcall SelectGadgetByIndex(Gui* menu, int index);
int __stdcall RenderLayer(Gui* menu, unsigned int flags);
void __stdcall CloseTopScreen(Gui* gui);
void __stdcall SetCurrentGuiContext(Gui* ctx);
void __stdcall SetDefaultFont(int*, int);

void __stdcall GetGadgetName(Gadget* entries, char* name, int index);

void __stdcall TranslateGadgetItems(GadgetAccessDialog* obj, int index);
void __stdcall SetGadgetGrayedOut(GadgetAccessDialog* obj, int index, int value);
void __stdcall SetGadgetGrayedOutByNameTwin(GadgetAccessDialog* obj, char* name, int);
int __stdcall IsAboveRectTop(IsAboveRectTopRect*, int);
int __stdcall IsBelowRectBottom(IsBelowRectBottomRect* obj, int value);
void __stdcall DrawGafBar(GadgetAccessDialog*, int index);
void __stdcall ResetListBox(GadgetAccessDialog* obj, int index);

#endif
