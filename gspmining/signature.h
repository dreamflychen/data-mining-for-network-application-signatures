#ifndef SIGNATURE_H
#define SIGNATURE_H
#include "stdcommonheader.h"
#include "layer.h"
#include "rule.h"

typedef vector<wregex> RegexVector;

struct SignatureFragment
{
	UCharVector _content;
	int _lowerPreOffset;
	int _upperPreOffset;
	SignatureFragment():_lowerPreOffset(0),_upperPreOffset(0)
	{
	}
	bool Merge(SignatureFragment &sf)
	{
		if(!UtilityFunc::IsTwoUCharVectorEqual(_content, sf._content))
			return false;
		if(this->_lowerPreOffset > sf._upperPreOffset || this->_upperPreOffset < sf._lowerPreOffset)
			return false;
		this->_lowerPreOffset = sf._lowerPreOffset<_lowerPreOffset?sf._lowerPreOffset:_lowerPreOffset;
		this->_upperPreOffset = sf._upperPreOffset<_upperPreOffset?_upperPreOffset:sf._upperPreOffset;
		return true;
	}
	const char* ToString(string &str)
	{
		str.clear();
		char cc[64];
		if(_lowerPreOffset == _upperPreOffset)
			sprintf(cc,"(%d)",_lowerPreOffset);
		else
			sprintf(cc,"(%d,%d)", _lowerPreOffset, _upperPreOffset);
		str += cc;
		string content;
		UtilityFunc::UCharVectorToString(content,_content);
		str += content;
		return str.c_str();
	}
	const wchar_t* ToRegexString(wstring &str)
	{
		str.clear();
		wstring regStr;
		char cc[32];
		wchar_t wcc[32];
		if(_lowerPreOffset==_upperPreOffset)
		{
			if(_lowerPreOffset > 0)
			{
				str += L"[\\u0000-\\uFFFF]";
				sprintf(cc,"{%d}",_lowerPreOffset);
				UtilityFunc::MultiCharToUnicodeChar(wcc,cc);
				str += wcc;
			}
		}
		else
		{
			str += L"[\\u0000-\\uFFFF]";
			sprintf(cc,"{%d,%d}",_lowerPreOffset,_upperPreOffset);
			UtilityFunc::MultiCharToUnicodeChar(wcc,cc);
			str += wcc;
		}
		str += UtilityFunc::UCharVectorToRegexString(regStr,_content);
		return str.c_str();
	}
};
typedef vector<SignatureFragment> SigFragVec;

struct Signature
{
	Signature():_mergeCount(0),_support(0),_count(0),_earn(0){}

	SigFragVec _fragSet;
	int _mergeCount;
	double _support;
	int _count;
	double _earn;
	const char* ToString(string &str);

	const wchar_t* ToRegexString(wstring &str)
	{
		str.clear();
		str+=L"^";
		wstring fragRegStr;
		for(size_t i=0; i<this->_fragSet.size(); ++i)
		{
			str += _fragSet[i].ToRegexString(fragRegStr);
		}
		return str.c_str();
	}
	
	void InitSigFragVecFromFragIntervalVec(FragIntervalVec &fv,int maxSize)
	{
		_fragSet.clear();
		for(size_t i=0; i<fv.size(); ++i)
		{
			SignatureFragment sf;
			sf._content = fv[i]._fPtr->_content;
			sf._lowerPreOffset = fv[i]._lowerPreOffset;
			sf._upperPreOffset = fv[i]._upperPreOffset;
			_fragSet.push_back(sf);
		}
		MergeContinue(maxSize);
	}
	
	bool Merge(Signature &sig);
	
	void MergeContinue(int maxKinds){
		SigFragVec temp;
		for(size_t i=0; i < _fragSet.size(); ++i)
		{
			if(i==0)
				temp.push_back(_fragSet[i]);
			else
			{
				if(!_fragSet[i]._lowerPreOffset && !_fragSet[i]._upperPreOffset)
				{
					SignatureFragment &sf = temp[temp.size()-1];
					sf._content.insert(sf._content.end(),_fragSet[i]._content.begin(),_fragSet[i]._content.end());
				}
				else
					temp.push_back(_fragSet[i]);
			}
		}
		if((int)temp.size()>maxKinds)
		{
			temp.resize(maxKinds);
		}
		_fragSet = temp;
	}
	
	bool IsPattern(ElementSeries &sr);
	
	int GetFragSize(){return _fragSet.size();}
	
	int GetLength(){
		int len = 0;
		for(size_t i = 0; i < _fragSet.size(); ++i)
		{
			len += (int)_fragSet[i]._content.size();
		}
		return len;
	}
	
	int GetLongestFragLength(){
		int len = 0;
		for(size_t i = 0; i < _fragSet.size(); ++i)
		{
			if((int)_fragSet[i]._content.size()>len)
				len = (int)_fragSet[i]._content.size();
		}
		return len;
	}
	
	double AverageFragLength()
	{
		double avg=0;
		for(size_t i = 0; i < _fragSet.size(); ++i)
		{
			avg += _fragSet[i]._content.size();
		}
		avg /= (double)GetFragSize();
		return avg;
	}
	
	int GetMinFloatPos()
	{
		int min = numeric_limits<int>::max();;
		for(size_t i = 0; i < _fragSet.size(); ++i)
		{
			int off = _fragSet[i]._upperPreOffset - _fragSet[i]._lowerPreOffset;
			if(off < min)
				min = off;
		}
		return min;
	}
	
	int GetFirstPos()
	{
		return _fragSet[0]._lowerPreOffset;
	}
	
	static bool DescendByEarn(Signature &one, Signature &two)
	{
		return one._earn > two._earn;
	}
};
typedef vector<Signature> SignatureSet;

struct SignatureCollection
{
private:
	
	void _RecursiveVisit(FragIntervalVec fvec,FragNodePtr fragPtr);
	
	void _CalcEarn();
	void _AddSignature(Signature &sig);
	
public:
	SignatureCollection():_maxSignatureLength(0),_maxSignatureFragCounts(0),_minSup(0),_maxKinds(MAX_KINDS),_identifyRate(0){}
	SignatureSet _set;
	ElementSequenceCol *_pEmSeqCol;
	IntVector _unkSet;
	
	int _maxSignatureLength;
	
	int _maxSignatureFragCounts;
	int _maxMergeCounts;
	int _maxKinds;
	
	double _minSup;
	
	double _identifyRate;
	
	void InitFromRuleForest(RuleForest &ruleForest,double minSup,int maxKinds=MAX_KINDS);
	
	void InitFromLayerClass(LayerRoot &layerRoot,double minSup,int maxKinds=MAX_KINDS);
	
	void InitEmpty(int maxKinds=MAX_KINDS);
	const char* ToString(string &str);
	const char* SignatureCollection::ToRegexString(string &str,string tag);
	void Merge(SignatureCollection &sc);
	void Update(ElementSequenceCol &escol);
	void ClearAll();
	
	void HeuristicCut();
};

struct SpecificSignature
{
	SignatureCollection _scol;
	bool _isBestSupport;
	double _support;
	SpecificSignature():_isBestSupport(false),_support(0)
	{
	}
};
typedef vector<SpecificSignature> SpecSigSet;

#endif