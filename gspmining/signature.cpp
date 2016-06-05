#include "signature.h"

const char* Signature::ToString(string &str)
{
	str.clear();
	for(size_t i=0; i<this->_fragSet.size(); ++i)
	{
		string temp;
		str += _fragSet[i].ToString(temp);
		if(i < _fragSet.size()-1)
			str += "->";
	}
	return str.c_str();
}

bool Signature::IsPattern(ElementSeries &sr)
{
	int pos = 0;
	int prePos = 0;
	for(size_t i=0; i < this->_fragSet.size(); ++i)
	{
		SignatureFragment &fi = _fragSet[i];
		pos = sr.FirstFind(pos,sr._content.size(),fi._content);
		if(pos == -1)
			return false;
		
		
		if(i==0 && pos >= fi._lowerPreOffset && pos <= fi._lowerPreOffset
			|| i>0 && (pos-prePos)>=fi._lowerPreOffset && (pos-prePos) <= fi._upperPreOffset)
		{
			pos = pos+fi._content.size();
			prePos = pos;
			
		}
		else
			return false;
	}
	return true;
	
}

bool Signature::Merge(Signature &sig)
{
	if(sig._fragSet.size() != _fragSet.size())
		return false;
	SigFragVec temp = _fragSet;
	for(size_t i=0;i<temp.size(); ++i)
	{
		if(!temp[i].Merge(sig._fragSet[i]))
		{
			return false;
		}
	}
	_fragSet = temp;
	++_mergeCount;
	return true;
}

void SignatureCollection::InitFromRuleForest(RuleForest &ruleForest,double minSup,int maxKinds)
{
	ClearAll();
	_minSup = minSup;
	_maxKinds = maxKinds;
	for(size_t i=0; i<ruleForest._treeEntrySet.size(); ++i)
	{
		FragIntervalVec fvec;
		_RecursiveVisit(fvec,ruleForest._treeEntrySet[i]);
	}
}

void SignatureCollection::_RecursiveVisit(FragIntervalVec fvec,FragNodePtr fragPtr)
{
	fvec.push_back(fragPtr->_frag);
	if(fragPtr->_child.size() == 0)
	{
		if(fragPtr->_allEmSeqSup>=_minSup)
		{
			Signature sig;
			sig.InitSigFragVecFromFragIntervalVec(fvec,_maxKinds);
			
			_AddSignature(sig);
		}
		return;
	}
	else
	{
		if(fragPtr->_ruleSet.size()>0)
		{
			if(fragPtr->_allEmSeqSup>=_minSup)
			{
				Signature sig;
				sig.InitSigFragVecFromFragIntervalVec(fvec,_maxKinds);
				
				_AddSignature(sig);
			}
		}
		for(size_t i=0; i<fragPtr->_child.size(); ++i)
		{	
			_RecursiveVisit(fvec,fragPtr->_child[i]);
		}
	}
}

void SignatureCollection::_CalcEarn()
{
	

	
	_maxSignatureFragCounts = 0;
	_maxSignatureLength = 0;
	_maxMergeCounts = 0;
	for(size_t i=0; i<this->_set.size();++i)
	{
		int is=_set[i].GetFragSize();
		if(is>_maxSignatureFragCounts)
			_maxSignatureFragCounts = is;
		is = _set[i].GetLength();
		if(is>_maxSignatureLength)
			_maxSignatureLength = is;
		is = _set[i]._mergeCount;
		if(is>_maxMergeCounts)
			_maxMergeCounts = is;
	}

	
	double A=2.0;
	double B=1.0;
	double C=1.0;
	double D=1.0;
	double E=1.0;
	for(size_t i=0; i<this->_set.size(); ++i)
	{
		Signature &sig = _set[i];
		int fp=sig.GetMinFloatPos();
		fp+=1;
		sig._earn = 
			A*sig._support/(double)fp +													
			B/(double)sig.GetFragSize() +												
			C*(double)sig.AverageFragLength()/(double)sig.GetLongestFragLength() +		
			D*(double)sig._mergeCount/((double)_maxMergeCounts+1) +							
			E/sqrt((double)(sig.GetFirstPos()+1));										
	}
}

void SignatureCollection::_AddSignature(Signature &sig)
{
	for(size_t i=0; i<this->_set.size();++i)
	{
		Signature &s=_set[i];
		if(s.Merge(sig))
			return;
	}
	_set.push_back(sig);
}

void SignatureCollection::InitFromLayerClass(LayerRoot &layerRoot,double minSup,int maxKinds)
{
	ClearAll();
	_minSup = minSup;
	_maxKinds = maxKinds;
	for(size_t i=0; i < layerRoot._child.size(); ++i)
	{
		if(!layerRoot._child[i]->_bUnderThreshold)
		{
			if(layerRoot._child[i]->_allEmSeqSup>= _minSup)
			{
				Signature sig;
				sig.InitSigFragVecFromFragIntervalVec(layerRoot._child[i]->_pRR->_set,_maxKinds);
				
				_AddSignature(sig);
			}
		}
	}
}

void SignatureCollection::InitEmpty(int maxKinds)
{
	ClearAll();
	_maxKinds = maxKinds;
}

const char* SignatureCollection::ToRegexString(string &str,string tag)
{
	str.clear();
	char cc[32];
	str += "Regex String:\n";
	for(size_t i=0; i < _set.size(); ++i)
	{
		string content;
		wstring regexStr;
		str += tag;
		str += " ";		
		sprintf(cc,"%f",_set[i]._earn);
		str += cc;
		str += " ";
		UtilityFunc::UnicodeCharToMultiChar(content,_set[i].ToRegexString(regexStr));
		str += content;
		str += '\n';
	}
	return str.c_str();
}

const char* SignatureCollection::ToString(string &str)
{
	str.clear();	
	char cc[128];
	sprintf(cc,"Signature size:%d; Unknow sequence Size:%d; Identify Rate:%f\n",
		this->_set.size(),this->_unkSet.size(),this->_identifyRate);
	str += cc;
	wstring reg;
	for(size_t i=0; i < _set.size(); ++i)
	{
		string content;
		str += _set[i].ToString(content);
		sprintf(cc," | counts: %d, support:%f, earn:%f, frag size:%d, merge count: %d",
			_set[i]._count,_set[i]._support,_set[i]._earn,_set[i]._fragSet.size(),_set[i]._mergeCount);
		str += cc;
		str += "\n\t";
		str +="Regex: ";
		UtilityFunc::UnicodeCharToMultiChar(content,_set[i].ToRegexString(reg));
		str += content;
		str +='\n';
	}
	return str.c_str();
}

void SignatureCollection::Merge(SignatureCollection &sc)
{
	size_t ssize = _set.size();
	for(size_t i = 0; i<sc._set.size(); ++i)
	{
		bool res = false;		
		for(size_t j=0; j<ssize; ++j)
		{
			if(_set[j].Merge(sc._set[i]))
			{
				res = true;
				break;
			}
		}
		if(!res)
			_set.push_back(sc._set[i]);
	}
}

void SignatureCollection::Update(ElementSequenceCol &escol)
{

	
	RegexVector regvec;
	regvec.resize(_set.size());
	wstring reg;
	for(size_t i=0; i<_set.size(); ++i)
	{
		regvec[i].assign(_set[i].ToRegexString(reg));
		
		_set[i]._count = 0;
		_set[i]._earn = 0;
		_set[i]._support = 0;
	}
	this->_pEmSeqCol = &escol;
	BoolVector bv;
	bv.resize(escol._seqVector.size());
	
	
	
	
	void *pbv = &bv[0];
	memset(pbv,0,sizeof(bool)*bv.size());

	for(size_t i=0; i<regvec.size(); ++i)
	{
		WCharVector wv;
		for(size_t j=0; j<escol._seqVector.size(); ++j)
		{
			ElementSequence &sv = escol._seqVector[j];
			UtilityFunc::UCharVectorToWCharVector(wv, sv._series._content);
			if(regex_search(wv.begin(),wv.end(),regvec[i]))
			{
				bv[j] = true;
				Signature &sig = _set[i];
				++sig._count;
			}
		}
	}
	
	
	for(size_t i=0; i < _set.size(); ++i)
	{
		Signature &sig = _set[i];		
		sig._support = (double)sig._count / (double)escol._size;
	}

	
	for(size_t i=0; i<escol._seqVector.size();++i)
	{
		if(!bv[i])
		{
			this->_unkSet.push_back(i);
		}
	}
	_identifyRate = (double)(escol._size-_unkSet.size())/(double)escol._size;
	

	_CalcEarn();
	sort(this->_set.begin(),this->_set.end(), Signature::DescendByEarn);
}

void SignatureCollection::ClearAll()
{
	this->_maxMergeCounts = 0;
	this->_maxSignatureFragCounts = 0;
	this->_maxSignatureLength = 0;
	this->_pEmSeqCol = 0;
	this->_set.clear();
	this->_unkSet.clear();
}

void SignatureCollection::HeuristicCut()
{
	SignatureSet ts;
	for(size_t i=0; i<_set.size(); ++i)
	{		
		if(_set[i]._fragSet.size()==1 && 
			_set[i]._fragSet[0]._content.size()==1 &&
			(_set[i]._fragSet[0]._content[0] == 0||_set[i]._fragSet[0]._content[0]==0xff))
		{
			continue;
		}
		else if(_set[i]._support < HEURISTIC_RATE)
		{
			continue;
		}
		else 
			ts.push_back(_set[i]);
	}
	this->_set = ts;
}