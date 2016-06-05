#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <time.h>
#include <regex>
#include "getopt.h"
using namespace std;

typedef vector<string> StrVec;
typedef vector<wregex> RegexVec;
typedef vector<unsigned short int> USIntVec;

#define DELTA_FILE_NUMBER	1000
#define RAND_BASE			10000
#define BUFFER_SIZE			1024
#define MATCH_SIZE			1000

string spath;
string dpath;
string epath;
int nfile;
bool bKey(false);
bool bRegex(false);
bool bCopy(false);
StrVec keyVec;
RegexVec regVec;
USIntVec usVec; 

void string_split(string &str, char delim, vector<string> &results)
{
	int cur_at;
	while( (cur_at = str.find_first_of(delim)) != str.npos )
	{
		if(cur_at > 0)
			results.push_back(str.substr(0, cur_at));
		str = str.substr(cur_at + 1);
	}
	if(str.length() > 0)
		results.push_back(str);
}

bool MovePayloadFile(const char* filename)
{	
	string sourceFullPath = spath;
	sourceFullPath += filename;
	string destFullPath = dpath;
	destFullPath += filename;	
	if(!bCopy)
	{
		if(!MoveFile(sourceFullPath.c_str(),destFullPath.c_str()))
		{
			printf("Moving file %s from %s to %s failed\n",filename,sourceFullPath.c_str(),destFullPath.c_str());	
			return false;
		}
	}
	else
	{
		if(!CopyFile(sourceFullPath.c_str(),destFullPath.c_str(),FALSE))
		{
			printf("Copy file %s from %s to %s failed\n",filename,sourceFullPath.c_str(),destFullPath.c_str());	
			return false;
		}
	}	
	return true;
}

int GetFileCount(string &path)
{
	char szFind[MAX_PATH*2];
	int count(0);
	strcpy(szFind,path.c_str());
	char c=szFind[strlen(szFind)-1];
	if(c != '\\' && c != '/')
	{
		strcat(szFind,"\\*.");
		path += "\\";
	}
	else
		strcat(szFind,"*.");
	strcat(szFind,"payload");
	WIN32_FIND_DATA fdata;
	HANDLE find = FindFirstFile(szFind, &fdata);
	if(find == INVALID_HANDLE_VALUE)
	{
		printf("Find file at path %s failed!\n",szFind);
		FindClose(find);
		return count;
	}	
	printf("Counting...\n");
	do{		
		++count;
		if((count % DELTA_FILE_NUMBER)==0)
		{
			printf(".");
		}
		if(( count % (DELTA_FILE_NUMBER*10)) ==0)
		{
			printf("%d",count);
		}
	}while(FindNextFile(find, &fdata));
	FindClose(find);
	printf("\nTotal count:%d\n",count);
	return count;
}

void MultiCharToUnicodeChar(wstring &destStr,const char *pSource)
{
	destStr.clear();
	size_t s = strlen(pSource);
	for(size_t i=0;i<s;++i)
	{
		destStr += pSource[i];
	}
}

bool RegexMatch(const char* filename)
{
	static unsigned char buffer[MATCH_SIZE];	
	string sourceFullPath = spath;
	sourceFullPath += filename;
	FILE *fp = fopen(sourceFullPath.c_str(),"rt");
	size_t readSize;
	if(fp)
	{
		readSize = fread(buffer,sizeof(char),MATCH_SIZE,fp);
		if(readSize)
		{
			usVec.clear();
			
			for(size_t i=0;i<readSize;++i)
			{
				unsigned int short v = buffer[i];
				usVec.push_back(v);				
			}
			for(size_t i=0; i< regVec.size();++i)
			{
				if(regex_search(usVec.begin(),usVec.end(),regVec[i]))
				{

					fclose(fp);
					return true;
				}
			}
		}
	}
	fclose(fp);
	return false;
}



bool InitRegex()
{
	static char buffer[BUFFER_SIZE];
	static char regexStr[BUFFER_SIZE*2];
	static char tag[BUFFER_SIZE];
	static double weight;
	FILE *fp = fopen(epath.c_str(),"rt");
	if(!fp)
	{
		printf("Open regex file %s failed\n",epath.c_str());
		return false;
	}
	char *res = fgets(buffer,BUFFER_SIZE,fp);
	while(res)
	{		
		int flag = sscanf(buffer,"%s %lf %s",tag,&weight,regexStr);
		if(EOF == flag)
		{
			printf("Analysis %s failed\n",buffer);
			return false;
		}
		wstring dstr;
		MultiCharToUnicodeChar(dstr,regexStr);
		wregex reg(dstr,regex_constants::icase);
		regVec.push_back(reg);
		res = fgets(buffer,BUFFER_SIZE,fp);
		while(res && res[0]=='\n')
			res = fgets(buffer,BUFFER_SIZE,fp);
	}
	printf("reg size:%d\n",regVec.size());
	fclose(fp);
	return true;
}

bool FindAllFiles()
{
	char szFind[MAX_PATH*2];
	int id(0);
    strcpy(szFind,spath.c_str());
	char c=szFind[strlen(szFind)-1];
	if(c != '\\' && c != '/')
	{
		strcat(szFind,"\\*.");
		spath += "\\";
	}
	else
		strcat(szFind,"*.");
	strcat(szFind,"payload");

	if(bRegex)
	{
		InitRegex();
	}

	
	int totalCount = GetFileCount(spath);
	if(totalCount == 0)
	{
		printf("Total file count is 0!\n");
		return false;
	}
	int needCount =  nfile;
	srand((int)time(0));
	WIN32_FIND_DATA fdata;
	HANDLE find = FindFirstFile(szFind, &fdata);
	int selectCount(0);
	if(find == INVALID_HANDLE_VALUE)
	{
		printf("Find file at path %s failed!\n",szFind);
		FindClose(find);
		return false;
	}	
	printf("\n");
	printf("Preparing moving...\n");
	StrVec sv;
	do
	{
		if(!bKey && !bRegex)
		{
			double urand = (double)(rand()%RAND_BASE)/(double)RAND_BASE;
			if(totalCount ==0)
				break;
			if(urand < (double)needCount/(double)totalCount)
			{
				--needCount;
				sv.push_back(fdata.cFileName);
				++selectCount;
				if((selectCount % DELTA_FILE_NUMBER)==0)
				{
					printf(".");
				}
				if(( selectCount % (DELTA_FILE_NUMBER*10)) ==0)
				{
					printf("%d",selectCount);
				}
			}
			--totalCount;
		}
		else if(bKey)
		{

			for(size_t ki = 0; ki < keyVec.size(); ++ki)
			{
				char *fpos(0);
				fpos = strstr(fdata.cFileName, keyVec[ki].c_str());
				if(!fpos)
					goto ENDPUSH;
			}
			++selectCount;
			sv.push_back(fdata.cFileName);
			ENDPUSH:;
		}
		else if(bRegex)
		{
			if(RegexMatch(fdata.cFileName))
			{
				++selectCount;
				sv.push_back(fdata.cFileName);
			}
		}
	}while(FindNextFile(find, &fdata));
	FindClose(find);
	printf("\nStart moving...\n");
	selectCount = 0;
	for(size_t i = 0; i< sv.size(); ++i)
	{
		if(!MovePayloadFile(sv[i].c_str()))
			return false;
		++selectCount;
		if((selectCount % DELTA_FILE_NUMBER)==0)
		{
			printf(".");
		}
		if(( selectCount % (DELTA_FILE_NUMBER*10)) ==0)
		{
			printf("%d",selectCount);
		}
	}
	printf(".Moving finished.\nMoved Count:%d\n",selectCount);
	return true;
}

void Usage()
{
	printf("======== Powered by Wynter Han. 2011-03-02 :)========\n");
	printf("A tool randomly selects specific number of payload files from a folder.\n");
	printf("Or selects specific payload files have the key words.\n");
	printf("The file should have a extension as '.payload'\n");
	printf("Usage:\n");
	printf("\t -n [selection number] -s [source folder] -d [destination folder]\n");
	printf("\t -k [key string split by ','] -s [source folder] -d [destination folder]\n");
	printf("\t -e [Regex file] -s [source folder] -d [destination folder]\n");
	printf("\t -c copy mode, e.g. -n 100 -s c:\temp -d c:\temp2 -c\n");
	printf("Regex file format: each regex rule occupies a new line\n");
}

bool GetCommand(int argc, char** argv)
{

	char *str_opt = "n:s:d:k:e:c";
	bool is_valid = true;
	char *pszParam;
	int result;
	result = GetOption(argc, argv, str_opt, &pszParam);
	int count = 3;
	int flag;
	char c;
	string keyStr;
	while(result != 0)
	{
		switch(result)
		{
		case 'c':
			bCopy = true;
			break;
		case 'h':
			return false;
		case'd':
			--count;
			dpath = pszParam;
			c =dpath.at(dpath.length()-1);
			if(c != '\\' && c != '/')
			{
				dpath += "\\";
			}
			break;
		case'e':
			--count;
			epath = pszParam;			
			bRegex = true;
			break;
		case 's':
			--count;
			spath = pszParam;
			break;
		case 'n':
			--count;
			flag = sscanf(pszParam,"%d",&nfile);
			if(EOF == flag)
			{
				printf("Param n is invalid!\n");
				return false;
			}
			break;
		case 'k':
			--count;
			keyStr = pszParam;
			string_split(keyStr,',',keyVec);
			bKey = true;
			break;
		}
		result=GetOption(argc, argv, str_opt, &pszParam);
	}
	if(count)
		is_valid = false;
	return is_valid;

}



int main(int argc, char** argv)
{
	if(!GetCommand(argc,argv))
	{
		Usage();
		return 0;
	}
	if(!FindAllFiles())
	{
		printf("Select File Failed!\n");
		return 0;
	}
	return 0;
}