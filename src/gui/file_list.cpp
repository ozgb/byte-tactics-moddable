#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "../debug/debug_lib.h"
#include "../graphics/object_pool.h"
#include "../util/registry.h"
int __cdecl _strcmpi(const char* a, const char* b);
#include "../files/files_api.h"
#include "../graphics/palette.h"
int __stdcall ScanDirectory(char* path, char* list, char* sizes, int mode, int flag, int what);
#include "../graphics/display.h"
unsigned int GetTicks();
#include "../graphics/draw.h"

#pragma pack(push, 1)

struct BlinkWord {
    char text[0x80];
    int x;
    int y;
    int colourA;
    float periodA;
    int colourB;
    float periodB;
    int phase;
    float nextToggle;
    int value;
};

#include "gadget.h"

struct ReqSub {
    char unknown_0[0x140];
    short knobPos;
};

struct FileRequester {
    Gui* gui;
    ReqSub* slider;
    char* nameGadget;
    char* maskGadget;
    char* pathGadget;
    char drive[0x10];
    char cwd[0x100];
    char save_drive[0x10];
    char save_cwd[0x100];
    char* names;
    char* sizes;
    char* selected;
    int mask;
    void (__stdcall* callback)(void*);
};

#include "layer.h"

#include "gui.h"

#pragma pack(pop)

struct Gadget;

Gadget* __stdcall FindGadgetChecked_B(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked_C(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetChecked_D(Gadget* entries, char* name);
Gadget* __stdcall FindGadgetOrNull(Gadget* entries, char* name);
int __stdcall IsGadgetNamed(Gadget* entries, int i, char* name);
int __stdcall FindGadgetIndex(Gadget* entries, char* name, int type);
Gadget* __stdcall FindGadgetChecked(Gadget* entries, char* name);
#include "dialogs.h"
#include "file_list.h"
void __stdcall SelectFontForEntry(Gadget* entries, int index);
void __stdcall SetTextColors(int param_1, int param_2);
void __stdcall DrawString(void* surface, const char* text, int x, int y, int maxWidth);

// FUNCTION: 0x4aefa0
void __stdcall SortFileList(char* list1, char* list2, int* keys, int count)
{
    char** ptr2;
    char** ptr1;
    int swapped;
    int n;
    char* buf2;
    char* buf1;
    int end;
    int start;
    int i;

    buf1 = (char*)GameAllocIgnoreTag("SORTED LIST1", 0x17700);
    buf2 = (char*)GameAllocIgnoreTag("SORTED LIST2", 0x17700);
    ptr1 = (char**)GameAllocIgnoreTag("PTR LIST1", 0x2ee0);
    ptr2 = (char**)GameAllocIgnoreTag("PTR LIST2", 0x2ee0);
    n = -1;
    for (i = 0; i < count; i++) {
        ptr1[i] = SkipTextLines(list1, i);
        if (list2)
            ptr2[i] = SkipTextLines(list2, i);
        if (ptr1[i][0] == '\\')
            n = i;
    }
    if (n != -1) {
        do {
            swapped = 0;
            for (i = 0; i < n; i++) {
                int diff;
                if (keys)
                    diff = keys[i] - keys[i+1];
                else
                    diff = _strcmpi(ptr1[i], ptr1[i+1]);
                if (diff > 0) {
                    char* t = ptr1[i];
                    ptr1[i] = ptr1[i+1];
                    ptr1[i+1] = t;
                    if (list2) {
                        t = ptr2[i];
                        ptr2[i] = ptr2[i+1];
                        ptr2[i+1] = t;
                    }
                    if (keys) {
                        int u = keys[i];
                        keys[i] = keys[i+1];
                        keys[i+1] = u;
                    }
                    swapped = 1;
                }
            }
        } while (swapped);
    }
    start = n + 1;
    end = count - 1;
    do {
        swapped = 0;
        for (i = start; i < end; i++) {
            int diff;
            if (keys)
                diff = keys[i+1] - keys[i];
            else
                diff = _strcmpi(ptr1[i], ptr1[i+1]);
            if (diff > 0) {
                char* t = ptr1[i];
                ptr1[i] = ptr1[i+1];
                ptr1[i+1] = t;
                if (list2) {
                    t = ptr2[i];
                    ptr2[i] = ptr2[i+1];
                    ptr2[i+1] = t;
                }
                if (keys) {
                    int u = keys[i];
                    keys[i] = keys[i+1];
                    keys[i+1] = u;
                }
                swapped = 1;
            }
        }
    } while (swapped);
    {
        char* p = buf1;
        char* q = buf2;
        for (i = 0; i < count; i++) {
            strcpy(p, ptr1[i]);
            p += strlen(ptr1[i]) + 1;
            if (list2) {
                strcpy(q, ptr2[i]);
                q += strlen(ptr2[i]) + 1;
            }
        }
        memcpy(list1, buf1, p - buf1);
        if (list2)
            memcpy(list2, buf2, q - buf2);
    }
    GameFreeThunk(ptr2);
    GameFreeThunk(ptr1);
    GameFreeThunk(buf2);
    GameFreeThunk(buf1);
}

// FUNCTION: 0x4af320
int __stdcall ScanDirectory(char* path, char* list, char* sizes, int mode, int flag, int what)
{
    char* first = list;
    int num = 0;
    char* times = (char*)GameAllocIgnoreTag("FILETIMES", 0x2ee0);
    long* tp = (long*)times;
    struct _finddata_t fd;
    char text[256];
    int find;

    if (mode == 1) {
        find = HAPI_FindFirst("*.", &fd, -1, 1);
        if (find != -1) {
            do {
                if (fd.name[0] != '.' && fd.attrib == 0x10) {
                    strncpy(list, "\\", 1);
                    list++;
                    strcpy(list, fd.name);
                    list += strlen(fd.name);
                    *list++ = 0;
                    if (sizes) {
                        strcpy(sizes, "<DIR>");
                        sizes += 6;
                    }
                    num++;
                }
            } while (HAPI_FindNext(find, &fd) != -1);
            HAPI_FindClose(find);
        }
    }
    {
        find = HAPI_FindFirst(path, &fd, -1, 1);
        if (find != -1) {
            do {
                if (fd.name[0] != '.' && fd.attrib != 0x10) {
                    strcpy(list, fd.name);
                    if (flag == 1) {
                        StripExtension(list);
                    }
                    list += strlen(list) + 1;
                    *tp++ = fd.time_write;
                    if (sizes) {
                        _itoa(HAPI_FileLengthByName(fd.name), text, 10);
                        strcpy(sizes, text);
                        sizes += strlen(text) + 1;
                    }
                    num++;
                }
            } while (HAPI_FindNext(find, &fd) != -1);
            HAPI_FindClose(find);
        }
    }
    if (what) {
        if (what == 2) {
            SortFileList(first, 0, 0, num);
        } else {
            SortFileList(first, 0, (int*)times, num);
        }
    }
    GameFreeThunk(times);
    *list = 0;
    return num;
}

// FUNCTION: 0x4af5b0
void __stdcall RefreshFileList(FileRequester* obj)
{
    GetCurrentDriveLetter(obj->drive);
    GetDriveDirectory(obj->drive, obj->cwd, 0x100);
    int n = ScanDirectory(obj->maskGadget + 0xb6, obj->names, obj->sizes, 1, 0, 0);
    SortFileList(obj->names, obj->sizes, 0, n);
    ConfigureListBoxByName(obj->gui, "SWIN", obj->names, n, nullptr);
    ConfigureListBoxByName(obj->gui, "SIZE", obj->sizes, n, nullptr);
    obj->slider->knobPos = 0;
    strcpy(obj->pathGadget + 0xb6, obj->cwd);
}

// FUNCTION: 0x4af670
void __stdcall FileRequesterHandler(Gui* gadget)
{
    Gadget* entries;
    char drive[2];
    int result;
    int i;
    int n;
    FileRequester* req;
    req = (FileRequester*)gadget->layer->data;
    if (gadget->hotGadgetIndex == -1) {
        ChangeDrive(req->save_drive);
        ChangeDirectory(req->save_cwd);
        GameFreeThunk(req);
        return;
    }

    entries = gadget->layer->entries;
    drive[1] = 0;
    result = 0;

    if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "LOAD")
        || IsGadgetNamed(entries, gadget->hotGadgetIndex, "SWIN")) {
        char* name = SkipTextLines(req->names,
                                  FindGadgetChecked(entries, "SWIN")->u.list.selected);
        if (name[0] == '\\') {
            strcpy(req->selected, name);
            for (i = 0; i < 10; i++) {
                if (req->selected[i] == ' ') {
                    req->selected[i] = 0;
                    break;
                }
            }
            n = (int)strlen(req->cwd);
            char* tail = req->selected;
            if (n == 3) {
                tail++;
            }
            strcat(req->cwd, tail);
            ChangeDirectory(req->cwd);
        } else {
            result = 1;
            strcpy(req->selected, req->cwd);
            strcat(req->selected, "\\");
            strcat(req->selected, name);
        }
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "CANC")) {
        result = 1;
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "PATH")) {
        n = (int)strlen(req->cwd);
        if (n > 0) {
            while (n > 0) {
                if (req->cwd[n] == '\\') {
                    if (req->cwd[n - 1] == ':') {
                        req->cwd[n + 1] = 0;
                        ChangeDirectory(req->cwd);
                    } else {
                        req->cwd[n] = 0;
                        ChangeDirectory(req->cwd);
                    }
                    break;
                }
                n--;
            }
        }
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "NAME")) {
        gadget->hotGadgetIndex = FindGadgetIndex(entries, "LOAD", 14);
        n = FindGadgetIndex(entries, "NAME", 3);
        result = 1;
        strcpy(req->selected, entries[n].u.text);
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "ADRV")) {
        drive[0] = 'A';
        ChangeDrive(drive);
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "BDRV")) {
        drive[0] = 'B';
        ChangeDrive(drive);
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "CDRV")) {
        drive[0] = 'C';
        ChangeDrive(drive);
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "DDRV")) {
        drive[0] = 'D';
        ChangeDrive(drive);
    } else if (IsGadgetNamed(entries, gadget->hotGadgetIndex, "VDRV")) {
        drive[0] = 'R';
        ChangeDrive(drive);
    }

    if (result == 1) {
        if (req->callback) {
            req->callback(req);
        }
    } else {
        RefreshFileList(req);
        MarkChanged(gadget);
        ClearSelectedGadget(gadget);
    }
}

// FUNCTION: 0x4afa30
FileRequester* Gui::OpenFileRequester(Gui* self, char* arg2, char* arg3, char* arg4)
{
    void* gui = LoadGuiLayer(self, "FILEREQ.GUI", 0);
    if (gui == NULL) {
        return NULL;
    }

    FileRequester* obj = (FileRequester*)GameAllocIgnoreTag("FILE REQUESTER DATA", 0x24c);
    obj->gui = self;
    strcpy(obj->cwd, arg2);
    StripFileName(obj->cwd);

    if (strlen(obj->cwd) == 0) {
        strcpy(obj->cwd, "NO PATH");
    }

    ((void**)gui)[2] = (void*)FileRequesterHandler;
    ((void**)gui)[3] = obj;
    obj->callback = 0;

    Gadget* entries = (Gadget*)self->layer->entries;
    obj->nameGadget = (char*)FindGadgetChecked_B(entries, "NAME");
    obj->maskGadget = (char*)FindGadgetChecked_B(entries, "MASK");
    obj->pathGadget = (char*)FindGadgetOrNull(entries, "PATH");
    Gadget* titl = FindGadgetChecked_C(entries, "TITL");
    obj->slider = (ReqSub*)FindGadgetChecked_D(entries, "SLID");

    short none = -1;
    entries->x = none;
    entries->y = none;
    strcpy(titl->u.text, arg4);
    titl->x = none;

    obj->names = (char*)GameAllocIgnoreTag("FILE NAMES", 0x17700);
    memset(obj->names, -1, 0x17700);

    obj->sizes = (char*)GameAllocIgnoreTag("FILE SIZES", 0xea60);
    memset(obj->sizes, -1, 0xea60);

    obj->mask = (int)arg3;
    obj->selected = arg2;

    StripPath(arg2);

    strcpy(obj->nameGadget + 0xb6, arg2);
    strcpy(obj->maskGadget + 0xb6, arg3);

    GetCurrentDriveLetter(obj->save_drive);
    GetDriveDirectory(obj->save_drive, obj->save_cwd, 0x100);
    RefreshFileList(obj);

    MarkChanged(self);

    return obj;
}

// FUNCTION: 0x4afc60
void __stdcall AllocBlinkWords(Gui* obj, int count)
{
    if (obj->words) {
        GameFreeThunk(obj->words);
        obj->words = 0;
    }
    obj->words = (BlinkWord*)GameAllocIgnoreTag("BlinkWords", count * sizeof(BlinkWord));
    for (int i = 0; i < count; i++)
        obj->words[i].text[0] = 0;
    obj->count = count;
    obj->active = 1;
    obj->words[0].value = -1;
}

// FUNCTION: 0x4afcf0
void __stdcall FreeBlinkWords(Gui* param_1)
{
    GameFreeThunk(param_1->words);
    param_1->words = nullptr;
    param_1->active = 0;
}

// FUNCTION: 0x4afd20
void __stdcall SetBlinkGadget(Gui* obj, int value)
{
    obj->words->value = value;
}

// FUNCTION: 0x4afd40
void __stdcall EnableBlinkWords(Gui* param_1)
{
    BlinkWord* words = param_1->words;
    if (words != nullptr) {
        param_1->active = 1;
    }
}

// FUNCTION: 0x4afd60
void __stdcall DisableBlinkWords(Gui* param_1)
{
    if (param_1->words != nullptr) {
        param_1->active = 0;
    }
}

// FUNCTION: 0x4afd80
int __stdcall AddBlinkWord(Gui* obj, const char* text, int p2, int p3,
                           int p4, int p5, float f6, float f7)
{
    BlinkWord* p = obj->words;
    if (p == 0)
        return 0;

    int count = obj->count;
    int i = 0;
    for (i = 0; i < count; i++, p++) {
        if (p->text[0] == 0)
            break;
    }
    if (i == count)
        return 0;

    int time = GetTicks();

    strncpy(obj->words[i].text, text, 0x80);
    obj->words[i].x = p2;
    obj->words[i].y = p3;
    obj->words[i].colourA = p4;
    obj->words[i].colourB = p5;
    obj->words[i].periodA = f6;
    obj->words[i].periodB = f7;

    int rate = GetTickRate();
    obj->words[i].nextToggle = (float)rate * f6 + (float)time;
    obj->words[i].phase = 0;
    return 1;
}

// FUNCTION: 0x4afe90
void __stdcall ClearBlinkWord(Gui* param1, int param2)
{
    int max = param1->count;
    if (param2 <= max) {
        param1->words[param2].text[0] = 0;
    }
}

// FUNCTION: 0x4afec0
void __stdcall ClearBlinkWords(Gui* obj)
{
    if (obj->words != 0) {
        for (int i = 0; i < obj->count; i++) {
            obj->words[i].text[0] = 0;
        }
    }
}

// FUNCTION: 0x4aff00
void __stdcall DrawBlinkWords(Gui* obj)
{
    if (obj->active == 0)
        return;

    int time = GetTicks();

    if (obj->words->value != -1)
        SelectFontForEntry((Gadget*)obj->layer->entries,
                     obj->words->value);

    for (int i = 0; i < obj->count; i++) {
        if (obj->words[i].text[0] == 0)
            continue;

        float ft = (float)time;
        if (obj->words[i].nextToggle < ft) {
            if (obj->words[i].phase != 0) {
                obj->words[i].nextToggle =
                    (float)GetTickRate() * obj->words[i].periodA + ft;
                obj->words[i].phase = 0;
            } else {
                obj->words[i].nextToggle =
                    (float)GetTickRate() * obj->words[i].periodB + ft;
                obj->words[i].phase = 1;
            }
        }

        if (obj->words[i].phase != 0)
            SetTextColors(obj->words[i].colourB, GetTextKeyColor());
        else
            SetTextColors(obj->words[i].colourA, GetTextKeyColor());

        DrawString((void*)*(int*)((char*)obj->layer->entries + 0xbc),
                     obj->words[i].text, obj->words[i].x,
                     obj->words[i].y, -1);
    }
}
