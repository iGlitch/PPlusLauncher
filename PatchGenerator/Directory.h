#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include <Windows.h>
#include <vector>
#include <memory>

using namespace std;
class Directory
{
public:
	Directory(TCHAR* target);
	~Directory();
	vector<TCHAR*> Files;
	vector<TCHAR*> Directories;
	vector<TCHAR*> AllFilesBelow();
	vector<TCHAR*> AllDirsBelow();
	vector<TCHAR*> AllRelativeFilepath();
	vector<TCHAR*> AllRelativeDirpath();
	void CopyDirectoryStructure(TCHAR* dstdir);
private:
	TCHAR* _target;
	vector<void*> ToFree;
	vector<TCHAR*> AllFiles;
	vector<TCHAR*> AllDirs;
	vector<TCHAR*> RelatFiles;
	vector<TCHAR*> RelatDirs;
};

#endif