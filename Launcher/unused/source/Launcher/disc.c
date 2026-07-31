#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/lwp_watchdog.h>

#include "disc.h"
#include "wdvd.h"
#include "memory.h"

#define PART_INFO_OFFSET	0x10000

int disc_type = IS_WII_DISC;

static struct
{
	u8 discid[0x8];
	u8 skip[0x10];
	u32 checkwii;
	u32 checkngc;
} disc_hdr ATTRIBUTE_ALIGN(32);

s32 Disc_Open()
{
	s32 ret = WDVD_Reset();
	if(ret < 0)
		return ret;
		
	ret = WDVD_ReadDiskId((u8*)Disc_ID);
	ret = WDVD_UnencryptedRead(&disc_hdr, 0x20, 0);
	if (disc_hdr.checkngc == NGC_MAGIC) {
		disc_type = IS_NGC_DISC;
	} else if (disc_hdr.checkwii == WII_MAGIC) {
		disc_type = IS_WII_DISC;
	} else {
		disc_type = IS_UNK_DISC;
	}
	
	return ret;
}

void Disc_SetLowMemPre()
{
	*BI2				= 0x817E5480; // BI2
	*(vu32*)0xCD00643C	= 0x00000000; // 32Mhz on Bus

	memset((u8*)Disc_ID, 0, 32);
	memset((void*)0x80001800, 0, 0x1800);

	DCFlushRange((void*)0x80000000, 0x3f00);
}

void Disc_SetLowMem(u32 IOS)
{
	*Sys_Magic			= 0x0D15EA5E; // Standard Boot Code
	*Sys_Version		= 0x00000001; // Version
	*Arena_L			= 0x00000000; // Arena Low
	*Bus_Speed			= 0x0E7BE2C0; // Console Bus Speed
	*CPU_Speed			= 0x2B73A840; // Console CPU Speed
	*Assembler			= 0x38A00040; // Assembler
	*OS_Thread			= 0x80431A80; // Thread Init
	*Dev_Debugger		= 0x81800000; // Dev Debugger Monitor Address
	*Simulated_Mem		= 0x01800000; // Simulated Memory Size
	*GameID_Address		= 0x80000000; // Fix for Sam & Max (WiiPower)

	memcpy((void*)Online_Check, (void*)Disc_ID, 4);
	memcpy((void*)0x80001800, (void*)Disc_ID, 8);
	*Current_IOS = (IOS << 16) | 0xffff;
	*Apploader_IOS = (IOS << 16) | 0xffff;

	DCFlushRange((void*)0x80000000, 0x3f00);
}

static struct {
	u32 offset;
	u32 type;
} partition_table[32] ATTRIBUTE_ALIGN(32);

static struct {
	u32 count;
	u32 offset;
	u32 pad[6];
} part_table_info ATTRIBUTE_ALIGN(32);

s32 Disc_FindPartition(u32 *outbuf)
{
	u32 offset = 0;
	u32 cnt = 0;

	s32 ret = WDVD_UnencryptedRead(&part_table_info, sizeof(part_table_info), PART_INFO_OFFSET);
	if(ret < 0)
		return ret;

	ret = WDVD_UnencryptedRead(&partition_table, sizeof(partition_table), part_table_info.offset);
	if(ret < 0)
		return ret;

	for(cnt = 0; cnt < part_table_info.count; cnt++)
	{
		if(partition_table[cnt].type == 0)
		{
			offset = partition_table[cnt].offset;
			break;
		}
	}

	if(offset == 0)
		return -1;
	WDVD_Seek(offset);

	*outbuf = offset;
	return 0;
}

void Disc_SetTime()
{
	settime(secs_to_ticks(time(NULL) - 946684800));
}

GXRModeObj *Disc_SelectVMode(u8 videoselected, u32 *rmode_reg)
{
	GXRModeObj *rmode = VIDEO_GetPreferredMode(0);

	bool progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();

	switch (CONF_GetVideo())
	{
		case CONF_VIDEO_PAL:
			if (CONF_GetEuRGB60() > 0)
			{
				*rmode_reg = VI_EURGB60;
				rmode = progressive ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
			}
			else
				*rmode_reg = VI_PAL;
			break;

		case CONF_VIDEO_MPAL:
			*rmode_reg = VI_MPAL;
			break;

		case CONF_VIDEO_NTSC:
			*rmode_reg = VI_NTSC;
			break;
	}

	const char DiscRegion = ((u8*)Disc_ID)[3];
	switch(videoselected)
	{
		case 0: // DEFAULT (DISC/GAME)
			switch(DiscRegion)
			{
				case 'W':
					break; // Don't overwrite wiiware video modes.
				// PAL
				case 'D':
				case 'F':
				case 'P':
				case 'X':
				case 'Y':
					if(CONF_GetVideo() != CONF_VIDEO_PAL)
					{
						*rmode_reg = VI_PAL;
						rmode = progressive ? &TVNtsc480Prog : &TVNtsc480IntDf;
					}
					break;
				// NTSC
				case 'E':
				case 'J':
				default:
					if(CONF_GetVideo() != CONF_VIDEO_NTSC)
					{
						*rmode_reg = VI_NTSC;
						rmode = progressive ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
					}
					break;
			}
			break;
		case 1: // SYSTEM
			break;
		case 2: // PAL50
			rmode =  &TVPal528IntDf;
			*rmode_reg = rmode->viTVMode >> 2;
			break;
		case 3: // PAL60
			rmode = progressive ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
			*rmode_reg = progressive ? TVEurgb60Hz480Prog.viTVMode >> 2 : rmode->viTVMode >> 2;
			break;
		case 4: // NTSC
			rmode = progressive ? &TVNtsc480Prog : &TVNtsc480IntDf;
			*rmode_reg = rmode->viTVMode >> 2;
			break;
		case 5: // PROGRESSIVE 480P
			rmode = &TVNtsc480Prog;
			*rmode_reg = DiscRegion == 'P' ? TVEurgb60Hz480Prog.viTVMode >> 2 : rmode->viTVMode >> 2;
			break;
		default:
			break;
	}
	return rmode;
}

void Disc_SetVMode(GXRModeObj *rmode, u32 rmode_reg)
{
	//video_clear();

	*Video_Mode = rmode_reg;
	DCFlushRange((void*)Video_Mode, 4);

	if(rmode != 0)
		VIDEO_Configure(rmode);

	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else while(VIDEO_GetNextField())
		VIDEO_WaitVSync();

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else while(VIDEO_GetNextField())
		VIDEO_WaitVSync();
}
