#ifndef WEAPON_AIM_COB_CB_H
#define WEAPON_AIM_COB_CB_H

class WeaponAimCobCb {
public:
    void** ppVtbl;
    int nAimCobDone;

    void OnAimCobReturn(int enable);
};

#endif
