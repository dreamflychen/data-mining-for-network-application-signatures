#include "audit.h"
#include "clocktime.h"
#include <omp.h>
void AuditSequence::_CountShingleInSequence(AuditElementMap &auMap,UCharVector &ucv, int start,int index)
{
	FragmentReference fr;
	int size = ucv.size();
	AdtEmMapIt it;
	int windowSize = _halfWindowSize*2+start;
	for(; start+_shingleSize <= size && start<windowSize; ++start)
	{
		fr.InitFromUCharVector(start, _shingleSize, ucv);
		
		it=auMap.find(fr);
		if(it == auMap.end())
		{
			FrequentCount c; 
			c._index = index;
			c._count = 1;
			auMap[fr] = c;
		}
		else
		{
			FrequentCount &c = it->second;
			if(c._index != index)
			{
				c._index = index;
				++c._count;
			}
		}	
	}
}

bool AuditSequence::_BuildWindow(AuditWindow &aw)
{
	AuditElementMap auMap;	
	EmSeqVector &emv = pEmCol->_seqVector;
	int flowCount = emv.size();
	
	for(int jflow = 0; jflow < flowCount; ++jflow)
	{
		
		UCharVector &ucv = emv[jflow]._series._content;
		int startPos = aw._startPos;
		if( startPos + _shingleSize > (int)ucv.size())
		{
			continue;
		}
		
		++aw._flowSize;
		_CountShingleInSequence(auMap,ucv,startPos, jflow);
	}
	if(!aw._flowSize)
		return false;
	_StatisticsWindows(aw, auMap);
	return true;
}

void AuditSequence::_StatisticsWindows(AuditWindow &aw, AuditElementMap &auMap)
{
	int min = numeric_limits<int>::max();
	int max = numeric_limits<int>::min();
	AdtEmMapIt it;
	FragmentReference ref;
	for(it = auMap.begin(); it != auMap.end(); ++it)
	{
		int count = (int)it->second._count;
		if(min > count)
			min = count;
		if(max < count)
		{
			max = count;
			ref = it->first;
		}
	}
	double factor = 0.0;
#ifdef ENABLE_FACTOR
	if(aw._startPos>0)
	{
		factor = pEmCol->_seqVector.size() - aw._flowSize;
		factor *=sin(((3.14/2)/(double)pEmCol->GetMaxElementSequenceLength())*(double)aw._startPos);
	}
#endif
	aw._maxSer.InitFromFragmentReference(ref);
	aw._maxFreq = (double)max/((double)aw._flowSize+factor);
	aw._minFreq = (double)min/((double)aw._flowSize+factor);
	aw._kindSize = auMap.size();
}


bool AuditSequence::GenerateAuditSequence(
		ElementSequenceCol &col		
		)
{	
	pEmCol = &col;
	_winSet.clear();
	int maxlen = col.GetMaxElementSequenceLength();
	int windowCount = maxlen/_halfWindowSize;
	if(windowCount<3)
	{
		printf("The window count cannot be lower than 3,now the window count is %d\n",windowCount);
		return false;
	}
	printf("Window: %d",windowCount);
	
	_winSet.resize(windowCount);
	AuditWindow *pEmptyaw = &_winSet[0];
	for(int i=0; i<windowCount; ++i)
	{
		
		pEmptyaw[i]._startPos = i*_halfWindowSize;
	}

	
	int flowCount = col._size;
	int iw=0;
	
#pragma omp parallel for lastprivate(iw) schedule(dynamic,4)
	for(iw = 0; iw <windowCount; ++iw)
	{		
		if(!_BuildWindow(pEmptyaw[iw]))
		{
			printf("No payload exceed the first shingle of the window %d!\n",iw);
			continue;
		}		
		printf(".");
	}
	
	for(iw = 0; iw<windowCount;++iw)
	{
		if(_winSet[iw]._flowSize==0)
			break;
	}
	if(iw<windowCount)
	{
		_winSet.resize(iw+2);
	}	
	
	return true;
}


bool AuditSequence::CalcSupport()
{
	_supportVector.clear();
	
	
	AdWinPtrSet ptrSet;
	int size = _winSet.size();
	ptrSet.resize(size);
	AdWinPtr adptr = &_winSet[0];
	for(int i=0;i<size;++i)
	{
		ptrSet[i] = (adptr+i);
	}
	
	sort(ptrSet.begin(), ptrSet.end(), AuditWindow::DescendByMaxFreq);
#ifdef ENABLE_WINDOW_SIZE_TEST
	_windowSupportVector.push_back(ptrSet[0]->_maxFreq);
#endif
#ifdef ENABLE_SHINGLE_SIZE_TEST
	_windowSupportVector.push_back(ptrSet[0]->_maxFreq);
#endif
	
	double max = -1.0;
	double dtemp;
	for(int j=0;j<size-1;++j)
	{
		dtemp = ptrSet[j]->_maxFreq-ptrSet[j+1]->_maxFreq;
		if(dtemp > max)
			max = dtemp;
	}
	
	
	
	int firstMaxPos;
	for(int k=0;k<size-1;++k)
	{
		dtemp = ptrSet[k]->_maxFreq-ptrSet[k+1]->_maxFreq;
		if(dtemp == max)
		{
			firstMaxPos = k+1;
			break;
		}
	}
	
	_lineSupport = 
		ptrSet[firstMaxPos]->_maxFreq + 
		(ptrSet[firstMaxPos-1]->_maxFreq - ptrSet[firstMaxPos]->_maxFreq)/2.0; 

	
	
	
	if(_lineSupport<_minLineSupport)
	{
		int lastPos = -1;
		
		for(int m=firstMaxPos-1;m>=0;--m)
		{
			if(ptrSet[m]->_maxFreq >= _minLineSupport)
			{
				lastPos = m;
			}
		}
		if(lastPos == -1)
		{
			return false;
			
		}
		for(int n=0;n<=lastPos;++n)
		{
			
			if(n == 0 ||
				n>0 && ptrSet[n]->_maxFreq != ptrSet[n-1]->_maxFreq)
				_supportVector.push_back(ptrSet[n]->_maxFreq);
		}
	}
	else
	{
		
		for(int n=0;n<=firstMaxPos-1;++n)
		{
			if(n == 0 ||
				n>0 && ptrSet[n]->_maxFreq != ptrSet[n-1]->_maxFreq)
			{
				if(ptrSet[n]->_maxFreq > _lineSupport)
					_supportVector.push_back(ptrSet[n]->_maxFreq);
			}
		}
		
		_supportVector.push_back(_lineSupport);
		
		//if(lowerSupport >= _minLineSupport)
		//	_supportVector.push_back(lowerSupport);
	}
	return true;
}

int AuditSequence::CalcIntervalLength(double support)
{
	int i = (int)this->_winSet.size() - 1;
	if(i<0)
		return 0;
	int maxLen = pEmCol->GetMaxElementSequenceLength();
	for(;i>=0;--i)
	{
		if(_winSet[i]._maxFreq >= support)
		{
			if(i+1 >= (int)_winSet.size())
			{
				return maxLen;
			}
			else
			{
				int length = _winSet[i+1]._startPos+ this->_halfWindowSize*2;
				if(length > maxLen)
					return maxLen;
				return length;
			}
		}
	}
	return 0;
}

const char * AuditSequence::ToMatLabTxtMatrix(string &str)
{
	str.clear();
	char tc[128];
	for(int i=0; i<(int)this->_winSet.size(); ++i)
	{
		sprintf(tc,"%d ",i);
		str += tc;
	}
	str +="\n";
	for(int i=0; i<(int)this->_winSet.size(); ++i)
	{
		sprintf(tc,"%d ",_winSet[i]._startPos);
		str += tc;
	}
	str +="\n";
	for(int i=0; i<(int)this->_winSet.size(); ++i)
	{
		sprintf(tc,"%d ",_winSet[i]._flowSize);
		str += tc;
	}
	str +="\n";
	for(int i=0; i<(int)this->_winSet.size(); ++i)
	{
		sprintf(tc,"%d ",_winSet[i]._kindSize);
		str += tc;
	}
	str +="\n";
	for(int i=0; i<(int)this->_winSet.size(); ++i)
	{
		sprintf(tc,"%f ",_winSet[i]._maxFreq);
		str += tc;
	}
	str +="\n";
	for(int i=0; i<(int)this->_winSet.size(); ++i)
	{
		sprintf(tc,"%f ",_winSet[i]._minFreq);
		str += tc;
	}
	
	str+="\n";
	string supStr;
	UtilityFunc::InputValueInArray(supStr,_lineSupport, _winSet.size());
	str += supStr;
	for(size_t i=0; i < _supportVector.size(); ++i)
	{
		str+="\n";
		UtilityFunc::InputValueInArray(supStr,_supportVector[i], _winSet.size());
		str += supStr;
	}
	return str.c_str();
}
const char * AuditSequence::ToSummaryString(string &str)
{
	str.clear();
	char tc[256];
	
	this->_lineSupport;
	this->_minLineSupport;
	this->_winSet;
	this->_supportVector;
	sprintf(tc,"Audit Info:\n\t Half Window Size: %d, Line Support: %.3f, Min Threshold: %.3f, Window Size: %d\n",
		this->_halfWindowSize,_lineSupport,_minLineSupport,_winSet.size());
	str += tc;
	sprintf(tc,"\t Support Vector(%d):",_supportVector.size());
	str += tc;
	for(int i=0;i<(int)_supportVector.size(); ++i)
	{		
		if(_supportVector[i] == _lineSupport)
			sprintf(tc,"*%.3f*",_supportVector[i]);
		else
			sprintf(tc,"%.3f",_supportVector[i]);
		str += tc;
		if(i < (int)_supportVector.size()-1)
			str+=", ";
	}
	return str.c_str();
}