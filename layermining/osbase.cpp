#include "osbase.h"
#include <Windows.h>

//windows
bool findAllFiles(RawNetflowSet &rfarray, string &path,const char *keyword)
{
	writedeflog(">Enter findAllFiles\n");
	char szFind[MAX_PATH];
	RawNetflowAttribute attr;
	int id(0);
    strcpy(szFind,path.c_str());
	char c=szFind[strlen(szFind)-1];
    if(c != '\\' && c != '/')
	{
		strcat(szFind,"\\*.");
		path += "\\";
	}
	else
		strcat(szFind,"*.");
	strcat(szFind,GV::payloadFileExtension);
	WIN32_FIND_DATA fdata;
	HANDLE find = FindFirstFile(szFind, &fdata);
	if(find == INVALID_HANDLE_VALUE)
	{
		writedeflog("Cannot get any file\n");
		writedeflog(">Leave findAllFiles\n");
		return false;
	}
	do
	{		
		//do not add files that do not contain keywords
		if(keyword && !strstr(fdata.cFileName, keyword))
			continue;
		attr.filename = fdata.cFileName;
		attr.id = id++;
		attr.length = fdata.nFileSizeLow;
		rfarray.push_back(attr);
	}while(FindNextFile(find, &fdata));
	FindClose(find);
	writedeflog(">Leave findAllFiles\n");
	return true;
}