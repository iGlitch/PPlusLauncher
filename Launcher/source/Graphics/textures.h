#ifndef _TEXTURES_H_
#define _TEXTURES_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gccore.h>

void loadTextures();

extern GXTexObj logoTexture;
extern GXTexObj menuAboutTexture;
extern GXTexObj menuBackTexture;
extern GXTexObj menuExitTexture;
extern GXTexObj menuInstallTexture;
extern GXTexObj menuInstallChannelTexture;
extern GXTexObj menuPlayTexture;
extern GXTexObj menuRepairFilesTexture;
extern GXTexObj menuToolsTexture;
extern GXTexObj menuUpdateTexture;
extern GXTexObj menuAddonsTexture;
extern GXTexObj addonInstalled;
extern GXTexObj addonConflict;
extern GXTexObj addonNotInstalled;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
