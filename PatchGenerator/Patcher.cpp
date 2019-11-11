// Patcher.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
#include "stdafx.h"
#include "bsdiff.h"

//#include "xdelta3.h"
#include "xdelta3.c"

//#include "zlib\include\zlib.h"
#include "Directory.h"
//#include "7-zip32.h"
#include "tinyxml2.h"
#include "ARC.h"
#include "md5.h"

#include <Shlwapi.h>
#include <shlobj.h>
#include <queue>
#include <memory>
#include <string>
#include <time.h>
#include <process.h>
#include <algorithm>
#include <atlstr.h>
#include <sys/stat.h>

bool usebsdiff = true;

//#include <Windows.h>

//#define fail(str, ...){ wprintf(_T(str), __VA_ARGS__);system("pause");return 0;}
inline void fail(const char* str, ...)
{
	va_list list;
	va_start(list, str);
	TCHAR buffer[500];
	mbstowcs_s(NULL, buffer, str, 500);
	vwprintf(buffer, list);
	va_end(list);
	system("pause");
}
#define TIMEOUT 60*5

typedef std::basic_string<TCHAR> tstring;

#define MAX_RESOURCE_COUNT 100
#ifdef _WIN32
#pragma pack(1)
#endif
typedef struct{
	char DiffName[16];
	unsigned char DecompSize[8];//decompressed size(not built)
	unsigned char CompSize[8];//compressed size
	unsigned char FinalSize[8];//final size
	unsigned char ChildrenCount;
	unsigned char pad;
	unsigned char Compressed[MAX_RESOURCE_COUNT];//hard code
	unsigned int Sizes[MAX_RESOURCE_COUNT];
}PatchHeader
#ifdef HW_RVL
__attribute__((packed))
#endif
;

struct Context{
	//z_stream* strm;
	uint8_t* buffer;
	int read;
};


template<typename T>
void Invoke(void* arg)
{
	(*((T*)arg))();
}

void WStrToLower(const TCHAR* src, TCHAR* buf)
{
	int len = lstrlenW(src);
	for (int i = 0; i < len; i++)
		buf[i] = towlower(src[i]);
}

enum PatchResult
{
	PatchResult_Fail = -1,
	PatchResult_bsdiff_Arc = 0,
	PatchResult_xdelta3_Arc = 1,
	PatchResult_bsdiff_Raw = 2,
	PatchResult_xdelta3_Raw = 3
};
static void offtout(off_t x, u_char *buf)
{
	off_t y;

	if (x < 0) y = -x; else y = x;

	buf[0] = y % 256; y -= buf[0];
	y = y / 256; buf[1] = y % 256; y -= buf[1];
	y = y / 256; buf[2] = y % 256; y -= buf[2];
	y = y / 256; buf[3] = y % 256; y -= buf[3];
	y = y / 256; buf[4] = y % 256; y -= buf[4];
	y = y / 256; buf[5] = y % 256; y -= buf[5];
	y = y / 256; buf[6] = y % 256; y -= buf[6];
	y = y / 256; buf[7] = y % 256;

	if (x < 0) buf[7] |= 0x80;
}
PatchResult Patch(_TCHAR* original, _TCHAR* modified, _TCHAR* output, bool forceXDelta3)
{

	PatchResult result = PatchResult_Fail;
	auto origF = CreateFile(
		original,
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (origF == INVALID_HANDLE_VALUE)
		fail("Failed to open %s\n", original);
	auto modF = CreateFile(
		modified,
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (modF == INVALID_HANDLE_VALUE)
		fail("Failed to open %s\n", modified);


	//open files
	auto origSize = GetFileSize(origF, NULL);
	auto modSize = GetFileSize(modF, NULL);
	auto origBuf = (uint8_t*)malloc(origSize * 4);//file size is lower than 4GB
	auto modBuf = (uint8_t*)malloc(modSize * 4);
	//analyze data
	ARCResource origRes = ARCResource(), modRes = ARCResource();
	if (!ReadFile(origF, origBuf, origSize, &origSize, NULL) || !ReadFile(modF, modBuf, modSize, &modSize, NULL))
	{
		printf("Failed to read %s", original);
		return result;
	}
	byte* origPo, *modPo;
	int diffOrigSize = origSize, diffModSize = modSize;
	auto ext = PathFindExtension(original);
	bool isArc = false;
	byte *arcDetailHeader;
	int archDetailHeaderSize = 0;

	if (false && (StrCmpI(ext, _T(".pac")) == 0 || StrCmpI(ext, _T(".pcs")) == 0))//won't be used
	{
		isArc = true;
		origRes.Load(origBuf, origSize);
		modRes.Load(modBuf, modSize);
		origPo = origRes.Data;
		modPo = modRes.Data;
		diffOrigSize = origRes.Size;
		diffModSize = modRes.Size;


		unsigned char decompSize[8];
		unsigned char compSize[8];
		unsigned char finalSize[8];

		offtout(modRes.Size, decompSize);
		offtout(modRes.FinalCmpSize, compSize);
		offtout(modSize, finalSize);

		unsigned char childrenCount = modRes.Compressed.size();
		unsigned char headerSize[8];
		archDetailHeaderSize = (8 * sizeof(unsigned char)) //headersize
			+ (8 * sizeof(unsigned char)) //decompSize
			+ (8 * sizeof(unsigned char)) //compSize
			+ (8 * sizeof(unsigned char)) //finalSize
			+ (1 * sizeof(unsigned char)) //ChildrenCount
			+ (childrenCount * sizeof(unsigned char)) //compressed
			+ (childrenCount * 8) //sizes
			;
		off_t headerPosition = 0;
		arcDetailHeader = (byte*)malloc(archDetailHeaderSize);
		offtout(archDetailHeaderSize, headerSize);
		memcpy(arcDetailHeader + headerPosition, headerSize, 8);
		headerPosition += 8 * sizeof(unsigned char);
		memcpy(arcDetailHeader + headerPosition, decompSize, 8);
		headerPosition += 8 * sizeof(unsigned char);
		memcpy(arcDetailHeader + headerPosition, compSize, 8);
		headerPosition += 8 * sizeof(unsigned char);
		memcpy(arcDetailHeader + headerPosition, finalSize, 8);
		headerPosition += 8 * sizeof(unsigned char);

		//Data for decompression
		arcDetailHeader[headerPosition] = childrenCount;
		headerPosition++;
		for (int i = 0; i < childrenCount; i++)
		{
			arcDetailHeader[headerPosition] = (unsigned char)modRes.Compressed[i];
			headerPosition++;
		}
		for (int i = 0; i < childrenCount; i++)
		{
			unsigned char expectedSize[8];
			offtout(modRes.ExpSize[i], expectedSize);

			memcpy(arcDetailHeader + headerPosition, expectedSize, 8);
			headerPosition += 8;
		}
	}
	else
	{
		origPo = origBuf;
		modPo = modBuf;
	}

	char outputFile[500];
	wcstombs_s(NULL, outputFile, 500, output, 500);
	char bsdiffOutput[500];
	sprintf_s(bsdiffOutput, "%s%s", outputFile, ".bsdiff");
	char xd3Output[500];
	sprintf_s(xd3Output, "%s%s", outputFile, ".xdelta3");
	FILE * pf;

	if (!forceXDelta3 && usebsdiff)
	{
		if ((fopen_s(&pf, bsdiffOutput, "wb")) != NULL)
			return PatchResult_Fail;//; err(1, "%s", argv[3]);
		if (isArc)
			fwrite(arcDetailHeader, sizeof(byte), archDetailHeaderSize, pf);

		if (bsdiff(origPo, diffOrigSize, modPo, diffModSize, pf, archDetailHeaderSize) == 0)
			result = (isArc ? PatchResult_bsdiff_Arc : PatchResult_bsdiff_Raw);
	}
	else
	{
		int r, ret;
		xd3_stream stream;
		xd3_config config;
		xd3_source source;

		memset(&stream, 0, sizeof (stream));
		memset(&source, 0, sizeof (source));

		xd3_init_config(&config, XD3_NOCOMPRESS);
		config.winsize = origSize * 3;

		//config.flags |= XD3_NOCOMPRESS;
		//config.smatch_cfg = XD3_SMATCH_FASTEST;
		xd3_config_stream(&stream, &config);


		source.blksize = diffOrigSize;
		source.curblk = origPo;

		/* Load 1st block of stream. */
		source.onblk = diffOrigSize;
		source.curblkno = 0;
		/* Set the stream. */
		xd3_set_source(&stream, &source);

		if ((fopen_s(&pf, xd3Output, "wb")) != NULL)
			return PatchResult_Fail;//; err(1, "%s", argv[3]);
		if (isArc)
			fwrite(arcDetailHeader, sizeof(byte), archDetailHeaderSize, pf);

		bool first = true;
		bool finished = false;
		do
		{
			if (first)
			{
				xd3_avail_input(&stream, modPo, diffModSize);
				first = false;
			}
			else
			{
				xd3_set_flags(&stream, XD3_FLUSH | stream.flags);
				xd3_avail_input(&stream, modPo, 0);
			}

		process:

			ret = xd3_encode_input(&stream);

			switch (ret)
			{
			case XD3_OUTPUT:

				//fprintf(stderr, "XD3_OUTPUT\n");				
				r = fwrite(stream.next_out, 1, stream.avail_out, pf);
				if (r != (int)stream.avail_out)
				{
					finished = true;
					break;
				}
				xd3_consume_output(&stream);
				goto process;
			case XD3_INPUT:
			case XD3_GOTHEADER:
			case XD3_WINSTART:
				goto process;
			case XD3_GETSRCBLK:
				//fail
				continue;
				break;
			case XD3_WINFINISH:
				//fprintf(stderr, "XD3_WINFINISH\n");
				result = (isArc ? PatchResult_xdelta3_Arc : PatchResult_xdelta3_Raw);
				finished = true;
				break;;
			default:
				//fail
				finished = true;;
			}

		} while (!finished);
		xd3_close_stream(&stream);
		xd3_free_stream(&stream);
	}
	if (isArc)
		free(arcDetailHeader);
	free(origBuf);
	free(modBuf);
	fclose(pf);
	CloseHandle(origF);
	CloseHandle(modF);
	return result;


}

template<typename T>
bool Contains(const T* src, const T* str, int srcLen, int strLen)
{
	for (int i = 0; i < srcLen - strLen + 1; i++)
	for (int j = 0; j < strLen; j++)
	{
		if (src[i + j] != str[j])
			break;
		if (j == strLen - 1)return true;
	}
	return false;
}

TCHAR* GetFileName(TCHAR* path)
{
	int len = lstrlen(path);
	int origin = 0;
	for (int i = 0; i < len; i++)
	{
		if (path[i] == _T('/') || path[i] == _T('\\'))
			origin = i + 1;
	}
	return path + origin;
}

TCHAR* GetExtension(TCHAR* path)
{
	int len = lstrlen(path);
	int origin = 0;
	for (int i = 0; i < len; i++)
	if (path[i] == _T('.'))
		return path + i;
	return NULL;
}

const char* GetFileName(const char* path)
{
	int len = strlen(path);
	int origin = 0;
	for (int i = 0; i < len; i++)
	{
		if (path[i] == '/' || path[i] == '\\')
			origin = i + 1;
		else if (path[i] == '.')
			return path + origin;
	}
	return NULL;
}

void WriteHash(const TCHAR* src, char result[33])
{
	FILE* f = NULL;
	_wfopen_s(&f, src, _T("rb"));
	int len = 0;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::shared_ptr<byte> buf((byte*)malloc(len));
	fread_s(buf.get(), len, len, 1, f);
	fclose(f);
	byte hash[16];
	md5_state_t state;
	md5_init(&state);
	md5_append(&state, buf.get(), len);
	md5_finish(&state, hash);

	memset(result, 0, 33);
	for (int i = 0; i < 16; i++)
		sprintf_s(result + i * 2, 3, "%02x", hash[i]);
}

void ReplYen(char* path)
{
	int len = strlen(path);
	for (int i = 0; i < len; i++)
	if (path[i] == '\\')
		path[i] = '/';
}

bool ContainsNum(const TCHAR* src)
{
	int srcLen = lstrlen(src);
	for (int i = 0; i < 10; i++)
	{
		TCHAR buf[2];
		buf[0] = buf[1] = _T('\0');
		_itow_s(i, buf, 2, 10);
		if (Contains<TCHAR>(src, buf, srcLen, 1))
			return true;
	}
	return false;
}

bool ContainsNum(const char* src)
{
	int srcLen = strlen(src);
	for (int i = 0; i < 10; i++)
	{
		char buf[2];
		buf[0] = buf[1] = '\0';
		_itoa_s(i, buf, 2, 10);
		if (Contains<char>(src, buf, srcLen, 1))
			return true;
	}
	return false;
}

void Replace(const TCHAR* target, const  TCHAR* search, const TCHAR* with, TCHAR* buf, int buflen)
{
	int tlen = lstrlen(target);
	int slen = lstrlen(search);
	int wlen = lstrlen(with);
	int index = -1;
	for (int i = 0; i < tlen - slen + 1; i++)
	for (int j = 0; j < slen; j++)
	if (target[i + j] != search[j])
		break;
	else if (j == slen - 1)
		index = i;
	if (index < 0)
		return;
	StrCpyW(buf, target);
	StrCpyW(buf + index, with);
	StrCpyW(buf + index + wlen, target + index + slen);
	//wmemset(buf + tlen + wlen - slen, _T('\0'), buflen - (tlen + wlen - slen));
}

int FindFileIndex(const TCHAR* name, vector<TCHAR*> from)
{
	TCHAR filename[500];
	TCHAR tmpFilename[500];

	wmemset(filename, _T('\0'), 500);
	StrCpyW(filename, name);

	wmemset(tmpFilename, _T('\0'), 500);
	StrCpyW(tmpFilename, name);

	//tolower
	int nameLen = lstrlen(filename);
	for (int k = 0; k < nameLen; k++)
	{
		filename[k] = towlower(filename[k]);
		tmpFilename[k] = filename[k];
	}
	int tmpNameLen = nameLen;



	int tmpFileIndex = -1;
	//new characters
	TCHAR* newNames[] = { _T("isaac"), _T("knuckles"), _T("lyn"), _T("mewtwo"), _T("roy"), _T("sami") };
	TCHAR* origNames[] = { _T("toonlink"), _T("sonic"), _T("pit"), _T("lucario"), _T("marth"), _T("snake") };
	for (int j = 0; j < _countof(newNames); j++)
	if (Contains<TCHAR>(tmpFilename, newNames[j], tmpNameLen, lstrlen(newNames[j])))
	{
		TCHAR temp[500];
		wmemset(temp, _T('\0'), 500);
		Replace(tmpFilename, newNames[j], origNames[j], temp, 500);
		wmemset(tmpFilename, _T('\0'), 500);
		lstrcpy(tmpFilename, temp);
		tmpNameLen += (lstrlen(origNames[j]) - lstrlen(newNames[j]));
		tmpFileIndex = FindFileIndex(tmpFilename, from);
	}

	bool fitModel = false;// Contains<TCHAR>(filename, _T("fit"), nameLen, 3) && ContainsNum(filename);
	auto ext = GetExtension(filename);

	int index = -1;
	//search
	auto origLen = from.size();
	for (int j = 0; j < (int)origLen; j++)
	{
		auto curname = GetFileName(from[j]);
		//tolower
		int curLen = lstrlen(curname);
		for (int k = 0; k < curLen; k++)
			curname[k] = towlower(curname[k]);

		if (lstrcmp(curname, filename) == 0)
		{
			index = j;
			break;
		}
		if (Contains<TCHAR>(curname, _T("_en"), curLen, 3))
		{
			TCHAR withoutEn[500];
			wmemset(withoutEn, _T('\0'), 500);
			wmemcpy_s(withoutEn, 500, curname, curLen - 7);//extension+_en

			if (Contains<TCHAR>(curname, _T(".pac"), curLen, 4))
				StrCatW(withoutEn, _T(".pac"));
			else if (Contains<TCHAR>(curname, _T(".pcs"), curLen, 4))
				StrCatW(withoutEn, _T(".pcs"));
			if (lstrcmp(withoutEn, filename) == 0)
			{
				index = j;
				break;
			}
		}
	}

	if (index < 0)
	{
		//alt costumes?
		for (int j = 0; j < (int)origLen; j++)
		{
			auto curname = GetFileName(from[j]);
			//tolower
			int curLen = lstrlen(curname);
			for (int k = 0; k < curLen; k++)
				curname[k] = towlower(curname[k]);
			if (fitModel&&Contains<TCHAR>(curname, _T("fit"), curLen, 3) && ContainsNum(curname) && (StrCmpW(ext, GetExtension(curname)) == 0))
			{
				TCHAR withoutNumOrig[500];
				TCHAR withoutNumMod[500];
				wmemset(withoutNumOrig, _T('\0'), 500);
				wmemset(withoutNumMod, _T('\0'), 500);
				wmemcpy_s(withoutNumOrig, 500, filename, nameLen - 6);
				wmemcpy_s(withoutNumMod, 500, curname, curLen - 6);
				if (lstrcmp(withoutNumMod, withoutNumOrig) == 0)
				{
					index = j;
					break;
				}
			}
		}
	}

	//temporary fix for mu_menumain
	/*if (Contains<TCHAR>(filename, _T("mu_menumain"), nameLen, 11))
		index = -1;*/
	if (index == -1)
		return tmpFileIndex;
	return index;
}

int _tmain(int argc, _TCHAR* argv[])
{
	time_t now = time(NULL);
	struct tm pnow;
	localtime_s(&pnow, &now);

	//get replace table
	tinyxml2::XMLDocument doc;
	doc.LoadFile("repl.xml");
	auto root = doc.RootElement();
	if (!root)
	{
		root = doc.NewElement("root");
		doc.InsertEndChild(root);
	}
	tinyxml2::XMLDocument reverseDoc;
	auto rRoot = reverseDoc.NewElement("update");
	reverseDoc.InsertEndChild(rRoot);

	char dstDir[500];
	memset(dstDir, 0, 500);
	int ignoreExisting = 0;
	int xmlonly = 0;
	int isBaseDvd = 0;

	TCHAR origTemp[500];//previous Version files path
	TCHAR modTemp[500];//Current Version files path
	TCHAR targetTemp[500];//Output path
	TCHAR dvdTemp[500];//DVD filesystem path
	wmemset(origTemp, _T('\0'), 500);
	wmemset(modTemp, _T('\0'), 500);
	wmemset(targetTemp, _T('\0'), 500);
	wmemset(dvdTemp, _T('\0'), 500);
	//load settings
	FILE* setting = NULL;
	_wfopen_s(&setting, argv[1], _T("r"));
	if (!setting)
		fail("Couldn't find argument file.\n");
	//read line by line
	fgetws(origTemp, 500, setting);
	origTemp[lstrlen(origTemp) - 1] = '\0';//original files path
	fgetws(modTemp, 500, setting);
	modTemp[lstrlen(modTemp) - 1] = '\0';//changed files path
	fgets(dstDir, 500, setting);
	dstDir[strnlen_s(dstDir, 500) - 1] = '\0';
	mbstowcs_s(NULL, targetTemp, dstDir, 500);
	fgetws(dvdTemp, 500, setting);
	dvdTemp[lstrlen(dvdTemp) - 1] = '\0';
	fscanf_s(setting, "%d\n", &ignoreExisting, sizeof(int));
	fscanf_s(setting, "%d", &xmlonly, sizeof(int));
	fscanf_s(setting, "%d", &isBaseDvd, sizeof(int));
	fclose(setting);
	Directory modDir(modTemp);
	Directory origDir(origTemp);
	Directory dvdDir(dvdTemp);

	auto modFiles = modDir.AllFilesBelow();
	auto origFiles = origDir.AllFilesBelow();
	auto dvdFiles = dvdDir.AllFilesBelow();
	auto modRelat = modDir.AllRelativeFilepath();
	auto origRelat = origDir.AllRelativeFilepath();
	auto dvdRelat = dvdDir.AllRelativeFilepath();

	modDir.CopyDirectoryStructure(targetTemp);
	int len = modFiles.size();
	int origLen = origFiles.size();
	for (int i = 0; i < len; i++)
	{
		bool hashedBaseFiles = false;
		bool hashedModFiles = false;
		char oldVersionHash[33];
		char newVersionHash[33];

		TCHAR dst[500];
		wsprintf(dst, _T("%s\\%s"), targetTemp, modRelat[i]);
		auto AddFile = [&doc, &modFiles, i, &root, &reverseDoc, &modRelat, &rRoot, &dst, &ignoreExisting, &hashedModFiles, &newVersionHash](void)
		{
			//wprintf_s(_T("Adding: %s\n"), modRelat[i]);
			char charname[500];
			wcstombs_s(NULL, charname, 500, GetFileName(modFiles[i]), 500);
			if (charname[0] >= '0'&&charname[0] <= '9')
			{
				char temp[500];
				strcpy_s(temp, charname);
				charname[0] = '_';
				strcpy_s(charname + 1, 499, temp);
			}
			auto newElem = doc.NewElement(charname);
			newElem->SetAttribute("base", GetFileName(charname[0] == '_' ? (charname + 1) : charname));
			root->InsertEndChild(newElem);


			char charPatchRelat[500];
			wcstombs_s(NULL, charPatchRelat, 500, modRelat[i], 500);
			ReplYen(charPatchRelat);

			char charDestination[500] = "/";
			wcstombs_s(NULL, charDestination + 1, 500 - 1, modRelat[i], 500 - 1);
			ReplYen(charDestination);

			auto rNewElem = reverseDoc.NewElement("file");
			rNewElem->SetAttribute("method", "add");

			rNewElem->SetAttribute("updateFile", charPatchRelat);

			rNewElem->SetAttribute("destinationFile", charDestination);
			if (!hashedModFiles)
			{
				char hash[33];
				WriteHash(modFiles[i], hash);
				rNewElem->SetAttribute("destinationMD5", hash);
			}
			else
				rNewElem->SetAttribute("destinationMD5", newVersionHash);
				
			hashedModFiles = true;




			rRoot->InsertEndChild(rNewElem);

			//copy file
			if (!ignoreExisting || PathFileExists(dst))
				CopyFile(modFiles[i], dst, false);

			wprintf_s(_T("Added\n"));

		};


		TCHAR name[500];
		wmemset(name, _T('\0'), 500);
		TCHAR tcharBuffer[500];


		//repl table search
		char buffer[500];
		wcstombs_s(NULL, buffer, 500, GetFileName(modFiles[i]), 500);
		if (buffer[0] >= '0'&&buffer[0] <= '9')
		{
			char temp[500];
			strcpy_s(temp, buffer);
			buffer[0] = '_';
			strcpy_s(buffer + 1, 499, temp);
		}
		tinyxml2::XMLElement* elem = NULL;
		if (elem = root->FirstChildElement(buffer))
		{
			mbstowcs_s(NULL, tcharBuffer, elem->Attribute("base"), 500);
			lstrcpy(name, GetFileName(tcharBuffer));
		}
		else
			lstrcpy(name, GetFileName(modFiles[i]));

		auto dvd = false;
		auto index = FindFileIndex(name, origFiles);
		if (false) // index < 0) //no disc required
		{
			dvd = true;
			index = FindFileIndex(name, dvdFiles);
		}

		if (isBaseDvd)
			dvd = true;
		wprintf_s(_T("%s: "), modRelat[i]);

		if (index < 0)
		{
			//if (!Contains<TCHAR>(name, _T("--"), lstrlen(name), 2))
			if (!xmlonly)
				AddFile();

			continue;
		}
		else
		{
			auto baseFiles = dvd ? dvdFiles : origFiles;
			auto baseRelat = dvd ? dvdRelat : origRelat;

			//create patch
			if (!dvd && PathFileExists(modFiles[i]))
			{
				WriteHash(baseFiles[index], oldVersionHash);
				hashedBaseFiles = true;
				WriteHash(modFiles[i], newVersionHash);
				hashedModFiles = true;
				if (strcmp(oldVersionHash, newVersionHash) == 0) //identical
				{
					wprintf_s(_T("Skipped\n"));
					continue;

				}
			}

			bool forceXDelta3 = false;// lstrcmpi(_T("STGKART.pac"), name) == 0;


			PatchResult presult = PatchResult_Fail;
			if ((!ignoreExisting || !PathFileExists(dst))
				&& !xmlonly
				& ((presult = Patch(baseFiles[index], modFiles[i], dst, forceXDelta3)) == PatchResult_Fail))
			{
				AddFile();
			}
			else
			{
				wprintf_s(_T("Patched\n"));
				//success
				char sourceLocation[5];
				char charOrigRelat[500];
				if (dvd)
				{
					sprintf_s(sourceLocation, "disc");
					wcstombs_s(NULL, charOrigRelat, 500, baseRelat[index], 500);

				}
				else
				{
					sprintf_s(sourceLocation, "sd");
					sprintf_s(charOrigRelat, "/");
					wcstombs_s(NULL, charOrigRelat + 1, 500 - 1, baseRelat[index], 500 - 1);
				}


				char charPatchRelat[500];
				wcstombs_s(NULL, charPatchRelat, 500, modRelat[i], 500);
				char newPatchFile[500];
				switch (presult)
				{
				case PatchResult_bsdiff_Arc:
				case PatchResult_bsdiff_Raw:
					sprintf_s(newPatchFile, "%s.bsdiff", charPatchRelat);
					break;
				case PatchResult_xdelta3_Arc:
				case PatchResult_xdelta3_Raw:
					sprintf_s(newPatchFile, "%s.xdelta3", charPatchRelat);
					break;
				default:
					break;
				}
				char charModRelat[500] = "/";
				wcstombs_s(NULL, charModRelat + 1, 500 - 1, modRelat[i], 500 - 1);

				ReplYen(charOrigRelat);
				ReplYen(newPatchFile);
				ReplYen(charModRelat);

				auto rNewElem = reverseDoc.NewElement("file");
				rNewElem->SetAttribute("method", "patch");
				rNewElem->SetAttribute("patchType", (int)presult);


				rNewElem->SetAttribute("sourceLocation", sourceLocation);

				rNewElem->SetAttribute("sourceFile", charOrigRelat);
				//get size of mod file
				FILE* fp = NULL;
				_wfopen_s(&fp, baseFiles[index], _T("rb"));
				fseek(fp, 0, SEEK_END);
				int len = ftell(fp);
				fclose(fp);
				rNewElem->SetAttribute("sourceLength", len);
				//get hash of original file

				if (!hashedBaseFiles)
					WriteHash(baseFiles[index], oldVersionHash);
				hashedBaseFiles = true;
				rNewElem->SetAttribute("sourceMD5", oldVersionHash);

				rNewElem->SetAttribute("updateFile", newPatchFile);

				//get hash of mod file

				if (!hashedBaseFiles)
					WriteHash(modFiles[i], newVersionHash);
				hashedModFiles = true;
				rNewElem->SetAttribute("destinationFile", charModRelat);
				rNewElem->SetAttribute("destinationMD5", newVersionHash);
				rRoot->InsertEndChild(rNewElem);
			}
		}
	}

	//check deleted files
	if (StrCmpW(origTemp, dvdTemp))
	for (int i = 0; i < origLen; i++)
	{
		int index = FindFileIndex(GetFileName(origFiles[i]), modFiles);
		if (index < 0)
		{
			char charDestination[500] = "/";
			wcstombs_s(NULL, charDestination + 1, 500 - 1, origRelat[i], 500 - 1);
			ReplYen(charDestination);
			auto rNewElem = reverseDoc.NewElement("file");
			rNewElem->SetAttribute("method", "delete");//this can be anything
			char hash[33];
			memset(hash, 0, 33);
			WriteHash(origFiles[i], hash);
			rNewElem->SetAttribute("destinationFile", charDestination);
			rNewElem->SetAttribute("destinationMD5", hash);
			rRoot->InsertEndChild(rNewElem);
		}
	}

	doc.SaveFile("repl.xml");

	char xmlPath[400];
	sprintf_s(xmlPath, 400, "%s\\update.xml", dstDir);
	reverseDoc.SaveFile(xmlPath);
	printf("Created all the patch files\n");
	if (!xmlonly)
	{
		printf("Compressing patch files\n");
		char request[400];
		sprintf_s(request, 400, "a -t7z %s\\..\\patch.7z %s\\* -mx=9 -ms=off", dstDir, dstDir);
		char output[1000];
		//SevenZip(NULL,request,output,1000);//this works worse than 7-zip.exe
	}

	time_t fin = time(NULL);
	struct tm pfin;
	localtime_s(&pfin, &fin);
	//not precise though
	printf("Elapsed time: %d min %d sec", pfin.tm_min - pnow.tm_min, pfin.tm_sec - pnow.tm_sec);

	return 0;
}

