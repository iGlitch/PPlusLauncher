#include <gccore.h>
#include "textures.h"

#include "video.h"
#include "tex_logo_tpl.h"
#include "tex_logo.h"
#include "en_tex_menus_tpl.h"
#include "en_tex_menus.h"
#include "tex_addons_tpl.h"
#include "tex_addons.h"


TPLFile logoTPL;
TPLFile menuTexturesTPL;
TPLFile addonTexturesTPL;
GXTexObj logoTexture;
GXTexObj menuAboutTexture;
GXTexObj menuBackTexture;
GXTexObj menuExitTexture;
GXTexObj menuInstallTexture;
GXTexObj menuInstallChannelTexture;
GXTexObj menuPlayTexture;
GXTexObj menuRepairFilesTexture;
GXTexObj menuToolsTexture;
GXTexObj menuUpdateTexture;
GXTexObj menuAddonsTexture;
GXTexObj addonInstalled;
GXTexObj addonConflict;
GXTexObj addonNotInstalled;

void loadTextures()
{
	TPL_OpenTPLFromMemory(&logoTPL, (void *)tex_logo_tpl, tex_logo_tpl_size);
	TPL_GetTexture(&logoTPL, logo, &logoTexture);

	//Language Select
	TPL_OpenTPLFromMemory(&menuTexturesTPL, (void *)en_tex_menus_tpl, en_tex_menus_tpl_size);
	TPL_GetTexture(&menuTexturesTPL, en_menu_about, &menuAboutTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_back, &menuBackTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_exit, &menuExitTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_install, &menuInstallTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_installchannel, &menuInstallChannelTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_play, &menuPlayTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_repairfiles, &menuRepairFilesTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_tools, &menuToolsTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_update, &menuUpdateTexture);
	TPL_GetTexture(&menuTexturesTPL, en_menu_addons, &menuAddonsTexture);

	TPL_OpenTPLFromMemory(&addonTexturesTPL, (void *)tex_addons_tpl, tex_addons_tpl_size);
	TPL_GetTexture(&addonTexturesTPL, addon_installed, &addonInstalled);
	TPL_GetTexture(&addonTexturesTPL, addon_conflict, &addonConflict);
	TPL_GetTexture(&addonTexturesTPL, addon_not_installed, &addonNotInstalled);
}