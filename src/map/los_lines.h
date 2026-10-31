#ifndef LOS_LINES_H
#define LOS_LINES_H

#include "../state/block_vector.h"

class TdfFile;

struct LosStep {
    unsigned short a;
    unsigned short b;
};

class LosLine : public BlockVector<LosStep> {
public:
    int GetLosLineStepCount();
    void GetLosLineStep(short index, unsigned short* out1, unsigned short* out2);
    void LoadLosLine(TdfFile* obj, short line, short mode);
};

typedef LosLine LosLineData;

class LosTable : public BlockVector<LosLine> {
public:
    LosLine* GetLine(short i) { return &(*this)[i]; }
    void SetNumLines(short n) { Resize(n * 4); }

    int GetLosLineCount();
    void ResizeLines(short n);
    LosLine* GetLosLine(short i);
    void FreeLines();
};

#endif
