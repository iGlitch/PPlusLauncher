#ifndef _FILEHOLDER_H_
#define _FILEHOLDER_H_

#include <stdio.h>
#include <unistd.h>
#include "Common.h"

class FileHolder
{
public:
	FileHolder(const char* filename, const char* mode);
	~FileHolder();
	int Size();
	int FSeek(long offset, int origin);
	size_t FRead(void* buf, size_t size, size_t n);
	size_t FWrite(const void* buf, size_t size, size_t n);
	char* FGetS(char* row, int len);
	int FEOF();
	int FPrintF(const char* format, ...);
	int FFlush();
	int FSync();
	int FClose();
	bool IsOpen();
	FILE* get_Raw();
	const char* filename;
private:
	FILE* _fp;
	
};

#endif