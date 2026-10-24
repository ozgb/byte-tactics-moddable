#ifndef CAMERA_H
#define CAMERA_H
class HapiBank;

class HapiBank;

void __stdcall CenterCameraOnRadarClick();
void __stdcall CenterCameraOnStartPosition();
void __stdcall ReadCameraPosition(HapiBank* file);
void __stdcall SaveCameraPosition(int slot);
void __stdcall RestoreCameraPosition(int index);
void __stdcall SetCommandLineUnusedFlagL(int val);
void __stdcall SetBypassDriveScan(int val);

void __stdcall WriteCameraPosition(HapiBank* file);

#endif
