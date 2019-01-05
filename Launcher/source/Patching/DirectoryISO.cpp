#include "DirectoryISO.h"
#include "../Common.h"
#include <dirent.h>
#include <stdio.h>
#include <malloc.h>
#include "../FileHolder.h"

DirectoryISO::DirectoryISO(const char* basePath)
{
	_basePath = basePath;
}

DirectoryISO::~DirectoryISO()
{

}

int DirectoryISO::Size(const char* file)
{
	CATSTR(combined, _basePath, file);
	FileHolder f(combined,"rb");
	return f.Size();
}

int DirectoryISO::Read(const char* file, int offsetInFile, unsigned char* buffer, int bufLen)
{
	CATSTR(combined,_basePath,file)
	FileHolder f(combined, "rb");
	f.FSeek(offsetInFile, SEEK_SET);
	int size = f.Size();
	if (size < 0)return -1;
	int ret=f.FRead(buffer, (size - offsetInFile)>bufLen ? bufLen : (size - offsetInFile),1);
	f.FClose();
	return ret;
}

void* DirectoryISO::Malloc(size_t size)
{
	return malloc(size);
}