#include "stdafx.h"
#include "Directory.h"
#include <tchar.h>
#include <Shlwapi.h>
#include <shlobj.h>

Directory::Directory(TCHAR* target)
{
	_target = target;

	WIN32_FIND_DATA fd;
	TCHAR query[500];
	wsprintf(query, _T("%s\\*.*"), target);
	auto search = FindFirstFile(query, &fd);
	if (search == INVALID_HANDLE_VALUE)return;
	do
	{
		auto entry = new TCHAR[500];
		wsprintf(entry, _T("%s\\%s"), target, fd.cFileName);
		ToFree.push_back(entry);
		if (fd.cFileName[0] != _T('.'))
		{
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				Directories.push_back(entry);
			else
				Files.push_back(entry);
		}
	} while (FindNextFile(search, &fd));
}

vector<TCHAR*> Directory::AllFilesBelow()
{
	AllFiles.clear();
	AllFiles.insert(AllFiles.end(), Files.begin(), Files.end());
	int len = Directories.size();
	for (int i = 0; i < len; i++)
	{
		auto dir=new Directory(Directories[i]);
		ToFree.push_back(dir);
		auto files = dir->AllFilesBelow();
		AllFiles.insert(AllFiles.end(), files.begin(), files.end());
	}
	return AllFiles;
}

vector<TCHAR*> Directory::AllDirsBelow()
{
	AllDirs.clear();
	AllDirs.insert(AllDirs.end(), Directories.begin(), Directories.end());
	int len = Directories.size();
	for (int i = 0; i < len; i++)
	{
		auto dir=new Directory(Directories[i]);
		ToFree.push_back(dir);
		auto dirs = dir->AllDirsBelow();
		AllDirs.insert(AllDirs.end(), dirs.begin(), dirs.end());
	}
	return AllDirs;
}

vector<TCHAR*> Directory::AllRelativeFilepath()
{
	auto vec = AllFilesBelow();
	RelatFiles.clear();
	int len = vec.size();
	for (int i = 0; i < len; i++)
	{
		auto relat = new TCHAR[500];
		ToFree.push_back(relat);
		PathRelativePathTo(relat, _target, FILE_ATTRIBUTE_DIRECTORY, vec[i], FILE_ATTRIBUTE_NORMAL);
		RelatFiles.push_back(relat + 2);
	}
	return RelatFiles;
}

vector<TCHAR*> Directory::AllRelativeDirpath()
{
	auto vec = AllDirsBelow();
	RelatDirs.clear();
	int len = vec.size();
	for (int i = 0; i < len; i++)
	{
		auto relat = new TCHAR[500];
		ToFree.push_back(relat);
		PathRelativePathTo(relat, _target, FILE_ATTRIBUTE_DIRECTORY, vec[i], FILE_ATTRIBUTE_DIRECTORY);
		RelatDirs.push_back(relat + 2);
	}
	return RelatDirs;
}

void Directory::CopyDirectoryStructure(TCHAR* dstdir)
{
	auto vec = AllRelativeDirpath();
	int len = vec.size();
	TCHAR buffer[500];
	for (int i = 0; i < len; i++)
	{
		wsprintf(buffer, _T("%s\\%s"), dstdir, vec[i]);
		SHCreateDirectory(NULL, buffer);
	}
}

Directory::~Directory()
{
	int len = ToFree.size();
	for (int i = 0; i < len; i++)
		delete ToFree[i];
}