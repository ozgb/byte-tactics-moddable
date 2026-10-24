#include <string.h>
#include <ctype.h>
#include "win_types.h"
#include "../../interfaces/host.h"

#pragma pack(push, 1)

struct Layer;

#include "gadget.h"

struct Layer {
    Layer* next;
    Gadget* entries;
    void (__stdcall* handler)(Gui*);
    int data;
    int flags;
    int redraw;
    int keyboardInput;
    int cb1c;
    int current;
    int surface;
    char unknown_28[0x13];
    int textHandler;
    Gadget gadgets[200];
};

struct Src_004ab400;

struct Ref_004ab400 {
    unsigned short index;
    unsigned short value;
    unsigned char kind;
    char unknown_5[3];
    Src_004ab400* src;
};

struct Flags_004ab400 {
    unsigned int active : 1;
    unsigned int rest : 31;
};

#include "mouse_event.h"

struct Rect_004b6720 {
    int left;
    int top;
    int right;
    int bottom;
};

#include "gui.h"

struct Record_004ab2b0 {
    unsigned char type;
    char unknown_1[0xb6 - 0x1];
    short count;
    char unknown_b8[0x13e - 0xb8];
};

int PlaceFeature(int, int, int, int, int);

struct Record_004ab310 {
    unsigned char type;
    char unknown_1[0xb6 - 0x1];
    union {
        short count;
        int callback;
    };
    char unknown_ba[0xbe - 0xba];
    int image;
    int frame;
    short index;
    char unknown_c8[0xcc - 0xc8];
};

struct Entry_004ab310 {
    Record_004ab310 record;
    char unknown_cc[0x15b - 0xcc];
};

struct Record_004ab3a0 {
    unsigned char type;
    char unknown_1[0xb6 - 0x1];
    short count;
    char unknown_b8[0xd6 - 0xb8];
};

struct Entry_004ab3a0 {
    Record_004ab3a0 record;
    char unknown_d6[0x15b - 0xd6];
};

#pragma pack(pop)

extern int* g_guiContext;

extern void __stdcall FadeRectangle(void* surface, int* rect, int mode);
extern char* __stdcall StripPath(char* path);
extern char* __stdcall ChangeExtension(char* out, char* in, char* ext);
#include "../files/files_api.h"
extern void* __cdecl GameAllocIgnoreTag(const char* path, unsigned int size);
extern int __stdcall ReadGuiFile(void* entry, char* path);
extern void __cdecl GameFreeThunk(void* p);
extern int __stdcall RenderLayer(Gui* menu, unsigned int flags);
extern void __cdecl HideSoftwareCursor(void);
extern void __cdecl ShowSoftwareCursor(void);
extern void __stdcall SelectAdjacentGadget(Gui* menu, int value);
extern void __stdcall TrySetFocus(Gui* menu, int value);
extern int __cdecl GetTextKeyColor(void);
extern void __stdcall SetTextColors(int a, int b);
extern void __stdcall SetFont(int a);
extern void __cdecl ClearKeyQueue(void);
#include "dialogs.h"

extern int __stdcall PointInRect(Rect_004b6720* r, int x, int y);
extern int __stdcall RectsOverlap(Rect_004b6720* a, Rect_004b6720* b);
extern void __stdcall DrawSurface(void* dest, void* image, int x, int y);

extern void __stdcall InitGafSequence(Ref_004ab400* ref, Src_004ab400* src, int index);
extern int __stdcall GetGafSequenceFrame(Ref_004ab400* ref);
#include "mouse.h"

extern void __stdcall AdvanceGafSequence(Ref_004ab400* ref, int step);
extern int __stdcall PeekMouseEvent(MouseEvent* out);
extern void __stdcall GetCurrentMouseEvent(MouseEvent* out);
extern void __stdcall PopMouseEvent(MouseEvent* out);
extern void __stdcall GetGadgetScreenRect(Gadget* table, int index, Rect_004b6720* out);
extern int __stdcall IsPointInRect(Rect_004b6720* r, int px, int py);

extern void __stdcall DrawTextInput(Gui* control, int index);
extern int __stdcall GetTextPixelWidth(unsigned char* text);
int PopKey();
void* GetDisplay();

static inline int FindPanel_004aa8f0(Layer* layer)
{
    Gadget* base = layer->entries;
    int i;
    for (i = 1; i < base->u.count + 1; i++) {
        if (strncmp(base[i].name, "PANEL", 0x10) == 0)
            return i;
    }
    return -1;
}

static inline void GetGadgetRect_inlined(Gadget* entry, Rect_004b6720* rect)
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

// FUNCTION: 0x4aa8f0
Layer* __stdcall LoadGuiLayer(Gui* menu, const char* name,
                                       unsigned int flags)
{
    Layer* layer;
    int ret = 1;
    int mask;
    int rect[4];
    char layerName[0x100];
    char guiName[0x100];
    Gadget* entry = 0;

    if (flags & 0x800) {
        Layer* cur = menu->layer;
        if (cur != 0) {
            GetGadgetRect_inlined(cur->entries, (Rect_004b6720*)rect);
            FadeRectangle(cur->entries->u.assets.surface, rect, -0x18);
            if (menu->layer != 0)
                menu->layer->redraw = 1;
        } else {
            FadeRectangle(0, 0, -0x18);
        }
    }
    strncpy(layerName, menu->str_9b6, 0x100);
    strcat(layerName, name);
    strncpy(guiName, name, 0x100);
    StripPath(guiName);
    ChangeExtension(layerName, layerName, "GUI");
    if (HAPI_FileLengthByName(layerName) != 0) {
        int mask = flags & 0x200;
        if (mask != 0) {
            layer = menu->layer;
            entry = &layer->entries[layer->entries->u.count + 1];
        } else {
            layer = (Layer*)GameAllocIgnoreTag(guiName, 0x10f57);
            memset(layer, 0, 0x10f57);
            entry = layer->gadgets;
        }
        if (ReadGuiFile(entry, layerName) != 0) {
          if (mask != 0) {
            int idx = FindPanel_004aa8f0(layer);
            if (idx != -1) {
                layer->entries[idx].active = 0;
                flags |= 0x20;
                int dx = (layer->entries[idx].width - entry->width) / 2 + layer->entries[idx].x;
                int dy = (layer->entries[idx].height - entry->height) / 2 + layer->entries[idx].y;
                int j = 1;
                if (j <= entry->u.count) {
                    Gadget* e = &entry[1];
                    do {
                        e->x += dx;
                        e->y += dy;
                        j++;
                        e++;
                    } while (j <= entry->u.count);
                }
            } else {
                int j = 1;
                if (j <= entry->u.count) {
                    Gadget* e = &entry[1];
                    do {
                        e->x += entry->x;
                        e->y += entry->y;
                        j++;
                        e++;
                    } while (j <= entry->u.count);
                }
            }
            layer->entries->u.count += entry->u.count;
            memcpy(entry, &entry[1], entry->u.count * 0x15b);
            entry = layer->entries;
          }
        } else {
            GameFreeThunk(layer);
        }
    }
    layer->entries = entry;
    layer->cb1c = 0;
    layer->surface = 0;
    layer->textHandler = 0;
    if ((flags & 0x200) == 0) {
        layer->next = menu->layer;
        menu->layer = layer;
    }
    layer->flags = 0;
    if ((flags & 0x200) == 0 && (flags & 0x80) != 0)
        layer->flags = 0x80;
    layer->flags |= flags & 0x800;
    if (menu->layer != 0)
        menu->layer->redraw = 1;
    if (menu->layer != 0)
        menu->layer->keyboardInput = 0;
    strncpy((char*)&entry->name[0], guiName, 0x10);
    menu->focus = -1;
    if ((flags & 0x400) == 0) {
        HideSoftwareCursor();
        ret = RenderLayer(menu, flags | 1);
        ShowSoftwareCursor();
    }
    {
        char* dst = entry->u.names.choice;
        if (strlen(dst) == 0) {
            int i = 1;
            while (i <= entry->u.count) {
                if (entry[i].type == 1 && (_strnicmp(entry[i].name, "OK", 2) == 0
                        || _strnicmp(entry[i].name, "NEXT", 4) == 0)) {
                    strcpy(dst, entry[i].name);
                    break;
                }
                i++;
            }
        }
        dst = entry->u.names.choice2;
        if (strlen(dst) == 0) {
            int i = 1;
            while (i <= entry->u.count) {
                if (entry[i].type == 1 && (_strnicmp(entry[i].name, "PREV", 4) == 0
                        || _strnicmp(entry[i].name, "Cancel", 6) == 0)) {
                    strcpy(dst, entry[i].name);
                    break;
                }
                i++;
            }
        }
        dst = entry->u.names.focusName;
        if (strlen(dst) != 0) {
            int i = 1;
            Gadget* e = &entry[1];
            while (i < entry->u.count + 1) {
                if (strncmp(e->name, dst, 0x10) == 0)
                    goto focusFound;
                i++;
                e++;
            }
            i = -1;
        focusFound:
            layer->current = i;
        } else {
            layer->current = 0;
            SelectAdjacentGadget(menu, 1);
        }
    }
    menu->hotGadgetIndex = -1;
    if (ret == 1) {
        if (entry->u.count == 1 && ((char*)entry)[0x15b] == 3) {
        Gadget* base = menu->layer->entries;
        Gadget* sub = &base[1];
        int r = GetTextKeyColor();
        unsigned int v = 0;
        v = menu->colours[sub->colours];
        SetTextColors(v, r);
        int i = 0;
        int j;
        int n = base->u.count + 1;
        for (j = 1; j < n; j++) {
            if (base[j].type != 7) {
                continue;
            }
            if (i == base[1].tab) {
                SetFont(*(int*)((char*)&base[j] + 0xd6));
                break;
            }
            i++;
        }
        if (j == base->u.count + 1)
            SetFont(*g_guiContext);
        TrySetFocus(menu, 1);
        menu->layer->current = 1;
        CommitTextEdit(menu, 1, (char*)&sub->u.count,
                     *(short*)((char*)sub + 0x138), 0);
        ClearKeyQueue();
        }
        return layer;
    }
    GameFreeThunk(layer);
    return 0;
}

// FUNCTION: 0x4aafe0
int __stdcall IsPointInEntryRect(Gui* obj)
{
    if (!obj->layer)
        return 0;
    Gadget* info = obj->layer->entries;
    Rect_004b6720 r;
    r.left = info->x;
    r.top = info->y;
    r.right = r.left + info->width - 1;
    r.bottom = r.top + info->height - 1;
    return PointInRect(&r, obj->pointX, obj->pointY);
}

// FUNCTION: 0x4ab040
int __stdcall HasLayer(Gui* obj)
{
    return obj->layer != 0;
}

// FUNCTION: 0x4ab060
int __stdcall IsScreenNamed(Gui* obj, const char* name)
{
    if (obj->layer && _strnicmp((char*)&obj->layer->entries->name[0], name, 0x10) == 0)
        return 1;
    return 0;
}

// FUNCTION: 0x4ab0a0
void __stdcall ClearSelectedGadget(void* gui)
{
    *(int*)((char*)gui + 0x60) = -1;
}

// FUNCTION: 0x4ab0b0
int __stdcall BlitLayers(Layer* node, void* surface, Rect_004b6720* clipRect)
{
    Rect_004b6720 rect;
    if (node == 0) {
        return 0;
    }
    BlitLayers(node->next, surface, clipRect);
    Gadget* g = node->entries;
    rect.left = g->x;
    rect.top = g->y;
    rect.right = g->width + rect.left - 1;
    rect.bottom = g->height + rect.top - 1;
    if (clipRect == 0) {
        if (node->redraw == 1) {
            node->redraw = 0;
            DrawSurface(surface, g->u.assets.surface, g->x, g->y);
        }
    } else {
        if (node->redraw != 1 && RectsOverlap(&rect, clipRect) == 0) {
            goto finish;
        }
        node->redraw = 0;
        DrawSurface(surface, g->u.assets.surface, g->x, g->y);
    }
finish:
    return 1;
}

// FUNCTION: 0x4ab170
void __stdcall BlitMenuLayers(Gui* gui, unsigned int* surface, int* rect)
{
    BlitLayers(gui->layer, surface, (Rect_004b6720*)rect);
}

// FUNCTION: 0x4ab190
void __stdcall SetDescListCleanupFlag(int gui, int flag)
{
    *(int*)(gui + 0xcc6) = flag;
}

// FUNCTION: 0x4ab1b0
void __stdcall AddTextGadget(Layer* obj, char* name, char* text,
                            int x, short y, int w, int flags)
{
    Gadget* entries = obj->entries;
    short n = entries->u.count;
    int index = n + 1;
    n++;
    entries->u.count = n;
    Gadget* e = &obj->entries[index];
    e->type = 5;
    e->x = x;
    e->y = y;
    if (w == -1)
        e->width = entries->width - x - 5;
    else
        e->width = w;
    e->height = 0xf;
    e->colours = 0xf;
    e->attribs = flags;
    e->team = 0;
    e->image = 0;
    e->textureNumber = 0;
    e->tab = 0;
    e->active = 1;
    e->commonAttribs = 0;
    strcpy(e->name, name);
    strncpy(e->u.text, text, 0x7f);
    e->u.text[0x7f] = 0;
}

// FUNCTION: 0x4ab290
int __stdcall SetBackgroundSurface(Gui* menu, int value)
{
    if (menu->layer != 0)
        menu->layer->surface = value;
    return 1;
}

// FUNCTION: 0x4ab2b0
int __stdcall AddButtonGadget(Gui* obj, Record_004ab2b0* record)
{
    Gadget* entries = obj->layer->entries;
    if (entries->u.count == 200) {
        return 0;
    }
    short n = ++entries->u.count;
    Record_004ab2b0* dst = (Record_004ab2b0*)&entries[n];
    *dst = *record;
    dst->type = 1;
    return 1;
}

// FUNCTION: 0x4ab310
int __stdcall AddHotspotGadget(Gui* obj, Record_004ab310* record)
{
    Entry_004ab310* entries = (Entry_004ab310*)obj->layer->entries;
    if (entries->record.count == 200) {
        return 0;
    }
    short n = ++entries->record.count;
    entries[n].record = *record;
    entries[n].record.type = 6;
    Entry_004ab310* dst = (Entry_004ab310*)&obj->layer->entries[n];
    dst->record.callback = 0;
    dst->record.image = 0;
    dst->record.frame = 0;
    dst->record.index = 0;
    return 1;
}

// FUNCTION: 0x4ab3a0
int __stdcall AddBarGadget(Gui* obj, Record_004ab3a0* record)
{
    Entry_004ab3a0* entries = (Entry_004ab3a0*)obj->layer->entries;
    if (entries->record.count == 200) {
        return 0;
    }
    short n = ++entries->record.count;
    entries[n].record = *record;
    entries[n].record.type = 0xd;
    return 1;
}

// FUNCTION: 0x4ab400
void __stdcall SetCursorAnimation(Gui* p, Src_004ab400* src)
{
    p->src_2c = src;
    InitGafSequence((Ref_004ab400*)&p->cursorIndex, src, 0);
    p->cursorFlags |= 1;
    SetCursorSprite(GetGafSequenceFrame((Ref_004ab400*)&p->cursorIndex));
}

static inline void SetSource(Gui* p, Src_004ab400* src)
{
    if ((Src_004ab400*)p->cursorSrc == src)
        return;
    InitGafSequence((Ref_004ab400*)&p->cursorIndex, src, 0);
    SetCursorSprite(GetGafSequenceFrame((Ref_004ab400*)&p->cursorIndex));
    p->cursorFlags |= 1;
}

static inline void SetValue(Gui* p, int value)
{
    if (GetCursorSprite() == value)
        return;
    SetCursorSprite(value);
    p->cursorSrc = 0;
    p->cursorFlags &= ~1;
}

// FUNCTION: 0x4ab440
void __stdcall SetCursorHover(Gui* p, int alt)
{
    if (alt) {
        if (p->src_28)
            SetSource(p, (Src_004ab400*)p->src_28);
        else
            SetValue(p, p->cursorHoverFrame);
    } else {
        if (p->src_2c)
            SetSource(p, (Src_004ab400*)p->src_2c);
        else
            SetValue(p, p->cursorDefaultFrame);
    }
}

// FUNCTION: 0x4ab4c0
void __stdcall ApplySharedCursorFrame(Gui* p)
{
    SetCursorSprite(p->cursorSharedFrame);
    p->cursorFlags &= ~1;
}

// FUNCTION: 0x4ab4e0
void __stdcall InitCursorFrames(Gui* p, int value)
{
    p->cursorSharedFrame = value;
    p->cursorDefaultFrame = value;
    p->cursorHoverFrame = value;
    SetCursorSprite(value);
    p->cursorFlags &= ~1;
    p->mouseKeyFlags = 0;
    p->src_28 = 0;
    p->src_2c = 0;
}

// FUNCTION: 0x4ab510
int __stdcall IsMouseButtonMessage(Gui* obj, unsigned char buttons)
{
    int& msg = obj->pointMessage;
    if (buttons & 1) {
        if (msg == WM_LBUTTONDOWN)
            return 1;
        if (msg == WM_LBUTTONDBLCLK)
            return 1;
    } else if (buttons & 2) {
        if (msg == WM_RBUTTONDOWN)
            return 1;
        if (msg == WM_RBUTTONDBLCLK)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4ab570
int __stdcall IsDoubleClickMessage(Gui* obj, unsigned char buttons)
{
    int& msg = obj->pointMessage;
    if (buttons & 1) {
        if (msg == WM_LBUTTONDBLCLK)
            return 1;
    } else if (buttons & 2) {
        if (msg == WM_RBUTTONDBLCLK)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4ab5b0
int __stdcall HasMouseKeyFlags(Gui* obj, unsigned int mask)
{
    return (obj->mouseKeyFlags & mask) != 0;
}

// FUNCTION: 0x4ab5d0
void __stdcall UpdateCursorAndMouse(Gui* p)
{
    if (p->cursorFlags & 1) {
        int old = p->cursorIndex;
        AdvanceGafSequence((Ref_004ab400*)&p->cursorIndex, p->animTimer);
        if (p->cursorIndex != old)
            SetCursorSprite(GetGafSequenceFrame((Ref_004ab400*)&p->cursorIndex));
    }

    MouseEvent e;
    if (PeekMouseEvent(&e) != 0) {
        if (p->layer != 0) {
            Rect_004b6720 r;
            GetGadgetScreenRect(p->layer->entries, 0, &r);
            if (IsPointInRect(&r, e.x, e.y) != 0 || e.keyFlags == 0) {
                PopMouseEvent(&e);
                p->mouseKeyFlags = e.keyFlags;
                *(MouseEvent*)&p->pointX = e;
            }
        }
    } else {
        GetCurrentMouseEvent((MouseEvent*)&p->pointX);
    }
}

// FUNCTION: 0x4ab690
void __stdcall SetClickMode(void* gui, int mode)
{
    *(int*)((char*)gui + 0x58) = mode;
    *(int*)(*(int*)((char*)gui + 0x18) + 0x37) = mode;
}

// FUNCTION: 0x4ab6b0
int __stdcall GetClickMode(Gui* obj)
{
    return obj->clickMode;
}

#pragma auto_inline(off)
// FUNCTION: 0x4ab6c0
void __stdcall CommitTextEdit(Gui* control, int index, char* text,
                            int maxLength, int clear)
{
    int length = strlen(text);
    if (clear == 0 && length <= maxLength) {
        control->cursor = length;
    } else {
        *text = 0;
        control->cursor = 0;
    }
    DrawTextInput(control, index);
}
#pragma auto_inline(on)

// FUNCTION: 0x4ab720
int __stdcall HandleTextEditKey(Gui* control, int index, int key)
{
    Layer* holder = control->layer;
    Gadget* entry = &holder->entries[index];
    int i;
    char* text = entry->u.text;
    int changed = 0;
    int last = 0;

    if (holder->keyboardInput == 0)
        key = PopKey();
    if (key != 0) {
        changed = 1;
        while (key != 0) {
            last = key;
            int len = strlen(text);
            switch (key) {
            case 0xf1:
                control->cursor = len;
                break;
            case 0xf0:
                control->cursor = 0;
                break;
            case 8:
                if (control->cursor != 0) {
                    control->cursor--;
                    int n = strlen(text);
                    for (i = control->cursor; i < n - 1; i++)
                        text[i] = text[i + 1];
                    text[n - 1] = 0;
                }
                break;
            case 0xf4:
                if (control->cursor != 0)
                    control->cursor--;
                break;
            case 0xf6:
                if (control->cursor < len)
                    control->cursor++;
                break;
            case 0xef:
                if (len != 0 && control->cursor < len) {
                    int n = strlen(text);
                    for (i = control->cursor; i < n - 1; i++)
                        text[i] = text[i + 1];
                    text[n - 1] = 0;
                }
                break;
            case 0xbf:
            case 0xee: {
                HWND hwnd = *(HWND*)((char*)GetDisplay() + 0x40);
                unsigned int copied = host_clipboard_text_get(hwnd, text, entry->status - 1);
                if (copied != 0) {
                    if (copied < 0x80)
                        memset(text + copied, 0, 0x80 - copied);
                    int width = GetTextPixelWidth((unsigned char*)text);
                    while (width > entry->width) {
                        if (strlen(text) == 0)
                            break;
                        text[strlen(text) - 1] = 0;
                        width = GetTextPixelWidth((unsigned char*)text);
                    }
                }
                break;
            }
            case 0x1b:
                goto done;
            default: {
                if (len == entry->status)
                    break;
                if (key < 0x20 || key > 0x7f)
                    break;
                if (*(unsigned char*)&entry->attribs & 2) {
                    if (!isalnum(key) && key != '_' && key != ' ' && key != '\'')
                        break;
                }
                char c[2];
                c[0] = (char)key;
                c[1] = 0;
                int width = GetTextPixelWidth((unsigned char*)entry->u.text) + GetTextPixelWidth((unsigned char*)c);
                if (width > entry->width - 4)
                    break;
                int n = entry->status - 1;
                for (i = n; i > control->cursor; i--)
                    text[i] = text[i - 1];
                text[control->cursor] = (char)key;
                control->cursor++;
                break;
            }
            }
            key = PopKey();
        }
    }
done:
    if (changed)
        DrawTextInput(control, index);
    return last;
}

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma pack(push, 1)
struct PalEntry_004aa8f0 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char flags;
};

struct Palette_004aa8f0 {
    char unknown_0[0xb2];
    PalEntry_004aa8f0 dest[256];
    char unknown_4b2[0x400];
    unsigned char table[256];
};

struct Slider_004aa8f0 {
    char unknown_0[0x140];
    unsigned char value;
};

struct PaletteDialog_004aa8f0 {
    char unknown_0[0x9b2];
    int index;
    char unknown_9b6[0xcb6 - 0x9b6];
    Slider_004aa8f0* red;
    Slider_004aa8f0* green;
    Slider_004aa8f0* blue;
};

#pragma pack(pop)

#include "../../link/globals.h"

int __stdcall FindGadgetIndex(Gadget* entries, const char* name, int type);
int __stdcall IsGadgetNamed(Gadget* entries, int index, char* name);
char* __stdcall Translate(char* text);
char* __stdcall WordWrapText(Gui* menu, char* text, int width, int index);
int __stdcall GetFontHeight();
int GetScreenWidth();
int GetScreenHeight();
void __stdcall SetGadgetActiveByName(Gui* menu, const char* name, int flag);
void __stdcall SetKeyboardInput(Gui* menu, int flag);
void __stdcall MarkChanged(Gui* menu);
void __stdcall UpdateMenu(Gui* menu);
int __stdcall SelectFontForEntry(Gadget* entries, int index);
int GetFont();
int __stdcall GetTextWidth(int font, unsigned char* text);
int __stdcall FillRectangle(void* surface, Rect_004b6720* rect, int color);
int __stdcall SetPaletteColors(unsigned char* src, int start, int count);

struct FileHandle;


// FUNCTION: 0x4abb20
int __stdcall OpenConfirmDialog(Gui* sub, char* title)
{
    Layer* layer = LoadGuiLayer(sub, "CONFIRM.GUI", 0);
    if (layer) {
        Gadget* gadgets = layer->entries;
        int i = FindGadgetIndex(gadgets, "TITL", 5);
        strcpy(gadgets[i].u.text, title);
        gadgets[i].x = -1;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4abba0
void __stdcall YesNoDialogHandler(Gui* gadget)
{
    Gadget* entries = gadget->layer->entries;
    int index = gadget->hotGadgetIndex;
    IsGadgetNamed(entries, index, "CHC1");
    IsGadgetNamed(entries, index, "CHC2");
}

// FUNCTION: 0x4abbd0
int __stdcall OpenYesNoDialog(Gui* sub, char* title, char* chc2, char* chc1)
{
    Layer* layer = LoadGuiLayer(sub, "YESORNO.GUI", 0x800);
    if (layer) {
        Gadget* entries = sub->layer->entries;
        Gadget* p1 = &entries[FindGadgetIndex(entries, "CHC1", 1)];
        Gadget* p2 = &entries[FindGadgetIndex(entries, "CHC2", 1)];
        Gadget* p3 = &entries[FindGadgetIndex(entries, "TITL", 5)];
        strcpy(p1->u.text, chc1);
        strcpy(p2->u.text, chc2);
        strcpy(p3->u.text, title);
        layer->handler = YesNoDialogHandler;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4abd00
void __stdcall MessageBoxHandler(Gui* menu)
{
    Layer* layer = menu->layer;
    IsGadgetNamed(layer->entries, menu->hotGadgetIndex, g_okGadgetName);
}

// FUNCTION: 0x4abd20
int __stdcall IsMessageBoxScreen(Gui* obj)
{
    if (!obj->layer)
        return 0;
    return strcmp((char*)&obj->layer->entries->name[0], "MSGBOX.GUI") == 0;
}

// FUNCTION: 0x4abd90
int __stdcall OpenMessageBox(Gui* gui, char* text, int wrapWidth, int centre, int autoHeight)
{
    Layer* layer = LoadGuiLayer(gui, "MSGBOX.GUI", 0x800);
    if (layer) {
        char name[0x100];
        char buf[0x100];
        strcpy(name, Translate(text));
        char* wrapped = WordWrapText(gui, name, wrapWidth, -1);
        RenderLayer(gui, 2);
        strncpy(buf, wrapped, 0xfe);
        Gadget* entries = gui->layer->entries;
        char* line = strtok(buf, "\n");
        int lines = 0;
        int y = 0x14;
        int next = layer->entries->u.count + 1;
        if (line) {
            do {
                AddTextGadget(layer, "TEXT", line, 0, y, -1, 2);
                y += GetFontHeight() + 5;
                line = strtok(0, "\n");
                lines++;
            } while (line);
        }
        int width;
        if (autoHeight) {
            width = 0;
            if (lines > 0) {
                char* p = entries[next].u.text;
                int i = lines;
                do {
                    if (width <= GetTextPixelWidth((unsigned char*)p))
                        width = GetTextPixelWidth((unsigned char*)p);
                    p += 0x15b;
                } while (--i);
            }
            width += 0x14;
        } else {
            width = wrapWidth;
        }
        entries->width = width;
        entries->height = lines * 25 + entries[1].height + 0x28;
        entries->x = (GetScreenWidth() - entries->width) / 2;
        entries->y = (GetScreenHeight() - entries->height) / 2;
        int i;
        for (i = 0; i <= entries->u.count; i++) {
            if (entries[i].type == 5) {
                entries[i].width = entries->width;
                entries[i].attribs = 2;
            }
        }
        if (centre) {
            int k = FindGadgetIndex(entries, "OK", 0xe);
            if (k != -1) {
                entries[k].y = entries->height - entries[k].height - 0xf;
                entries[k].x = entries->width - entries[k].width - 0xf;
                strcpy(entries->u.names.choice, "OK");
                strcpy(entries->u.names.choice2, "OK");
            }
        } else {
            SetGadgetActiveByName(gui, "OK", 0);
        }
        SetKeyboardInput(gui, 1);
        RenderLayer(gui, 1);
        layer->handler = MessageBoxHandler;
        MarkChanged(gui);
        UpdateMenu(gui);
        GameFreeThunk(wrapped);
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4ac080
void __stdcall NotExistDialogHandler(Gui* obj)
{
    IsGadgetNamed(obj->layer->entries, obj->hotGadgetIndex, g_okGadgetName);
}

// FUNCTION: 0x4ac0a0
int __stdcall OpenNotExistDialog(Gui* sub, char* name)
{
    Layer* dialog = LoadGuiLayer(sub, "NOTEXIST.GUI", 0);
    if (dialog) {
        Gadget* gadgets = sub->layer->entries;
        int i = FindGadgetIndex(gadgets, "NAME", 5);
        strcpy(gadgets[i].u.text, name);
        gadgets[i].x = -1;
        dialog->handler = NotExistDialogHandler;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4ac130
void __stdcall Choice3DialogHandler(Gui* gui)
{
    Gadget* p = gui->layer->entries;
    FindGadgetIndex(p, "CHC1", 0xe);
    FindGadgetIndex(p, "CHC2", 0xe);
    FindGadgetIndex(p, "CHC3", 0xe);
}

// FUNCTION: 0x4ac170
int __stdcall OpenChoice3Dialog(Gui* menu, const char* title, const char* choice1,
                           const char* choice2, const char* choice3)
{
    Layer* dialog = LoadGuiLayer(menu, "CHOICE3.GUI", 0);
    if (dialog != 0) {
        Gadget* entries = menu->layer->entries;
        RenderLayer(menu, 1);
        Gadget* e1 = &entries[FindGadgetIndex(entries, "CHC1", 1)];
        Gadget* e2 = &entries[FindGadgetIndex(entries, "CHC2", 1)];
        Gadget* e3 = &entries[FindGadgetIndex(entries, "CHC3", 1)];
        Gadget* et = &entries[FindGadgetIndex(entries, "TITL", 5)];
        strcpy(e1->u.text, choice1);
        strcpy(e2->u.text, choice2);
        strcpy(e3->u.text, choice3);
        strcpy(et->u.text, title);
        et->x = 0xffff;
        dialog->handler = Choice3DialogHandler;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4ac300
void __stdcall InputDialogHandler(Gui* gui)
{
    Gadget* esi = gui->layer->entries;
    int edi = gui->hotGadgetIndex;
    IsGadgetNamed(esi, edi, "CHC1");
    IsGadgetNamed(esi, edi, "CHC2");
    IsGadgetNamed(esi, edi, "INPT");
}

// FUNCTION: 0x4ac340
int __stdcall OpenInputDialog(Gui* sub, char* title, char* input, char* chc2, char* chc1, int unused)
{
    Layer* dialog = LoadGuiLayer(sub, "INPUT.GUI", 0);
    if (dialog) {
        Gadget* gadgets = sub->layer->entries;
        Gadget* g = &gadgets[FindGadgetIndex(gadgets, "INPT", 3)];
        strcpy(g->u.text, input);
        g = &gadgets[FindGadgetIndex(gadgets, "TITL", 5)];
        strcpy(g->u.text, title);
        g->x = -1;
        Gadget* g1 = &gadgets[FindGadgetIndex(gadgets, "CHC1", 1)];
        Gadget* g2 = &gadgets[FindGadgetIndex(gadgets, "CHC2", 1)];
        strcpy(g1->u.text, chc1);
        strcpy(g2->u.text, chc2);
        dialog->handler = InputDialogHandler;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4ac4c0
char* __stdcall WordWrapText(Gui* menu, char* text, int width, int index)
{
    Gadget* gadgets = menu->layer->entries;
    text += 1; text -= 1;
    int len = strlen(text);
    if (index != -1)
        SelectFontForEntry(gadgets, index);
    int w;
    if (index == -1)
        w = GetTextPixelWidth((unsigned char*)"d");
    else
        w = GetTextWidth(GetFont(), (unsigned char*)"d");
    int size = len + 3 * (len / (width / w)) + 2;
    char* buf = (char*)GameAllocIgnoreTag("WordWrap", size);
    memset(buf, 0, size);
    int i = 0;
    char c = *text;
    char* s = text;
    char* p = buf;
    while (c != 0) {
        if (*s == (char)0xff)
            break;
        buf[i] = *s;
        char next = s[1];
        i++;
        s++;
        if (next == ' ' || next == '\n' || next == '-') {
            int wrapped = 0;
            int m;
            if (index == -1)
                m = GetTextPixelWidth((unsigned char*)p);
            else
                m = GetTextWidth(GetFont(), (unsigned char*)p);
            if (width <= m) {
                buf[i] = 0;
                wrapped = 1;
                char ch = s[-1];
                i--;
                s--;
                while (ch != ' ' && ch != '-') {
                    i += 1; i -= 1;
                    buf[i] = 0;
                    ch = s[-1];
                    i--;
                    s--;
                }
                buf[i] = '\r';
                i++;
                buf[i] = '\n';
                i++;
                s++;
            }
            if (wrapped)
                p = buf + i;
        }
        c = *s;
        if (c == '\n')
            p = buf + i + 1;
    }
    buf[i] = 0;
    return buf;
}

static inline int Measure_004ac610(unsigned char* text, int charset)
{
    if (charset == -1)
        return GetTextPixelWidth(text);
    return GetTextWidth(GetFont(), text);
}

// FUNCTION: 0x4ac610
void __stdcall TruncateTextWithEllipsis(Gui* obj, unsigned char* text, int limit,
                            int charset, int flag)
{
    Gadget* entries = obj->layer->entries;
    if (charset != -1)
        SelectFontForEntry(entries, charset);

    int dots;
    int width;
    if (charset == -1) {
        width = GetTextPixelWidth(text);
        if (width < limit)
            return;
    } else {
        width = GetTextWidth(GetFont(), text);
        if (width < limit)
            return;
    }

    dots = flag ? Measure_004ac610((unsigned char*)"...", charset) : 0;

    unsigned char* end = text;
    while (*end)
        end++;

    while (width + dots > limit) {
        if (end == text)
            break;
        end--;
        *end = 0;
        width = Measure_004ac610(text, charset);
    }

    if (flag)
        strcpy((char*)end, "...");
}

struct Bitmap_004cb170;
struct Blk;
struct Blob32_4cb330;
struct BlobRec_004b4270;
struct Block_00421620;
struct Block_004b1ec0;
struct Block_004c5ff0;
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;

// FUNCTION: 0x4ac710
void __stdcall BuildColorRemapTable(PalEntry_004aa8f0* src, PalEntry_004aa8f0* dest, unsigned char* table)
{
    for (int n = 256; n != 0; n--) {
        int i = 0;
        int best = 9999999;
        PalEntry_004aa8f0* p = dest;
        int b = src->b;
        int g = src->g;
        int r = src->r;
        int bestIndex;
        for (; i < 256; i++, p++) {
            int d = abs(b - p->b) + abs(g - p->g) + abs(r - p->r);
            if (d < best) {
                best = d;
                bestIndex = i;
            }
        }
        src++;
        *table++ = (unsigned char)bestIndex;
    }
}

void __stdcall ApplySlidersToPaletteEntry(PaletteDialog_004aa8f0* obj, PalEntry_004aa8f0* palette);

// FUNCTION: 0x4ac7d0
void __stdcall RemapPaletteToClosestIndices(Palette_004aa8f0* pal, PalEntry_004aa8f0* src, PalEntry_004aa8f0* copy)
{
    memcpy(pal->dest, copy, 0x400);

    unsigned char* table = pal->table;
    PalEntry_004aa8f0* p = pal->dest;
    for (int n = 256; n != 0; n--) {
        int i = 0;
        int best = 9999999;
        PalEntry_004aa8f0* q = src;
        int bestIndex;
        for (; i < 256; i++, q++) {
            int d = abs(p->r - q->r) + abs(p->b - q->b) + abs(p->g - q->g);
            if (d < best) {
                best = d;
                bestIndex = i;
            }
        }
        p++;
        *table++ = (unsigned char)bestIndex;
    }
}

// FUNCTION: 0x4ac8a0
void __stdcall CopyPaletteEntries(char* obj, void* dest)
{
    memcpy(dest, obj + 0xb2, 0x100 * sizeof(int));
}


// FUNCTION: 0x4ac8c0
void __stdcall DrawColorGrid(Gui* obj)
{
    Gadget *grid, *gadgets;
    gadgets = obj->layer->entries;
    int index = FindGadgetIndex(gadgets, "COLS", 6);
    grid = &gadgets[index];
    void* surface = gadgets->u.assets.surface;
    int x0 = grid->x + gadgets->x;
    int y = gadgets->y + grid->y;
    Rect_004b6720 rect;
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            rect.left = x0 + col * 8;
            rect.top = y + row * 8;
            rect.right = rect.left + 7;
            rect.bottom = rect.top + 7;
            FillRectangle(surface, &rect, row * 16 + col);
        }
    }
}

// FUNCTION: 0x4aca20
void __stdcall ShowPaletteEntryRgb(Gui* obj, unsigned char* colors, int index)
{
    Gadget* entries = obj->layer->entries;
    Gadget* e;
    e = &entries[FindGadgetIndex(entries, "RED", 4)];
    e->knobPos = colors[index * 4];
    e = &entries[FindGadgetIndex(entries, "GREN", 4)];
    e->knobPos = colors[index * 4 + 1];
    e = &entries[FindGadgetIndex(entries, "BLUE", 4)];
    e->knobPos = colors[index * 4 + 2];
    RenderLayer(obj, 4);
}

// FUNCTION: 0x4acae0
void __stdcall LerpPaletteRange(unsigned char* data, int a, int b)
{
    unsigned char buf[768];
    memcpy(buf, data, 0x400);

    int lo = a < b ? a : b;
    int hi = a > b ? a : b;

    int base[3];
    int delta[3];
    int den = hi - lo;
    int i0 = lo * 3;
    base[0] = buf[i0 + 0];
    base[1] = buf[i0 + 1];
    base[2] = buf[i0 + 2];
    unsigned char* dst = &buf[i0];
    int i1 = hi * 3;
    delta[0] = buf[i1 + 0] - base[0];
    delta[1] = buf[i1 + 1] - base[1];
    delta[2] = buf[i1 + 2] - base[2];

    for (int i = lo; i < hi; i++) {
        for (int j = 0; j < 3; j++) {
            *dst = (unsigned char)(base[j] + delta[j] * (i - lo) / den);
            dst++;
        }
    }

    memcpy(data, buf, 0x400);
}

struct Object_004ac970;

// FUNCTION: 0x4acbe0
int __stdcall GetColorCellAt(Gui* obj, int x, int y)
{
    Gadget* gadgets = obj->layer->entries;
    int index = FindGadgetIndex(gadgets, "COLS", 6);
    Gadget* grid = &gadgets[index];
    int col = (x - gadgets->x - grid->x) / 8;
    int row = (y - gadgets->y - grid->y) / 8;
    if (col < 0) {
        col = 0;
    }
    if (col > 15) {
        col = 15;
    }
    if (row < 0) {
        row = 0;
    }
    if (row > 15) {
        row = 15;
    }
    return row * 16 + col;
}

// FUNCTION: 0x4acc70
void __stdcall ApplySlidersToPaletteEntry(PaletteDialog_004aa8f0* obj, PalEntry_004aa8f0* palette)
{
    PalEntry_004aa8f0 c;
    c.r = obj->red->value;
    c.g = obj->green->value;
    c.b = obj->blue->value;
    palette[obj->index] = c;
    SetPaletteColors((unsigned char*)&c, obj->index, 1);
}

// FUNCTION: 0x4accd0
void __stdcall WriteTabs(FileHandle* file, int depth)
{
    char tab = '\t';
    for (int i = 0; i < depth; i++)
        HAPI_WriteFile(file, &tab, 1);
}
