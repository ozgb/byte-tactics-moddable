#ifndef ONLINE_REPORT_H
#define ONLINE_REPORT_H

struct Net_4c97b0;

char* __stdcall GetEnumSessionsErrorText();
int __stdcall GetCreatedLobbyInterface(Net_4c97b0* net);
int __stdcall QueryOnlineFlags(int);
int __stdcall RIReportGameChat(int arg1, int arg2);
void __stdcall RISetCallbacks(int report, int notify);

#endif
