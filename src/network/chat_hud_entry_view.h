#ifndef CHAT_HUD_ENTRY_VIEW_H
#define CHAT_HUD_ENTRY_VIEW_H

#pragma pack(push, 1)
struct ChatHudEntryView {
    char text[64];
    unsigned int time;
    unsigned short field_44;
    char field_46;
    unsigned char flags;
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ChatHudEntryView) == 72, "ChatHudEntryView");
#endif

#endif
