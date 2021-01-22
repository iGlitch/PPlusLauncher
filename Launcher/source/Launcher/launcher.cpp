#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>
#include <sdcard/wiisd_io.h>
#include <sys/dir.h>

#include "launcher.h"
#include "codehandler.h"
#include "apploader.h"
#include "codehandleronly.h"
#include "multidol.h"
#include "patchcode.h"
#include "disc.h"
#include "wdvd.h"
#include "video_tinyload.h"

#define GAMECONFIG		"sd:/Project+/gc.txt"
#define	 Disc_ID		((vu32*)0x80000000)

u8 config_bytes[16] ATTRIBUTE_ALIGN(32);
int codes_state = 0;
u8 *tempcodelist = (u8 *)0x90080000;
u8 *codelist = NULL;
u8 *codelistend = (u8 *)0x80003000;
u32 codelistsize = 0;
char gameidbuffer[8];

int patch_state = 0;
char *tempgameconf = (char *)0x90080000;
u32 *gameconf = NULL;
u32 tempgameconfsize = 0;
u32 gameconfsize = 0;
u8 configwarn = 0;


const unsigned char defaultgameconfig[] = {
	0x64, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x3a, 0x0a, 0x63, 0x6f, 0x64, 0x65, 0x6c, 0x69, 0x73,
	0x74, 0x65, 0x6e, 0x64, 0x20, 0x3d, 0x20, 0x38, 0x30, 0x30, 0x30, 0x33, 0x30, 0x30, 0x30
};
const int defaultgameconfig_size = sizeof(defaultgameconfig);
void LaunchTitle();
bool valuesInitialized = false;
void initValues()
{
	bool geckoattached = false;
	config_bytes[0] = 0xCD; // language
	config_bytes[1] = 0x00; // video mode
	config_bytes[2] = 0x07; // hook type
	config_bytes[3] = 0x00; // file patcher
	config_bytes[4] = 0x01; // cheats
	config_bytes[5] = 0x00; // no paused start
	config_bytes[6] = 0x00; // gecko slot b	
	config_bytes[7] = 0x00; //debugger
	config_bytes[8] = 0x00; // recovery hook
	config_bytes[9] = 0x00; // region free
	config_bytes[10] = 0x00; // no copy
	config_bytes[11] = 0x00; // button skip
	config_bytes[12] = 0x00; // video modes patch
	config_bytes[13] = 0x00; // country string patch
	config_bytes[14] = 0x00; // aspect ratio patch
	config_bytes[15] = 0x00; // reserved
	valuesInitialized = true;
}

void sd_copy_gameconfig(char *gameid) {

	FILE* fp;
	u32 ret;
	u32 filesize = 0;
	s32 gameidmatch, maxgameidmatch = -1, maxgameidmatch2 = -1;
	u32 i, j, numnonascii, parsebufpos;
	u32 codeaddr, codeval, codeaddr2, codeval2, codeoffset;
	u32 temp, tempoffset;
	char parsebuffer[18];

	memcpy(tempgameconf, defaultgameconfig, defaultgameconfig_size);
	tempgameconf[defaultgameconfig_size] = '\n';
	tempgameconfsize = defaultgameconfig_size + 1;

	fp = fopen(GAMECONFIG, "rb");

	if (fp) {
		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		ret = fread((void*)tempgameconf + tempgameconfsize, 1, filesize, fp);
		fclose(fp);
		if (ret == filesize)
			tempgameconfsize += filesize;
	}

	// Remove non-ASCII characters
	numnonascii = 0;
	for (i = 0; i < tempgameconfsize; i++)
	{
		if (tempgameconf[i] < 9 || tempgameconf[i] > 126)
			numnonascii++;
		else
			tempgameconf[i - numnonascii] = tempgameconf[i];
	}
	tempgameconfsize -= numnonascii;

	*(tempgameconf + tempgameconfsize) = 0;
	gameconf = (u32 *)((tempgameconf + tempgameconfsize) + (4 - (((u32)(tempgameconf + tempgameconfsize)) % 4)));

	for (maxgameidmatch = 0; maxgameidmatch <= 6; maxgameidmatch++)
	{
		i = 0;
		while (i < tempgameconfsize)
		{
			maxgameidmatch2 = -1;
			while (maxgameidmatch != maxgameidmatch2)
			{
				while (i != tempgameconfsize && tempgameconf[i] != ':') i++;
				if (i == tempgameconfsize) break;
				while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0)) i--;
				if (i != 0) i++;
				parsebufpos = 0;
				gameidmatch = 0;
				while (tempgameconf[i] != ':')
				{
					if (tempgameconf[i] == '?')
					{
						parsebuffer[parsebufpos] = gameid[parsebufpos];
						parsebufpos++;
						gameidmatch--;
						i++;
					}
					else if (tempgameconf[i] != 0 && tempgameconf[i] != ' ')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ')
						break;
					else
						i++;
					if (parsebufpos == 8) break;
				}
				parsebuffer[parsebufpos] = 0;
				if (strncasecmp("DEFAULT", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 7)
				{
					gameidmatch = 0;
					goto idmatch;
				}
				if (strncmp(gameid, parsebuffer, strlen(parsebuffer)) == 0)
				{
					gameidmatch += strlen(parsebuffer);
				idmatch:
					if (gameidmatch > maxgameidmatch2)
					{
						maxgameidmatch2 = gameidmatch;
					}
				}
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13)) i++;
			}
			while (i != tempgameconfsize && tempgameconf[i] != ':')
			{
				parsebufpos = 0;
				while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13))
				{
					if (tempgameconf[i] != 0 && tempgameconf[i] != ' ' && tempgameconf[i] != '(' && tempgameconf[i] != ':')
						parsebuffer[parsebufpos++] = tempgameconf[i++];
					else if (tempgameconf[i] == ' ' || tempgameconf[i] == '(' || tempgameconf[i] == ':')
						break;
					else
						i++;
					if (parsebufpos == 17) break;
				}
				parsebuffer[parsebufpos] = 0;

				if (strncasecmp("codeliststart", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 13)
				{
					sscanf(tempgameconf + i, " = %x", (u32 *)&codelist);
				}
				if (strncasecmp("codelistend", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
				{
					sscanf(tempgameconf + i, " = %x", (u32 *)&codelistend);
				}
				if (strncasecmp("hooktype", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
				{
					ret = sscanf(tempgameconf + i, " = %u", &temp);
					if (ret == 1)
					if (temp <= 7)
						config_bytes[2] = temp;
				}
				if (strncasecmp("poke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 4)
				{
					ret = sscanf(tempgameconf + i, "( %x , %x", &codeaddr, &codeval);
					if (ret == 2)
					{
						*(gameconf + (gameconfsize / 4)) = 0;
						gameconfsize += 4;
						*(gameconf + (gameconfsize / 4)) = (u32)NULL;
						gameconfsize += 8;
						*(gameconf + (gameconfsize / 4)) = codeaddr;
						gameconfsize += 4;
						*(gameconf + (gameconfsize / 4)) = codeval;
						gameconfsize += 4;
						DCFlushRange((void *)(gameconf + (gameconfsize / 4) - 5), 20);
					}
				}
				if (strncasecmp("pokeifequal", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 11)
				{
					ret = sscanf(tempgameconf + i, "( %x , %x , %x , %x", &codeaddr, &codeval, &codeaddr2, &codeval2);
					if (ret == 4)
					{
						*(gameconf + (gameconfsize / 4)) = 0;
						gameconfsize += 4;
						*(gameconf + (gameconfsize / 4)) = codeaddr;
						gameconfsize += 4;
						*(gameconf + (gameconfsize / 4)) = codeval;
						gameconfsize += 4;
						*(gameconf + (gameconfsize / 4)) = codeaddr2;
						gameconfsize += 4;
						*(gameconf + (gameconfsize / 4)) = codeval2;
						gameconfsize += 4;
						DCFlushRange((void *)(gameconf + (gameconfsize / 4) - 5), 20);
					}
				}
				if (strncasecmp("searchandpoke", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 13)
				{
					ret = sscanf(tempgameconf + i, "( %x%n", &codeval, &tempoffset);
					if (ret == 1)
					{
						gameconfsize += 4;
						temp = 0;
						while (ret == 1)
						{
							*(gameconf + (gameconfsize / 4)) = codeval;
							gameconfsize += 4;
							temp++;
							i += tempoffset;
							ret = sscanf(tempgameconf + i, " %x%n", &codeval, &tempoffset);
						}
						*(gameconf + (gameconfsize / 4) - temp - 1) = temp;
						ret = sscanf(tempgameconf + i, " , %x , %x , %x , %x", &codeaddr, &codeaddr2, &codeoffset, &codeval2);
						if (ret == 4)
						{
							*(gameconf + (gameconfsize / 4)) = codeaddr;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeaddr2;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeoffset;
							gameconfsize += 4;
							*(gameconf + (gameconfsize / 4)) = codeval2;
							gameconfsize += 4;
							DCFlushRange((void *)(gameconf + (gameconfsize / 4) - temp - 5), temp * 4 + 20);
						}
						else
							gameconfsize -= temp * 4 + 4;
					}

				}
				if (strncasecmp("videomode", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 9)
				{
					ret = sscanf(tempgameconf + i, " = %u", &temp);
					if (ret == 1)
					{
						if (temp == 0)
						{
							if (config_bytes[1] != 0x00)
								configwarn |= 1;
							config_bytes[1] = 0x00;
						}
						else if (temp == 1)
						{
							if (config_bytes[1] != 0x01)
								configwarn |= 1;
							config_bytes[1] = 0x01;
						}
						else if (temp == 2)
						{
							if (config_bytes[1] != 0x02)
								configwarn |= 1;
							config_bytes[1] = 0x02;
						}
						else if (temp == 3)
						{
							if (config_bytes[1] != 0x03)
								configwarn |= 1;
							config_bytes[1] = 0x03;
						}
					}
				}
				if (strncasecmp("language", parsebuffer, strlen(parsebuffer)) == 0 && strlen(parsebuffer) == 8)
				{
					ret = sscanf(tempgameconf + i, " = %u", &temp);
					if (ret == 1)
					{
						if (temp == 0)
						{
							if (config_bytes[0] != 0xCD)
								configwarn |= 2;
							config_bytes[0] = 0xCD;
						}
						else if (temp > 0 && temp <= 10)
						{
							if (config_bytes[0] != temp - 1)
								configwarn |= 2;
							config_bytes[0] = temp - 1;
						}
					}
				}
				if (tempgameconf[i] != ':')
				{
					while ((i != tempgameconfsize) && (tempgameconf[i] != 10 && tempgameconf[i] != 13)) i++;
					if (i != tempgameconfsize) i++;
				}
			}
			if (i != tempgameconfsize) while ((tempgameconf[i] != 10 && tempgameconf[i] != 13) && (i != 0)) i--;
		}
	}

	tempcodelist = ((u8 *)gameconf) + gameconfsize;
}


void sd_copy_codes(char *filename) {

	FILE* fp;
	u32 ret;
	u32 filesize;
	char filepath[256];

	DIR *pdir = opendir("/Project+/");
		if (pdir == NULL){
			codes_state = 1;	// dir not found
			return;
		}

	closedir(pdir);
	fflush(stdout);

	sprintf(filepath, "sd:/Project+/%s.gct", filename);

	fp = fopen(filepath, "rb");
	if (!fp) {
		codes_state = 1;	// codes not found
		return;
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	codelistsize = filesize;

	if ((codelist + codelistsize) > codelistend)
	{
		fclose(fp);
		codes_state = 4;
		return;
	}

	ret = fread((void*)tempcodelist, 1, filesize, fp);
	if (ret != filesize){
		fclose(fp);
		codelistsize = 0;
		codes_state = 1;	// codes not found
		return;
	}

	fclose(fp);
	DCFlushRange((void*)tempcodelist, filesize);

	codes_state = 2;
}

void __Sys_ResetCallback(void)
{
	LaunchTitle();
}

u32 GameIOS = 0;
u32 vmode_reg = 0;
u32 AppEntrypoint = 0;
GXRModeObj *vmode = NULL;
u8 vidMode = 0;
u8 *code_buf = NULL;
u32 code_size = 0;
int patchVidModes = 0x0;
u8 vipatchselect = 0;
u8 countryString = 0;
int aspectRatio = -1;
u8 debuggerselect = 0;
void LaunchTitle()
{
	if (!valuesInitialized)
		initValues();

	Disc_SetLowMemPre();
	WDVD_Init();
	if (Disc_Open() != 0) {
		//printf("Cannot open disc");
		sleep(2);
		return 0;
	}
	if((*(u32*)Disc_ID & 0xFFFFFF00) == 0x52534200){
	memset(gameidbuffer, 0, 8);
	memcpy(gameidbuffer, (char*)0x80000000, 6);
	codelist = (u8 *)0x800028B8;
	sd_copy_gameconfig(gameidbuffer);
	sd_copy_codes(gameidbuffer);
	__io_wiisd.shutdown();
	printf("\x1b[2J");

	configbytes[0] = config_bytes[0];
	configbytes[1] = config_bytes[1];
	hooktype = config_bytes[2];
	debuggerselect = config_bytes[7];
	u8 codesselect = config_bytes[4];
	code_buf = tempcodelist;
	code_size = codelistsize;

	if (code_size <= 0)
	{
		code_buf = NULL;
		code_size = 0;
		return;
	}
	if (code_size > (u32)codelistend - (u32)codelist)
	{
		code_buf = NULL;
		code_size = 0;
		return;
	}

	countryString = config_bytes[13];
	switch (config_bytes[1]) {
	case 0:
		vidMode = 0;
		break;
	case 1:
		vidMode = 3;
		break;
	case 2:
		vidMode = 2;
		break;
	case 3:
		vidMode = 4;
		break;
	}
	switch (config_bytes[12]) {
	case 0:
		break;
	case 1:
		vipatchselect = 1;
		break;
	case 2:
		patchVidModes = 0;
		break;
	case 3:
		patchVidModes = 1;
		break;
	case 4:
		patchVidModes = 2;
		break;
	}
	if (config_bytes[14] > 0)
		aspectRatio = (int)config_bytes[14] - 1;

	u32 offset = 0;
	Disc_FindPartition(&offset);
	WDVD_OpenPartition(offset, &GameIOS);
	vmode = Disc_SelectVMode(0, &vmode_reg);
	AppEntrypoint = Apploader_Run(vidMode, vmode, vipatchselect, countryString, patchVidModes, aspectRatio);

	//load_handler();

	memcpy((void*)0x80001800, codehandleronly, codehandleronly_size);
	if (code_size > 0 && code_buf)
	{
		memcpy((void*)0x80001906, &codelist, 2);
		memcpy((void*)0x8000190A, ((u8*)&codelist) + 2, 2);
	}

	DCFlushRange((void*)0x80001800, codehandleronly_size);
	ICInvalidateRange((void*)0x80001800, codehandleronly_size);


	// Load multidol handler
	memcpy((void*)0x80001000, multidol, multidol_size);
	DCFlushRange((void*)0x80001000, multidol_size);
	ICInvalidateRange((void*)0x80001000, multidol_size);
	memcpy((void*)0x8000119C, axnextframehooks, 12);
	memcpy((void*)0x80001198, axnextframehooks + 3, 4);
	DCFlushRange((void*)0x80001198, 16);

	if (codelist)
		memset(codelist, 0, (u32)codelistend - (u32)codelist);

	if (code_size > 0 && code_buf)
	{
		memcpy(codelist, code_buf, code_size);
		DCFlushRange(codelist, (u32)codelistend - (u32)codelist);
	}

	//apply_pokevalues();

	u32 *codeaddr, *codeaddr2, *addrfound = NULL;

	if (gameconfsize != 0)
	{
		for (u32 i = 0; i < (gameconfsize / 4); i++)
		{
			if (*(gameconf + i) == 0)
			{
				if (((u32 *)(*(gameconf + i + 1))) == NULL ||
					*((u32 *)(*(gameconf + i + 1))) == *(gameconf + i + 2))
				{
					*((u32 *)(*(gameconf + i + 3))) = *(gameconf + i + 4);
					DCFlushRange((void *)*(gameconf + i + 3), 4);
					if (((u32 *)(*(gameconf + i + 1))) == NULL)
						;// printf("identify_apply_pokevalues: poke ");
				i += 4;
			}
			else
			{
				codeaddr = (u32 *)(*(gameconf + i + *(gameconf + i) + 1));
				codeaddr2 = (u32 *)(*(gameconf + i + *(gameconf + i) + 2));
				if (codeaddr == 0 && addrfound != NULL)
					codeaddr = addrfound;
				else if (codeaddr == 0 && codeaddr2 != 0)
					codeaddr = (u32 *)((((u32)codeaddr2) >> 28) << 28);
				else if (codeaddr == 0 && codeaddr2 == 0)
				{
					i += *(gameconf + i) + 4;
					continue;
				}
				if (codeaddr2 == 0)
					codeaddr2 = codeaddr + *(gameconf + i);
				addrfound = NULL;
				while (codeaddr <= (codeaddr2 - *(gameconf + i)))
				{
					if (memcmp(codeaddr, gameconf + i + 1, (*(gameconf + i)) * 4) == 0)
					{
						*(codeaddr + ((*(gameconf + i + *(gameconf + i) + 3)) / 4)) = *(gameconf + i + *(gameconf + i) + 4);
						if (addrfound == NULL) addrfound = codeaddr;
						DCFlushRange((void *)(codeaddr + ((*(gameconf + i + *(gameconf + i) + 3)) / 4)), 4);
					}
					codeaddr++;
				}
				i += *(gameconf + i) + 4;
			}
	}}}
	

	//sleep(1);
	WDVD_Close();
	//sleep(1);
	Disc_SetLowMem(GameIOS);
	Disc_SetTime();
	Disc_SetVMode(vmode, vmode_reg);
	u32 level = IRQ_Disable();
	__IOS_ShutdownSubsystems();
	//__exception_closeall();
	*(vu32*)0xCC003024 = 1;

	SYS_SetResetCallback(__Sys_ResetCallback);

	if (AppEntrypoint == 0x3400)
	{
		if (hooktype)
		{
			asm volatile (
				"lis %r3, returnpointdisc@h\n"
				"ori %r3, %r3, returnpointdisc@l\n"
				"mtlr %r3\n"
				"lis %r3, 0x8000\n"
				"ori %r3, %r3, 0x18A8\n"
				"nop\n"
				"mtctr %r3\n"
				"bctr\n"
				"returnpointdisc:\n"
				"bl DCDisable\n"
				"bl ICDisable\n"
				"li %r3, 0\n"
				"mtsrr1 %r3\n"
				"lis %r4, AppEntrypoint@h\n"
				"ori %r4,%r4,AppEntrypoint@l\n"
				"lwz %r4, 0(%r4)\n"
				"mtsrr0 %r4\n"
				"rfi\n"
				);
		}
	}
	else
	{
		asm volatile (
			"lis %r3, AppEntrypoint@h\n"
			"ori %r3, %r3, AppEntrypoint@l\n"
			"lwz %r3, 0(%r3)\n"
			"mtlr %r3\n"
			"lis %r3, 0x8000\n"
			"ori %r3, %r3, 0x18A8\n"
			"nop\n"
			"mtctr %r3\n"
			"bctr\n"
			);
	}
	IRQ_Restore(level);
	}
	else return 0;
}

