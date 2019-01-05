#include "stdafx.h"
#include "ApplyPatch.h"
#include "bspatch.h"
#include "../Common.h"
#include "FrozenMemories.h"
#include "../FileHolder.h"
#include <stdio.h>
#ifdef WIN32
#include <vector>
#include <memory>
#elif HW_RVL
#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>
#endif

typedef struct{
	byte* buf;
	int read;
}Context;


//src:original data, srcLen:the length of src, dst:pointer of buffer to write, patch:buffer that contains patch, patchLen:the length of patch
bool ApplyPatchARC(bool usebsPatch, byte* sourceBuffer, int sourceLength, byte * patchBuffer, int patchLength, byte * outputBuffer, int outputLength)
{
	//Load original file
	ARCResource arcData;
	if (!arcData.Load(sourceBuffer, sourceLength))
		return false;
	if (!arcData.IsCompressed())
		free(sourceBuffer);
	/*if (arcData.IsCompressed())
	{
	free(sourceBuffer);
	sourceBufferFreed = true;
	}*/

	//buffer[0]:empty buffer[1]:empty buffer[2]: orig.Data


	//u8* patchFileBuffer = buffers[0] = (u8*)malloc(patchSet.str->length);
	//memset(patchFileBuffer, 0, patchSet.str->length);

	//buffers[0]: patch buffer buffer[1]:empty buffer[2]:orig.Data

	int headerPosition = 0;
	int headerSize = (int)offtin(patchBuffer);//patched file size
	headerPosition += 8;
	int decompSize = (int)offtin(patchBuffer + headerPosition);//patched file size
	headerPosition += 8;
	int cmpSize = (int)offtin(patchBuffer + headerPosition);//patched file size
	headerPosition += 8;
	int finalSize = (int)offtin(patchBuffer + headerPosition);//patched file size
	headerPosition += 8;
	unsigned char childrenCount = patchBuffer[headerPosition];

	headerPosition++;
	std::vector<bool> compressed;
	for (int i = 0; i < childrenCount; i++)
		compressed.push_back((bool)patchBuffer[headerPosition++]);

	std::vector<int> sizes;
	for (int i = 0; i < childrenCount; i++)
	{
		int expectedSize = (int)offtin(patchBuffer + headerPosition);//patched file size
		sizes.push_back(expectedSize);

		headerPosition += 8;
	}

	//patchSet.sZip->ExtractIntoMemory(patchSet.str->fileindex, patchFileBuffer, patch.str->length);
	//header check
	int patchedLength = getbsPatchOutputSize(patchBuffer + headerSize);
	u8* patchedBuffer = (u8*)malloc(patchedLength);
	memset(patchedBuffer, 0, patchedLength);

	if (bspatch(arcData.Data, arcData.Size, patchBuffer + headerSize, patchLength - headerSize, patchedBuffer, patchedLength) != 0)
	{
		//free(arcData.Data);
		free(patchedBuffer);
		return false;
	}
	free(patchBuffer);
	//delete arcData;
	/*PatchHeader hdr = *(PatchHeader*)pBuf;
	char tmp[17];
	memcpy(tmp, hdr.DiffName, 16);
	tmp[16] = '\0';
	if (strcmp(tmp, "ENDSLEY/BSDIFF43") != 0)return false;
	uint64_t size = offtin(hdr.DecompSize);//patched file size
	uint64_t cmpSize = offtin(hdr.CompSize);
	uint64_t finalSize = offtin(hdr.FinalSize);
	std::vector<bool> compressed;
	std::vector<int> sizes;
	for (int i = 0; i<hdr.ChildrenCount; i++)
	compressed.push_back(hdr.Compressed[i]>0);
	for (int i = 0; i < hdr.ChildrenCount; i++)
	{
	uint curSize = hdr.Sizes[i];
	#ifdef WIN32
	curSize = _byteswap_ulong(curSize);
	#endif
	sizes.push_back((int)curSize);
	}
	//prepare for patch
	Context cont;
	cont.buf = pBuf + sizeof(PatchHeader);
	cont.read = 0;
	struct bspatch_stream strm;
	strm.opaque = &cont;
	strm.read = read;
	//buffer for patched data
	auto patchResult = buffers[1] = (u8*)malloc(size);
	memset(patchResult, 0, size);
	//buffers[0]: patch buffer buffers[1]:patch result buffers[2]:orig.Data
	//Let's go.
	if (bspatch(orig.Data, orig.Size, patchResult, size, &strm) < 0)return false;
	RELEASE_PTR(buffers[0]);
	RELEASE_PTR(buffers[2]);
	//buffers[0]: empty buffers[1]:patch result buffers[2]:empty
	//Replace data*/
	//arcData.Replace(patchedBuffer, (int)cmpSize, compressed, sizes);
	//Build data
	arcData.Replace(patchedBuffer, cmpSize, compressed, sizes);
	arcData.Build(outputBuffer);
	free(patchedBuffer);
	//free(arcData.Data);
	return true;
	//buffers[0]: final data buffers[1]: empty buffers[2]: empty
}
int getOutputSizeARC(bool usebsPatch, u8 * patchBuffer){
	int headerPosition = 0;
	int headerSize = (int)offtin(patchBuffer);//patched file size
	headerPosition += 8;
	int decompSize = (int)offtin(patchBuffer + headerPosition);//patched file size
	headerPosition += 8;
	int cmpSize = (int)offtin(patchBuffer + headerPosition);//patched file size
	headerPosition += 8;
	int finalSize = (int)offtin(patchBuffer + headerPosition);//patched file size
	return finalSize;
}

int getOutputSizeOther(bool usebsPatch, u8 * patchBuffer){
	return getbsPatchOutputSize(patchBuffer);
}

bool ApplyPatchOther(bool usebsPatch, byte* sourceBuffer, int sourceLength, byte * patchBuffer, int patchLength, byte * outputBuffer, int outputLength)
{
	if (bspatch(sourceBuffer, sourceLength, patchBuffer, patchLength, outputBuffer, outputLength) != 0)
		return false;
	return true;
	/*auto p = buffers[1] = (u8*)malloc(patch.str->length);
	memset(p, 0, patch.str->length);
	//[0]=src [1]=patch [2]=empty
	patch.sZip->ExtractIntoMemory(patch.str->fileindex, p, patch.str->length);
	PatchHeader* hdr = (PatchHeader*)p;
	uint64_t size = offtin(hdr->FinalSize);

	//prepare for patch
	Context cont;
	cont.buf = p + sizeof(PatchHeader);
	cont.read = 0;
	struct bspatch_stream strm;
	strm.opaque = &cont;
	strm.read = read;
	*dst = buffers[2] = (u8*)malloc(size);
	memset(*dst, 0, size);
	//[0]=src [1]=patch [2]=result
	bool ret = bspatch(src, srcLen, *dst, size, &strm) == 0;
	RELEASE_PTR(buffers[0]);
	RELEASE_PTR(buffers[1]);

	return ret;*/
}