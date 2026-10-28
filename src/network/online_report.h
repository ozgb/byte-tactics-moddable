#ifndef ONLINE_REPORT_H
#define ONLINE_REPORT_H

struct NetReportBlockView;

char* __stdcall GetEnumSessionsErrorText();
int __stdcall GetCreatedLobbyInterface(NetReportBlockView* net);
int __stdcall QueryOnlineFlags(int);
int __stdcall RIReportGameChat(int arg1, int arg2);
void __stdcall RISetCallbacks(int report, int notify);

#endif
