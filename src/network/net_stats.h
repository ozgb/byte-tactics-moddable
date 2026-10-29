#ifndef NET_STATS_H
#define NET_STATS_H

struct MessageByteCounter {
    int a;
    int b;
};

void __stdcall RegisterVtolOrders();
void __stdcall StepAllGafSequences(void);
void __stdcall ResetNetStats();
void __stdcall CountMessage(unsigned char kind, int amount, int player);
void __stdcall CountPacket(int size, int overhead, int sent);
void __stdcall GetByteRates(unsigned int* sent, unsigned int* received);
void __stdcall SumMessageStats();

void __stdcall FormatNetStats(char* text);

#endif
