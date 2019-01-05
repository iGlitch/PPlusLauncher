#ifndef _DVDISO_H_
#define _DVDISO_H_

#include <gctypes.h>
#include "ISOInterface.h"

typedef struct
{
	u8 Type;
	u8 NameOffset[3];
	u32 Offset;
	u32 Len;
}FileEntry;

class DVDISO: public ISOInterface
{
public:
	DVDISO();
	~DVDISO();
	int Size(const char* file);
	int Read(const char* file,int offsetInFile,unsigned char* buffer,int bufLen);
	virtual void* Malloc(size_t size);
	int DVDInitCallback(uint32_t status, uint32_t error);
	bool IsReady();
private:
	FileEntry Search(const char* file);
	bool _ready;
	char _region;
};
#endif