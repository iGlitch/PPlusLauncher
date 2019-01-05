#ifndef _DIRECTORYISO_H_
#define _DIRECTORYISO_H_

#include <gctypes.h>
#include "ISOInterface.h"

//ISO interface for already extracted files
//Used during update patch
class DirectoryISO : public ISOInterface
{
public:
	DirectoryISO(const char* basePath);
	~DirectoryISO();
	int Size(const char* file);
	int Read(const char* file, int offsetInFile, unsigned char* buffer, int bufLen);
	void* Malloc(size_t size);
private:
	const char* _basePath;
};
#endif