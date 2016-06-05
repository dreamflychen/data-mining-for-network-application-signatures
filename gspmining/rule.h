#ifndef RULE_H
#define RULE_H

#include "stdcommonheader.h"
#include "fragment.h"

struct ElementRule
{
	//the offset to the head
	IntVector _posVec;
	//indexing element vertex
	int _index;
	ElementRule():_index(0){}
};

typedef vector<ElementRule> ElementRuleVec;

struct FragPtrVecHashAndEq
{
	//generage hash value
	size_t operator()(const FragPtrVec &ptrRef) const
	{
		register unsigned int nr=1, nr2=4;
		UCharVector ucv;
		for(int i=0;i<(int)ptrRef.size();++i)
		{
			int j=0;
			int length = ptrRef[i]->_content.size();
			const unsigned char *s = &(ptrRef[i]->_content[0]);
			while (length--)
			{
				nr ^= (((nr & 63)+nr2)*((unsigned int) (unsigned char) s[j++]))+ (nr << 8);
				nr2 += 3;
			}
		}
		return(nr);	
	}
	//compare
	bool operator()(const FragPtrVec& left, const FragPtrVec& right) const
	{	
		if(left.size() != right.size())
			return false;
		for(size_t i=0;i<left.size();++i)
		{
			if(!UtilityFunc::IsTwoUCharVectorEqual(left.at(i)->_content,right.at(i)->_content))
				return false;
		}		
		return true;
	}
};

struct FragInterval
{
	FragPtr _fPtr;
	int _upperPreOffset;
	int _lowerPreOffset;
	int _upperPos;
	int _lowerPos;
	FragInterval():_fPtr(0),_upperPreOffset(0),_lowerPreOffset(0),_upperPos(0),_lowerPos(0){}
	//verify if the given fragment is in the current fragment
	bool IsGivenInThis(FragInterval &fi)
	{
		if(UtilityFunc::IsTwoUCharVectorEqual(fi._fPtr->_content,_fPtr->_content)
			&& this->_upperPos>=fi._upperPos &&
			   this->_lowerPos<=fi._lowerPos)
			   return true;
		return false;
	}
	bool IsIntersect(FragInterval &fi)
	{
		if(UtilityFunc::IsTwoUCharVectorEqual(fi._fPtr->_content,_fPtr->_content))
		{
			if(this->_upperPos<fi._lowerPos || this->_lowerPos>fi._upperPos)
				return false;
			return true;
		}
		return false;
	}
	bool Merge(FragInterval &fi)
	{
		if(IsIntersect(fi))
		{
			this->_upperPos = this->_upperPos>fi._upperPos?_upperPos:fi._upperPos;
			this->_lowerPos = this->_lowerPos<fi._lowerPos?_lowerPos:fi._lowerPos;
			this->_upperPreOffset = this->_upperPreOffset>fi._upperPreOffset?_upperPreOffset:fi._upperPreOffset;
			this->_lowerPreOffset = this->_lowerPreOffset<fi._lowerPreOffset?_lowerPreOffset:fi._lowerPreOffset;
			return true;
		}
		return false;
	}
	const char* ToString(string &str)
	{
		str.clear();
		char cc[32];
		if(_lowerPreOffset == _upperPreOffset)
			sprintf(cc,"(%d",_lowerPreOffset);
		else
			sprintf(cc,"(%d~%d",_lowerPreOffset,_upperPreOffset);
		str += cc;
		if(_lowerPos == _upperPos)
			sprintf(cc,"[%d])",_lowerPos);
		else
			sprintf(cc,"[%d~%d])",_lowerPos,_upperPos);
		str += cc;
		string content;
		UtilityFunc::UCharVectorToString(content, _fPtr->_content);
		str += content;
		return str.c_str();
	}
};

typedef vector<FragInterval> FragIntervalVec;

struct RelatedRule;
typedef RelatedRule* RelRulePtr;
typedef vector<RelRulePtr> RelRulePtrVec;


struct RelatedRule
{
	RelatedRule():_count(0),_support(0.0){}	

	//offset of each fragment
	ElementRuleVec _emRuleVec;
	
	FragIntervalVec _set;
	//the number of samples
	int _count;
	//the support value of the current rule
	double _support;


	//update the interval between each fragment
	void UpdateInterval();

	
	static bool DescentRelatedRulePtrByCount(RelRulePtr &i,RelRulePtr &j)
	{		
		return (i->_count > j->_count);
	}
 

	static bool AscendRelatedRulePtrByLength(RelRulePtr &i,RelRulePtr &j)
	{
		if(i->_set.size()!=j->_set.size())
			return (i->_set.size() < j->_set.size());
		size_t sumi(0),sumj(0);
		for(size_t k=0;k<i->_set.size();++k)
		{
			sumi += i->_set[k]._fPtr->_content.size();
			sumj += j->_set[k]._fPtr->_content.size();
		}
		return sumi < sumj;
	}

	int FindFragPos(FragInterval &fi)
	{
		for(size_t i = 0; i<this->_set.size(); ++i)
		{
			if(this->_set[i].IsGivenInThis(fi))
				return i;
		}
		return -1;
	}


	bool isPrefixOfGiven(RelatedRule &rr)
	{
		if(this->_set.size() >= rr._set.size())
			return false;
		for(size_t i=0; i<this->_set.size(); ++i)
		{
			if(!_set[i].IsGivenInThis(rr._set[i]))
				return false;
		}
		return true;
	}


	bool IsSpecialOfGiven(RelatedRule &rr)
	{
		if(this->_set.size()<rr._set.size())
			return false;
		else if(this->_set.size() == rr._set.size())
		{
			for(size_t i=0; i<_set.size();++i)
			{

				if(!rr._set[i].IsGivenInThis(this->_set[i]))
					return false;
			}
			return  true;
		}
		else
		{
			size_t nextpos = 0;
			for(size_t i=0;i<rr._set.size();++i)
			{
				size_t k = nextpos;
				for(;k<this->_set.size();++k)
				{
					if(rr._set[i].IsGivenInThis(this->_set[k]))
					{
						nextpos = k+1;
						break;
					}
				}
				if(k >= _set.size())
					return false;
			}
			return true;
		}
		return false;
	}



	const char* ToString(string &str)
	{
		char cc[256];
		str.clear();
		for(size_t j=0; j<_set.size(); ++j)//output each fragment
		{
			string content;
			FragInterval &fi = _set[j];			
			str += fi.ToString(content);		
			if(j!=_set.size()-1)
				str += " -> ";
		}
		sprintf(cc," |count:%d; sup:%.3f; frag size:%d",_count,_support,_set.size());
		str += cc;
		return str.c_str();
	}
	
};

struct RelRulePtrCompare
{
	bool operator()(const RelRulePtr s1, const RelRulePtr s2) const
	{
		return s1<s2;
	}

};

typedef set<RelRulePtr,RelRulePtrCompare> RelRulPtrSet;
typedef RelRulPtrSet::iterator RelRulPtrSetIt;

typedef unordered_map<FragPtrVec,RelatedRule,FragPtrVecHashAndEq,FragPtrVecHashAndEq>  FragRuleMap;
typedef FragRuleMap::iterator FragRuleMapIt;


struct IntElementSequence
{
private:

	IntVector _iSeq;
public:
	void GenerateSeqFromUCharSeq(ElementSequence &emSeq,int logicLength);

	void ReplaceSequenceByFragment(FragPosVec &fpv,Fragment &frag,ElementSequence &emSeq);
};

struct RelatedRuleCollection
{
private:
	
	FragAllLevCol *pFragAllCol;
	ElementSequenceCol *pEmCol;

	void AddElementSeqToMap(FragPosVec &fpv,ElementSequence &emSeq,int index);

	void GenerateFragPtrVecFromElement(ElementSequence &emSeq,int index);
public:

	FragRuleMap	_frmap;
	IntVector	_unkEmVec;

	void GenerateRelatedRuleCol(FragAllLevCol &fcol,ElementSequenceCol &emCol)
	{
		_frmap.clear();
		_unkEmVec.clear();
		pFragAllCol = &fcol;
		pEmCol = &emCol;
		for(int i=0;i<(int)emCol._seqVector.size();++i)
		{
			GenerateFragPtrVecFromElement(emCol._seqVector[i],i);
		}

		for(FragRuleMapIt it = this->_frmap.begin(); it != _frmap.end(); ++it)
		{
			it->second.UpdateInterval();
		}
	}
	int GetTotalRecognizedSize()
	{
		int total=0;
		for(FragRuleMapIt it = this->_frmap.begin(); it != _frmap.end(); ++it)
		{
			total +=it->second._count;
		}
		return total;
	}
	const char* ToString(string &str);
	
};

#endif