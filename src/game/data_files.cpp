#include <stdio.h>
#include <string.h>

#include "../util/tdf.h"

#pragma pack(push, 1)
#include "game.h"
#include "anim_file_entry.h"
#include "../state/sim_globals.h"
#pragma pack(pop)

#include "../../link/globals.h"

#include "../debug/debug_lib.h"
void* __cdecl GameReallocTagged(void* block, const char* name, unsigned int size);
#include "../graphics/object_pool.h"
#include "../graphics/palette.h"
#include "main.h"
#include "../files/files_api.h"
#include "../graphics/display.h"
char* __stdcall Translate(char* text);
#include "../gui/dialogs.h"
#include "data_files.h"

// FUNCTION: 0x429000
void CheckGpfVersion()
{
    TdfFile parser;
    char buf[64];
    char path[256];
    int found = 0;

    BuildDataPath(path, "gamedata", "version", "tdf");
    if ((&parser)->LoadFile(path)) {
        if ((&parser)->SelectRecord("Version")) {
            if (parser.current->GetFieldString(buf, "GPFVersion", 0x40, DAT_005119b8)) {
                found = 1;
                if (_strcmpi("v3.0", buf) != 0) {
                    OpenMessageBox((Gui *)g_game->menu,
                                 Translate("Warning!  Your copy of Revision.GPF is the wrong version for this executable.  You may experience some problems if you continue playing.  Please download the latest version of the TA patch from www.cavedog.com and reinstall the patch."),
                                 0x1e0, 1, 1);
                }
            }
        }
        if (found == 0) {
            OpenMessageBox((Gui *)g_game->menu,
                         Translate("Warning!  Your copy of Revision.GPF is the wrong version for this executable.  You may experience some problems if you continue playing.  Please download the latest version of the TA patch from www.cavedog.com and reinstall the patch."),
                         0x1e0, 1, 1);
        }
    }
}

// FUNCTION: 0x4290f0
char* __stdcall BuildDataPath(char* buf, const char* dir, const char* name, const char* ext)
{
    char* side = (char*)GetPreferredLanguage();
    if (side) {
        sprintf(buf, "%s-%s\\%s", dir, side, name);
        if (ext != 0 && strlen(ext) != 0) {
            StripExtension(buf);
            strcat(buf, ".");
            strcat(buf, ext);
        }
        FileHandle* file = HAPI_OpenFileRead(buf);
        if (file) {
            HAPI_CloseFile(file);
            return buf;
        }
    }
    strcpy(buf, dir);
    strcat(buf, "\\");
    strcat(buf, name);
    if (ext != 0 && strlen(ext) != 0) {
        StripExtension(buf);
        strcat(buf, ".");
        strcat(buf, ext);
    }
    return buf;
}

void* __stdcall LoadPcx(char* path, int outPalette);

// FUNCTION: 0x429290
void* __stdcall LoadBitmapByName(const char* name, int palette)
{
    char path[256];
    BuildDataPath(path, "bitmaps", name, "PCX");
    void* bitmap = LoadPcx(path, palette);
    if (bitmap == 0) {
        FatalError(path);
    }
    return bitmap;
}


// FUNCTION: 0x4292e0
void* __stdcall LoadFontByName(const char* name)
{
    char path[256];
    BuildDataPath(path, "fonts", name, "FNT");
    void* font = HAPI_LoadFile(path, 0);
    if (font == 0) {
        FatalError(path);
    }
    return font;
}

#include "../graphics/pcx.h"

// FUNCTION: 0x429330
void* __stdcall LoadPaletteByName(char* name)
{
    char path[256];
    char namepath[256];
    void* result;

    BuildDataPath(namepath, "palettes", name, "PAL");
    if (HAPI_FileLengthByName(namepath) != 0) {
        result = HAPI_LoadFile(namepath, 0);
        int bad = (result == 0);
        if (bad) {
            FatalError(namepath);
            return result;
        }
    }
    else {
        result = GameAllocIgnoreTag("PALETTE", 0x400);
        BuildDataPath(path, "palettes", name, "PCX");
        if (LoadPcxPalette(path, (unsigned int *)result) == 0) {
            FatalError(path);
        }
        WriteBufferToFile(namepath, result, 0x400);
        BuildDataPath(path, "palettes", "PALETTE", "ALP");
        RemoveFile(path);
        BuildDataPath(path, "palettes", "PALETTE", "LHT");
        RemoveFile(path);
        BuildDataPath(path, "palettes", "PALETTE", "SHD");
        RemoveFile(path);
    }
    return result;
}

#include "../sound/sound.h"

// FUNCTION: 0x429470
int __stdcall LoadSoundByName(char* name, const char* file)
{
    int i;
    for (i = 0; i < g_game->soundCount; i++) {
        if (name != 0) {
            if (g_game->soundNames_33e13[i][0] != 0 &&
                _strnicmp(g_game->soundNames_33e13[i], name, 0x20) == 0)
                return i;
        } else {
            if (_strnicmp(g_game->soundFiles[i], file, 0x20) == 0)
                return i;
        }
    }
    if (g_game->soundCount + 1 >= 0x100)
        return 0;
    (*(void*(*)[256])&g_game->sounds)[g_game->soundCount] = LoadSoundFile(file);
    strncpy(g_game->soundFiles[g_game->soundCount], file, 0x20);
    if (name != 0)
        strncpy(g_game->soundNames_33e13[g_game->soundCount], name, 0x20);
    else
        strcpy(g_game->soundNames_33e13[g_game->soundCount], DAT_005119b8);
    g_game->soundCount++;
    return i;
}

#include "../graphics/gaf_frame.h"

struct RadarPicHeader {
    int w;
    int h;
    int unknown;
    char header[0x40];
};

#include "../graphics/gaf.h"

// FUNCTION: 0x4295b0
GafFrame* __stdcall LoadRadarPic(char* path, int* outX, int* outY)
{
    FileHandle* file = HAPI_OpenFileRead(path);
    if (file == 0) {
        return 0;
    }
    HAPI_FileLength(file);
    RadarPicHeader pic;
    HAPI_readfromfile(file, pic.header, 0x40);
    GafFrame* bmp;
    if (pic.header[0x2c] & 1) {
        HAPI_SeekFile(file, *(int*)(pic.header + 0x28));
        HAPI_readfromfile(file, &pic.w, 8);
        bmp = AllocFrame("RADARPIC", pic.w, pic.h);
        HAPI_readfromfile(file, bmp->pixelsOrLayers, pic.w * pic.h);
    } else {
        bmp = 0;
    }
    if (outX != 0 && outY != 0) {
        *outX = *(int*)(pic.header + 4);
        *outY = *(int*)(pic.header + 8);
    }
    HAPI_CloseFile(file);
    return bmp;
}

// FUNCTION: 0x429660
char* __stdcall LoadFileWithProgress(char* path)
{
    FileHandle* file = HAPI_OpenFileRead(path);
    if (file == 0) {
        FatalError(path);
    }
    int size = HAPI_FileLength(file);
    int chunk = size / 10;
    char* buf = (char*)GameAllocIgnoreTag(path, size);
    int done = 0;
    for (int progress = 9; progress <= 90; progress += 9) {
        done += HAPI_readfromfile(file, buf + done, chunk);
        g_game->loadProgress = progress;
    }
    if (done < size) {
        HAPI_readfromfile(file, buf + done, size - done);
    }
    HAPI_CloseFile(file);
    return buf;
}

void* __stdcall LoadGaf(char* path);

// FUNCTION: 0x429700
void* __stdcall LoadAnimGaf(char* name)
{
    char path[256];
    int i;
    for (i = 0; i < g_game->count_147ab; i++) {
        if (_strcmpi(((AnimFileEntry*)g_game->entries_147af)[i].name, name) == 0) {
            return ((AnimFileEntry*)g_game->entries_147af)[i].gaf;
        }
    }
    BuildDataPath(path, "anims", name, "GAF");
    void* gaf = LoadGaf(path);
    if (gaf == 0) {
        FatalError(path);
    }
    g_game->entries_147af = (AnimFileEntry*)((AnimFileEntry*)GameReallocTagged(((AnimFileEntry*)g_game->entries_147af), "Animation Files",
                                             (g_game->count_147ab + 1) * 0x44));
    strcpy(((AnimFileEntry*)g_game->entries_147af)[g_game->count_147ab].name, name);
    ((AnimFileEntry*)g_game->entries_147af)[g_game->count_147ab].gaf = gaf;
    g_game->count_147ab++;
    return gaf;
}

void* __stdcall FindGafEntry(void* gaf, const char* name);

// FUNCTION: 0x429850
void __stdcall FindGafSequence(int gaf, char* name)
{
    FindGafEntry((void*)gaf, name);
}

struct FrameFlagView {
    char unknown_0[2];
    char flag;
};

#define FLAG(p) (((FrameFlagView*)(p))->flag = 0)

// FUNCTION: 0x429870
void LoadGameResources()
{
    char* gaf;

    g_game->fxGaf = (int)LoadAnimGaf("fx");
    gaf = (char*)g_game->fxGaf;
    g_game->smoke1 = (void*)((int)FindGafEntry(gaf, "smoke 1"));
    g_game->smoke2 = (void*)((int)FindGafEntry(gaf, "smoke 2"));
    g_game->fire1 = (int)FindGafEntry(gaf, "fire1");
    g_game->alfboom1 = (int)FindGafEntry(gaf, "alfboom1");
    FLAG(g_game->alfboom1);
    g_game->radlogo = (void*)((int)FindGafEntry(gaf, "radlogo"));
    g_game->radlogohigh = (void*)((int)FindGafEntry(gaf, "radlogohigh"));
    g_game->nuclogo = (void*)((int)FindGafEntry(gaf, "nuclogo"));
    g_game->src1 = (void*)((int)FindGafEntry(gaf, "h2oboom2"));
    FLAG(((int)g_game->src1));
    g_game->src2 = (void*)((int)FindGafEntry(gaf, "lavasplash"));
    FLAG(((int)g_game->src2));
    g_game->gaf_147bb = (void*)((int)FindGafEntry(gaf, "cannonshell"));
    g_game->gaf_147bf = (void*)((int)FindGafEntry(gaf, "plasmasm"));
    g_game->gaf_147c3 = (void*)((int)FindGafEntry(gaf, "plasmamd"));
    g_game->gaf_147c7 = (void*)((int)FindGafEntry(gaf, "ultrashell"));
    g_game->gaf_147cb = (void*)((int)FindGafEntry(gaf, "plasmasm"));
    g_game->flamestreamGaf = (void*)((int)FindGafEntry(gaf, "flamestream"));
    g_game->src3 = (void*)((int)FindGafEntry(gaf, "explosion"));
    FLAG(((int)g_game->src3));
    g_game->explode2 = (int)FindGafEntry(gaf, "explode2");
    FLAG(g_game->explode2);
    g_game->explode3 = (int)FindGafEntry(gaf, "explode3");
    FLAG(g_game->explode3);
    g_game->explode4 = (int)FindGafEntry(gaf, "explode4");
    FLAG(g_game->explode4);
    g_game->explode5 = (int)FindGafEntry(gaf, "explode5");
    FLAG(g_game->explode5);
    g_game->nuke1 = (int)FindGafEntry(gaf, "nuke1");
    FLAG(g_game->nuke1);
    g_game->gaf_1480f = (void*)((int)FindGafEntry(gaf, "shadow"));

    g_game->igTitles = (int)LoadAnimGaf("igtitles");
    gaf = (char*)g_game->igTitles;
    g_game->image_14813 = (unsigned short*)((int)FindGafEntry(gaf, "igvictory"));
    g_game->image_14817 = (unsigned short*)((int)FindGafEntry(gaf, "igdefeat"));
    g_game->igpaused = (int)FindGafEntry(gaf, "igpaused");

    g_game->vismasks = (int)LoadAnimGaf("vismasks");
    gaf = (char*)g_game->vismasks;
    g_game->losTable = (FrameTable*)((int)FindGafEntry(gaf, "vismask"));

    g_game->fog = (int)LoadAnimGaf("fog");
    gaf = (char*)g_game->fog;
    g_game->black1Seq = (int)FindGafEntry(gaf, "Black1");
    g_game->black2 = (int)FindGafEntry(gaf, "Black2");
    g_game->black3 = (int)FindGafEntry(gaf, "Black3");
    g_game->black4 = (int)FindGafEntry(gaf, "Black4");
    g_game->gray1Seq = (int)FindGafEntry(gaf, "Gray1");
    g_game->gray2 = (int)FindGafEntry(gaf, "Gray2");
    g_game->gray3 = (int)FindGafEntry(gaf, "Gray3");
    g_game->gray4 = (int)FindGafEntry(gaf, "Gray4");

    g_game->cursorsGaf = (int)LoadAnimGaf("cursors");
    gaf = (char*)g_game->cursorsGaf;
    g_game->cursorAttack = (int)FindGafEntry(gaf, "cursorattack");
    g_game->cursorAirstrike = (int)FindGafEntry(gaf, "cursorairstrike");
    g_game->cursorTooFar = (int)FindGafEntry(gaf, "cursortoofar");
    g_game->cursorCapture = (int)FindGafEntry(gaf, "cursorcapture");
    g_game->cursorDefend = (int)FindGafEntry(gaf, "cursordefend");
    g_game->cursorRepair = (int)FindGafEntry(gaf, "cursorrepair");
    g_game->cursorPatrol = (int)FindGafEntry(gaf, "cursorpatrol");
    g_game->cursorPickup = (int)FindGafEntry(gaf, "cursorpickup");
    g_game->cursorTeleport = (int)FindGafEntry(gaf, "cursorteleport");
    g_game->cursorReclamate = (int)FindGafEntry(gaf, "cursorreclamate");
    g_game->cursorLoad = (int)FindGafEntry(gaf, "cursorload");
    g_game->cursorUnload = (int)FindGafEntry(gaf, "cursorunload");
    g_game->cursorMove = (int)FindGafEntry(gaf, "cursormove");
    g_game->cursorSelect = (int)FindGafEntry(gaf, "cursorselect");
    g_game->cursorFindSite = (int)FindGafEntry(gaf, "cursorfindsite");
    g_game->cursorRed = (int)FindGafEntry(gaf, "cursorred");
    g_game->cursorGrn = (int)FindGafEntry(gaf, "cursorgrn");
    g_game->cursorNormal = (unsigned short*)((int)FindGafEntry(gaf, "cursornormal"));
    g_game->cursorHourglass = (int)FindGafEntry(gaf, "cursorhourglass");
    g_game->pathIcon = (int)FindGafEntry(gaf, "pathicon");
    g_game->cursorRevive = (int)FindGafEntry(gaf, "cursorrevive");

    TdfFile parser;
    char buf[256];

    BuildDataPath(buf, "gamedata", "sidedata", "TDF");
    (&parser)->LoadFile(buf);
    sprintf(buf, "GENERAL");
    (&parser)->ResetCurrentRecord();
    if ((&parser)->SelectRecord(buf) == 1) {
        g_game->baseHeight = parser.current->GetFieldInt("baseheight", 0x1e0);
    } else {
        g_game->baseHeight = 0x1e0;
    }

    int i = 0;
    while (1) {
        sprintf(buf, "SIDE%d", i);
        (&parser)->ResetCurrentRecord();
        if (!(&parser)->SelectRecord(buf))
            break;
        int intgaf = parser.current->GetFieldString(buf, "intgaf", 0x1e, DAT_005119b8);
        if (intgaf != 0) {
            char* side = (char*)LoadAnimGaf(buf);
            if (side) {
                g_game->panelTop[i] = (int)side;
                (*(int(*)[5])&g_game->sidePanelTopSeq)[i] = (int)FindGafEntry(side, "PANELTOP");
                (*(int(*)[5])&g_game->sidePanelSideSeq)[i] = (int)FindGafEntry(side, "PANELSIDE");
                (*(int(*)[5])&g_game->sidePanelBotSeq)[i] = (int)FindGafEntry(side, "PANELBOT");
            }
        }
        i++;
    }
    (&parser)->Unload();
}

// FUNCTION: 0x42a010
void FreeAnimFiles()
{
    int i;
    for (i = 0; i < g_game->count_147ab; i++) {
        GameFreeThunk(((AnimFileEntry*)g_game->entries_147af)[i].gaf);
    }
    GameFreeThunk(((AnimFileEntry*)g_game->entries_147af));
    g_game->count_147ab = 0;
    g_game->entries_147af = (AnimFileEntry*)(0);
    for (i = 0; i < g_game->sides; i++) {
        g_game->panelTop[i] = 0;
    }
    g_game->gafAnimClearedSlot = g_game->fxGaf = g_game->igTitles =
        g_game->vismasks = g_game->fog = g_game->cursorsGaf = 0;
}

// FUNCTION: 0x42a0e0
void __stdcall AddAnimplayPointer(void* item)
{
    g_game->data_148eb = (int*)((void**)GameReallocTagged(((void**)g_game->data_148eb), "Animplay Pointers", (g_game->count_148e7 + 1) * 4));
    ((void**)g_game->data_148eb)[g_game->count_148e7] = item;
    g_game->count_148e7++;
}

struct GafSequenceInitRef {
    unsigned short index;
    unsigned short value;
    unsigned char kind;
    char unknown_5[3];
    void* src;
};

struct ModelTextureEntry {
    int type;
    char unknown_4[0xc];
    void* name;
    char unknown_14[8];
    int flags;
};

struct TexturedModel {
    char unknown_0[8];
    int count;
    char unknown_c[0x1c];
    ModelTextureEntry* entries;
    TexturedModel* child1;
    TexturedModel* child2;
};

void __stdcall InitGafSequence(GafSequenceInitRef* ref, void* src, int index);

// FUNCTION: 0x42a140
void __stdcall BindModelTextures(TexturedModel* model, const char* name)
{
    if (model->child1)
        BindModelTextures(model->child1, name);
    if (model->child2)
        BindModelTextures(model->child2, name);

    ModelTextureEntry* elem = model->entries;
    for (int i = 0; i < model->count; i++, elem++) {
        if (elem->name) {
            unsigned short* entry;
            elem->flags &= ~4;
            for (int j = 0; j < g_game->count_148df; j++) {
                entry = (unsigned short*)FindGafEntry(g_game->blocks[j], (const char*)elem->name);
                if (entry)
                    break;
            }
            if (entry == 0) {
                entry = (unsigned short*)FindGafEntry(g_game->logos, (const char*)elem->name);
                if (entry != 0 && *entry == 10)
                    elem->flags |= 4;
                if (entry == 0) {
                    elem->flags |= 1;
                    elem->type = 0xd1;
                    continue;
                }
            }
            if (*entry > 1) {
                InitGafSequence((GafSequenceInitRef*)&elem->name, entry, 0);
                elem->flags |= 2;
                if (!(elem->flags & 4)) {
                    g_game->data_148eb = (int*)((void**)GameReallocTagged(((void**)g_game->data_148eb), "Animplay Pointers", (g_game->count_148e7 + 1) * 4));
                    ((void**)g_game->data_148eb)[g_game->count_148e7] = &elem->name;
                    g_game->count_148e7++;
                }
                continue;
            }
            elem->name = *(void**)((char*)entry + 0x28);
            elem->flags &= ~2;
        }
    }
}

#include "../graphics/pcx.h"
void __stdcall MirrorObject(void* data);

// FUNCTION: 0x42a2c0
void* __stdcall LoadObject3d(const char* name)
{
    char path[256];
    BuildDataPath(path, "objects3d", name, "3DO");
    void* data = Load3do(path);
    if (data == 0) {
        FatalError(path);
    }
    MirrorObject(data);
    BindModelTextures((TexturedModel*)data, name);
    return data;
}

// FUNCTION: 0x42a320
void LoadGameFonts()
{
    g_game->fontComix = (int)(LoadFontByName("COMIX"));
    g_game->fontSmlFont = LoadFontByName("smlfont");
}

// FUNCTION: 0x42a3b0
void FreeGameFonts()
{
    GameFreeThunk(g_game->fontSmlFont);
    g_game->fontSmlFont = 0;
    GameFreeThunk(((void*)g_game->fontComix));
    g_game->fontComix = (int)(0);
}

// FUNCTION: 0x42a400
int LoadDefaultPalette()
{
    void* palette = LoadPaletteByName("PALETTE");
    memcpy(g_game->paletteRgba, palette, 0x400);
    GameFreeThunk(palette);
    return 1;
}

struct TextureFindData {
    char unknown_0[0x14];
    char name[260];
};


// FUNCTION: 0x42a440
void LoadTextureGafs()
{
    char path[256];
    TextureFindData fd;

    BuildDataPath(path, "textures", "*", "GAF");
    int count = CountDirectoryEntries(path, 0);
    g_game->count_148df = count - 1;
    void** texturePtrs = (void**)GameAllocIgnoreTag("TEXTURE PTRS", count * 4);
    g_game->blocks = texturePtrs;
    int handle = HAPI_FindFirst(path, &fd, -1, 1);
    if (handle != -1) {
        int progress = 0;
        int r;
        do {
            if (_strcmpi(fd.name, "logos.GAF") == 0) {
                texturePtrs--;
            } else {
                BuildDataPath(path, "textures", fd.name, "GAF");
                *texturePtrs = LoadGaf(path);
                g_game->progress = (char)(progress / (count - 1));
            }
            r = HAPI_FindNext(handle, &fd);
            texturePtrs++;
            progress += 100;
        } while (r == 0);
        g_game->progress = 100;
        HAPI_FindClose(handle);
    }
    g_game->count_148e7 = 0;
    g_game->data_148eb = (int*)(0);
}

// FUNCTION: 0x42a570
void FreeTextureGafs()
{
    for (int i = 0; i < g_game->count_148df; i++) {
        GameFreeThunk(g_game->blocks[i]);
        g_game->blocks[i] = 0;
    }
    GameFreeThunk(g_game->blocks);
    g_game->blocks = 0;
    if (((void**)g_game->data_148eb)) {
        GameFreeThunk(((void**)g_game->data_148eb));
        g_game->data_148eb = (int*)(0);
        g_game->count_148e7 = 0;
    }
}

#include <vector>

#include "../units/unit_def.h"

#include "../util/string_ref.h"

#include "../util/math.h"
#include "../debug/debug_lib.h"

// FUNCTION: 0x42a610
void __stdcall ComputeUnitScriptChecksum(UnitDef* def)
{
    if (def->scriptChecksum)
        return;

    ProtectBlockReadWrite(((void*)g_game->defs));

    char path[256];
    int size;
    BuildDataPath(path, "scripts", def->unitname, "cob");
    void* data = HAPI_LoadFile(path, &size);
    if (data) {
        def->scriptChecksum ^= ComputeChecksum((unsigned char*)data, size);
        GameFreeThunk(data);
    }

    std::vector<StringRef> files;
    char name[64];
    strcpy(name, def->unitname);
    strcat(name, "*");

    BuildDataPath(path, "guis", name, "gui");
    ListDirectory(path, 0, &files);

    for (unsigned int i = 0; i < files.size(); i++) {
        BuildDataPath(path, "guis", files[i].p, "gui");
        void* data2 = HAPI_LoadFile(path, &size);
        if (data2) {
            def->scriptChecksum ^= ComputeChecksum((unsigned char*)data2, size);
            GameFreeThunk(data2);
        }
    }

    BuildDataPath(path, "download", def->unitname, "tdf");
    FileHandle* f = HAPI_OpenFileRead(path);
    if (f) {
        size = HAPI_FileLength(f);
        if (size > 0) {
            void* data3 = HAPI_LoadOpenFile(path, f, 0);
            if (data3) {
                def->scriptChecksum ^= ComputeChecksum((unsigned char*)data3, size);
                GameFreeThunk(data3);
            }
        }
        HAPI_CloseFile(f);
    }

    def->scriptChecksum ^= def->weaponHashXor;
    ProtectBlockReadOnly(((void*)g_game->defs));
}

// FUNCTION: 0x42a870 ??_ETdfFile@@QAEPAXI@Z
static TdfFile* s_array = new TdfFile[1];
