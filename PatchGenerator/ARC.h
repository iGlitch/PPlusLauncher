#ifndef _ARC_H_
#define _ARC_H_

#ifdef HW_RVL
#include <c++/4.6.3/memory>
#include <c++/4.6.3/vector>
#include <gctypes.h>
#elif WIN32
#include <memory>
#include <vector>
#endif

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char byte;

class ARCResource{
public:
	ARCResource();
	//Load and parse data, then return if it succeeded
	bool Load(byte* data,int size);
	//Build data into buffer and return if it succeeded
	bool Build(byte* buf);
	//Replace the pointer which points data with given pointer
	void Replace(byte* data,int compsize,std::vector<bool> compressed,std::vector<int> expandedSize);
	//Loaded data
	byte* Data;
	//The size when the all nodes are decompressed(bsdiff uses this size)
	int Size;
	//The final size of this resource(this resource is compressed)
	int FinalCmpSize;
	//Indicates if the child at the index is compressed or not
	//I am aware of that the use of vector<bool> should usually be avoided
	std::vector<bool> Compressed;
	//Expanded sizes of children
	std::vector<int> ExpSize;
private:
	std::shared_ptr<byte> _buf; //for compressed
	bool _isComped;
};

#endif