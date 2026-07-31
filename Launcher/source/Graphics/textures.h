#ifndef _TEXTURES_H_
#define _TEXTURES_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gccore.h>

void setTexturesBasePath(const char *exePath);
int loadTextures();
void unloadTextures();
bool isTexturesLoaded();

extern GXTexObj logoTexture;
extern GXTexObj backgroundTileTexture;
extern GXTexObj stylishmTexture;
extern GXTexObj menuPlayTexture;
extern GXTexObj menuUpdateTexture;
extern GXTexObj menuAddonsTexture;
extern GXTexObj menuSettingsTexture;
extern GXTexObj menuExitTexture;
extern GXTexObj addonInstalled;
extern GXTexObj addonConflict;
extern GXTexObj addonNotInstalled;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
