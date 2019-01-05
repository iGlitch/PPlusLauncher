#ifndef ADDONFILE_H
#define ADDONFILE_H

#ifdef HW_RVL
#include <memory>
#include <vector>
#include <gctypes.h>
#elif WIN32
#include <memory>
#include <vector>
#endif
#include <string.h>
#include <stdio.h>
#include "7z\7ZipFile.h"


enum AddonFileState
{
	ADDON_FILE_STATE_UNKNOWN = -1,
	ADDON_FILE_STATE_NOT_INSTALLED = 0,
	ADDON_FILE_STATE_INSTALLED = 1,
	ADDON_FILE_STATE_ONLINE = 2,
	ADDON_FILE_STATE_INCOMPLETE = 3,
	ADDON_FILE_STATE_CONFLICTING = 4,
	ADDON_FILE_STATE_UPGRADE = 5,
};

class AddonFile
{
public:

	AddonFile(const char* filePath);
	~AddonFile();
	int Install();
	int Uninstall();
	bool Download();
	bool Upgrade();
	int CheckState();
	const char* FilePath;
	char title[64];
	char code[8];
	int open();
	f32 version;
	bool isLocal;

	AddonFileState state;
	SzFile* sZip;
private:		
	bool logChangedFile(const char * fileName);
	bool logRestoredFile(const char * fileName);
	int checkIfChanged(const char * fileName);
	std::vector<char *> files;
};


#endif