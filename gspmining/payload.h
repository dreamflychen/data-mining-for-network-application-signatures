#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "stdcommonheader.h"
#include "clocktime.h"
#include "utilityfunc.h"



struct PayloadFileAttribute
{
	PayloadFileAttribute():_length(0),_id(-1),_flag(0),_offset(0){}

	

	string _filename;

	string _tag;
	
	unsigned int _length;

	int _id;

	int _flag;
	
	
	static bool AscendRawNetflowAttribute(PayloadFileAttribute *i,PayloadFileAttribute *j) 
	{ 
		return ( i->_length < j->_length ); 
	}ggc          
};


typedef PayloadFileAttribute* PfaPtr;

typedef vector<PfaPtr> PfaPtrVector;


struct PayloadFileCollection
{
protected:
	
	
	int _allAddedFileSize;
	
	int _addedFileSize;
	
	int _omitedFileSize;
	
	int _zeroLengthFileSize;

	
	static int _fileId;
public:
	

	
	string _path;			
	
	string _keyword;	
	
	PfaPtrVector _pfaSet;
	
	PfaPtrVector _pfaMap;

	PayloadFileCollection():_allAddedFileSize(0),_addedFileSize(0),_omitedFileSize(0),_zeroLengthFileSize(0){}
	virtual ~PayloadFileCollection();

	

	
	virtual bool Read(size_t &readSize,void *buffer,size_t emSize, size_t count, const char*filepath,int oid);
	
	
	virtual bool LoadCollectionFromPath(const char*path,const char* keyword);
	
	int GetFileSize();
	
	PfaPtr *GetPfaPtrPtr();
	
	void ClearAll();
	
	void ClearFlag();
	
	virtual void AddFile(const char* filename, int filelength, int state);
	
	virtual bool _AddFile(const char* filename, int filelength, int state);
	
	void AscendSortByFileLength();
	
	const char* ToString(string &str);	
	const char* ToSummaryString(string &str);
};

struct PayloadPackFileCollection : public PayloadFileCollection
{
private:
	FILE *_findex;
	FILE * _fpack;
	unsigned int _offset;
	char _tag[BUFFER_SIZE];
	void ReadIndexFile(const char *keyword);

public:
	PayloadPackFileCollection():_findex(0),_fpack(0){}
	~PayloadPackFileCollection()
	{
		if(_findex)
			fclose(_findex);
		if(_fpack)
			fclose(_fpack);
	}
	static void GenPackFilePath(string &fullPath, string &strIndexPath,string &packfileStr);
	virtual bool _AddFile(const char* filename, int filelength, int state);
	bool LoadCollectionFromPath(const char* path, const char* keyword);
	
	virtual bool Read(size_t &readSize,void *buffer,size_t emSize, size_t count, const char*filepath,int oid);
};

#endif