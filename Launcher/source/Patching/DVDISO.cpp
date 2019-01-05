#include "DVDISO.h"

extern "C"{
#include "fst_J.h"
#include "fst_U.h"
}

#include <di\di.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>
#include <ogc\cache.h>
#include <malloc.h>
#include <ctype.h>
#include <ogc\lwp.h>
#include <limits.h>
#include "../Common.h"

void* data=NULL;
bool init=false;
int DVDISO::DVDInitCallback(uint32_t status, uint32_t error)
{
	switch(status)
	{
	case DVD_READY:
	case DVD_INIT:
		break;
	case DVD_NO_DISC:
		printf("Warning:No disk\n");
		break;
	case DVD_UNKNOWN:
	case DVD_IOS_ERROR:
	case DVD_D0:
	case DVD_A8:
		printf("Caught some error");
		break;
	default:
		break;
	}

	init=true;
	data=NULL;
	return 0;
}

int cb(uint32_t status, uint32_t error)
{
	return ((DVDISO*)data)->DVDInitCallback(status,error);
}

DVDISO::DVDISO()
{
	DI_SetInitCallback(cb);
	data=this;
	if(DI_Init())
		printf("Failed to initialize DVD drive");
	DI_Mount();

	while (!init)LWP_YieldThread();
	u64 id;
	if(DI_ReadDiscID(&id))
		printf("Failed to read disk ID");
	if((id>>40)!=0x525342);	//RSB
	_region=(char)((id>>32)&0xFF);

	//get partition info
	static struct {
		u32 count;
		u32 offset;
		u32 pad[6];
	} tableOffset __attribute__((aligned(32)));

	int result=0;
	if((result=DI_UnencryptedRead(&tableOffset,sizeof(tableOffset),0x40000>>2)));
	static struct {
		u32 offset;
		u32 type;
	} partition_table[32] __attribute__((aligned(32)));
	if((result=DI_UnencryptedRead(partition_table,sizeof(partition_table),tableOffset.offset)))
	{
		printf("Failed to read partition table:%d\n",result);
	}
	if(partition_table[1].type!=0);

	
	if((result=DI_OpenPartition(partition_table[1].offset)))
		printf("Failed to open partition:%d\n",result);
	_ready=true;
}

DVDISO::~DVDISO()
{
	DI_Reset();
	DI_StopMotor();
	DI_Close();
	DI_Eject();
	_ready=false;
	_region='\0';
}

FileEntry DVDISO::Search(const char* file)
{
	static FileEntry dummy;
	dummy.Len = UINT_MAX;

	const u8* fst=NULL;
	switch(_region)
	{
	case 'E':
		fst=fst_U;
		break;
	case 'J':
		fst=fst_J;
		break;
	}
	FileEntry* root=(FileEntry*)fst,*cur=root;
	u32 offNames=root->Len*sizeof(FileEntry);

	char buf[256];
	strcpy(buf,file);
	char* toSearch = strtok(buf, "/"); 
	char lowerBuf1[256];
	while((u32)cur<(u32)(fst+offNames))
	{
		int offset=(cur->NameOffset[0]<<16)+(cur->NameOffset[1]<<8)+cur->NameOffset[2];
		memset(lowerBuf1, 0, sizeof(lowerBuf1));
		ToLower((char*)&fst[offNames + offset], lowerBuf1);
		if(!strcmp(lowerBuf1,toSearch))
		{
			toSearch=strtok(NULL,"/");
			if(toSearch==NULL)
				break;
			ToLower(toSearch, toSearch);
		}
		else if(cur->Type==1&&cur!=root)//directory
		{
			cur=(FileEntry*)((u32)root+cur->Len*sizeof(FileEntry));
			continue;
		}
		cur++;
	}
	if (((u32)cur) >= (u32)(fst + offNames))
		return dummy;
	return *cur;
}

int DVDISO::Size(const char* file)
{
	FileEntry result=Search(file);
	return result.Len == UINT_MAX ? -1 : (int)result.Len;
}

int DVDISO::Read(const char* file,int offsetInFile,unsigned char* buffer,int bufLen)
{
	FileEntry result=Search(file);
	if(result.Len==UINT_MAX)return -1;
	/*unsigned char* temp[1024] __attribute__((aligned(32)));
	int read = 0;
	while (read<bufLen)
	{
		int toRead = (bufLen - read < sizeof(temp)) ? (bufLen - read) : sizeof(temp);
		DI_Read(temp,toRead ,result.Offset+ offsetInFile + read);
		memcpy(buffer + read, temp, toRead);
		read += toRead;
	}
	return read;*/
	return DI_Read(buffer, bufLen, result.Offset + offsetInFile);
}

bool DVDISO::IsReady(){return _ready;}

void* DVDISO::Malloc(size_t size)
{
	return memalign(32,size);
}