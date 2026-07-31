#ifndef _ISOINTERFACE_H_
#define _ISOINTERFACE_H_

#include <stddef.h>

//Handles ISO I/O
class ISOInterface
{
public:
	virtual ~ISOInterface(){}
	virtual int Size(const char* file)=0;
	virtual int Read(const char* file,int offset,unsigned char* buffer,int bufLen)=0;
	virtual void* Malloc(size_t size);
};

#endif