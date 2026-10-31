#ifndef STATISTICS_H
#define STATISTICS_H

struct DpspGuid {
    unsigned long data1;
    unsigned long data2;
    unsigned long data3;
    unsigned long data4;
};

struct OnlineReportPlayer {
    char* name;
    int id;
    int flags;
    char* side;
    int numAllies;
    OnlineReportPlayer** allies;
};

struct OnlineReportScore {
    char* name;
    int value;
};

struct OnlineReportScoreBoard {
    int score;
    int count;
    OnlineReportScore** scores;
};

int __stdcall AllocScoreTables();
bool __stdcall IsReporterDllLoaded();

int __stdcall FillScoreTables();

#endif
