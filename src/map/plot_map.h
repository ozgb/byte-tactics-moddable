#ifndef PLOT_MAP_H
#define PLOT_MAP_H

class HapiBank;

void __stdcall SaveMetalPlotmap(HapiBank* file);
void __stdcall SavePlayerFeaturesPlotmap(HapiBank* file);
void __stdcall LoadMetalPlotmap(HapiBank* file);
void __stdcall LoadPlayerFeaturesPlotmap(HapiBank* file);
void __stdcall SaveMappingData(HapiBank* file);
void __stdcall LoadMappingData(HapiBank* file);

#endif
