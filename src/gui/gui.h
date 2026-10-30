#ifndef GUI_H
#define GUI_H
struct GadgetAccessDialog;
struct IsAboveRectTopRect;
struct IsBelowRectBottomRect;

struct Gadget;

struct BlinkWord;
struct DrawListBoxDialog;
struct SyncAssocGadgetsDialog;
struct SetListBoxScrollByNameDialog;
struct DrawProgressBarGadgetDialog;
struct DrawHotspotDialog;
struct Gadget;
struct Rect;

struct FileRequester;
struct Font;
struct FontGlyphTable;
struct RedrawGadgetRectDialog;
struct ResetSliderGadgetDialog;
struct ScheduleGadgetEndTickDialog;
struct Gadget;
struct ResetProgressBarDialog;
struct DialogPointView;
struct DialogOriginView;
struct Rect;
#include "../util/mouse_event.h"

#pragma pack(push, 1)

struct Layer;

struct Gui {
    Font* font;
    void* gaf;
    void* values[3];
    FontGlyphTable* language;
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
    MouseEvent point;
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
    MouseEvent saved;
    char unknown_94[2];
    int time;
    int animTimer;
    int inputEnabled;
    int keyCommands;
    BlinkWord* words;
    int count;
    int active;
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

    FileRequester* OpenFileRequester(Gui* self, char* arg2, char* arg3, char* arg4);
};

#pragma pack(pop)

int __stdcall HasGadgetNamed(Gui* obj, char* name, int unused);
void __stdcall ClearChanged(void*);
void __stdcall ClearLayerChanged(Gui* param);
int __stdcall GetKeyboardInput(Gui* obj);
void __stdcall ClearFocus(Gui*);
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
void __stdcall ConfigureListBoxByName(Gui* menu, char* name, char* lines, int count, Font* flag);
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

void __stdcall SetGadgetName(Gui* obj, char* name, char* text);
void __stdcall RunWhileScreenNamed(Gui* menu, char* name);
void __stdcall EnableKeyCommands(Gui* p);
void __stdcall DisableKeyCommands(Gui* obj);
void __stdcall MarkChanged(Gui* obj);
void __stdcall MarkLayerChanged(Gui* obj);
void __stdcall DrawSlider(Gui* gui, int gadgetIndex);
void __stdcall SetFontsPath(Gui* obj, const char* dir);
void __stdcall SetGuiPath(Gui* obj, const char* dir);
void __stdcall SetAnimsPath(Gui* obj, const char* dir);
int __stdcall IsCurrentGadgetNamed(Gui* gui, char* name);
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
void __stdcall SetDefaultFont(Font**, Font*);

void __stdcall GetGadgetName(Gadget* entries, char* name, int index);

void __stdcall TranslateGadgetItems(GadgetAccessDialog* obj, int index);
void __stdcall SetGadgetGrayedOut(GadgetAccessDialog* obj, int index, int value);
void __stdcall SetGadgetGrayedOutByNameTwin(GadgetAccessDialog* obj, char* name, int);
int __stdcall IsAboveRectTop(IsAboveRectTopRect*, int);
int __stdcall IsBelowRectBottom(IsBelowRectBottomRect* obj, int value);
void __stdcall DrawGafBar(GadgetAccessDialog*, int index);
void __stdcall ResetListBox(GadgetAccessDialog* obj, int index);

#endif
