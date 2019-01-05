#include "stdafx.h"
#include "ARC.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Common.h"
#ifdef HW_RVL
#include <c++\4.6.3\memory>
#elif WIN32
#include <memory>
#endif

#define ARCHEADER_SIZE 0x40
#define ARCFILEHEADER_SIZE 0x20
//These may be unnecessary
#define ARCFILETYPE_NONE 0x0
#define ARCFILETYPE_MISCDATA 0x1
#define ARCFILETYPE_MODELDATA 0x2
#define ARCFILETYPE_TEXTUREDATA 0x3
#define ARCFILETYPE_ANIMATIONDATA 0x4
#define ARCFILETYPE_SCENEDATA 0x5
#define ARCFILETYPE_TYPE6 0x6
#define ARCFILETYPE_TYPE7 0x7
#define ARCFILETYPE_ARCHIVE 0x8
//Compression types
#define COMPTYPE_NONE 0x0
#define COMPTYPE_LZ77 0x1
#define COMPTYPE_HUFFMAN 0x2
#define COMPTYPE_RUNLENGTH 0x3
#define COMPTYPE_DIRRERENTIAL 0x8
//Compression data parser




inline uint Align(uint x, uint base) { return x + (x%base != 0 ? (base - x%base) : 0); }
#ifndef WIN32
inline uint _byteswap_ulong(uint x){return (x&0xFF)<<24 | (x&0xFF00)<<8 | (x&0xFF0000)>>8 | x>>24;}
#endif


typedef unsigned char uint24[3];
unsigned long ReadU24(uint24 k)
{
	return (unsigned long)(k[0] << 16 | k[1] << 8 | k[2]);
} /* end ReadU24 */
unsigned long WriteU24(unsigned long j, uint24 k)
{
	k[0] = (j >> 16) & 0xFF;
	k[1] = (j >> 8) & 0xFF;
	k[2] = j & 0xFF;
	return j;
} /* end WriteU24 */



#ifdef _WIN32
#pragma pack(1)
#endif
typedef struct
#ifdef HW_RVL
__attribute__((packed))
#endif
{
	byte _algorithm;
	uint24 _size;
	uint _extSize;
}CompressionHeader;


#ifdef _WIN32
#pragma pack(1)
#endif
typedef struct
#ifdef HW_RVL
__attribute__((packed))
#endif
{
	uint _tag; //ARC
	ushort _version; //0x0101
	ushort _numFiles;
	uint _unk1;
	uint _unk2;
}ARCHeader;

typedef struct
#ifdef HW_RVL
__attribute__((packed))
#endif
{
	ushort _type;
	ushort _index;
	uint _size;
	byte _groupIndex;
	byte _padding;
	ushort _flags;
	ushort _redirectIndex; //The index of a different file to read.
	uint _pad1;
	uint _pad2;
	uint _pad3;
	uint _pad4;
	uint _pad5;
}ARCFileHeader;
#ifdef WIN32
#pragma pack()
#endif



bool getLargeSize(CompressionHeader *hdr)
{
	return (ReadU24(hdr->_size) == 0L);
}

unsigned long getSize(CompressionHeader *hdr)
{
	return ReadU24(hdr->_size);
}

unsigned int getAlgorithm(CompressionHeader *hdr)
{
	return hdr->_algorithm;
}
unsigned int getParameter(CompressionHeader *hdr)
{
	return hdr->_algorithm & 0xF;
}

unsigned int getExpandedSize(CompressionHeader *hdr)
{
	if (getLargeSize(hdr))
		return hdr->_extSize;
	return getSize(hdr);
}

bool isExtendedLZ77(CompressionHeader *hdr)
{
	return getParameter(hdr) != 0;
}

bool hasLegitCompression(CompressionHeader *hdr)
{

	int algorithm = getAlgorithm(hdr);
	int mask = 0;
	for (int i = 0; i < 4; i++)
		mask |= 1 << i;
	return ((byte)((algorithm >> 4) & mask) && algorithm != 0);
}

bool IsDataCompressed(byte* addr, int length)
{
	CompressionHeader hdr = *(CompressionHeader*)addr;
#ifndef WIN32
	hdr = _byteswap_ulong(hdr);
#endif
	if (getExpandedSize(&hdr) < (uint)length)
		return false;

	if (!hasLegitCompression(&hdr))
		return false;

	bool isTag = true;
	for (int i = 0; i < 4; i++)
	{
		if (addr[i] < 0x30 || (addr[i] > 0x39 && addr[i] < 0x41) || (addr[i] > 0x5A && addr[i] < 0x61) || addr[i] > 0x7A)
		{
			if (i == 3 && addr[i] == 0)
				break;
			bool isTag = false;
			break;
		}
	}

	if (isTag)
		return false;

	switch (getAlgorithm(&hdr))
	{
	case COMPTYPE_LZ77:
		return getParameter(&hdr) == 0;
	default:return false;
	}
}

bool Expand(CompressionHeader* header, byte* dstAddress, int dstLen)
{
	CompressionHeader hdr = *header;
#ifndef WIN32
	hdr = _byteswap_ulong(hdr);
#endif
	if (getAlgorithm(&hdr) != COMPTYPE_LZ77 || getParameter(&hdr) != 0)
	{
		printf("Compression header does not match LZ77 format\n");//error
		return false;
	}

	for (byte* srcPtr = (byte*)header + 4, *dstPtr = (byte*)dstAddress, *ceiling = dstPtr + dstLen; dstPtr < ceiling;)
	for (byte control = *srcPtr++, bit = 8; (bit-- != 0) && (dstPtr != ceiling);)
	if ((control & (1 << bit)) == 0)
	{
		*dstPtr = *srcPtr;
		dstPtr++; srcPtr++;
	}
	else
	{
		int num = (*srcPtr >> 4) + 3;
		int offset = (*srcPtr++ & 0xF) << 8;
		offset |= *srcPtr++;
		offset += 2;
		for (; (dstPtr != ceiling) && (num-- > 0);)
		{
			*dstPtr = dstPtr[-offset + 1];
			dstPtr++;
		}
	}
	return true;
}

const int WindowMask = 0xFFF;
const int WindowLength = 4096;
const int PatternLength = 18;
const int MinMatch = 3;
ushort* _First, *_Next, *_Last;
int _wIndex, _wLength;

inline ushort MakeHash(byte* ptr)
{
	return (ushort)((ptr[0] << 6) ^ (ptr[1] << 3) ^ ptr[2]);
}

int FindPattern(byte* sPtr, int length, int* matchOffset)
{
	if (length < MinMatch)return 0;
	length = length < PatternLength ? length : PatternLength;

	byte* mPtr;
	int bestLen = MinMatch - 1, bestOffset = 0, index;
	for (int offset = _First[MakeHash(sPtr)]; offset != 0xFFFF; offset = _Next[offset])
	{
		if (offset < _wIndex)mPtr = sPtr - _wIndex + offset;
		else mPtr = sPtr - _wLength - _wIndex + offset;

		if (sPtr - mPtr < 2)break;

		for (index = bestLen + 1; (--index >= 0) && (mPtr[index] == sPtr[index]););
		if (index >= 0)continue;
		for (index = bestLen; (++index < length) && (mPtr[index] == sPtr[index]););

		bestOffset = (int)(sPtr - mPtr);
		if ((bestLen = index) == length)break;
	}

	if (bestLen < MinMatch)return 0;

	*matchOffset = bestOffset;
	return bestLen;
}

void Consume(byte* ptr, int length, int remaining)
{
	int last, inOffset, inVal, outVal;
	for (int i = length<(remaining - 2) ? length : (remaining - 2); i-->0;)
	{
		if (_wLength == WindowLength)
		{
			//Remove node
			outVal = MakeHash(ptr - WindowLength);
			if ((_First[outVal] = _Next[_First[outVal]]) == 0xFFFF)
				_Last[outVal] = 0xFFFF;
			inOffset = _wIndex++;
			_wIndex &= WindowMask;
		}
		else
			inOffset = _wLength++;

		inVal = MakeHash(ptr++);
		if ((last = _Last[inVal]) == 0xFFFF)
			_First[inVal] = (ushort)inOffset;
		else
			_Next[last] = (ushort)inOffset;

		_Last[inVal] = (ushort)inOffset;
		_Next[inOffset] = 0xFFFF;
	}
}

byte _dataAddr[(0x1000 + 0x10000 + 0x10000) * 2];
int Compact(int type, byte* srcAddr, int srcLen, byte* out)
{
	if (type != COMPTYPE_LZ77)return false;
	int dstLen = 4, bitCount;
	byte control;

	byte* sPtr = (byte*)srcAddr;
	int matchLength, matchOffset = 0;

	//Initialize

	memset(_dataAddr, 0, sizeof(_dataAddr));
	_Next = (ushort*)_dataAddr;
	_First = _Next + WindowLength;
	_Last = _First + 0x10000;

	memset(_dataAddr, 0xFF, 0x40000);
	_wIndex = _wLength = 0;

	//write header
	uint header = 0;
	header = (header & 0xFFFFFF0F) | ((COMPTYPE_LZ77 & 0x0F) << 4);
	header = (srcLen << 8) | (header & 0xFF);
#ifndef WIN32
	header = _byteswap_ulong(header);
#endif
	*(uint*)out = header;
	out += sizeof(uint);

	byte blockBuffer[17];
	int dInd = 0;
	int remaining = srcLen;

	while (remaining > 0)
	{
		dInd = 1;
		for (bitCount = 0, control = 0; (bitCount<8) && (remaining>0); bitCount++)
		{
			control <<= 1;
			if ((matchLength = FindPattern(sPtr, remaining, &matchOffset)))
			{
				control |= 1;
				blockBuffer[dInd++] = (byte)(((matchLength - 3) << 4) | ((matchOffset - 1) >> 8));
				blockBuffer[dInd++] = (byte)(matchOffset - 1);
			}
			else
			{
				matchLength = 1;
				blockBuffer[dInd++] = *sPtr;
			}
			Consume(sPtr, matchLength, remaining);
			sPtr += matchLength;
			remaining -= matchLength;
		}
		//Left-align bits
		control <<= 8 - bitCount;

		//Write buffer
		blockBuffer[0] = control;
		memcpy(out, blockBuffer, dInd);
		out += dInd;
		dstLen += dInd;
	}
	return dstLen;
}

ARCResource::ARCResource()
{
	Data = NULL;
	Size = FinalCmpSize = 0;
	Compressed = std::vector<bool>();
	ExpSize = std::vector<int>();
	_isComped = false;
}

bool ARCResource::Load(byte* data, int size)
{
	std::shared_ptr<byte> tmp;
	if ((_isComped = IsDataCompressed(data, size)) == true)
	{
		CompressionHeader* hdr = (CompressionHeader*)data;
#ifdef HW_RVL
		int expSize = ExpandedSize(_byteswap_ulong(*hdr));
#elif WIN32
		int expSize = getExpandedSize(hdr);
#endif
		FinalCmpSize = expSize;
		tmp = std::shared_ptr<byte>((byte*)malloc(expSize));
		if (!Expand(hdr, tmp.get(), expSize))return false;
		data = tmp.get();
	}
	else
		FinalCmpSize = size;
	//normal arc population operation
	ARCHeader* Header = (ARCHeader*)data;
	ARCFileHeader* entry = (ARCFileHeader*)(data + ARCHEADER_SIZE);
	ushort numFiles = Header->_numFiles;
#ifdef WIN32
	numFiles = _byteswap_ushort(numFiles);
#endif
	//We calculate total expanded size first
	Size = ARCHEADER_SIZE;
	for (int i = 0; i < numFiles; i++)
	{
		byte* addr = (byte*)entry + ARCFILEHEADER_SIZE;
		uint entrySize = entry->_size;
#ifdef WIN32
		entrySize = _byteswap_ulong(entrySize);
#endif
		CompressionHeader hdr = *(CompressionHeader*)addr;
#ifdef HW_RVL
		hdr = _byteswap_ulong(hdr);
#endif
		int curSize = IsDataCompressed(addr, entrySize) ? (getExpandedSize(&hdr)) : entrySize;
		Size += ARCFILEHEADER_SIZE + curSize;
		ExpSize.push_back(curSize);

		entry = (ARCFileHeader*)((uint)entry + Align(entrySize, ARCFILEHEADER_SIZE) + ARCFILEHEADER_SIZE);
	}
	//Allocate memory
	_buf = std::shared_ptr<byte>((byte*)malloc(Size));
	memset(_buf.get(), 0, Size);
	byte* cur = _buf.get();
	memcpy(cur, data, ARCHEADER_SIZE);
	cur += ARCHEADER_SIZE;
	entry = (ARCFileHeader*)(data + ARCHEADER_SIZE);
	for (int i = 0; i < numFiles; i++)
	{
		memcpy(cur, (void*)entry, ARCFILEHEADER_SIZE);
		cur += ARCFILEHEADER_SIZE;
		uint entrySize = entry->_size;
#ifdef WIN32
		entrySize = _byteswap_ulong(entrySize);
#endif

		byte* addr = (byte*)entry + ARCFILEHEADER_SIZE;
		if (IsDataCompressed(addr, entrySize))
		{
			Compressed.push_back(true);
			CompressionHeader hdr = *(CompressionHeader*)addr;
#ifdef HW_RVL
			hdr = _byteswap_ulong(hdr);
#endif
			if (!Expand((CompressionHeader*)addr, cur, getExpandedSize(&hdr)))return false;
			cur += getExpandedSize(&hdr);
		}
		else
		{
			Compressed.push_back(false);
			memcpy(cur, addr, entrySize);
			cur += entrySize;
		}
		entry = (ARCFileHeader*)((uint)entry + ARCFILEHEADER_SIZE + Align(entrySize, ARCFILEHEADER_SIZE));
	}
	Data = _buf.get();
	return true;
}

bool ARCResource::Build(byte* buf)
{
	std::shared_ptr<byte> tmp;
	byte* write = buf;
	if (_isComped)
	{
		//tmp=std::shared_ptr<byte>((byte*)calloc(1,FinalCmpSize));
		//write=tmp.get();
		write = Data;
	}
	memcpy(write, Data, ARCHEADER_SIZE);
	write += ARCHEADER_SIZE;
	ARCHeader* hdr = (ARCHeader*)Data;
	ARCFileHeader* entry = (ARCFileHeader*)(Data + ARCHEADER_SIZE);
	ushort numEntries = hdr->_numFiles;
#ifdef WIN32
	numEntries = _byteswap_ushort(numEntries);
#endif
	for (int i = 0; i < numEntries; i++)
	{
		memcpy(write, (byte*)entry, ARCFILEHEADER_SIZE);
		write += ARCFILEHEADER_SIZE;
		if (Compressed[i])
			write += Align(Compact(COMPTYPE_LZ77, (byte*)entry + ARCFILEHEADER_SIZE, ExpSize[i], write), ARCFILEHEADER_SIZE);
		else
		{
			memcpy(write, (byte*)entry + ARCFILEHEADER_SIZE, ExpSize[i]);
			write += Align(ExpSize[i], ARCFILEHEADER_SIZE);
		}
		entry = (ARCFileHeader*)((byte*)entry + ARCFILEHEADER_SIZE + ExpSize[i]);
	}
	if (_isComped)
		Compact(COMPTYPE_LZ77, tmp.get(), FinalCmpSize, buf);
	return true;
}

//data:pointer to replace, compsize:Compresed total size, compressed:compressed children flag, expandedSize:expanded children size
void ARCResource::Replace(byte* data, int compsize, std::vector<bool> compressed, std::vector<int> expandedSize)
{
	Compressed = compressed;
	Data = data;
	FinalCmpSize = compsize;
	ExpSize = expandedSize;
}
