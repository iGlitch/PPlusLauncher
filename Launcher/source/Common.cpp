#include "Common.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "FileHolder.h"
#include <malloc.h>
#include "Patching/tinyxml2.h"

char infoMusicPath[ISFS_MAXPATH] = "";
char infoVersionStr[32] = "";
f32 infoVersion = 0.0f;
char gameVersionStr[32] = "";
f32 gameVersion = 0.0f;
GXColor selectionColor = (GXColor){ 255, 255, 255, 255 };

extern u8 * configFileData;
extern int configFileSize;
extern u8 * infoFileData;
extern int infoFileSize;

const char* GetFileName(const char* path)
{
	int len = strlen(path);
	int origin = 0;
	for (int i = 0; i<len; i++)
	{
		if (path[i] == '-' || path[i] == '/')
			origin = i + 1;
		else if (path[i] == '.')
			return path + origin;
	}
	return NULL;
}

const char* GetExtension(const char* filepath)
{
	int len = strlen(filepath);
	for (int i = 0; i<len; i++)
	if (filepath[i] == '.')
		return filepath + i;
	return NULL;
}

void GetDirectory(const char* src, char* buf)
{
	int len = strlen(src);
	int mark = 0;
	for (int i = 0; i<len; i++)
	if (src[i] == '/')
		mark = i;
	else if (src[i] == '.')
		break;
	memcpy(buf, src, mark);
}

bool Contains(const char* target, const char* str)
{
	int len = strlen(target);
	int matchLen = strlen(str);
	for (int i = 0; i<len - matchLen + 1; i++)
	for (int j = 0; j<matchLen; j++)
	{
		if (target[i + j] != str[j])break;
		if (j == matchLen - 1)return true;
	}
	return false;
}

void ToLower(const char* str, char* buf)
{
	int len = strlen(str);
	for (int i = 0; i < len; i++)
		buf[i] = tolower(str[i]);
}

void loadInfoFile()
{
    char infoPath[ISFS_MAXPATH];
    snprintf(infoPath, sizeof(infoPath), "%s/info.xml", codesBasePath);

    FileHolder infoFile(infoPath, "r");
    if (infoFile.IsOpen())
    {
        if (infoFileSize != 0)
            delete infoFileData;
        infoFileSize = infoFile.Size();
        infoFileData = (u8*)malloc(infoFileSize);
        memset(infoFileData, 0, infoFileSize);

        infoFile.FRead(infoFileData, infoFileSize, 1);
        infoFile.FClose();

        tinyxml2::XMLDocument doc;
        if (doc.Parse((char*)infoFileData, infoFileSize) == tinyxml2::XML_SUCCESS)
        {
            tinyxml2::XMLElement* root = doc.RootElement();
            if (root)
            {
                tinyxml2::XMLElement* game = root->FirstChildElement("game");
                if (game)
                {
                    tinyxml2::XMLElement* gameVer = game->FirstChildElement("version");
                    if (gameVer && gameVer->GetText())
                    {
                        strncpy(gameVersionStr, gameVer->GetText(), sizeof(gameVersionStr) - 1);
                        gameVersion = (f32)atof(gameVersionStr);
                    }
                    tinyxml2::XMLElement* gameName = game->FirstChildElement("name");
                    if (gameName && gameName->GetText())
                        strncpy(projectName, gameName->GetText(), sizeof(projectName) - 1);
                }

                tinyxml2::XMLElement* launcher = root->FirstChildElement("launcher");
                if (launcher)
                {
                    tinyxml2::XMLElement* colorElem = launcher->FirstChildElement("selectionColor");
                    if (colorElem)
                    {
                        int r = selectionColor.r, g = selectionColor.g, b = selectionColor.b;
                        colorElem->QueryIntAttribute("r", &r);
                        colorElem->QueryIntAttribute("g", &g);
                        colorElem->QueryIntAttribute("b", &b);
                        selectionColor = (GXColor){ (u8)r, (u8)g, (u8)b};
                    }

                    tinyxml2::XMLElement* music = launcher->FirstChildElement("musicPath");
                    if (music && music->GetText())
                        strncpy(infoMusicPath, music->GetText(), sizeof(infoMusicPath) - 1);


                    tinyxml2::XMLElement* urlElem = launcher->FirstChildElement("updateUrl");
                    if (urlElem && urlElem->GetText())
                        strncpy(updateUrl, urlElem->GetText(), sizeof(updateUrl) - 1);


                }
            }
        }
    }
}
