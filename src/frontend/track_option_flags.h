#ifndef TRACK_OPTION_FLAGS_H
#define TRACK_OPTION_FLAGS_H

#pragma pack(push, 1)
union TrackOptionFlags {
    char notrak;
    unsigned short word;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(TrackOptionFlags) == 2, "TrackOptionFlags");
#endif

#endif
