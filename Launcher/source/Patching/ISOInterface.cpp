#include "ISOInterface.h"
#include <malloc.h>

void* ISOInterface::Malloc(size_t size)
{
	return malloc(size);
}