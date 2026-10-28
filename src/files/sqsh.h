#ifndef SQSH_H
#define SQSH_H
struct SquashChunkHeader;

struct SquashChunkHeader;

int __cdecl LzssFreeWindow(void);
int __stdcall LzssFreeTree();
void __stdcall LzssContractNode(int oldNode, int newNode);
void __stdcall LzssReplaceNode(int oldNode, int newNode);
int __stdcall LzssFindNextNode(int index);
void __stdcall LzssDeleteString(int t);
int __stdcall LzssAddString(int pos, int* out);
int __stdcall LzssSetPreset(unsigned char* src, int len);
int __stdcall SquashGetPackType(unsigned char* header);
int __stdcall SquashUnpackedSize(unsigned char* header);
int __stdcall SquashEncrypt(char* param1, unsigned int param2);
int __stdcall SquashDecrypt(unsigned char* data, unsigned int size);
int __stdcall SquashChecksum(unsigned char* data, int size);
void __stdcall SquashDumpHeader(SquashChunkHeader* header);

int __stdcall SquashPack(SquashChunkHeader* chunk, int* chunkSize, char* data, int size, int method, int encrypt);
int __stdcall SquashUnpack(char* dest, char* data);
unsigned int __stdcall SquashMaxPackedSize(unsigned int value, int mode);
void* __stdcall SquashErrorString(int code);

#endif
