#ifndef _PATCHER_H_
#define _PATCHER_H_

#include "ISOInterface.h"
#include "DVDISO.h"

#include <stdio.h>
#include <stdlib.h>
#include <gctypes.h>
#include <dirent.h>
#include <debug.h>
#include <vector>
#include <memory>
#include <assert.h>
#include <malloc.h>
#include <di/di.h>
#include "ApplyPatch.h"
#include "Patcher.h"
#include "DVDISO.h"
#include "7z\7ZipFile.h"
#include "FrozenMemories.h"
#include "7z\CreateSubfolder.h"
#include "tinyxml2.h"
#include "../FileHolder.h"
#include "../Common.h"
#include "md5.h"
#include "../Launcher/wdvd.h"
#include "../Launcher/disc.h"



bool PMPatch(const char* patchfile, wchar_t * currentInfoText, bool &bForceCancel, f32 &fProgressPercentage);
bool PMPatchVerify(const char* sPatchFilePath, wchar_t * sCurrentInfoText, bool &bForceCancel, f32 &fProgressPercentage);
void WriteHash(const byte* src, int len, char result[32]);
void WriteHash(const char* src, char result[32]);
#endif