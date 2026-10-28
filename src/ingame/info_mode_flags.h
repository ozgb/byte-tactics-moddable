#ifndef INFO_MODE_FLAGS_H
#define INFO_MODE_FLAGS_H

#pragma pack(push, 1)
union InfoModeFlags {
    unsigned short raw;
    struct {
        unsigned short b0 : 1;
        unsigned short b1 : 1;
        unsigned short rest : 14;
    };
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(InfoModeFlags) == 2, "InfoModeFlags");
#endif

#endif
