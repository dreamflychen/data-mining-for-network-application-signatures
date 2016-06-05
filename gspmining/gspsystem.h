#ifndef GSPSYSTEM_H
#define GSPSYSTEM_H

#include "stdcommonheader.h"

#include "constdefine.h"

typedef unordered_map<string, FILE*> FileMap;
typedef FileMap::iterator	FileIt;

class CGspSystem
{
private:
	

	
	static FileMap _fileMap;
	
	static char _logfilePerfix[];	
	
	static string _filename;

	

	
	static void _GenDefaultLogFileName(string &str);
	static bool _WriteFileLog(const char *file,const char*format, va_list ap);
public:
	

	
	static char *_payloadFileExtension;

	

	
	static bool InitSystem();
	
	
	static bool DestroySystem();

	
	static bool WriteLog(const char *file,const char* format, ...);

	
	static bool CloseLog(const char *file);
	
	
	static bool WriteDefaultLog(const char *format, ...);

	
};

#endif