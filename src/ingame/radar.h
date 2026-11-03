#ifndef RADAR_H
#define RADAR_H
struct GafFrame;

void __stdcall FreeRadar();
struct RadarMappedCapture;
struct RadarDrawCapture;

void UpdateRadarMapped(const RadarMappedCapture* capture);
void DrawRadarUnits(const RadarDrawCapture* capture);

void __stdcall ResizeRadarPicture(GafFrame* pic, int x, int y, int w, int h);
void __stdcall CalcRadarViewportRect(int*);

void __stdcall DrawRadar(void*);

#endif
