#include <map>

struct CobScriptHeader {
    int unknown_0;
    int count_1;
    int count_2;
    int unknown_c;
    int checksum;
    int count_3;
    int offset_18;
    int offset_1c;
    int offset_20;
    int offset_24;
    int offset_28;
};

struct CobScriptOffsetEntry {
    int unknown_0;
    int offset;
};

#include "../files/files_api.h"
#include "math.h"
#include "../graphics/object_pool.h"

// FUNCTION: 0x4b2290 _$E6
// FUNCTION: 0x4b2340 _$E4
static std::map<int, int> s_cobScriptCache;

static void Relocate(CobScriptHeader* data)
{
    data->offset_18 += (int)data;
    data->offset_1c += (int)data;
    for (int i = 0; i < data->count_1; i++)
        ((int*)data->offset_1c)[i] += (int)data;
    data->offset_20 += (int)data;
    for (int j = 0; j < data->count_2; j++)
        ((int*)data->offset_20)[j] += (int)data;
    data->offset_24 += (int)data;
    data->offset_28 += (int)data;
    for (int k = 0; k < data->count_3; k++)
        ((CobScriptOffsetEntry*)data->offset_28)[k].offset += (int)data;
}

// FUNCTION: 0x4b2450
CobScriptHeader* __stdcall LoadCobScript(char* name)
{
    CobScriptHeader* data = (CobScriptHeader*)HAPI_LoadFile(name, 0);
    if (data == 0)
        return 0;
    int sum = ComputeChecksum((unsigned char*)data, HAPI_FileLengthByName(name));
    s_cobScriptCache[(int)data] = sum;
    Relocate(data);
    return data;
}

// FUNCTION: 0x4b2540
void __stdcall FreeCobScript(int key)
{
    if (key != 0) {
        int local_key = key;
        s_cobScriptCache.erase(local_key);
        GameFreeThunk((void*)key);
    }
}

// FUNCTION: 0x4b26f0
int __stdcall GetCobChecksum(int key)
{
    return s_cobScriptCache[key];
}

// FUNCTION: 0x4b2840 ?begin@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAE?AViterator@12@XZ
// FUNCTION: 0x4b2850 ?insert@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAE?AU?$pair@Viterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@_N@2@ABU?$pair@HH@2@@Z
// FUNCTION: 0x4b2ac0 ?erase@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAE?AViterator@12@V312@@Z
// FUNCTION: 0x4b2fb0 ?_Erase@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x4b3000 ??0?$pair@Viterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@_N@std@@QAE@ABViterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@1@AB_N@Z
// FUNCTION: 0x4b3020 ?_Insert@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAE?AViterator@12@PAU_Node@12@0ABU?$pair@HH@2@@Z
// FUNCTION: 0x4b3310 ?_Lrotate@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x4b3370 ?_Min@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@KGPAU_Node@12@PAU312@@Z
// FUNCTION: 0x4b33b0 ?_Rrotate@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x4b3410 ?_Buynode@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEPAU_Node@12@PAU312@W4_Redbl@12@@Z
// FUNCTION: 0x4b3430 ?_Lbound@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IBEPAU_Node@12@ABH@Z
// FUNCTION: 0x4b3490 ?_Ubound@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IBEPAU_Node@12@ABH@Z
// FUNCTION: 0x4b34f0 ?_Dec@iterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAEXXZ
// FUNCTION: 0x4b3590 ?_Inc@iterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAEXXZ
