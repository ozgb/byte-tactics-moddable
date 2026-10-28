#ifndef FLAGS_00496CE0_H
#define FLAGS_00496CE0_H

#pragma pack(push, 1)
struct PreBattleFlags {
    unsigned short low : 4;
    unsigned short flag : 1;
    unsigned short rest : 11;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(PreBattleFlags) == 2, "Flags_00496ce0");
#endif

#endif
