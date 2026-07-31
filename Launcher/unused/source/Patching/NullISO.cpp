#include "NullISO.h"

int NullISO::Size(const char* file)
{
	return -1;
}

int NullISO::Read(const char* file, int offsetInFile, unsigned char* buffer, int bufLen)
{
	return -1;
}

void* NullISO::Malloc(size_t size)
{
	return NULL;
}