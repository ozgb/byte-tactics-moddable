struct FileHandle;

#include "data_files.h"
#include "../files/files_api.h"

// FUNCTION: 0x4352d0
int __stdcall CampaignFileExists(char* name)
{
    char path[256];
    BuildDataPath(path, "camps", name, "TDF");
    FileHandle* file = HAPI_OpenFileAppend(path);
    if (file) {
        HAPI_CloseFile(file);
        return 1;
    }
    return 0;
}
