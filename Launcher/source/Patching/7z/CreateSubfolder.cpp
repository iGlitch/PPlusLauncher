#include <string.h>
#include <malloc.h>
#include <sys\dir.h>
#include <debug.h>
#include "CreateSubfolder.h"

bool CreateSubfolder(const char * fullpath)
{
	if(!fullpath)
		return false;

	//! make copy of string
	int length = strlen(fullpath);

	char *dirpath = (char *) malloc(length+2);
	memset(dirpath, 0, length + 2);
	if(!dirpath)
		return false;

	strcpy(dirpath, fullpath);

	//! remove unnecessary slashes
	while(length > 0 && dirpath[length-1] == '/')
		--length;

	dirpath[length] = '\0';

	//! if its the root then add one slash
	char * notRoot = strrchr(dirpath, '/');
	if(!notRoot)
		strcat(dirpath, "/");

	int ret;
	//! clear stack when done as this is recursive
	{
		struct stat filestat;
		ret = stat(dirpath, &filestat);
	}

	//! entry found
	if(ret == 0)
	{
		free(dirpath);
		return true;
	}
	//! if its root and stat failed the device doesnt exist
	else if(!notRoot)
	{
		free(dirpath);
		return false;
	}

	//! cut to previous slash and do a recursion
	*notRoot = '\0';

	bool result = false;

	if(CreateSubfolder(dirpath))
	{
		//! the directory inside which we create the new directory exists, so its ok to create
		//! add back the slash for directory creation
		*notRoot = '/';
		//! try creating the directory now
		result = (mkdir(dirpath, 0777) == 0);
	}

	free(dirpath);

	return result;
}