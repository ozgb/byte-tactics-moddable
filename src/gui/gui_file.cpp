#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "win_types.h"

#include "../util/tdf.h"

#include "../files/file_handle.h"
#include "../state/sim_globals.h"

#include "../../link/globals.h"

#include "../files/files_api.h"
#include "dialogs.h"
#include "gui.h"
#include "../graphics/palette.h"
void* __stdcall LoadGaf(char* path);
void* __stdcall GetGafFrame(void* table, int index);
#include "../graphics/object_pool.h"

static inline void WriteTabsInline(FileHandle* file, int depth)
{
    char tab = '\t';
    for (int i = 0; i < depth; i++)
        HAPI_WriteFile(file, &tab, 1);
}

// FUNCTION: 0x4acd00
void __stdcall WriteSectionHeader(FileHandle* file, char* name, int depth)
{
    char line[100];
    sprintf(line, "[%s]", name);
    WriteTabsInline(file, depth - 1);
    HAPI_WriteFile(file, line, strlen(line));
    HAPI_WriteFile(file, (void*)"\n", 1);
    WriteTabsInline(file, depth);
    HAPI_WriteFile(file, (void*)"{\n", 2);
}

// FUNCTION: 0x4acda0
void __stdcall WriteSectionEnd(FileHandle* file, int depth)
{
    char tab = '\t';
    for (int i = 0; i < depth; i++)
        HAPI_WriteFile(file, &tab, 1);
    HAPI_WriteFile(file, (void*)"}\n", 2);
}

#pragma auto_inline(off)
// FUNCTION: 0x4acde0
void __stdcall WriteKeyValue(FileHandle* file, char* key, char* value, int depth)
{
    char tab = '\t';
    char line[100];
    for (int i = 0; i < depth; i++)
        HAPI_WriteFile(file, &tab, 1);
    sprintf(line, "%s=%s;\n", key, value);
    HAPI_WriteFile(file, line, strlen(line));
}
#pragma auto_inline(on)

#pragma pack(push, 1)
struct GuiFileRangeTail {
    short range;
    short maxchars;
    char pad0[0x13c - 0x13a];
    int thick;
    short knobpos;
    short knobsize;
    void* sliderCallback;
    char pad1[0x15b - 0x148];
};

struct GuiFileLinkTail {
    char link[0x11];
    char hotkey;
    char pad0[0x15b - 0x148];
};

union GuiFileTail {
    GuiFileRangeTail s34;
    GuiFileLinkTail s5;
};

struct GuiFileCommonFields {
    unsigned char id;
    unsigned char assoc;
    char name[0x11];
    short xpos;
    short ypos;
    short width;
    short height;
    int attribs;
    int colorf;
    int colorb;
    unsigned char texturenumber;
    unsigned char fontnumber;
    unsigned char active;
    unsigned char commonattribs;
    char unknown_2b[8];
    char help[0x81];
    unsigned short gaffile;
    char text[0x80];
    GuiFileTail tail;
};
#pragma pack(pop)

#pragma auto_inline(off)
// FUNCTION: 0x4ace50
void __stdcall WriteCommonFields(GuiFileCommonFields* obj, FileHandle* out, int indent)
{
    char tab;
    char* value;
    char line[100];
    char num[100];
    int i;

    value = _itoa(obj->id, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "id", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->assoc, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "assoc", value);
    HAPI_WriteFile(out, line, strlen(line));

    strncpy(num, obj->name, 16);
    num[16] = 0;
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "name", num);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->xpos, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "xpos", value);
    HAPI_WriteFile(out, line, strlen(line));

    if (obj->id == 0 && DAT_0051fba8 != 0 && obj->ypos >= 0)
        obj->ypos -= 0x1e0;

    value = _itoa(obj->ypos, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "ypos", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->width, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "width", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->height, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "height", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->attribs, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "attribs", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->colorf, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "colorf", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->colorb, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "colorb", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa((signed char)obj->texturenumber, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "texturenumber", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa((signed char)obj->fontnumber, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "fontnumber", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa((signed char)obj->active, num, 10);
    WriteTabs(out, indent);
    sprintf(line, "%s=%s;\n", "active", value);
    HAPI_WriteFile(out, line, strlen(line));

    WriteKeyValue(out, "commonattribs", _itoa((signed char)obj->commonattribs, num, 10), indent);
    WriteKeyValue(out, "help", obj->help, indent);
    WriteKeyValue(out, "gaffile", _itoa(obj->gaffile & 1, num, 10), indent);
}
#pragma auto_inline(on)

// FUNCTION: 0x4ad350
void __stdcall ReadCommonSection(GuiFileCommonFields* obj, TdfFile* tree)
{
    if (tree->SelectRecord("COMMON") == 1) {
        obj->id = (unsigned char)tree->current->GetFieldInt("id", 0);
        obj->assoc = (unsigned char)tree->current->GetFieldInt("assoc", 0);
        tree->current->GetFieldString(obj->name, "name", 0x10, DAT_005119b8);
        obj->xpos = (short)tree->current->GetFieldInt("xpos", 0);
        obj->ypos = (short)tree->current->GetFieldInt("ypos", 0);
        obj->width = (short)tree->current->GetFieldInt("width", 0);
        obj->height = (short)tree->current->GetFieldInt("height", 0);
        obj->attribs = tree->current->GetFieldInt("attribs", 0);
        obj->colorf = (unsigned short)tree->current->GetFieldInt("colorf", 0);
        obj->colorb = (unsigned short)tree->current->GetFieldInt("colorb", 0);
        obj->texturenumber = (unsigned char)tree->current->GetFieldInt("texturenumber", 0);
        obj->fontnumber = (unsigned char)tree->current->GetFieldInt("fontnumber", 0);
        obj->active = (unsigned char)tree->current->GetFieldInt("active", 0);
        obj->commonattribs = (unsigned char)tree->current->GetFieldInt("commonattribs", 0);
        tree->current->GetFieldString(obj->help, "help", 0x80, DAT_005119b8);
        memset(obj->help, 0, 0x81);
        strncpy(obj->help, Translate(obj->help), 0x80);
        int gf = tree->current->GetFieldInt("gaffile", 0);
        obj->gaffile = (gf ^ obj->gaffile) & 1 ^ obj->gaffile;
    }
}

struct WritePanelFieldsRecord {
    char unknown_0[0xb6];
    short totalgadgets;
    char unknown_0xb8[0x11];
    signed char major;
    signed char minor;
    signed char revision;
    char crdefault[16];
    char escdefault[16];
    char defaultfocus[16];
    char panel[16];
};

#pragma auto_inline(off)
// FUNCTION: 0x4ad4f0
void __stdcall WritePanelFields(WritePanelFieldsRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    char* value;
    int d = indent;
    int i;

    value = _itoa(obj->totalgadgets, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "totalgadgets", value);
    HAPI_WriteFile(out, line, strlen(line));

    d++;
    sprintf(line, "[%s]", "VERSION");
    tab = '\t';
    for (i = 0; i < d - 1; i++)
        HAPI_WriteFile(out, &tab, 1);
    HAPI_WriteFile(out, line, strlen(line));
    HAPI_WriteFile(out, (void*)"\n", 1);
    WriteTabs(out, d);
    HAPI_WriteFile(out, (void*)"{\n", 2);

    value = _itoa(obj->major, num, 10);
    tab = '\t';
    for (i = 0; i < d; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "major", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->minor, num, 10);
    tab = '\t';
    for (i = 0; i < d; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "minor", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->revision, num, 10);
    tab = '\t';
    for (i = 0; i < d; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "revision", value);
    HAPI_WriteFile(out, line, strlen(line));

    tab = '\t';
    for (i = 0; i < d; i++)
        HAPI_WriteFile(out, &tab, 1);
    HAPI_WriteFile(out, (void*)"}\n", 2);

    strncpy(num, obj->panel, 16);
    num[16] = 0;
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "panel", num);
    HAPI_WriteFile(out, line, strlen(line));

    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "crdefault", obj->crdefault);
    HAPI_WriteFile(out, line, strlen(line));
    WriteTabs(out, indent);

    sprintf(line, "%s=%s;\n", "escdefault", obj->escdefault);
    HAPI_WriteFile(out, line, strlen(line));

    WriteKeyValue(out, "defaultfocus", obj->defaultfocus, indent);
}
#pragma auto_inline(on)

#pragma pack(push, 1)
struct ReadPanelFieldsRecord {
    char unknown_0[0xb6];
    short totalGadgets;
    char unknown_b8[0x11];
    char major;
    char minor;
    char revision;
    char crdefault[0x10];
    char escdefault[0x10];
    char defaultfocus[0x10];
    char panel[0x10];
};
#pragma pack(pop)

// FUNCTION: 0x4ad890
void __stdcall ReadPanelFields(ReadPanelFieldsRecord* obj, TdfFile* src)
{
    obj->totalGadgets = (short)src->current->GetFieldInt("totalgadgets", 0);
    src->current->GetFieldString(obj->panel, "panel", 0x10, DAT_005119b8);
    src->current->GetFieldString(obj->crdefault, "crdefault", 0x10, DAT_005119b8);
    src->current->GetFieldString(obj->escdefault, "escdefault", 0x10, DAT_005119b8);
    src->current->GetFieldString(obj->defaultfocus, "defaultfocus", 0x10, DAT_005119b8);
    if (src->SelectRecord("VERSION") == 1) {
        obj->major = (char)src->current->GetFieldInt("major", 0);
        obj->minor = (char)src->current->GetFieldInt("minor", 0);
        obj->revision = (char)src->current->GetFieldInt("revision", 0);
    }
}

struct WriteStatusFieldRecord {
    char unknown_0[0xb6];
    short status;
};

// FUNCTION: 0x4ad980
void __stdcall WriteStatusField(WriteStatusFieldRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    char* value = _itoa(obj->status, num, 10);
    tab = '\t';
    for (int i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "status", value);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadStatusFieldRecord {
    char unknown_0[0xb6];
    short status;
};

struct ReadStatusFieldSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

// FUNCTION: 0x4ada10
void __stdcall ReadStatusField(ReadStatusFieldRecord* a, ReadStatusFieldSource* b)
{
    a->status = (short)b->tdf->GetFieldInt("status", 0);
}

#pragma pack(push, 2)
struct WriteButtonFieldsRecord {
    char unknown_0[0xb6];
    char text[0x80];
    unsigned char stages;
    short status;
    signed char quickkey;
    char frameBase;
    unsigned char grayedout;
};
#pragma pack(pop)

// FUNCTION: 0x4ada40
void __stdcall WriteButtonFields(WriteButtonFieldsRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    int i;
    char* value;

    value = _itoa(obj->status, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "status", value);
    HAPI_WriteFile(out, line, strlen(line));

    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "text", obj->text);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->quickkey, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "quickkey", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->grayedout & 1, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "grayedout", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->stages, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "stages", value);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadButtonFieldsSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

struct ReadButtonFieldsRecord {
    char unknown_0[0xb6];
    char text[0x80];
    unsigned char stages;
    char stageIndex;
    short status;
    unsigned char quickkey;
    char frameBase;
    unsigned short grayedout;
};

// FUNCTION: 0x4adc70
void __stdcall ReadButtonFields(ReadButtonFieldsRecord* obj, ReadButtonFieldsSource* src)
{
    obj->status = (short)src->tdf->GetFieldInt("status", 0);
    memset(obj->text, 0, 0x80);
    src->tdf->GetFieldString(obj->text, "text", 0x80, DAT_005119b8);
    strncpy(obj->text, Translate(obj->text), 0x80);
    char local[20];
    src->tdf->GetFieldString(local, "quickkey", 0x13, DAT_005119b8);
    if (isalpha((unsigned char)local[0]))
        obj->quickkey = (unsigned char)local[0];
    else
        obj->quickkey = (unsigned char)atoi(local);
    int gray = src->tdf->GetFieldInt("grayedout", 0);
    obj->grayedout = (gray ^ obj->grayedout) & 1 ^ obj->grayedout;
    obj->stages = (unsigned char)src->tdf->GetFieldInt("stages", 0);
}

struct WriteListBoxFieldsRecord {
    char unknown_0[0xda];
    short itemheight;
};

// FUNCTION: 0x4add60
void __stdcall WriteListBoxFields(WriteListBoxFieldsRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    char* value = _itoa(obj->itemheight, num, 10);
    tab = '\t';
    for (int i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "itemheight", value);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadListBoxFieldsSource {
    char unknown_0[4];
    TdfRecord* obj;
};

#pragma pack(push, 2)
struct ReadListBoxFieldsRecord {
    char unknown_0[0xce];
    int callback;
    char unknown_d2[4];
    int user;
    unsigned short itemheight;
};
#pragma pack(pop)

// FUNCTION: 0x4addf0
void __stdcall ReadListBoxFields(ReadListBoxFieldsRecord* param1, ReadListBoxFieldsSource* param2)
{
    param1->callback = 0;
    param1->user = 0;
    param1->itemheight = (unsigned short)param2->obj->GetFieldInt("itemheight", 0);
}

#pragma pack(push, 1)
struct WriteTextInputFieldsRecord {
    char unknown_0[0xb6];
    char text[0x82];
    short maxchars;
};
#pragma pack(pop)

// FUNCTION: 0x4ade20
void __stdcall WriteTextInputFields(WriteTextInputFieldsRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    int i;
    char* value = _itoa(obj->maxchars, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "maxchars", value);
    HAPI_WriteFile(out, line, strlen(line));
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "text", obj->text);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadTextInputFieldsSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

#pragma pack(push, 1)
struct ReadTextInputFieldsRecord {
    char unknown_0[0xb6];
    char text[0x82];
    short maxchars;
};
#pragma pack(pop)

// FUNCTION: 0x4adf10
void __stdcall ReadTextInputFields(ReadTextInputFieldsRecord* obj, ReadTextInputFieldsSource* src)
{
    obj->maxchars = src->tdf->GetFieldInt("maxchars", 0);
    if (obj->maxchars > 0x80)
        obj->maxchars = 0x80;
    src->tdf->GetFieldString(obj->text, "text", 0x80, DAT_005119b8);
    strcpy(obj->text, Translate(obj->text));
}

#pragma pack(push, 1)
struct SliderFieldsRecord {
    char unknown_0[0xb6];
    char text[0x80];
    short range;
    char unknown_138[4];
    int thick;
    short knobpos;
    short knobsize;
    void* sliderCallback;
};
#pragma pack(pop)

// FUNCTION: 0x4adf90
void __stdcall WriteSliderFields(SliderFieldsRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    int i;

    char* value = _itoa(obj->range, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "range", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->thick, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "thick", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->knobpos, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "knobpos", value);
    HAPI_WriteFile(out, line, strlen(line));

    value = _itoa(obj->knobsize, num, 10);
    tab = '\t';
    for (i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "knobsize", value);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadSliderFieldsSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

// FUNCTION: 0x4ae170
void __stdcall ReadSliderFields(SliderFieldsRecord* obj, ReadSliderFieldsSource* src)
{
    obj->range = src->tdf->GetFieldInt("range", 0);
    obj->thick = (short)src->tdf->GetFieldInt("thick", 0);
    obj->knobpos = src->tdf->GetFieldInt("knobpos", 0);
    obj->knobsize = src->tdf->GetFieldInt("knobsize", 0);
    obj->sliderCallback = 0;
    src->tdf->GetFieldString(obj->text, "text", 0x80, DAT_005119b8);
    strcpy(obj->text, Translate(obj->text));
}

struct WriteTextLinkFieldsRecord {
    char unknown_0[0xb6];
    char text[0x80];
    char link[0x80];
};

// FUNCTION: 0x4ae230
void __stdcall WriteTextLinkFields(WriteTextLinkFieldsRecord* obj, FileHandle* out, int indent)
{
    char line[100];
    WriteTabsInline(out, indent);
    sprintf(line, "%s=%s;\n", "text", obj->text);
    HAPI_WriteFile(out, line, strlen(line));
    WriteTabsInline(out, indent);
    sprintf(line, "%s=%s;\n", "link", obj->link);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadTextLinkFieldsSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

#pragma pack(push, 1)
struct ReadTextLinkFieldsRecord {
    char unknown_0[0xb6];
    char text[0x80];
    char link[0x11];
    char hotkey;
};
#pragma pack(pop)

// FUNCTION: 0x4ae300
void __stdcall ReadTextLinkFields(ReadTextLinkFieldsRecord* obj, ReadTextLinkFieldsSource* src)
{
    obj->hotkey = 0;
    obj->link[0] = 0;
    memset(obj->text, 0, sizeof(obj->text));
    src->tdf->GetFieldString(obj->text, "text", 0x80, DAT_005119b8);
    strncpy(obj->text, Translate(obj->text), 0x7f);
    src->tdf->GetFieldString(obj->link, "link", 0x10, DAT_005119b8);
}

struct WriteHotOrNotFieldRecord {
    char unknown_0[0xc8];
    unsigned int hotornot : 1;
};

// FUNCTION: 0x4ae380
void __stdcall WriteHotOrNotField(WriteHotOrNotFieldRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    char* value = _itoa(obj->hotornot, num, 10);
    tab = '\t';
    for (int i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "hotornot", value);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadHotOrNotFieldSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

struct ReadHotOrNotFieldRecord {
    char unknown_0[0xc8];
    unsigned int hotornot : 1;
};

// FUNCTION: 0x4ae410
void __stdcall ReadHotOrNotField(ReadHotOrNotFieldRecord* obj, ReadHotOrNotFieldSource* src)
{
    int value = src->tdf->GetFieldInt("hotornot", 0);
    obj->hotornot = value;
}

struct WriteFontFilenameFieldRecord {
    char unknown_0[0xb6];
    char text[0x80];
};

// FUNCTION: 0x4ae440
void __stdcall WriteFontFilenameField(WriteFontFilenameFieldRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    tab = '\t';
    for (int i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "filename", obj->text);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadFontFilenameFieldSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

#pragma pack(push, 1)
struct ReadFontFilenameFieldRecord {
    char unknown_0[0xb6];
    char text[0x80];
};
#pragma pack(pop)

// FUNCTION: 0x4ae4b0
void __stdcall ReadFontFilenameField(ReadFontFilenameFieldRecord* obj, ReadFontFilenameFieldSource* src)
{
    src->tdf->GetFieldString(obj->text, "filename", 0x20, DAT_005119b8);
}

struct WriteSurfFilenameFieldRecord {
    char unknown_0[0xb6];
    char text[0x80];
};

// FUNCTION: 0x4ae4e0
void __stdcall WriteSurfFilenameField(WriteSurfFilenameFieldRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    tab = '\t';
    for (int i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "filename", obj->text);
    HAPI_WriteFile(out, line, strlen(line));
}

struct ReadSurfFilenameFieldSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

#pragma pack(push, 1)
struct ReadSurfFilenameFieldRecord {
    char unknown_0[0xb6];
    char text[0x80];
};
#pragma pack(pop)

// FUNCTION: 0x4ae550
void __stdcall ReadSurfFilenameField(ReadSurfFilenameFieldRecord* obj, ReadSurfFilenameFieldSource* src)
{
    src->tdf->GetFieldString(obj->text, "filename", 0x20, DAT_005119b8);
}

#pragma pack(push, 1)
struct WriteNuttinFieldRecord {
    char unknown_0[0xb6];
    int nuttin;
};
#pragma pack(pop)

// FUNCTION: 0x4ae580
void __stdcall WriteNuttinField(WriteNuttinFieldRecord* obj, FileHandle* out, int indent)
{
    char tab;
    char line[100];
    char num[100];
    char* value = _itoa(obj->nuttin, num, 10);
    tab = '\t';
    for (int i = 0; i < indent; i++)
        HAPI_WriteFile(out, &tab, 1);
    sprintf(line, "%s=%s;\n", "nuttin", value);
    HAPI_WriteFile(out, line, strlen(line));
}

#pragma pack(push, 1)
struct ReadNuttinFieldRecord {
    char unknown_0[0xb6];
    int nuttin;
};
#pragma pack(pop)

struct ReadNuttinFieldSource {
    char unknown_0[4];
    TdfRecord* tdf;
};

// FUNCTION: 0x4ae610
void __stdcall ReadNuttinField(ReadNuttinFieldRecord* a, ReadNuttinFieldSource* b)
{
    a->nuttin = b->tdf->GetFieldInt("nuttin", 0);
}

// FUNCTION: 0x4ae630
void __stdcall WriteGuiFile(char* obj, char* name)
{
    int index;
    char button[100];
    char slider[100];
    char header[100];
    char common[100];
    char gadget[100];
    char path[256];
    char backup[256];
    char hot[100];
    char edit[100];
    char empty[100];
    char list[100];
    ChangeExtension(name, path, "GUI");
    if (HAPI_FileLengthByName(path)) {
        ChangeExtension(name, backup, "BGU");
        RemoveFile(backup);
        RenameFile(path, backup);
    }
    FileHandle* out = HAPI_CreateFile(path);
    char* p = obj;
    for (index = 0; index < *(short*)(obj + 0xb6) + 1; index++, p += 0x15b) {
        sprintf(gadget, "GADGET%d", index);
        sprintf(header, "[%s]", gadget);
        HAPI_WriteFile(out, header, strlen(header));
        HAPI_WriteFile(out, (void*)"\n", 1);
        WriteTabs(out, 1);
        HAPI_WriteFile(out, (void*)"{\n", 2);
        sprintf(common, "[%s]", "COMMON");
        {
            char t1 = '\t';
            for (int i = 0; i < 1; i++) HAPI_WriteFile(out, &t1, 1);
        }
        HAPI_WriteFile(out, common, strlen(common));
        HAPI_WriteFile(out, (void*)"\n", 1);
        WriteTabs(out, 2);
        HAPI_WriteFile(out, (void*)"{\n", 2);
        WriteCommonFields((GuiFileCommonFields*)p, out, 2);
        {
            int j = 2;
            char t2 = '\t';
            do { HAPI_WriteFile(out, &t2, 1); } while (--j);
        }
        HAPI_WriteFile(out, (void*)"}\n", 2);
        switch (*(unsigned char*)p) {
        case 0:
            WritePanelFields((WritePanelFieldsRecord*)p, out, 1);
            break;
        case 1:
            WriteKeyValue(out, "status", _itoa(*(short*)(p + 0x138), button, 10), 1);
            WriteKeyValue(out, "text", p + 0xb6, 1);
            WriteKeyValue(out, "quickkey", _itoa(*(signed char*)(p + 0x13a), button, 10), 1);
            WriteKeyValue(out, "grayedout", _itoa(*(unsigned char*)(p + 0x13c) & 1, button, 10), 1);
            WriteKeyValue(out, "stages", _itoa(*(unsigned char*)(p + 0x136), button, 10), 1);
            break;
        case 2:
            WriteKeyValue(out, "itemheight", _itoa(*(short*)(p + 0xda), list, 10), 1);
            break;
        case 3:
            WriteKeyValue(out, "maxchars", _itoa(*(short*)(p + 0x138), edit, 10), 1);
            WriteKeyValue(out, "text", p + 0xb6, 1);
            break;
        case 4:
            WriteKeyValue(out, "range", _itoa(*(short*)(p + 0x136), slider, 10), 1);
            WriteKeyValue(out, "thick", _itoa(*(int*)(p + 0x13c), slider, 10), 1);
            WriteKeyValue(out, "knobpos", _itoa(*(short*)(p + 0x140), slider, 10), 1);
            WriteKeyValue(out, "knobsize", _itoa(*(short*)(p + 0x142), slider, 10), 1);
            break;
        case 5:
            WriteKeyValue(out, "text", p + 0xb6, 1);
            WriteKeyValue(out, "link", p + 0x136, 1);
            break;
        case 6:
            WriteKeyValue(out, "hotornot", _itoa(*(unsigned int*)(p + 0xc8) & 1, hot, 10), 1);
            break;
        case 7:
            WriteKeyValue(out, "filename", p + 0xb6, 1);
            break;
        case 8:
            WriteKeyValue(out, "filename", p + 0xb6, 1);
            break;
        case 10:
            WriteKeyValue(out, "nuttin", _itoa(*(int*)(p + 0xb6), empty, 10), 1);
            break;
        }
        {
            char t3 = '\t';
            for (int i = 0; i < 1; i++) HAPI_WriteFile(out, &t3, 1);
        }
        HAPI_WriteFile(out, (void*)"}\n", 2);
    }
    HAPI_CloseFile(out);
}

#pragma pack(push, 1)
struct GuiFileListBoxBody {
    char pad0[0xce - 0xb6];
    int callback;
    char pad1[0xd6 - 0xce - 4];
    int user;
    short itemheight;
    char pad2[0x136 - 0xdc];
};

struct GuiFileHotOrNotBody {
    char pad0[0xc8 - 0xb6];
    unsigned int hotornot : 1;
    char pad1[0x136 - 0xcc];
};

union GuiFileBody {
    char text[0x80];
    int nuttin;
    short total;
    GuiFileListBoxBody s2;
    GuiFileHotOrNotBody s6;
};

struct GuiFileGadget {
    unsigned char type;
    char pad0[0xb6 - 0x001];
    GuiFileBody body;
    GuiFileTail tail;
};
#pragma pack(pop)

// FUNCTION: 0x4aeac0
int __stdcall ReadGuiFile(GuiFileGadget* obj, char* name)
{
    TdfFile parser;
    int i;
    int ret = 0;
    char path[256];
    ChangeExtension(name, path, "GUI");
    if ((&parser)->LoadFile(path) == 1) {
        ret = 1;
        i = 0;
        while (1) {
            (&parser)->ResetCurrentRecord();
            if (!(&parser)->SelectRecordAt(i))
                break;
            int cur = (&parser)->GetCurrentRecord();
            GuiFileGadget* e = obj + i;
            ReadCommonSection((GuiFileCommonFields*)e, &parser);
            (&parser)->SetCurrentRecord(cur);
            switch (e->type) {
            case 0:
                ReadPanelFields((ReadPanelFieldsRecord*)e, &parser);
                break;
            case 1:
                ReadButtonFields((ReadButtonFieldsRecord*)e, (ReadButtonFieldsSource*)&parser);
                break;
            case 2:
                e->body.s2.callback = 0;
                e->body.s2.user = 0;
                e->body.s2.itemheight = (short)parser.current->GetFieldInt("itemheight", 0);
                break;
            case 3:
                e->tail.s34.maxchars = (short)parser.current->GetFieldInt("maxchars", 0);
                if (e->tail.s34.maxchars > 0x80)
                    e->tail.s34.maxchars = 0x80;
                parser.current->GetFieldString(e->body.text, "text", 0x80, DAT_005119b8);
                strcpy(e->body.text, Translate(e->body.text));
                break;
            case 4:
                e->tail.s34.range = (short)parser.current->GetFieldInt("range", 0);
                e->tail.s34.thick = (short)parser.current->GetFieldInt("thick", 0);
                e->tail.s34.knobpos = (short)parser.current->GetFieldInt("knobpos", 0);
                e->tail.s34.knobsize = (short)parser.current->GetFieldInt("knobsize", 0);
                e->tail.s34.sliderCallback = 0;
                parser.current->GetFieldString(e->body.text, "text", 0x80, DAT_005119b8);
                strcpy(e->body.text, Translate(e->body.text));
                break;
            case 5:
                e->tail.s5.link[0] = 0;
                e->tail.s5.hotkey = 0;
                memset(e->body.text, 0, sizeof(e->body.text));
                parser.current->GetFieldString(e->body.text, "text", 0x80, DAT_005119b8);
                strncpy(e->body.text, Translate(e->body.text), 0x7f);
                parser.current->GetFieldString(e->tail.s5.link, "link", 0x10, DAT_005119b8);
                break;
            case 6:
                {
                    int value = parser.current->GetFieldInt("hotornot", 0);
                    e->body.s6.hotornot = value;
                }
                break;
            case 7:
                parser.current->GetFieldString(e->body.text, "filename", 0x20, DAT_005119b8);
                break;
            case 8:
                parser.current->GetFieldString(e->body.text, "filename", 0x20, DAT_005119b8);
                break;
            case 10:
                e->body.nuttin = parser.current->GetFieldInt("nuttin", 0);
                break;
            }
            i++;
        }
        obj->body.total = (short)(i - 1);
        (&parser)->Unload();
    }
    return ret;
}

// FUNCTION: 0x4aeda0
void __stdcall FreeGafSlot(Gui* obj, int i)
{
    if (obj->values[i] != 0) {
        GameFreeThunk(obj->values[i]);
        obj->values[i] = 0;
        obj->language = 0;
    }
}

struct LoadGafIntoSlotGafTable {
    unsigned short count;
    char unknown_2[0x28 - 0x2];
    void* entries[1];
};

// FUNCTION: 0x4aedd0
void __stdcall LoadGafIntoSlot(Gui* obj, const char* name, int index)
{
    int unknown;
    char path[256];
    strncpy(path, obj->str_ab6, 0x100);
    strcat(path, name);
    ChangeExtension(path, path, "GAF");
    if (HAPI_FileLengthByName(path)) {
        void* gaf = LoadGaf(path);
        obj->values[index] = gaf;
        LoadGafIntoSlotGafTable* table = *(LoadGafIntoSlotGafTable**)((char*)gaf + 0xc);
        char* e = (char*)GetGafFrame(table, 0x49);
        int d;
        if (e)
            d = *(unsigned short*)(e + 2);
        else
            d = unknown;
        for (int i = 0; i < table->count; i++) {
            char* f = (char*)GetGafFrame(table, i);
            if (f)
                *(short*)(f + 6) -= d;
        }
        obj->language = (FontGlyphTable*)obj->values[index];
    } else {
        obj->values[index] = 0;
        return;
    }
}

// FUNCTION: 0x4aeee0
void __stdcall LoadGafFile(Gui* obj, const char* name)
{
    char path[256];
    strncpy(path, obj->str_ab6, 0x100);
    strcat(path, name);
    ChangeExtension(path, path, "GAF");
    if (HAPI_FileLengthByName(path)) {
        obj->gaf = LoadGaf(path);
    }
}

// FUNCTION: 0x4aef80
void __stdcall FreeCommonGuiGaf(Gui* obj)
{
    if (obj->gaf) {
        GameFreeThunk(obj->gaf);
        obj->gaf = 0;
    }
}
