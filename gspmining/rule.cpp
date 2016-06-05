#include "rule.h"

void IntElementSequence::GenerateSeqFromUCharSeq(ElementSequence &emSeq,int logicLength)
{
	int s = (int)emSeq._series._content.size()>logicLength?logicLength:(int)emSeq._series._content.size();
	_iSeq.resize(s);
	for(int i=0;i<s;++i)
	{
		_iSeq[i] = emSeq._series._content[i];
	}
}

{
	int s=_iSeq.size();
	int c = frag._content[0];
	ElementSeries &es = emSeq._series;
	IntVecPtr *ip = es._pPosTable;
	if(!ip||!ip[c])return;
	IntVector &iv = *ip[c];
	int pos;
	for(int i=0;i<(int)iv.size();++i)
	{
		pos = iv[i];
		if(pos>=s)
			break;
		if(pos+(int)frag._content.size()>s)
			break;
		int j=0;
		for(; j<(int)frag._content.size();++j)
		{
			if(_iSeq[j+pos] != frag._content[j])
				break;
		}
		if(j == (int)frag._content.size())
		{
			
			if( frag.SearchPosCount(pos) )
			{
				
				for(int k=pos; k<pos+(int)frag._content.size();++k)
				{
					_iSeq[k] = -1;
				}
				
				FragmentPos fragPos;
				fragPos._fPtr = &frag;
				fragPos._pos = pos;
				fpv.push_back(fragPos);
			}
		}

	}
}

void RelatedRule::UpdateInterval()
{		
	
	for(int i=0;i<(int)_set.size();++i)
	{
		int minPos = numeric_limits<int>::max();
		int maxPos = numeric_limits<int>::min();
		int minPreOffset = numeric_limits<int>::max();
		int maxPreOffset = numeric_limits<int>::min();
		
		for(int j=0; j<(int)_emRuleVec.size();++j)
		{
			int v = _emRuleVec[j]._posVec[i];
			if(minPos > v)
				minPos = v;
			if(maxPos < v)
				maxPos = v;
			if(i>0)
				v = _emRuleVec[j]._posVec[i]-
						( _emRuleVec[j]._posVec[i-1] + _set[i-1]._fPtr->_content.size() );
			if(minPreOffset > v)
				minPreOffset = v;
			if(maxPreOffset < v)
				maxPreOffset = v;
		}
		_set[i]._lowerPos = minPos;
		_set[i]._upperPos = maxPos;
		_set[i]._lowerPreOffset = minPreOffset;
		_set[i]._upperPreOffset = maxPreOffset;
	}
}

void RelatedRuleCollection::AddElementSeqToMap(FragPosVec &fpv,ElementSequence &emSeq,int index)
{
	FragPtrVec fptrV;
	fptrV.resize(fpv.size());
	
	ElementRule er;
	er._index = index;
	er._posVec.resize(fpv.size());
	for(int i=0;i<(int)fpv.size();++i)
	{
		fptrV[i] = fpv[i]._fPtr;
		er._posVec[i] = fpv[i]._pos;
	}
	FragRuleMapIt mit = _frmap.find(fptrV);
	if(mit == _frmap.end())
	{
		RelatedRule rr;
		rr._count = 1;
		rr._support = (double)rr._count/(double)pEmCol->_size;
		rr._emRuleVec.push_back(er);
		rr._set.resize(fptrV.size());
		for(int i=0;i<(int)fptrV.size();++i)
		{
			rr._set[i]._fPtr = fptrV[i];
		}
		_frmap[fptrV] = rr;
	}
	else
	{
		RelatedRule &rr = _frmap[fptrV];
		++rr._count;
		rr._support = (double)rr._count/(double)pEmCol->_size;
		rr._emRuleVec.push_back(er);
	}
}

void RelatedRuleCollection::GenerateFragPtrVecFromElement(ElementSequence &emSeq,int index)
{
	
	IntElementSequence ies;
	ies.GenerateSeqFromUCharSeq(emSeq,pFragAllCol->_logicLength);
	FragPosVec fpv;
	FragPtrVec &fptr = pFragAllCol->_closeFragSet;
	for(int i=0; i<(int)fptr.size();++i)
	{
		ies.ReplaceSequenceByFragment(fpv,*fptr[i],emSeq);
	}
	
	if(fpv.size()>0)
	{
		sort(fpv.begin(),fpv.end(),FragmentPos::AscentFragmentPos);
		AddElementSeqToMap(fpv,emSeq,index);
	}
	else
	{
		
		_unkEmVec.push_back(index);
	}
}

const char* RelatedRuleCollection::ToString(string &str)
{
	char cc[256];
	str.clear();
	sprintf(cc,"Related Rule Size: %d; unknow flow size:%d\n",_frmap.size(),_unkEmVec.size());
	str = cc;
	RelRulePtrVec rv;
	for(FragRuleMapIt it = this->_frmap.begin(); it != _frmap.end(); ++it)
	{
		rv.push_back(&it->second);
	}
	sort(rv.begin(),rv.end(),RelatedRule::DescentRelatedRulePtrByCount);
	for(size_t i=0; i<rv.size();++i)
	{
		RelatedRule &rr = *rv[i];
		string rrstr;
		rr.ToString(rrstr);
		str += rrstr;
		str +='\n';
		sprintf(cc,"\t Sample File: %s",  
			pEmCol->_pPfCol->_pfaMap[pEmCol->_seqVector[rr._emRuleVec.at(0)._index]._rawFlowId]->_filename.c_str());
		str += cc;
		str += "\n";
	}
	return str.c_str();
}