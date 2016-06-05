#include "constdefine.h"
#include "payload.h"
#include "osbase.h"
#include "gspsystem.h"
#include "clocktime.h"

PayloadFileCollection::~PayloadFileCollection(){
	
	
	ClearAll();
	
}


int PayloadFileCollection::_fileId(0);


bool PayloadFileCollection::Read(size_t &readSize,void *buffer,size_t emSize, size_t count, const char*filepath,int index)
{
	FILE *fp = fopen(filepath, BINARY_READ);
	if(fp)
	{
		readSize=fread(buffer,emSize,count,fp);
		fclose(fp);
		return true;
	}
	else
		readSize = 0;
	return false;
}


bool PayloadFileCollection::LoadCollectionFromPath(const char* path, const char* keyword){
	CClockTime clockT;
	clockT.Begin();
	if(path && strlen(path))
		_path = path;
	else
	{
		_path = "";
		CGspSystem::WriteDefaultLog("*Error - The path is empty at PayloadFileCollection::LoadCollectionFromPath\n");
		return false;
	}

	if(keyword && strlen(keyword))
	{
		_keyword = keyword;
	}
	else
		keyword = 0;

	FindAllFiles(*this,_path,keyword);
	clockT.End();
	CGspSystem::WriteDefaultLog("-File Path: %s\n",_path.c_str());
	CGspSystem::WriteDefaultLog("-File loaded size %d, time cost: %f seconds.\n",
		this->_addedFileSize, clockT.GetDurationSeconds());
	return true;
}

int PayloadFileCollection::GetFileSize(){
	return this->_pfaSet.size();
}


PfaPtr *PayloadFileCollection::GetPfaPtrPtr(){
	if(!_pfaSet.size())
		return 0;
	return &_pfaSet[0];
}


void PayloadFileCollection::ClearAll(){
	
	
	int size = (int)this->_pfaSet.size();
	if(size)
	{
		PfaPtr *pptr = &(this->_pfaSet[0]);	
#pragma omp parallel for
		for(int i=0; i < size; ++i)
		{
			delete pptr[i];
		}
	}
	
	_pfaSet.clear();
	_pfaMap.clear();
	_allAddedFileSize = 0;
	_addedFileSize = 0;
	_omitedFileSize = 0;
	_zeroLengthFileSize = 0;

}

void PayloadFileCollection::ClearFlag(){
	PfaPtr *pptr = &(this->_pfaSet[0]);
	int size = this->_pfaSet.size();
#pragma omp parallel for
	for(int i=0; i < size; ++i)
	{
		pptr[i]->_flag = 0;
	}
}

bool PayloadFileCollection::_AddFile(const char* filename, int filelength, int state)
{
	PayloadFileAttribute *pa = new PayloadFileAttribute;
	pa->_filename = filename;
	pa->_id = _pfaMap.size();
	pa->_length = filelength;
	_pfaSet.push_back(pa);
	_pfaMap.push_back(pa);
	return true;
}

void PayloadFileCollection::AddFile(const char* filename, int filelength, int state)
{
	
	switch(state)
	{
	case BEGIN_ADD_FILE:
		_allAddedFileSize = 0;
		_addedFileSize = 0;
		_omitedFileSize = 0;
		_zeroLengthFileSize = 0;
		printf("Begin Load File: ");
		break;
	case END_ADD_FILE:
		_pfaSet.shrink_to_fit();
		_pfaMap.shrink_to_fit();
		printf("...%d\n",_addedFileSize);
		printf("File Loading is finished.\n");
		printf("File set summary: %d loaded, %d all, %d omited, %d zero length.\n",
			_addedFileSize, _allAddedFileSize, _omitedFileSize, _zeroLengthFileSize);
		break;
	case ADD_FILE:
		if(filelength)
		{	
			if(_AddFile(filename,filelength,state))
				++_addedFileSize;
			else
				++_omitedFileSize;
		}
		else
		{
			++_zeroLengthFileSize;
		}
		++_allAddedFileSize;
		break;
	case OMIT_FILE:
		++_omitedFileSize;
		++_allAddedFileSize;
		break;
	}
	if(_addedFileSize>0 && _addedFileSize % DELTA_FILE_NOTIFY_SIZE==0)
	{
		printf(".");
		if(_addedFileSize % (DELTA_FILE_NOTIFY_SIZE*5) == 0)
			printf("%d",_addedFileSize);
	}
	
}
void PayloadFileCollection::AscendSortByFileLength()
{
	sort(this->_pfaSet.begin(),this->_pfaSet.end(),PayloadFileAttribute::AscendRawNetflowAttribute);
}


const char* PayloadFileCollection::ToString(string &str)
{
	char buf[32];
	unsigned int size = this->_pfaSet.size();
	str.clear();
	str += "Total File Size: ";
	sprintf(buf,"%d",size);
	str += buf;
	str += "\n";
	if(size)
	{
		PfaPtr *pptr = &(this->_pfaSet[0]);	
		PfaPtr ptr(0);
		for(unsigned int i=0; i < size; ++i)
		{
			ptr = pptr[i];
			str += "\t"+ptr->_filename;
			str += "\t\t\t\t";
			sprintf(buf,"%d",ptr->_id);
			str += buf;
			str += "\t\t\t\t";
			sprintf(buf,"%d",ptr->_length);
			str += buf;
			str += "\t\t\t\t";
			sprintf(buf,"%d",ptr->_flag);
			str += buf;
			str += '\n';
		}
	}
	return str.c_str();
}

const char* PayloadFileCollection::ToSummaryString(string &str)
{
	char tc[1024];
	str.clear();
	sprintf(tc, "Files Path:%s; Filter key words:%s\nAdded Files: %d, All Files: %d, Omited Files: %d, Zero Files: %d", 
		_path.c_str(),
		_keyword.c_str(),
		_addedFileSize, _allAddedFileSize, _omitedFileSize, _zeroLengthFileSize);
	str = tc;
	return str.c_str();
}

void PayloadPackFileCollection::ReadIndexFile(const char *keyword)
{
	char buf[BUFFER_SIZE];
	char filename[BUFFER_SIZE];
	int size;

	AddFile(0,0,BEGIN_ADD_FILE);
	char *res = fgets(buf,BUFFER_SIZE,_findex);
	while(res && res[0]=='\n')
		res = fgets(buf,BUFFER_SIZE,_findex);
	while(res)
	{			
		int flag = sscanf(buf,"%s %s %d %u",_tag,filename,&size,&_offset);
		if(EOF == flag)
		{
			printf("Analysis %s failed!\n",buf);
			return;
		}
		if(keyword && !strstr(filename, keyword))
		{
			AddFile(filename,size,OMIT_FILE);
			continue;
		}
		AddFile(filename,size,ADD_FILE);
		res = fgets(buf,BUFFER_SIZE,_findex);
		while(res && res[0]=='\n')
			res = fgets(buf,BUFFER_SIZE,_findex);
	}
	AddFile(0,0,END_ADD_FILE);

}

void PayloadPackFileCollection::GenPackFilePath(string &fullPath, string &strIndexPath,string &packfileStr)
{
	size_t found = strIndexPath.find_last_of("\\/");
	string purePath = strIndexPath.erase(found);
	purePath+="\\";
	purePath+=packfileStr;
	if(purePath.at(purePath.length()-1) == '\n')
	{
		purePath.erase(purePath.length()-1);
	}
	fullPath.clear();
	fullPath = purePath;
}
bool PayloadPackFileCollection::_AddFile(const char* filename, int filelength, int state)
{
	PayloadFileAttribute *pa = new PayloadFileAttribute;
	pa->_filename = filename;
	pa->_id = _pfaMap.size();
	pa->_length = filelength;
	pa->_offset = _offset;
	pa->_tag = this->_tag;
	_pfaSet.push_back(pa);
	_pfaMap.push_back(pa);
	return true;
}

bool PayloadPackFileCollection::LoadCollectionFromPath(const char* path, const char* keyword){
	CClockTime clockT;
	clockT.Begin();
	char buf[BUFFER_SIZE];
	_findex = fopen(path,TEXT_READ);
	if(!_findex)
	{
		printf("Open pack index file %s failed!\n",path);
		return false;
	}	
	const char* packPath = fgets(buf,512,_findex);

	string packfileStr = packPath;
	string strIndexPath = path;
	string packfullPath;
	GenPackFilePath(packfullPath,strIndexPath,packfileStr);

	_fpack = fopen(packfullPath.c_str(),BINARY_READ);
	if(!_fpack)
	{
		printf("Open pack file %s failed!\n", packfullPath.c_str());
	}
	if(keyword && strlen(keyword))
	{
		_keyword = keyword;
	}
	else
		keyword = 0;

	ReadIndexFile(keyword);
	clockT.End();
	CGspSystem::WriteDefaultLog("-File Path: %s\n",_path.c_str());
	CGspSystem::WriteDefaultLog("-File loaded size %d, time cost: %f seconds.\n",
		this->_addedFileSize, clockT.GetDurationSeconds());
	return true;
}

bool PayloadPackFileCollection::Read(size_t &readSize,void *buffer,size_t emSize, size_t count, const char*filepath,int oid)
{
	PfaPtr ptr =  this->_pfaMap[oid];
	if(0 == fseek(this->_fpack,ptr->_offset,SEEK_SET))
	{
		if(ptr->_length*sizeof(char)>=count*emSize)
			readSize = fread(buffer,emSize,count,this->_fpack);
		else
			readSize = fread(buffer,sizeof(char),ptr->_length,this->_fpack);
		return true;
	}
	else 
	{
		readSize = 0;
		return false;
	}
	return true;
}