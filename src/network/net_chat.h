#ifndef NET_CHAT_H
#define NET_CHAT_H

int __stdcall ScrollToNextMessageUnit(void);

int __stdcall ExpireOldestMessage();

void __stdcall CycleMessageUnits(void);

#endif
