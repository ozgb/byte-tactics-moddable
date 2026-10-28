#ifndef LOS_LINES_H
#define LOS_LINES_H

#include <vector>

class TdfFile;

struct LosStep {
    unsigned short a;
    unsigned short b;
};

struct LosLineData {
    std::vector<LosStep> v;
};

typedef std::vector<LosStep> LosStepVector;

class LosLine : public LosStepVector {
public:
    int GetLosLineStepCount();
    void GetLosLineStep(short index, unsigned short* out1, unsigned short* out2);
    void LoadLosLine(TdfFile* obj, short line, short mode);
};

class LosTable : public std::vector<LosLineData> {
public:
    LosLineData* GetLine(short i) { return &(*this)[i]; }
    void SetNumLines(short n) { resize(n * 4); }

    int GetLosLineCount();
    void ResizeLines(short n);
    LosLine* GetLosLine(short i);
    void FreeLines();
};

#endif
