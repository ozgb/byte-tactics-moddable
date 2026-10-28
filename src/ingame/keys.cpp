#include "win_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#pragma pack(push, 1)

struct Layer {
    int unknown_0;
    int value;
};

typedef Layer Struct_004958c0;

class Mission;

#include "../gui/gui.h"

struct Flags_00495e90_37f2f {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short rest : 14;
};

#include "../network/player_info.h"

#include "../network/player.h"

struct FindData_00495930 {
    char unknown_0[0x14];
    char name[260];
};

struct BmpWriter {
    char unknown_0[0xc];
    void* file;
    BmpWriter* Init();
    bool Open(const char* name, int width, int height);
    bool WriteRows(void* image, int x, int rows, int unused_4, int y, int unused_6, int srcY);
    void Close();
};

int RIReport(int, int, int, int, int, int, int, int, int, int);
void WalkFrameChain(int*, int*, int, int, int*, int, int*, int*, int, int*);

#include "../graphics/gaf_frame.h"

struct Surface;

struct SrcHolder_00495a30 {
    char unknown_0[0xbc];
    Surface* frame;
};

#include "../graphics/surface.h"

#include "../game/game.h"

#pragma pack(pop)

#include "../map/mission.h"

#include "../orders/mission_type.h"
#include "../state/sim_globals.h"

#include "../../link/globals.h"

int __stdcall FindGadgetIndexBySubstring(int value, const char* name);
#include "../files/files_api.h"
#include "keys.h"
#include "../graphics/gaf.h"
#include "../debug/debug_lib.h"
#include "../game/main.h"
void* __stdcall GetDisplay();
void __stdcall FrameFromSurface(GafFrame* dst, Surface* src);
#include "control_panel.h"
#include "selection.h"
#include "info_panel.h"
void __stdcall DrawFrame(Surface* surf, GafFrame* pal, int x, int y);
#include "../map/line_of_sight.h"
#include "../graphics/object_pool.h"
#include "../gui/keyboard.h"
#include "../gui/keyboard.h"
#include "../game/game_state.h"
#include "../gui/dialogs.h"
#include "../game/settings.h"
void __stdcall PlaySoundByName(const char* name, int param);
#include "game_dialogs.h"
void __stdcall ExecuteCommandLine(int commandLine, int mask);
void __stdcall CollectSelectedUnits(void* param);
#include "camera.h"
#include "../network/net_chat.h"
#include "../network/packets.h"
void __stdcall SetDescListCleanupFlag(Gui* gui, int param);
#include "../network/net_game.h"
void __stdcall BroadcastPacket(int id, void* packet, int size);
#include "../frontend/options.h"
#include "../graphics/pcx.h"
void __stdcall IssueOrderToSelection(void* a, int b, MissionType kind, int d, int e, int f);
int __stdcall FindOrderByType(int unit, MissionType kind);
void __stdcall DeleteOrder(int unit, int arg);
void __cdecl operator delete(void* p);

// FUNCTION: 0x495860
void SelectStopOrder(void)
{
    int index;

    g_game->orderMode = 1;
    g_game->inputFlags &= 0xdf;
    index = FindGadgetIndexBySubstring(g_game->gui.layer->value, "STOP");
    if (index != -1) {
        ClearGroupStatus(&g_game->gui, index);
    }
}

// FUNCTION: 0x4958c0
void __stdcall SetOrSelectStopOrder(int set)
{
    int index;

    if (set) {
        g_game->inputFlags |= 0x20;
        return;
    }
    g_game->orderMode = 1;
    g_game->inputFlags &= 0xdf;
    index = FindGadgetIndexBySubstring(g_game->gui.layer->value, "STOP");
    if (index != -1) {
        ClearGroupStatus(&g_game->gui, index);
    }
}

// FUNCTION: 0x495930
void __stdcall BuildScreenshotPath(char* out, const char* dir, const char* name, const char* ext)
{
    bool needSep = false;
    FindData_00495930 fd;
    int max = 0;

    if (*dir != 0) {
        if (dir[strlen(dir) - 1] != '\\') {
            needSep = true;
        }
    }
    sprintf(out, "%s%s%s*.%s", dir, needSep ? g_pathSepBackslash : DAT_005119b8, name, ext);
    int handle = HAPI_FindFirst(out, &fd, -1, 1);
    if (handle >= 0) {
        do {
            int val = atoi(fd.name + strlen(name));
            if (val > max) {
                max = val;
            }
        } while (HAPI_FindNext(handle, &fd) == 0);
        HAPI_FindClose(handle);
    }
    sprintf(out, "%s%s%s%04i.%s", dir, needSep ? g_pathSepBackslash : DAT_005119b8, name, max + 1, ext);
}

// FUNCTION: 0x495a30
void __stdcall WriteScreenshot(char* dir, char* name, int x, int y, int w, int h)
{
    int var24;
    unsigned int savedC;
    unsigned short fl;
    char filename[260];
    int bw, off27, sy;
    BuildScreenshotPath(filename, dir, name, "bmp");

    int y2;
    BmpWriter bmp;
    bmp.Init();
    if (bmp.Open(filename, w, h)) {
        GafFrame* bm;
        off27 = g_game->viewCullMinX;
        int bh, off2b;
        bw = g_game->screenTilesX * 16;
        off2b = g_game->viewCullMinY;
        bh = (g_game->screenTilesY * 16) - 1;

        SetOutOfMemoryHandler(0);
        bm = AllocFrame("ScreenShot", w, bh);
        if (bm == 0) {
            bh /= 2;
            bm = AllocFrame("ScreenShot", w, bh);
        }
        InstallOutOfMemoryHandler();
        if (bm != 0) {
            int scrollX;
            Surface surf;
            scrollX = g_game->scrollX;
            GafFrame pal;
            fl = g_game->mapFlags;
            int savedbit0, scrollY = g_game->scrollY, bit6;
            int savedbit1;
            savedbit0 = fl & 1;
            savedbit1 = (fl >> 1) & 1;

            int savedA;
            g_game->mapFlags = fl & ~1;
            g_game->mapFlags &= ~2;
            RecalculateLineOfSight(1);
            savedA = g_game->otherFlags & 1;
            g_game->otherFlags = (unsigned short)(g_game->otherFlags & ~1);
            int savedB = (g_game->uiOptionFlags >> 6) & 1;

            g_game->uiOptionFlags = (unsigned short)(g_game->uiOptionFlags & ~0x40);
            savedC = g_game->textLines;

            g_game->textLines = 0;
            SurfaceFromFrame(&surf, bm);
            FrameFromSurface(&pal, ((SrcHolder_00495a30*)GetDisplay())->frame);

            pal.transparency = 0;
            int row = 0;

            if (h > row) {
                int var1c, negbh;
                var24 = bh;
                unsigned int var10 = 0;
                negbh = -bh;
                var1c = -1;
                while (1) {
                    ClearFrame(bm, 0);
                    int col, rows;
                    col = 0;
                    if (col < w) do {
                        int right;
                        Rect box;
                        box.left = col;
                        SetCameraPosition(x + col, y + row, 0);
                        CollectVisibleUnitIds();
                        DrawBattleFrame(1, 0);
                        right = (col + bw) - 1;
                        if (right >= surf.width) right = surf.width - 1;
                        box.top = 0;
                        box.right = right;
                        box.bottom = bh - 1;
                        surf.SetClipRect(box);
                        DrawFrame(&surf, &pal, g_game->scrollX - x - off27,
                                     g_game->scrollY - row - y - off2b);
                    } while (((col += bw), (col < w)));
                    sy = var10;
                    int srcY = 0;
                    rows = bh;
                    y2 = row;
                    if (var1c < -1) {
                        y2 = row + 1;
                        sy = var1c;
                        rows = bh - 1;
                        srcY = 1;
                    }
                    if (y2 + rows > h) rows = h + sy;
                    if (!bmp.WriteRows(&surf, w, rows, 0, row, 0, srcY)) break;
                    if (var24 < h) {
                        row = row - 1;
                        var10++;
                        var1c = var1c + 1;
                        var24 = var24 - 1;
                    }
                    row += bh;
                    var10 += negbh;
                    var1c = var1c + negbh;
                    var24 += bh;
                    if (row >= h) break;
                }
            }
            GameFreeThunk(bm);
            g_game->otherFlags = (unsigned short)(g_game->otherFlags ^ ((savedA ^ g_game->otherFlags) & 1));
            bit6 = (savedB & 1) << 6;
            g_game->uiOptionFlags = (unsigned short)((((unsigned short)g_game->uiOptionFlags) & ~0x40) | bit6);
            g_game->textLines = savedC;
            SetCameraPosition(scrollX, scrollY, 0);
            g_game->mapFlags = (unsigned short)(g_game->mapFlags ^ ((savedbit0 ^ g_game->mapFlags) & 1));
            g_game->mapFlags = (unsigned short)((unsigned short)((g_game->mapFlags & ~2) | ((savedbit1 & 1) << 1)));
            RecalculateLineOfSight(1);
            CollectVisibleUnitIds();
            DrawBattleFrame(1, 1);
        }
    }
    bmp.Close();
}

// FUNCTION: 0x495e90
void HandleGameKey(void)
{
    int event = PopKey();
    if (event == 0)
        return;

    int key = IsKeyDown(0xf9);

    switch (event) {
    case 0x1b:
        if (g_game->ordersPanelFlags_37ebe.b0) {
            g_game->ordersPanelFlags_37ebe.b0 = 0;
            int r = IsScreenNamed(&g_game->gui, g_game->name);
            if (r == 0) {
                g_game->unitIndex = 0;
                CloseTopScreen(&g_game->gui);
            }
        } else {
            if (g_game->orderMode != 1) {
                g_game->orderMode = 1;
                g_game->inputFlags = g_game->inputFlags & 0xdf;
                int handle = FindGadgetIndexBySubstring(g_game->gui.layer->value, "STOP");
                if (handle != -1)
                    ClearGroupStatus(&g_game->gui, handle);
            } else {
                ClearSelection();
                PopUntilNamedLayout(1);
            }
        }
        break;

    case 0xc5:
    case 0xc6:
    case 0xc7:
    case 0xc8:
    case 0xc9:
    case 0xca:
    case 0xcb:
    case 0xcc:
    case 0xcd:
        CreateSquad(event - 0xc4);
        PlaySoundByName("CreateSquad", 0);
        break;

    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        if (g_game->flags_37f06.b8) {
            if (IsKeyDown(0xfb) != 0) {
                OpenBuildMenuPage(event - 0x31);
            } else {
                SelectSquad(event - 0x30, key);
                PlaySoundByName("SelectSquad", 0);
            }
        } else {
            if (IsKeyDown(0xfb) != 0) {
                SelectSquad(event - 0x30, key);
                PlaySoundByName("SelectSquad", 0);
            } else
                OpenBuildMenuPage(event - 0x31);
        }
        break;

    case 0xd2:
    case 0xd3:
    case 0xd4:
    case 0xd5:
        PlaySoundByName("SelectSquad", 0);
        SaveCameraPosition(event - 0xd2);
        break;

    case 0xe6:
    case 0xe7:
    case 0xe8:
    case 0xe9:
        PlaySoundByName("SelectSquad", 0);
        RestoreCameraPosition(event - 0xe6);
        break;

    case 0x21:
    case 0x23:
    case 0x2a:
    case 0x60:
    case 0x7e:
        g_game->flags_37f06.b0 = !g_game->flags_37f06.b0;
        SaveSettings();
        break;

    case 0x2c:
        StepBuildMenuPageBack(1);
        break;

    case 0x2e:
        StepBuildMenuPage(1);
        break;

    case 0xf8: {
        unsigned char data[4];
        data[0] = 0x19;
        data[1] = 0;
        g_game->flags_38a51.b0 = !g_game->flags_38a51.b0;
        data[2] = (unsigned char)(g_game->flags_38a51.b0);
        BroadcastPacket(GetLocalDpid(), data, 3);
        break;
    }

    case 0xe2:
        if (key != 0) {
            if (g_game->hoverUnitId != 0) {
                g_game->unitInfoHoverTrack = 1;
                g_game->unitInfoHoverUnitId = g_game->hoverUnitId;
            } else {
                g_game->unitInfoHoverTrack = 0;
            }
        } else {
            OpenUnitInfoDialog();
        }
        break;

    case 9:
        if (g_game->net->GetGameType() == 3) {
            if (!g_game->ordersPanelFlags_37ebe.b2)
                ToggleTabMenu();
            break;
        }
    case 0xe3:
        if (key != 0) {
            if (g_game->hoverUnitId != 0) {
                g_game->unitBuilderHoverTrack = 1;
                g_game->unitBuilderHoverUnitId = g_game->hoverUnitId;
            } else {
                g_game->unitBuilderHoverTrack = 0;
            }
        } else {
            if (!g_game->ordersPanelFlags_37ebe.b0) {
                OpenInGameOptions();
                g_game->ordersPanelFlags_37ebe.b0 = 1;
            }
        }
        break;

    case 0xe4:
        CycleMessageUnits();
        break;

    case 0xd7: {
        if ((*(Flags_00495e90_37f2f*)&g_game->uiOptionFlags_37f2f).b1) {
            if (g_game->movieCaptureIndex != 0) {
                g_game->movieCaptureIndex = 0;
            } else {
                g_game->movieCaptureIndex = 0;
                char path[0x100];
                char findData[0x118];
                sprintf(path, "%s\\MOVIE*", g_game->movieCaptureDir);
                int h = HAPI_FindFirst(path, findData, -1, 1);
                if (h >= 0) {
                    do {
                        int n = atoi(&findData[0x19]);
                        if (n > g_game->movieCaptureIndex)
                            g_game->movieCaptureIndex = n;
                    } while (HAPI_FindNext(h, findData) == 0);
                    HAPI_FindClose(h);
                }
                g_game->movieCaptureIndex++;
                sprintf(g_game->text_38b53, "%s\\MOVIE%03i",
                        g_game->movieCaptureDir, g_game->movieCaptureIndex);
                MakeDirectoryPath(g_game->text_38b53);
                DrawBattleFrame(0, 1);
                SaveScreenshot(g_game->text_38b53, "FRAM");
                (*(int*)&g_game->nextMovieFrameTick) = g_game->ticks;
            }
        }
        break;
    }

    case 0xec:
        if ((*(Flags_00495e90_37f2f*)&g_game->uiOptionFlags_37f2f).b1) {
            g_game->endGameFlags_3923b.b1 = !g_game->endGameFlags_3923b.b1;
            if (g_game->endGameFlags_3923b.b1) {
                SetDescListCleanupFlag(&g_game->gui, 0);
            } else {
                g_game->endGameFlags_3923b.b0 = 0;
                g_game->debugMode = 0;
                SetDescListCleanupFlag(&g_game->gui, 1);
            }
        }
        break;

    case 0x5c:
        if ((*(Flags_00495e90_37f2f*)&g_game->uiOptionFlags_37f2f).b1)
            ExecuteCommandLine(0, -1);
        break;

    case 0xe5:
        g_game->flags_37f06.b7 = !g_game->flags_37f06.b7;
        break;

    case 0xed:
        ResetChatHudIndices();
        break;

    case 0xaa:
        SelectAllIdleUnits();
        break;

    case 0xab:
    case 0xae:
    case 0xaf:
    case 0xb0:
    case 0xb1:
    case 0xb2:
    case 0xb3:
    case 0xb4:
    case 0xb5:
    case 0xb6:
    case 0xb7:
    case 0xb8:
    case 0xb9:
    case 0xba:
    case 0xbb:
    case 0xbd:
    case 0xbe:
    case 0xbf:
    case 0xc0:
    case 0xc1:
    case 0xc2: {
        char buf[7];
        sprintf(buf, "CTRL_%c", event - 0x69);
        SelectUnitsByCategory(buf, key);
        break;
    }

    case 0xac:
        SelectUnitsByCategory("CTRL_C", key);
        FindLocalCommander();
        break;

    case 0xc3:
        SelectUnitsOfSameTypes();
        break;

    case 0xad: {
        std::vector<int> sel;
        CollectSelectedUnits(&sel);
        int found = 0;
        MissionType order("SELFDESTRUCT");
        for (std::vector<int>::iterator it = sel.begin(); it != sel.end(); ++it) {
            int r = FindOrderByType(*it, order);
            if (r != 0) {
                found = 1;
                DeleteOrder(*it, r);
            }
        }
        if (found == 0)
            IssueOrderToSelection(g_game->orders_2c76, 0, order, 0, 0, 0);
        break;
    }

    case 0x68:
        if (g_game->net->GetGameType() == 3)
            OpenShareDialog();
        break;

    case 0x6e:
        FocusNextLocalUnit();
        break;

    case 0xbc:
        SelectAllVisibleUnits();
        break;

    case 0x74:
        CycleCameraFollow(0);
        break;

    case 0x54:
        CycleCameraFollow(1);
        break;

    case 0xd:
        PlaySoundByName("SmallButton", 0);
        OpenTalkDialog();
        break;

    case 0x2d:
    case 0x5f:
        if (!(g_game->endGameFlags_3923b.raw & 2)) {
            Player* pl = &g_game->players[g_game->localPlayer];
            if (pl->active != 0 && (pl->info->flags_9b & 0x40) != 0)
                break;
            if (g_game->gameSpeed <= 1)
                break;
            SetGameSpeed(g_game->gameSpeed - 1, 1);
        }
        break;

    case 0x2b:
    case 0x3d:
        if (!(g_game->endGameFlags_3923b.raw & 2)) {
            Player* pl = &g_game->players[g_game->localPlayer];
            if (pl->active != 0 && (pl->info->flags_9b & 0x40) != 0)
                break;
            if (g_game->gameSpeed >= 0x14)
                break;
            SetGameSpeed(g_game->gameSpeed + 1, 1);
        }
        break;

    default:
        break;
    }

    if (g_game->endGameFlags_3923b.b1)
        HandleDebugHotkey(event);
}
