#ifndef PACK_H
#define PACK_H
#include "stdcommonheader.h"
#include "constdefine.h"
#include "clocktime.h"
#include "payload.h"
#include "osbase.h"
#include "utilityfunc.h"
struct PayloadFileCollectionPack : public PayloadFileCollection
{
private:
	FILE *_packFP;
	FILE *_indexFP;
	string _tag;
	int _retainSize;
	UCharVector _buffer;
	unsigned int _offset;
public:
	virtual bool _AddFile(const char* filename, int filelength, int state);
	int LoadFilesToPack(const char* dirpath,const char* tag,FILE *packFP,
		FILE *indexFP,int retainSize,unsigned int offset);
};

class CPack
{
private:
	char _buffer[BUFFER_SIZE];//buffer to store directory index
	char _path[BUFFER_SIZE];//buffer to store directory path
	char _tag[BUFFER_SIZE];//tag buffer
	char _target[BUFFER_SIZE];//path for target index file
	char _packPath[BUFFER_SIZE];//path for target packet file
	FILE *_source;//record file in a directory
	FILE *_index;//index file
	FILE *_pack;//package file
	int _retainSize;
	int _offset;
	bool PackDir()
	{
		PayloadFileCollectionPack pack;
		_offset = pack.LoadFilesToPack(_path,_tag,_pack,_index,_retainSize,_offset);
		return true;
	}
public:
	bool InitPack(const char* indexFilePath,const char* targetPath,int retainSize);
	bool Execute();
};

#endif