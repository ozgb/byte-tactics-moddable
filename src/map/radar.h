#ifndef RADAR_H
#define RADAR_H
struct GafFrame;

void __stdcall FreeRadar();
void __stdcall UpdateRadarMapped();
void __stdcall DrawRadarUnits(void);

void __stdcall ResizeRadarPicture(GafFrame* pic, int x, int y, int w, int h);
void __stdcall CalcRadarViewportRect(int*);

void __stdcall DrawRadar(void*);

#endif
