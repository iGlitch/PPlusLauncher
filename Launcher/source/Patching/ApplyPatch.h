#ifndef _APPLYPATCH_H_
#define _APPLYPATCH_H_

#include "ARC.h"
#include "7z\7ZipFile.h"
#include "bspatch.h"
#define MAX_RESOURCE_COUNT 100

#ifdef _WIN32
#pragma pack(1)
#endif
typedef struct
#ifdef HW_RVL
__attribute__((packed))
#endif
{
	char DiffName[16];//ENDSLEY/BSDIFF43
	unsigned char DecompSize[8];//size with all children decompressed.
	unsigned char CompSize[8];//size with all children compressed. this file itself isn't compressed.
	unsigned char FinalSize[8];//final size. if this file was initially compressed, compressed size.
	unsigned char ChildrenCount;//count of children
	unsigned char pad;
	unsigned char Compressed[MAX_RESOURCE_COUNT];//hard code. if there are files that contains more than 30 files it will crash.
	unsigned int Sizes[MAX_RESOURCE_COUNT];//expanded sizes of children
}PatchHeader;
#ifdef WIN32
#pragma pack()
#endif

typedef struct {
	SzFile* sZip;
	ArchiveFileStruct* str;
}PatchLoadSet;

bool ApplyPatchARC(bool usebsPatch, byte* sourceBuffer, int sourceLength, byte * patchBuffer, int patchLength, byte * outputBuffer, int outputLength);
bool ApplyPatchOther(bool usebsPatch, byte* sourceBuffer, int sourceLength, byte * patchBuffer, int patchLength, byte * outputBuffer, int outputLength);
int getOutputSizeARC(bool usebsPatch, u8 * patchBuffer);
int getOutputSizeOther(bool usebsPatch, u8 * patchBuffer);


#endif