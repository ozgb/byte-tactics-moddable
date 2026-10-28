#include "../../interfaces/ref.h"
#include "../gui/mouse_event.h"

#pragma pack(push, 1)
struct StartPositionEntry {
    int type;
    int id;
    short x;
    short z;
};

class StartPositionNetView {
public:
    char unknown_0[0xdb4];
    Ref<StartPositionEntry> entries;
    int entry_count;
};

#include "../game/game.h"
#pragma pack(pop)

#include "../files/files_api.h"

// GLOBAL: 0x511de8
extern Game* g_game;

#include "../../link/globals.h"


#include "control_panel.h"

static inline void SetPos(int x, int y)
{
    g_game->scrollX = x;
    g_game->scrollY = y;
}

// FUNCTION: 0x41d0f0
void CenterCameraOnRadarClick()
{
    MouseEvent r = (*(MouseEvent*)&g_game->view);
    int y = g_game->mapHeight * (r.y - g_game->posY) / g_game->screen_h - g_game->viewHeight / 2;
    int x = g_game->mapWidth * (r.x - g_game->posX) / g_game->width_142eb - g_game->viewWidth / 2;
    g_game->scrollX = x;
    g_game->scrollY = y;
    g_game->viewDirtyFlags_142f1 |= 2;
    ClampCameraPosition();
    g_game->x2 = g_game->scrollX;
    g_game->y2 = g_game->scrollY;
    g_game->mapFlags &= 0xfff7;
    g_game->value_1434b = 0;
    g_game->value_142f3 = 0;
    (*(int*)&g_game->tracked) = 0;
}

// FUNCTION: 0x41d1f0
void CenterCameraOnStartPosition()
{
    int i = 0;
    StartPositionEntry* e = ((StartPositionNetView *)g_game->net.get())->entries;
    int count = ((StartPositionNetView *)g_game->net.get())->entry_count;
    for (; i < count; i++, e++) {
        if (e->type == 1 && e->id == 0) {
            SetPos(e->x - g_game->viewWidth / 2, e->z - g_game->viewHeight / 2);
            g_game->viewDirtyFlags_142f1 |= 2;
            ClampCameraPosition();
            g_game->x2 = g_game->scrollX;
            g_game->y2 = g_game->scrollY;
            g_game->mapFlags &= 0xfff7;
            return;
        }
    }
}

// FUNCTION: 0x41d2b0
void __stdcall ReadCameraPosition(HapiBank* file)
{
    file->OpenAccount(g_cameraAccount);
    int z = file->GetIntegerItem(g_cameraZPosition, g_game->scrollY);
    int x = file->GetIntegerItem(g_cameraXPosition, g_game->scrollX);
    g_game->scrollX = x;
    g_game->scrollY = z;
    g_game->viewDirtyFlags_142f1 |= 2;
    ClampCameraPosition();
    g_game->x2 = g_game->scrollX;
    g_game->y2 = g_game->scrollY;
    g_game->mapFlags &= 0xfff7;
}

// FUNCTION: 0x41d360
void __stdcall WriteCameraPosition(HapiBank* file)
{
    file->OpenAccount(g_cameraAccount);
    file->SetIntegerItem(g_cameraXPosition, g_game->scrollX);
    file->SetIntegerItem(g_cameraZPosition, g_game->scrollY);
}

// FUNCTION: 0x41d3b0
void __stdcall SaveCameraPosition(int slot)
{
    g_game->xs[slot] = g_game->scrollX;
    g_game->ys[slot] = g_game->scrollY;
    g_game->valid[slot] = 1;
}

// FUNCTION: 0x41d3f0
void __stdcall RestoreCameraPosition(int index)
{
    if (g_game->valid != 0) {
        g_game->value_1434b = 0;
        g_game->value_142f3 = 0;
        (*(int*)&g_game->tracked) = 0;
        SetPos(g_game->xs[index], g_game->ys[index]);
        g_game->viewDirtyFlags_142f1 |= 2;
        ClampCameraPosition();
        g_game->x2 = g_game->scrollX;
        g_game->y2 = g_game->scrollY;
        g_game->mapFlags &= 0xfff7;
    }
}

// FUNCTION: 0x41d4a0
void __stdcall SetCommandLineUnusedFlagL(int val)
{
    g_commandLineUnusedFlagL = val;
}

// FUNCTION: 0x41d4b0
void __stdcall SetBypassDriveScan(int val)
{
    g_cdBypassDriveScan = val;
}
