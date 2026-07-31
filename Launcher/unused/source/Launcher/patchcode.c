
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <sys/unistd.h>

#include "patchcode.h"

u32 hooktype;
u8 configbytes[2];
u8 menuhook;

extern void patchhook(u32 address, u32 len);
extern void patchhook2(u32 address, u32 len);
extern void patchhook3(u32 address, u32 len);

extern void multidolpatchone(u32 address, u32 len);
extern void multidolpatchtwo(u32 address, u32 len);

extern void regionfreejap(u32 address, u32 len);
extern void regionfreeusa(u32 address, u32 len);
extern void regionfreepal(u32 address, u32 len);

extern void removehealthcheck(u32 address, u32 len);

extern void copyflagcheck1(u32 address, u32 len);
extern void copyflagcheck2(u32 address, u32 len);
extern void copyflagcheck3(u32 address, u32 len);
extern void copyflagcheck4(u32 address, u32 len);
extern void copyflagcheck5(u32 address, u32 len);
extern void copyflagcheck6(u32 address, u32 len);
extern void copyflagcheck7(u32 address, u32 len);
extern void copyflagcheck8(u32 address, u32 len);

extern void patchupdatecheck(u32 address, u32 len);

extern void movedvdhooks(u32 address, u32 len);

extern void multidolhook(u32 address);
extern void langvipatch(u32 address, u32 len, u8 langbyte);
extern void vipatch(u32 address, u32 len);

const u32 viwiihooks[4] = {0x7CE33B78,0x38870034,0x38A70038,0x38C7004C};
const u32 axnextframehooks[4] = {0x3800000E, 0x7FE3FB78, 0xB0050000, 0x38800080};

const u32 multidolhooks[4] = {0x7C0004AC, 0x4C00012C, 0x7FE903A6, 0x4E800420};
const u32 multidolchanhooks[4] = {0x4200FFF4, 0x48000004, 0x38800000, 0x4E800020};

const u32 langpatch[3] = {0x7C600775, 0x40820010, 0x38000000};


bool dogamehooks(void *addr, u32 len, bool channel)
{
	/*
	0 No Hook
	1 VBI
	7 AXNextFrame Hook
	*/

	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end)
	{
		switch(hooktype)
		{
			case 0x00:
				break;

			case 0x01:
				if(memcmp(addr_start, viwiihooks, sizeof(viwiihooks))==0)
				{
					patchhook((u32)addr_start, len);
				}
				break;

			case 0x07:
				if(memcmp(addr_start, axnextframehooks, sizeof(axnextframehooks))==0)
				{
					patchhook((u32)addr_start, len);
				}
				break;
		}
		addr_start += 4;
	}
	return true;
}


