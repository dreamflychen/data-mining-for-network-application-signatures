#ifndef ELEMENT_H
#define ELEMENT_H
#include "stdcommonheader.h"
#include "payload.h"
#include "constdefine.h"
#include "utilityfunc.h"


struct FragmentReference;


struct ElementSeries
{
public:
	ElementSeries():_pPosTable(0){}
	virtual ~ElementSeries();
	
	

	
	UCharVector _content;
	
	IntVecPtr *_pPosTable;

	

	
	void GeneratePosTable();
	
	void SetCompact();
	
	unsigned char* GetUCharContentPtr();

	
	void InitFromFragmentReference(FragmentReference &ref);

	
	void ClipElementSeries(int length);

	
	int FirstFind(int start, int end,const UCharVector &pattern);

	
};


struct ElementSequence
{
public:
	ElementSequence():_rawFlowId(0){}

	
	ElementSeries _series;		
	
	int _rawFlowId;		
};

typedef vector<ElementSequence> EmSeqVector;

struct ElementSequenceCol
{
protected:
	
	int _maxLength;
public:
	ElementSequenceCol():_size(0),_maxLength(0),_minSize(-1),_maxSize(-1),_retainSize(RETAIN_LENGTH),_pPfCol(0){};

	

	PayloadFileCollection *_pPfCol;
	
	int _minSize;
	
	int _maxSize;
	
	int _retainSize;

	
	EmSeqVector _seqVector;
	
	int _size;
	
	
	
	void InitPosTable();
	
	void setParam(int minSize,int maxSize,int retainSize)
	{
		_minSize = minSize;
		_maxSize = maxSize;
		_retainSize = retainSize;
	}

	
	int GetMaxElementSequenceLength()
	{
		return _maxLength;
	}

	
	virtual int ReLoadEmSeqFromFileSet(PayloadFileCollection &fcol);
	int ReloadEmSeqFromUnkSet(IntVector &iv);
	
	
	void ClipSeqColLength(int length);

	
	void ClearAll();

	void ConvertUnkVectorToFileIdVector(IntVector &dest, IntVector &source);

	virtual const char* ToSummaryString(string &str);
};

struct ElementSequenceColMaxPercent : public ElementSequenceCol
{
	ElementSequenceColMaxPercent():_percent(1){}

	
	
	
	double _percent;
	
	

	
	void setParam(double percent, int retainSize)
	{
		_percent = percent;
		_retainSize = retainSize;
	}

	
};

#endif