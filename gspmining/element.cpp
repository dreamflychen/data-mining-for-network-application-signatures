#include "element.h"
#include "audit.h"
ElementSeries::~ElementSeries(){
	SetCompact();
}


void ElementSeries::GeneratePosTable(){
	if(_pPosTable)
		SetCompact();
	_pPosTable = new IntVecPtr[256];
	memset(_pPosTable, 0, sizeof(IntVecPtr)*256);
	
	int size =(int) this->_content.size();
	if(size)
	{
		if( (int)this->_content.capacity()>size )
			this->_content.shrink_to_fit();
		unsigned char *pchar = this->GetUCharContentPtr();
		unsigned char c;
		IntVecPtr pVec;

		for(int i=0; i<size;++i)
		{
			c = pchar[i];			
			pVec = _pPosTable[c];	
			if(pVec)
			{
				pVec->push_back(i);
			}
			else
			{
				pVec = new IntVector;
				pVec->push_back(i);
				_pPosTable[c] = pVec;
			}
		}
		for(int j=0; j<256;++j)
		{
			pVec = _pPosTable[j];
			if(pVec)
				pVec->shrink_to_fit();
		}
	}
}

void ElementSeries::SetCompact(){
	IntVecPtr p;
	if(_pPosTable)
	{
#pragma omp parallel for private(p)
		for(int i=0; i<256;++i)
		{
			p = _pPosTable[i];
			if(p)
				delete p;
		}
		delete []_pPosTable;
		_pPosTable = 0;
	}	
}


unsigned char* ElementSeries::GetUCharContentPtr(){
	if(!_content.size())
		return 0;
	return &_content[0];
}

void ElementSeries::InitFromFragmentReference(FragmentReference &ref)
{
	SetCompact();
	this->_content.resize(ref._length);
	memcpy(&_content[0],ref._pv,ref._length);
	this->_content.shrink_to_fit();
}


void ElementSeries::ClipElementSeries(int length)
{
	if((int)this->_content.size()>length)
	{
		this->_content.resize(length);
		this->_content.shrink_to_fit();
		if(this->_pPosTable)
		{
			this->GeneratePosTable();
		}
	}
	
}


int ElementSeries::FirstFind(int start, int end,const UCharVector &pattern)
{
	
	int searchlen=end-start;
	if(end<=start || (int)pattern.size()>searchlen || !this->_pPosTable)
		return -1;
	int c = pattern[0];
	IntVecPtr piv = _pPosTable[c];
	if(!piv)
		return -1;
	IntVector riv = *piv;
	for(int i=0; i<(int)piv->size(); ++i)
	{
		int pos = riv[i];
		if(pos<start)
			continue;
		else if(pos>=end)
			break;
		else if(end - pos<(int)pattern.size())
			break;
		else if(pos+pattern.size()>this->_content.size())
			break;
		
		if(!memcmp(&_content[pos],&pattern[0],pattern.size()))
			return pos;
	}
	return -1;
}

void ElementSequenceCol::InitPosTable()
{
#pragma omp parallel for schedule(dynamic,10000)
	for(int i=0;i<(int)this->_seqVector.size();++i)
	{
		this->_seqVector[i]._series.GeneratePosTable();
	}	
}



int ElementSequenceCol::ReLoadEmSeqFromFileSet(PayloadFileCollection &fcol)
{
	ClearAll();
	this->_pPfCol = &fcol;
	
	if(_maxSize!=-1 && _maxSize < _minSize)
		return 0;
	
	fcol.AscendSortByFileLength();
	int size = fcol.GetFileSize();
	if(!size)
		return 0;

	
	int startPos,endPos;
	PfaPtr *pPfaPtr = fcol.GetPfaPtrPtr();
	PfaPtr pfaPtr;
	if(_minSize == -1)
		startPos=0;
	else
	{
		startPos = 0;
		for(int i=0;i<size; ++i)
		{
			pfaPtr = pPfaPtr[i];
			if((int)pfaPtr->_length >= _minSize)
			{
				startPos = i;
				break;
			}
		}
	}
	if(startPos == size)
		return 0;
	
	if(_maxSize == -1)
		endPos = size-1;
	else
	{
		endPos = -1;
		for(int i=size-1; i>=0; --i)
		{
			pfaPtr = pPfaPtr[i];
			if((int)pfaPtr->_length <= _maxSize)
			{
				endPos = i;
				break;
			}
		}
	}
	if(endPos == -1 || endPos < startPos)
		return 0;

	
	_maxLength = (int)pPfaPtr[endPos]->_length;
	if(_maxLength > _retainSize)
		_maxLength = (int)_retainSize;
	string path;
	unsigned char* buf = new unsigned char[_maxLength];
	size_t readSize;
	_seqVector.resize(endPos-startPos+1);
	_seqVector.shrink_to_fit();
	for(int i=startPos;i<=endPos;++i)
	{
		 pfaPtr = pPfaPtr[i];
		 path = fcol._path + pfaPtr->_filename;
		 int orid = pPfaPtr[i]->_id;
		 if(fcol.Read(readSize,buf,sizeof(unsigned char),_maxLength,path.c_str(),orid))
		 {			 
			 ElementSequence &es=_seqVector[i-startPos];
			 es._rawFlowId = pfaPtr->_id;
			 es._series._content.resize(readSize);
			 es._series._content.shrink_to_fit();
			 memcpy(es._series.GetUCharContentPtr(),buf,readSize);
		 }
		 else
			 printf("Cannot read file %s at ElementSequenceCol::ReLoadEmSeqFromFileSetDefault", path.c_str());	
		 if((i+1-startPos)%1000 == 0)
			 printf(".");
		 if((i+1-startPos)%10000 == 0)
			 printf("%d",i+1-startPos);
		 if(i==endPos)
			 printf("\n");
	}
	delete []buf;
	this->_size = _seqVector.size();
	printf("Element Sequence instances:%d, min size:%d, max size:%d, retain size:%d.\n",_size, pPfaPtr[startPos]->_length,
		pPfaPtr[endPos]->_length, _maxLength);
	return this->_size;
}

int ElementSequenceCol::ReloadEmSeqFromUnkSet(IntVector &iv)
{
	for(size_t i=0; i<_seqVector.size(); ++i)
	{
		_seqVector[i]._series.SetCompact();		
	}
	EmSeqVector emv;
	int maxLen(0);
	for(size_t i=0; i<iv.size(); ++i)
	{
		ElementSequence &es = _seqVector[iv[i]];
		if((int)es._series._content.size() > maxLen)
			maxLen = es._series._content.size();
		emv.push_back(es);
	}
	_seqVector = emv;
	_seqVector.shrink_to_fit();
	_maxLength = maxLen;
	this->_size = _seqVector.size();
	return this->_size;
}


int ElementSequenceColMaxPercent::ReLoadEmSeqFromFileSet(PayloadFileCollection &fcol)
{
	ClearAll();
	fcol.AscendSortByFileLength();
	int fsize = fcol.GetFileSize();
	if(!fsize)
		return 0;
	int clipPos =int( (double)fsize * (1.0-_percent));
	if(clipPos < 0)
	{
		clipPos = 0;
		return 0;
	}
	if(clipPos > fsize)
		return 0;
	PfaPtr *pPfaPtr = fcol.GetPfaPtrPtr();
	PfaPtr pfaPtr;

	_maxLength = _retainSize;
	pfaPtr = pPfaPtr[fsize-1];
	if((int)pfaPtr->_length<_retainSize)
		_maxLength = pfaPtr->_length;
	string path;
	unsigned char* buf = new unsigned char[_maxLength];
	int readSize;
	FILE *fp;
	_seqVector.resize(fsize - clipPos);
	_seqVector.shrink_to_fit();
	for(int i=clipPos;i<fsize;++i)
	{
		 pfaPtr = pPfaPtr[i];
		 path = fcol._path + pfaPtr->_filename;
		 fp = fopen(path.c_str(), BINARY_READ);
		 if(fp)
		 {
			 readSize = fread(buf, sizeof(unsigned char),_maxLength,fp);
			 fclose(fp);
			 ElementSequence &es=_seqVector[i-clipPos];
			 es._rawFlowId = pfaPtr->_id;
			 es._series._content.resize(readSize);
			 es._series._content.shrink_to_fit();
			 memcpy(es._series.GetUCharContentPtr(),buf,readSize);
		 }
		 else
			 printf("Cannot read file %s at ElementSequenceCol::ReLoadEmSeqFromFileSetByMaxPercent", path.c_str());	 

	}
	delete []buf;
	this->_size = _seqVector.size();
	return this->_size;
}


void ElementSequenceCol::ClipSeqColLength(int length)
{
	if(_maxLength > length)
		_maxLength = length;
	int size =(int)this->_seqVector.size();
	for(int i=0; i<size; ++i)
	{
		_seqVector[i]._series.ClipElementSeries(length);		
	}
}


{
	_seqVector.clear();
	_size = 0;
	_maxLength = 0;
}

void ElementSequenceCol::ConvertUnkVectorToFileIdVector(IntVector &dest, IntVector &source)
{
	dest.clear();
	for(size_t i=0; i<source.size(); ++i)
	{
		dest.push_back(this->_seqVector[i]._rawFlowId);
	}
}

const char* ElementSequenceCol::ToSummaryString(string &str)
{
	char tc[256];
	str.clear();
	sprintf(tc,"Element Max Length:%d; Max Size:%d; Min Size:%d; Retain Size:%d; Element Size:%d",
		_maxLength,_maxSize,_minSize,_retainSize,_size);
	str = tc;
	return str.c_str();
}