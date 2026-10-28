#ifndef CHAT_RECIPIENTS_H
#define CHAT_RECIPIENTS_H

#pragma pack(push, 1)
struct ChatRecipients {
    int a;
    int b;
    short c;
    char d;
    void Clear()
    {
        a = 0;
        b = 0;
        c = 0;
        d = 0;
    }
};
#pragma pack(pop)

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
static_assert(sizeof(ChatRecipients) == 11, "ChatRecipients");
#endif

#endif
