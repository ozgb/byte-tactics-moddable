#include "weapon_aim_cob_cb.h"

// FUNCTION: 0x481490
void WeaponAimCobCb::OnAimCobReturn(int enable)
{
    if (enable) {
        nAimCobDone = 1;
    }
}

// FUNCTION: 0x4814b0
int __stdcall AimCobStub(int, int, int, int)
{
    return 0;
}
