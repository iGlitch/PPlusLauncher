#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "textures.h"
#include "../Common.h"

#define MAX_TPL_SIZE (1 * 1024 * 1024)

static char texturesBasePath[ISFS_MAXPATH] = "";
static TPLFile texturesTPL;
static void *texturesData = NULL;
static bool texturesLoaded = false;

GXTexObj logoTexture = {};
GXTexObj backgroundTileTexture = {};
GXTexObj stylishmTexture = {};
GXTexObj menuPlayTexture = {};
GXTexObj menuUpdateTexture = {};
GXTexObj menuAddonsTexture = {};
GXTexObj menuSettingsTexture = {};
GXTexObj menuExitTexture = {};
GXTexObj addonInstalled = {};
GXTexObj addonConflict = {};
GXTexObj addonNotInstalled = {};

enum TextureIndex {
	TEX_logo = 0,
	TEX_backgroundtile,
	TEX_stylishm,
	TEX_en_menu_play,
	TEX_en_menu_update,
	TEX_en_menu_addons,
	TEX_en_menu_settings,
	TEX_en_menu_exit,
	TEX_addon_installed,
	TEX_addon_conflict,
	TEX_addon_not_installed
};

// Helper function to get texture pointer array (eliminates duplication)
static GXTexObj** getTexturePointers(u32 *outCount)
{
	static GXTexObj* textures[] = {
		&logoTexture, &backgroundTileTexture, &stylishmTexture,
		&menuPlayTexture, &menuUpdateTexture, &menuAddonsTexture,
		&menuSettingsTexture, &menuExitTexture,
		&addonInstalled, &addonConflict, &addonNotInstalled
	};
	if (outCount) *outCount = sizeof(textures) / sizeof(textures[0]);
	return textures;
}

void setTexturesBasePath(const char *exePath)
{
	if (!exePath) return;

	const char *lastSlash = strrchr(exePath, '/');
	if (lastSlash)
	{
		size_t len = lastSlash - exePath + 1;
		if (len >= sizeof(texturesBasePath))
			len = sizeof(texturesBasePath) - 1;
		memcpy(texturesBasePath, exePath, len);
		texturesBasePath[len] = '\0';
	}
}

static void* loadTPLToMemory(const char *filename, u32 *outSize)
{
	char tplPath[ISFS_MAXPATH];
	snprintf(tplPath, sizeof(tplPath), "%s%s", texturesBasePath, filename);

	FILE *f = fopen(tplPath, "rb");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	long fileSize_l = ftell(f);
	fseek(f, 0, SEEK_SET);

	// Sanity check: reject invalid sizes or files larger than 1MB
	if (fileSize_l <= 0 || fileSize_l > MAX_TPL_SIZE)
	{
		fclose(f);
		return NULL;
	}
	u32 fileSize = (u32)fileSize_l;

	u8 *data = (u8*)memalign(32, fileSize);
	if (!data)
	{
		fclose(f);
		return NULL;
	}

	if (fread(data, 1, fileSize, f) != fileSize)
	{
		free(data);
		fclose(f);
		return NULL;
	}
	fclose(f);

	// Flush cache with 32-byte aligned size for safety
	DCFlushRange(data, (fileSize + 31) & ~31);
	if (outSize) *outSize = fileSize;
	return data;
}

int loadTextures()
{
	// Prevent double-load memory leak
	if (texturesLoaded) return 0;

	u32 fileSize;
	texturesData = loadTPLToMemory("textures.tpl", &fileSize);
	if (!texturesData) return -1;

	TPL_OpenTPLFromMemory(&texturesTPL, texturesData, fileSize);

	u32 count;
	GXTexObj** textures = getTexturePointers(&count);

	for (u32 i = 0; i < count; i++)
	{
		if (TPL_GetTexture(&texturesTPL, i, textures[i]) != 0)
		{
			// Cleanup on failure
			TPL_CloseTPLFile(&texturesTPL);
			free(texturesData);
			texturesData = NULL;
			return -1;
		}
	}

	texturesLoaded = true;
	return 0;
}

bool isTexturesLoaded()
{
	return texturesLoaded;
}

void unloadTextures()
{
	if (!texturesLoaded) return;

	TPL_CloseTPLFile(&texturesTPL);
	if (texturesData)
	{
		free(texturesData);
		texturesData = NULL;
	}

	u32 count;
	GXTexObj** textures = getTexturePointers(&count);
	for (u32 i = 0; i < count; i++)
		memset(textures[i], 0, sizeof(GXTexObj));

	texturesLoaded = false;
}
