#include "Common.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
	if (src[i] == '/' || src[i] == '-')
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