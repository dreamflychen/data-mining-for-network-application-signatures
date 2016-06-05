#ifndef FRAGMENT_H
#define FRAGMENT_H

#include "stdcommonheader.h"
#include "element.h"
#include "constdefine.h"
#include "audit.h"
#include "clocktime.h"

struct FragLevPos
{
private:
	int *_pFirstCount;
	int *_pCurrentCount;
	
	bool _IsCurrentLevelSatisfy(int minCount)
	{
		for(int i=0; i<_size;++i)
		{
			if(_pCurrentCount[i] >= minCount)
				return true;
		}
		return false;
	}
	
	void _UpdateNextLevel(int minCount)
	{
		int temp;
		int nextPos;
		for(int i=0;i<_size; ++i)
		{
			temp = _pCurrentCount[i];
			nextPos = i+_level-1;
			if(nextPos < _size)
				_pCurrentCount[i] = temp + _pFirstCount[nextPos];
			else
				_pCurrentCount[i] = _pCurrentCount[i-1];
		}
	}
public:

	FragLevPos(int length):_level(1),_size(length){
		_pFirstCount = new int[length];
		_pCurrentCount = new int[length];
		memset(_pFirstCount, 0, sizeof(int)*length);
		memset(_pCurrentCount, 0, sizeof(int)*length);
	}

	int _level;
	int _size;
	int GetPosCount(int pos)
	{
		return this->_pCurrentCount[pos];
	}
	void AddPosCount(int pos)
	{
		++_pFirstCount[pos];
	}
	void ZeroPosCount(int pos)
	{
		this->_pCurrentCount[pos]=0;
	}
	bool Update(int minCount,int maxLev)
	{
		memcpy(_pCurrentCount,_pFirstCount,sizeof(int)*_size);
		while(!_IsCurrentLevelSatisfy(minCount))
		{
			++_level;
			if(_level > maxLev)
			{
				--_level;
				return false;
			}
			else
			{
				_UpdateNextLevel(minCount);
			}
		}
		return true;		
	}
	~FragLevPos(){delete []_pFirstCount; delete []_pCurrentCount;}
};

struct CandidateFragmentAttr{
	int _count;						
	union{
	int _index;						
	FragLevPos *_pPosCount;				
	};
	int _leftItem;					
	int _rightItem;					
	CandidateFragmentAttr():_count(0),_index(-1),_leftItem(-1),_rightItem(-1){}
};



typedef unordered_map<UCharVector, CandidateFragmentAttr, UCharVectorHashAndEq,UCharVectorHashAndEq> CdFragMap;
typedef CdFragMap::iterator CdFragMapIt;
typedef unordered_map<UCharVector, IntVector, UCharVectorHashAndEq,UCharVectorHashAndEq> PartFragMap;
typedef PartFragMap::iterator PtFragMapIt;

struct FrequentPos
{
	int _pos;
	int _count;
	FrequentPos():_pos(0),_count(0){}
	const char* ToString(string &str) const;
};

typedef vector<FrequentPos> FreqPosVector;

struct FragPosSet
{
	int _level;
	FreqPosVector _posVec;
};


struct Fragment
{
	UCharVector _content;			
	int _count;						
	double _support;					
	bool _isClosedItem;				
	int _leftItem;					
	int _rightItem;					
	FragPosSet _posSet;
	
	Fragment():_count(0),_support(0),_isClosedItem(true),_leftItem(-1),_rightItem(-1){}

	
	void InitPosVector(FragLevPos &pos,int logicLength,int minCount);

	
	void InitContent(UCharVector &content);

	
	void InitFragment(int count, double support,bool isClosedItem, 
		int leftItem, int rightItem);

	
	int SearchPosCount(int pos);
	const char* ToString(string &str) const;

	static bool DescentFragmentSort(Fragment *i,Fragment *j)
	{
		if(i->_content.size()!=j->_content.size())
			return ( i->_content.size() > j->_content.size());
		return (i->_count > j->_count);
	}
};
typedef Fragment * FragPtr;
typedef vector<FragPtr> FragPtrVec;



typedef vector<Fragment> FragmentSet;
typedef FragmentSet::iterator FragSetIt;


struct FragmentPos
{
	
	FragPtr _fPtr;
	
	int _pos;

	FragmentPos():_fPtr(0),_pos(0){}

	static bool AscentFragmentPos(FragmentPos &i,FragmentPos &j)
	{		
		return (i._pos<j._pos);
	}

};



typedef vector<FragmentPos> FragPosVec;


struct FragmentCollection
{
private:
	
	int _minCount;

	
	int _minRelCount;
	
	double _cosine;
	
	int _logicLength;
	
	int _maxOffset;

	FragmentCollection *_preFragCol;	
	ElementSequenceCol *_pSeqCol;		

	
	bool _Level_1_Mining();
	
	bool _Level_2_Mining();
	
	bool _Level_3_N_Mining();

	
	virtual bool _RelationAnalysis(FragLevPos *pos, Fragment &fragment);
public:
	FragmentCollection():_minCount(0),_minRelCount(0),_cosine(0),_logicLength(0),_maxOffset(0){}

	FragmentSet _set;				
	int _level;					

	
	bool GenerateFrequentFragment(FragmentCollection *preFragCol,ElementSequenceCol *pSeqCol,int logicLength,
		int minCount,double cosine, int minRelCount,int maxOffset)
	{
		_set.clear();
		_preFragCol = preFragCol;
		_pSeqCol = pSeqCol;
		_logicLength = logicLength;
		_minCount = minCount;
		_minRelCount = minRelCount;
		_cosine = cosine;
		_maxOffset = maxOffset;
		if(!preFragCol)
		{
			_level = 1;
			return _Level_1_Mining();
		}
		else if(preFragCol->_level == 1)
		{
			_level = 2;
			return _Level_2_Mining();
		}
		else
		{
			_level = preFragCol->_level +1;
			return _Level_3_N_Mining();
		}
	}
	const char* ToString(string &str) const;
};

typedef FragmentCollection * FragColPtr;
typedef vector<FragColPtr> FragColPtrSet;

struct FragAllLevCol
{
private:
	
	int _minCount;
	
	int _minRelCount;
	
	ElementSequenceCol *_pSeqCol;	
public:
	FragAllLevCol():_pSeqCol(0),_minSupport(MIN_SUPPORT),_cosine(COSINE),_scale(SCALE),_logicLength(0),_minCount(0),_minRelCount(0),_maxOffset(MAX_OFFSET){}	
	~FragAllLevCol(){
		for(int i=0;i<(int)_fragSet.size();++i)
		{
			delete _fragSet[i];
		}
	}
	
	double _minSupport;
	
	double _cosine;
	
	double _scale;
	
	int _logicLength;
	
	int _maxOffset;

	FragColPtrSet	_fragSet;
	FragPtrVec		_closeFragSet;

	size_t getCloseFragSetSize()
	{
		return _closeFragSet.size();
	}

	double getAverageCloseFragSize()
	{
		size_t cfSize = _closeFragSet.size();
		if(cfSize==0)
			return 0;
		double sum = 0;
		for(size_t i=0; i<cfSize; ++i)
		{
			sum+=_closeFragSet[i]->_content.size();
		}
		return (sum/(double)cfSize);
	}

	void setParam(double minSup,double cos,double scale,int maxOffset){
		_minSupport = minSup;
		_cosine = cos;
		_scale = scale;
		_maxOffset = maxOffset;
	}

	void ClearAll()
	{
		for(int i=0; i<(int)_fragSet.size(); ++i)
		{
			delete _fragSet[i];
		}
		_fragSet.clear();
		this->_closeFragSet.clear();
	}

	
	FragPtrVec &GenerateClosedFrag()
	{
		for(int i=0; i<(int)_fragSet.size(); ++i)
		{
			FragColPtr &fs = _fragSet[i];
			for(int j=0;j< (int)fs->_set.size();++j)
			{
				Fragment &fg = fs->_set[j];
				if(fg._isClosedItem == true)
				{
					this->_closeFragSet.push_back(&fg);
				}
			}
		}
		
		_closeFragSet.shrink_to_fit();
		return _closeFragSet;
	}

	bool GenerateAllLevel(ElementSequenceCol *_pSeqCol, int logicLength)
	{
		ClearAll();
		_logicLength = logicLength;
		_minCount = (int)(_minSupport * _pSeqCol->_size);
		_minRelCount = (int)(_minSupport*_pSeqCol->_size*_scale);

		bool res = true;
		int level=1;
		FragmentCollection *pPreFragCol = 0;
		
		while(res)
		{						
			FragmentCollection *pfcol = new FragmentCollection;
			res = pfcol->GenerateFrequentFragment(pPreFragCol, _pSeqCol,logicLength, _minCount, _cosine, _minRelCount,_maxOffset);
			if(!res)
			{				
				delete pfcol;
				break;
			}
			else
			{
				pPreFragCol = pfcol;
				_fragSet.push_back(pPreFragCol);
				++level;
			}
			printf(".");
		}
		return (GenerateClosedFrag().size() != 0);
	}

	const char* ToString(string &str) const;
};

#endif