#ifndef FILE_HANDLE_H
#define FILE_HANDLE_H

struct OPENHAPIFILE;
struct Info;

struct FileHandle {
    void* fp;
    OPENHAPIFILE* shared;
    Info* info;
    unsigned int pos;
    int* buffer;
    unsigned char* buffer2;
    char name[0x100];

    void SetFileName(const char* text);
};

#endif
