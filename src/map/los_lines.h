#ifndef LOS_LINES_H
#define LOS_LINES_H

#include <vector>

class TdfFile;

struct Elem_00434020 {
    unsigned short a;
    unsigned short b;
};

struct Elem_00434360 {
    std::vector<Elem_00434020> v;
};

typedef std::vector<Elem_00434020> Vec_004336f0;

class LosLine : public Vec_004336f0 {
public:
    int GetLosLineStepCount();
    void GetLosLineStep(short index, unsigned short* out1, unsigned short* out2);
    void LoadLosLine(TdfFile* obj, short line, short mode);
};

class LosTable : public std::vector<Elem_00434360> {
public:
    Elem_00434360* GetLine(short i) { return &(*this)[i]; }
    void SetNumLines(short n) { resize(n * 4); }

    int GetLosLineCount();
    void ResizeLines(short n);
    LosLine* GetLosLine(short i);
    void FreeLines();
};

#endif
