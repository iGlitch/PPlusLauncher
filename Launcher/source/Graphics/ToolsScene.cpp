#include <stdlib.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "video.h"
#include "ToolsScene.h"
#include "stdlib.h"
#include "FreeTypeGX.h"
#include "..\Audio\sfx.h"
#include "..\Patching\tinyxml2.h"
#include "textures.h"



CToolsScene::CToolsScene(f32 w, f32 h)
{
	m_eNextScreen = SCENE_TOOLS;

	m_fScreenWidth = w;
	m_fScreenHeight = h;
	m_bIsLoaded = false;
	m_iMenuSelectionAnimationFrames = 15;
	m_fMaxNewsScrollFrames = f32(60 * 30);
	m_fCurrentNewsScrollFrame = f32(-1);

};

void CToolsScene::Load()
{
	m_iMenuSelectedIndex = -1;
	m_iMenuSelectionFrame = 0;
	m_iDrawFrameNumber = 0;
	m_sPrevDStickX = s8(0);
	m_sPrevDStickY = s8(0);
	m_eNextScreen = SCENE_TOOLS;

	char projectMVersion[20] = "Unknown";


	tinyxml2::XMLDocument infoDoc;
	if (infoFileSize != 0 && infoDoc.Parse((char *)infoFileData, infoFileSize) == (int)tinyxml2::XML_NO_ERROR)
	{
		tinyxml2::XMLElement* cur = infoDoc.RootElement();
		if (cur)
		{
			cur = cur->FirstChildElement("game");
			if (cur)
			{
				cur = cur->FirstChildElement("version");
				if (cur && cur->FirstChild() && cur->FirstChild()->ToText())
				{
					const char* text = cur->FirstChild()->ToText()->Value();
					sprintf(projectMVersion, text);
				}
			}
		}
	}

	
	showAboutPopup = false;
	aboutPopup = new Popup(screenwidth, screenheight, 0.90f, 0.55f, 0.40f, 0.75f);
	aboutPopup->setAnimationFrames(15, true);
	aboutPopup->setSelectionTextItems(0, 1, L"OK");


	char line2[50];
	sprintf(line2, "Project+ Version: %s", projectMVersion);
	aboutLine2Text = charToWideChar(line2);

	

	//aboutLine2Text = (wchar_t*)malloc(sizeof(wchar_t)* 50);
	//aboutLine3Text = (wchar_t*)malloc(sizeof(wchar_t)* 50);
	//sprintf(aboutLine2Text, "Project+ Version: %s", projectMVersion);
	//sprintf(aboutLine3Text, "Project+ Codeset: %s", projectMCodeset);

	aboutPopup->setLineTextItems(2, L"Launcher Version: 1.0", aboutLine2Text);

	
};

void CToolsScene::Unload()
{
	m_iMenuSelectedIndex = -1;
	m_iMenuSelectionFrame = 0;
	m_iDrawFrameNumber = 0;
	m_sPrevDStickX = s8(0);
	m_sPrevDStickY = s8(0);
	showAboutPopup = false;
	delete aboutPopup;
	free(aboutLine2Text);
	
};

void CToolsScene::HandleInputs(u32 gcPressed, s8 dStickX, s8 dStickY, s8 cStickX, s8 cStickY, u32 wiiPressed)
{
	if (m_iDrawFrameNumber < 60 || m_iMenuSelectedIndex == -1)
		return;

	if (showAboutPopup)
	{
		if (gcPressed & PAD_BUTTON_A || wiiPressed & WPAD_BUTTON_A)
		{
			if (aboutPopup->getSelectedIndex() == 0)
			{
				showAboutPopup = false;
				aboutPopup->reset();
				playSFX(SFX_BACK);
			}
		}
		else if (gcPressed & PAD_BUTTON_B || wiiPressed & WPAD_BUTTON_B)
		{
			showAboutPopup = false;
			aboutPopup->reset();
			playSFX(SFX_BACK);
		}
	}
	else
	{

		if ((dStickY > STICK_DEADZONE && m_sPrevDStickY <= STICK_DEADZONE) || gcPressed & PAD_BUTTON_UP || wiiPressed & WPAD_BUTTON_UP)
		{
			if (m_iMenuSelectedIndex == 0)
				m_iMenuSelectedIndex = 3;
			else
				m_iMenuSelectedIndex--;
			m_iMenuSelectionFrame = 0L;
			playSFX(SFX_SELECT);
		}

		if ((dStickY < -STICK_DEADZONE && m_sPrevDStickY >= -STICK_DEADZONE) || (gcPressed & PAD_BUTTON_DOWN || wiiPressed & WPAD_BUTTON_DOWN)){
			if (m_iMenuSelectedIndex == 3)
				m_iMenuSelectedIndex = 0L;
			else
				m_iMenuSelectedIndex++;
			m_iMenuSelectionFrame = 0L;
			playSFX(SFX_SELECT);
		}

		if (gcPressed & PAD_BUTTON_A || wiiPressed & WPAD_BUTTON_A)
		{
			switch (m_iMenuSelectedIndex)
			{
			case 0:
				//m_eNextScreen = SCENE_CODESET;
				playSFX(SFX_CONFIRM);				
				break;
			case 1:
				break;
			case 2:
				showAboutPopup = true;
				playSFX(SFX_CONFIRM);
				break;
			case 3:
				m_eNextScreen = SCENE_MAIN_MENU;
				playSFX(SFX_CONFIRM);
				break;
			}
		}
		else
		{
			if (gcPressed & PAD_BUTTON_B || wiiPressed & WPAD_BUTTON_B)
			{
				m_iMenuSelectedIndex = 3;
				m_eNextScreen = SCENE_MAIN_MENU;
				playSFX(SFX_BACK);
			}

		}
	}
	m_sPrevDStickX = dStickX;
	m_sPrevDStickY = dStickY;

}

void CToolsScene::Draw()
{
	if (m_iDrawFrameNumber == 60)
		m_iMenuSelectedIndex = 0L;
	if (m_iMenuSelectedIndex != -1 && m_iMenuSelectionFrame < m_iMenuSelectionAnimationFrames)
		m_iMenuSelectionFrame++;

	f32 yPos = 46.0f;
	f32 offset = 0.0F;
	if (m_iDrawFrameNumber < 15)
		offset = 11.5f * (m_iDrawFrameNumber - 15);


	drawNewsBox(yPos, offset, 65);
	
	yPos += 20.0f;

	//MenuItem0
	drawMenuItem(0, yPos, 300, 196, 33, 21, 15, &menuRepairFilesTexture, (m_iMenuSelectedIndex == 0));
	yPos += 70.0f;

	//MenuItem1
	drawMenuItem(0, yPos, 275, 258, 33, 29, 15, &menuInstallChannelTexture, (m_iMenuSelectedIndex == 1));
	yPos += 70.0f;

	//MenuItem2
	drawMenuItem(0, yPos, 250, 100, 33, 37, 15, &menuAboutTexture, (m_iMenuSelectedIndex == 2));
	yPos += 70.0f;

	//MenuItem3
	drawMenuItem(0, yPos, 225, 82, 33, 45, 15, &menuBackTexture, (m_iMenuSelectedIndex == 3));
	yPos += 78.0f;

	if (m_iDrawFrameNumber < 15)
		offset = ((yPos - m_fScreenHeight) / 10.0f) * (m_iDrawFrameNumber - 15);
	else
		offset = 0.0f;
	

	switch (m_iMenuSelectedIndex)
	{
	case 0:
		drawInfoBox(yPos + offset, 36.0F, L"Install Project+. A copy of Super Smash Bros. Brawl is required.");
		break;
	case 1:
		drawInfoBox(yPos + offset, 36.0F, L"Check for updates and use other various Project+ related tools.");
		break;
	case 2:
		drawInfoBox(yPos + offset, 36.0F, L"View version information, credits, and additional info.");
		break;
	case 3:
		drawInfoBox(yPos + offset, 36.0F, L"Return to the Wii System Menu.");
		break;
	default:
		break;
	}
	//if (frameNumber <= 45)
	if (showAboutPopup)
		aboutPopup->draw();

	
	if (m_iDrawFrameNumber <= 60)
		m_iDrawFrameNumber++;

}


