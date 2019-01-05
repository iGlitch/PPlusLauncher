#ifndef _NULLISO_H_
#define _NULLISO_H_

#include "ISOInterface.h"

//ISO interface which returns nothing
class NullISO :public ISOInterface
{
public:
	int Size(const char* file);
	int Read(const char* file, int offsetInFile, unsigned char* buffer, int bufLen);
	void* Malloc(size_t size);
};

#endif