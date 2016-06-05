#include "pack.h"

bool PayloadFileCollectionPack::_AddFile(const char* filename, int filelength, int state)
{

	if(_retainSize!=-1)
		filelength = _retainSize>filelength?filelength:_retainSize;
	if((int)_buffer.size()<filelength)
	{
		_buffer.resize(filelength);
	}
	string nameStr = this->_path;
	nameStr += filename;
	FILE *tfp=fopen(nameStr.c_str(),BINARY_READ);
	if(!tfp)
	{
		printf("Read file %s failed!\n",nameStr.c_str());
		return false;
	}
	else
	{
		fread(&_buffer[0],sizeof(char),filelength,tfp);
		fwrite(&_buffer[0],sizeof(char),filelength,_packFP);
		fclose(tfp);
		fprintf(_indexFP,"%s %s %d %u\n",this->_tag.c_str(),filename,filelength,_offset);
		_offset += filelength;
	}
	return true;

}
int PayloadFileCollectionPack::LoadFilesToPack(const char* dirpath,const char* tag,FILE *packFP,FILE *indexFP,int retainSize,unsigned int offset)
{
	_packFP = packFP;
	_path = dirpath;
	_retainSize = retainSize;
	_indexFP = indexFP;
	_tag = tag;
	_offset = offset;
	if(retainSize != -1)
	{
		_buffer.resize(retainSize);
	}
	FindAllFiles(*this,_path,0);
	return _offset;
}

bool CPack::InitPack(const char* indexFilePath,const char* targetPath,int retainSize)
{
	_retainSize = retainSize;
	if(strlen(indexFilePath)==0 || strlen(targetPath)==0)
	{
		return false;
	}		
	_source = fopen(indexFilePath, TEXT_READ);
	if(!_source)
	{
		printf("Open index file %s failed!\n",indexFilePath);
		return false;
	}
	
	string dateStr;
	CClockTime::GetUnderlineDateAndTime(dateStr);
	strcpy(_target,targetPath);
	char c = _target[strlen(_target)-1];
	if( c!= '\\' && c !='/')
	{
		strcat(_target,"\\");
	}
	string tpath = _target;
	strcat(_target,INDEX_FILE_PERFIX);
	strcat(_target,dateStr.c_str());
	strcat(_target,".txt");
	_index = fopen(_target,TEXT_REWRITE);
	if(!_index)
	{
		printf("Create index file %s failed!\n",_target);
		return false;
	}
	
	strcpy(_packPath,tpath.c_str());
	strcat(_packPath,PACK_FILE_PERFIX);
	strcat(_packPath,dateStr.c_str());
	strcat(_packPath,PACK_EXTENSION);
	_pack = fopen(_packPath,BINARY_REWRITE);
	if(!_pack)
	{
		printf("Creat pack file %s failed!\n",_pack);
		return false;
	}
	fprintf(this->_index,"%s%s%s\n",PACK_FILE_PERFIX,dateStr.c_str(),PACK_EXTENSION);
	return true;
}
bool CPack::Execute()
{	
	char *pc=fgets(_buffer,BUFFER_SIZE,_source);
	_offset = 0;
	while(pc && pc[0] == '\n')
		pc=fgets(_buffer,BUFFER_SIZE,_source);
	while(pc)
	{
		if(pc[0]!='#')
		{
			int flag = sscanf(pc,"%s %s",_tag,_path);
			{
				printf("Analysis %s failed\n",pc);
				return false;
			}
			else if(!PackDir())
			{
				printf("Pack dir %s as %s to %s failed\n",_path,_tag,_packPath);
			}
		}
		pc = fgets(_buffer,BUFFER_SIZE,_source);
		while(pc && pc[0] == '\n')
		{
			pc = fgets(_buffer,BUFFER_SIZE,_source);
		}
	}
	fclose(_pack);
	fclose(_index);
	fclose(_source);
	return true;
}