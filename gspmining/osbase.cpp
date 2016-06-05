#include "stdcommonheader.h"
#include <Windows.h>
#include <Psapi.h> 
#include "osbase.h"
#include "gspsystem.h"

bool FindAllFiles(PayloadFileCollection &col, string &path,const char *keyword)
{
	char szFind[MAX_PATH*2];
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
	strcat(szFind,CGspSystem::_payloadFileExtension);
	WIN32_FIND_DATA fdata;
	HANDLE find = FindFirstFile(szFind, &fdata);
	if(find == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	col.AddFile(0,0,BEGIN_ADD_FILE);
	do
	{		
		
		if(keyword && !strstr(fdata.cFileName, keyword))
		{
			col.AddFile(fdata.cFileName,fdata.nFileSizeLow,OMIT_FILE);
			continue;
		}
		col.AddFile(fdata.cFileName,fdata.nFileSizeLow,ADD_FILE);
	}while(FindNextFile(find, &fdata));
	col.AddFile(0,0,END_ADD_FILE);
	return true;
}

bool GetMemoryUsage(MemoryState &mstate)
{
	PROCESS_MEMORY_COUNTERS pmc;  
	if(GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))  
	{  
		mstate._peakWorkingSetSize = (double)pmc.PeakWorkingSetSize/(1024.0*1024.0);
		mstate._workingSetSize = (double)pmc.WorkingSetSize/(1024.0*1024.0);
		mstate._pagefileUsage = (double)pmc.PagefileUsage/(1024.0*1024.0);
		mstate._peakPagefileUsage = (double)pmc.PeakPagefileUsage/(1024.0*1024.0);
		return true;  
	}  
	return false;  
}