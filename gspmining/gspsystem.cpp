#include "gspsystem.h"
#include "clocktime.h"


FileMap CGspSystem::_fileMap;
char CGspSystem::_logfilePerfix[]= DEFAULT_LOG_FILE_PREFIX;
char *CGspSystem::_payloadFileExtension = PAYLOAD_EXTENSION;
string CGspSystem::_filename;



void CGspSystem::_GenDefaultLogFileName(string &str)
{
	string timeStr;
	str.clear();
	str=_logfilePerfix;
	str+=CClockTime::GetUnderlineDateAndTime(timeStr);
	str+=".txt";
}


bool CGspSystem::_WriteFileLog(const char *file,const char*format, va_list ap)
{
	FileIt it = _fileMap.find(file);
	if(it != _fileMap.end())
	{
		FILE *fp = it->second;
		if(!fp)
			return false;
		vfprintf(fp,format,ap);
	}
	else
		return false;
	return true;
}



bool CGspSystem::InitSystem(){
	printf("=====init system=====\n");
	
	_GenDefaultLogFileName(_filename);
	FILE *fp = fopen(_filename.c_str(), LOGFILE_MODE);
	if(fp)
	{
		_fileMap[_filename.c_str()] = fp;
		printf("Open file %s as default log file.\n", _filename.c_str());
	}
	else
	{
		printf("Open file %s error\n", _filename.c_str());
		return false;
	}
	string timeStr;
	WriteDefaultLog(":Init System at %s", CClockTime::GetReadAbleDateAndTime(timeStr));
	return true;
}


bool CGspSystem::DestroySystem(){
	string timeStr;	
	WriteDefaultLog(":Destroy System at %s", CClockTime::GetReadAbleDateAndTime(timeStr));
	FILE *fp;
	
	for(FileIt it = _fileMap.begin(); it != _fileMap.end(); ++it)
	{
		printf("-Close file: %s\n", it->first.c_str());
		fp = it->second;
		if(fp)
		{
			fclose(fp);
			it->second = 0;
		}
		else
			printf("Invalid file handle, file name is %s.\n",it->first.c_str());
	}
	_fileMap.clear();
	printf("-----destroy system-----\n");	
	return true;
}


bool CGspSystem::WriteLog(const char *file,const char* format, ...){
	bool res;
	FILE *fp;
	FileIt it;	
	va_list ap;
	it =  _fileMap.find(file);
	if(it == _fileMap.end())
	{
		fp = fopen(file,LOGFILE_MODE);
		if(!fp)
			return false;
		_fileMap[file] = fp;
	}

	va_start(ap, format);
	res = _WriteFileLog(file, format,ap);
	va_end(ap);
	return res;
}


bool CGspSystem::CloseLog(const char *file){
	FileIt it;	
	it =  _fileMap.find(file);
	if(it == _fileMap.end())
	{
		return true;
	}
	fclose(it->second);
	_fileMap.erase(it->first);
	return true;
}


bool CGspSystem::WriteDefaultLog(const char *format, ...){
	bool res;
	va_list ap;
	va_start(ap, format);
	res =  _WriteFileLog(_filename.c_str(), format,ap);
	va_end(ap);
	return res;
}


	FILE *fp;
	for(FileIt it = _fileMap.begin(); it != _fileMap.end(); ++it)
	{
		fp = it->second;
		if(fp)
		{
			fflush(fp);
		}		
	}
	return true;
}