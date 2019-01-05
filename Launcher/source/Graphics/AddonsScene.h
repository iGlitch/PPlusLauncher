#ifndef ADDONSSCENE_H
#define ADDONSSCENE_H
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>

#ifdef HW_RVL
#include <memory>
#include <vector>
#include <gctypes.h>
#elif WIN32
#include <memory>
#include <vector>
#endif

#include "GraphicsScene.h"
#include "video.h"
#include "Popup.h"
#include "..\Patching\AddonFile.h"



class CAddonsScene : public GraphicsScene
{
public:
	CAddonsScene(f32 w, f32 h);
	virtual void Draw();
	virtual void Load();
	virtual void Unload();
	virtual void HandleInputs(u32 gcPressed, s8 dStickX, s8 dStickY, s8 cStickX, s8 cStickY, u32 wiiPressed);

	virtual void Focus() { }; //not used
	virtual bool Work();
	wchar_t * sInfoText;
	

private:
	CAddonsScene() {}
	
	void drawSelectionMenu(float yPos);

	
	bool enumeratedFiles;
	int installIndex;
	std::vector<AddonFile *> addonFiles;
	
};

#endif