#ifndef OSBASE_H_
#define OSBASE_H_

#include "payload.h"


struct MemoryState
{
	
	double _peakWorkingSetSize;
	
	double _workingSetSize;
	
	double _pagefileUsage;
	
	double _peakPagefileUsage;
	MemoryState():_peakWorkingSetSize(0),
		_workingSetSize(0),
		_pagefileUsage(0),_peakPagefileUsage(0){}
};

bool FindAllFiles(PayloadFileCollection &col, string &path,const char *keyword);


#endif