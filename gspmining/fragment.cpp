#include "fragment.h"
#include "audit.h"

const char* FrequentPos::ToString(string &str) const
{
	char s[32];
	str.clear();
	sprintf(s,"(%d:%d)",this->_pos,this->_count);
	str=s;
	return str.c_str();
}

void Fragment::InitPosVector(FragLevPos &pos,int logicLength,int minCount)
{
	this->_posSet._posVec.clear();
	this->_posSet._level = pos._level;

	for(int i=0;i<logicLength;++i)
	{
		if(pos.GetPosCount(i)>=minCount)
		{
			FrequentPos fp;
			fp._count = pos.GetPosCount(i);
			fp._pos = i;
			this->_posSet._posVec.push_back(fp);
		}
	}
	this->_posSet._posVec.shrink_to_fit();
}

void Fragment::InitContent(UCharVector &content)
{
	this->_content.clear();
	this->_content = content;
	this->_content.shrink_to_fit();
}

void Fragment::InitFragment(int count, double support,bool isClosedItem, 
	int leftItem, int rightItem)
{
	this->_count = count;
	this->_support = support;
	this->_isClosedItem = isClosedItem;
	this->_leftItem = leftItem;
	this->_rightItem = rightItem;
}



int Fragment::SearchPosCount(int pos)
{
	int maxCount = 0;
	for(size_t i=0; i< this->_posSet._posVec.size(); ++i)
	{
		FrequentPos &f = _posSet._posVec[i];
		if(pos < f._pos+_posSet._level && pos >= f._pos)
		{
			if(maxCount < f._count)
				maxCount = f._count;
		}
		else if(pos >= f._pos+_posSet._level)
		{
			continue;
		}
		else
			break;
	}
	return maxCount;

	
	/*
	int low = 0, high = this->_posVec.size()-1, mid;
	if(_posVec[low]._pos == pos)
		return _posVec[low]._count;
	while(low <= high)
	{
		mid = low + ((high-low)/2);
		if(_posVec[mid]._pos==pos)
		{
			return _posVec[mid]._count;
		}
		if(_posVec[mid]._pos>pos)
			high = mid-1;
		else
			low=mid+1;
	}
	return 0;*/
}
const char* Fragment::ToString(string &str) const
{
	char s[32];
	str.clear();
	string strContent;
	UtilityFunc::UCharVectorToString(strContent,(this->_content));
	str = strContent;
	sprintf(s,"|[%d:%d-pos:",_posSet._level,_posSet._posVec.size());
	str += s;
	for(size_t i=0;i<this->_posSet._posVec.size();++i)
	{
		string strPos;
		this->_posSet._posVec[i].ToString(strPos);
		strPos += " ";
		str += strPos;
	}
	str +="] ";
	str +="[";
	str +="";
	sprintf(s,"count:%d ",_count);
	str += s;
	sprintf(s,"sup:%.3f ",_support);
	str += s;
	sprintf(s,"(L:%d,R:%d)]",_leftItem,_rightItem);
	str +=s;
	if(_isClosedItem)
		str += "*";
	return str.c_str();
}
bool FragmentCollection::_Level_1_Mining()
{		

	FrequentCount *ptable = new FrequentCount[256];

	EmSeqVector &sv = _pSeqCol->_seqVector;
	int size = sv.size();
	IntVecPtr *pPosTable;
	IntVecPtr pPos;
	
	for(int i=0; i<size; ++i)
	{
		pPosTable = sv[i]._series._pPosTable;
		for(int i=0; i<256;++i)
		{
			pPos = pPosTable[i];
			if(pPos)
			{
				if((*pPos)[0] < this->_logicLength)
				{
					
					++ptable[i]._count;
				}
			}
		}
	}

	
	for(int j=0; j<256; ++j)
	{
		if((int)ptable[j]._count >= _minCount)
		{
			ptable[j]._pLevPosCount = new FragLevPos(_logicLength);
			
		}
		else
			ptable[j]._pLevPosCount = 0;
	}

	
	for(int i=0; i<size; ++i)
	{
		UCharVector &ucv = sv[i]._series._content;
		int itsize = (int)ucv.size() > _logicLength ? _logicLength : ucv.size();
		unsigned char c;
		for(int j=0; j < itsize; ++j)
		{
			c = ucv[j];
			if(ptable[c]._pLevPosCount)
			{
				ptable[c]._pLevPosCount->AddPosCount(j);
			}
		}
	}
	for(int i=0;i<256;++i)
	{
		if(ptable[i]._pLevPosCount)
		{
			
			if(ptable[i]._pLevPosCount->Update(_minRelCount,_maxOffset))
			{
				
				Fragment mg;

				UCharVector tcv;
				tcv.push_back((unsigned char)i);
				mg.InitFragment(ptable[i]._count,(double)ptable[i]._count/(double)size,
					true,-1,-1);
				this->_set.push_back(mg);
				Fragment &refFrag = _set[_set.size()-1];
				refFrag.InitContent(tcv);
				refFrag.InitPosVector(*(ptable[i]._pLevPosCount), _logicLength, _minRelCount);
			}
			delete ptable[i]._pLevPosCount;
		}
	}
	delete []ptable;
	_set.shrink_to_fit();
	if(!_set.size())
		return false;
	return true;
}

bool FragmentCollection::_Level_2_Mining()
{
	
	if(this->_preFragCol->_level > 1) 
		return false;

	
	FrequentCount *ptable = new FrequentCount[65536];
	
	int ctable[256];
	memset(ctable,0,sizeof(int)*256);
	
	FragmentSet &preSet = this->_preFragCol->_set;
	int size = (int)preSet.size();
	int c;
	for(int i=0;i<size;++i)
	{
		c = preSet[i]._content[0];
		ctable[c] = i;
	}

	
	unsigned short candidate;	
	for(int i=0;i<size;++i)
	{
		for(int j=0;j<size;++j)
		{
			candidate = preSet[i]._content[0] | (preSet[j]._content[0] <<8 );
			ptable[candidate]._count = 0x80000000;
		}
	}
	
	
	EmSeqVector &sv = _pSeqCol->_seqVector;
	size = sv.size();
	
	for(int i=0; i<size; ++i)
	{
		UCharVector &uv = sv[i]._series._content;
		int seriesSize = ((int)uv.size() > _logicLength ? _logicLength : uv.size())-1;
		for(int j=0;j<seriesSize;++j)
		{
			unsigned short uread = *((unsigned short *)(&uv[j]));
			FrequentCount &pfc = ptable[uread];
			if(IS_CANDIDATE(pfc._count) && pfc._index!=i)
			{
				pfc._index = i;
				++pfc._count;
			}
		}

	}

	
	
	bool nosupport = true;
	for(int i=0; i<65536; ++i)
	{
		FrequentCount &pfc=ptable[i];
		if(pfc._index >= 0 && (int)GET_COUNT(pfc._count) >= _minCount)
		{			
			nosupport = false;
			pfc._pLevPosCount = new FragLevPos(_logicLength);
			pfc._count &= 0x7FFFFFFF;
			
		}
		else
			pfc._pLevPosCount = 0;
	}
	if(nosupport)
		return false;

	
	size = sv.size();
	for(int i=0; i<size; ++i)
	{
		UCharVector &uv = sv[i]._series._content;
		int seriesSize = ((int)uv.size() > _logicLength ? _logicLength : uv.size())-1;
		for(int j=0;j<seriesSize;++j)
		{
			unsigned short uread = *((unsigned short *)(&uv[j]));
			FrequentCount &pfc = ptable[uread];
			if(pfc._pLevPosCount)
			{
				
				pfc._pLevPosCount->AddPosCount(j);
			}
		}
	}

	
	for(int i=0;i<65535; ++i)
	{
		FrequentCount &pfc = ptable[i];
		if(pfc._pLevPosCount)
		{
			if(pfc._pLevPosCount->Update(_minRelCount,_maxOffset))
			{
				
				Fragment mg;

				UCharVector tcv;
				tcv.resize(2);			
				*((unsigned short *)(&tcv[0]))=(unsigned short)i;
				mg.InitFragment(ptable[i]._count,(double)ptable[i]._count/(double)size,
					true,ctable[(i&0x000000FF)],ctable[(i &0x0000FF00)>>8]);		
				
				
				if(_RelationAnalysis(pfc._pLevPosCount,mg))
				{
					this->_set.push_back(mg);
					Fragment &refFrag = _set[_set.size()-1];
					refFrag.InitContent(tcv);
					refFrag.InitPosVector(*(ptable[i]._pLevPosCount), _logicLength, _minRelCount);
					_preFragCol->_set[mg._leftItem]._isClosedItem = false;
					_preFragCol->_set[mg._rightItem]._isClosedItem = false;
				}			
			}
			delete pfc._pLevPosCount;
		}
	}
	
	delete []ptable;
	if(!_set.size())
		return false;
	return true;
}

bool FragmentCollection::_Level_3_N_Mining()
{
	
	PartFragMap table;
	CdFragMap	cdmap;
	FragmentSet &fset = _preFragCol->_set;
	int size = (int)fset.size();
	
	for(int i=0 ; i < size; ++i)
	{
		UCharVector tuv(fset[i]._content.begin()+1, fset[i]._content.end());
		PtFragMapIt fit = table.find(tuv); 
		if(fit == table.end())
		{
			IntVector a;
			a.push_back(i);
			table[tuv] = a;
		}
		else
		{
			fit->second.push_back(i);
		}
	}

	
	for(int i=0;i<size;++i)
	{
		UCharVector tuv(fset[i]._content.begin(), fset[i]._content.end()-1);
		PtFragMapIt fit=table.find(tuv);
		if(fit != table.end())
		{
			IntVector &iv = fit->second;
			for(int j=0;j<(int)iv.size();++j)
			{
				UCharVector &leftcv = fset[iv[j]]._content;
				UCharVector cdIndex(leftcv.begin(),leftcv.end());
				cdIndex.push_back(fset[i]._content.back());
				CandidateFragmentAttr attr;
				attr._leftItem = iv[j];
				attr._rightItem = i;
				cdmap[cdIndex]=attr;
			}
		}
	}
	if(cdmap.size()==0)
		return false;

	
	EmSeqVector &emv = _pSeqCol->_seqVector;

	/*for(int i=0; i< (int)emv.size(); ++i)
	{
		CdFragMapIt cit;
		UCharVector &uv = emv[i]._series._content;
		int endlen = ((int)uv.size()>_logicLength?_logicLength:uv.size())-_level+1;
		for(int j=0;j<endlen;++j)
		{
			UCharVector tuv(uv.begin()+j,uv.begin()+j+_level);
			cit = cdmap.find(tuv);
			if(cit!=cdmap.end() && cit->second._index!=i)
			{
				cit->second._index = i;
				++(cit->second._count);
			}
		}
	}*/
	


	for(CdFragMapIt cit=cdmap.begin(); cit != cdmap.end();++cit )
	{
		for(int i=0; i< (int)emv.size(); ++i)
		{
			int endPos = (int)emv[i]._series._content.size()>_logicLength?_logicLength:(int)emv[i]._series._content.size();
			if(emv[i]._series.FirstFind(0,endPos,cit->first)!=-1)
				++cit->second._count;
			
		}

	}

	
	
	for(CdFragMapIt cit=cdmap.begin(); cit != cdmap.end(); )
	{
		if(cit->second._count < this->_minCount)
		{
			cdmap.erase(cit++);
		}				
		else
		{
			
			cit->second._pPosCount = new FragLevPos(_logicLength);
			
			++cit;
		}
	}		
	if(cdmap.size()==0)
		return false;
	
	
/*	for(int i=0; i< (int)emv.size(); ++i)
	{
		CdFragMapIt cit;
		UCharVector &uv = emv[i]._series._content;
		int endlen = ((int)uv.size()>_logicLength?_logicLength:uv.size())-_level+1;
		for(int j=0;j<endlen;++j)
		{
			UCharVector tuv(uv.begin()+j,uv.begin()+j+_level);
			cit = cdmap.find(tuv);
			if(cit!=cdmap.end())
			{
				++(cit->second._pPosCount[j]);
			}
		}
	}*/
	

	
	for(CdFragMapIt cit=cdmap.begin(); cit != cdmap.end();++cit )
	{
		for(int i=0; i< (int)emv.size(); ++i)
		{
			const UCharVector &ucharv = cit->first;
			int c = ucharv[0];
			ElementSequence &emvi = emv[i];
			IntVecPtr ivp = emvi._series._pPosTable[c];
			if(ivp)
			{
				int pos;
				IntVector &rip = *ivp;
				
				int endPos = (int)emvi._series._content.size()>_logicLength?_logicLength:(int)emvi._series._content.size();
				for(int j=0;j<(int)rip.size();++j)
				{
					pos = rip[j];
					if(pos >= endPos || pos+(int)ucharv.size()>endPos)
						break;
					if(memcmp(&(emvi._series._content[pos]),&(ucharv[0]),cit->first.size())==0)
					{
						(cit->second._pPosCount->AddPosCount(pos));
					}
				}
			}
		}
	}
	
	for(CdFragMapIt cit=cdmap.begin(); cit != cdmap.end(); ++cit)
	{
		
		if(cit->second._pPosCount->Update(_minRelCount, _maxOffset))
		{
			
			Fragment mg;

			UCharVector tcv(cit->first.begin(),cit->first.end());
			mg.InitFragment(cit->second._count,(double)cit->second._count/(double)emv.size(),
				true,cit->second._leftItem,cit->second._rightItem);			
			
			if(_RelationAnalysis(cit->second._pPosCount,mg))
			{
				this->_set.push_back(mg);
				Fragment &refFrag = _set[_set.size()-1];
				refFrag.InitContent(tcv);
				refFrag.InitPosVector(*(cit->second._pPosCount), _logicLength, _minRelCount);
				_preFragCol->_set[mg._leftItem]._isClosedItem = false;
				_preFragCol->_set[mg._rightItem]._isClosedItem = false;
			}			
		}
		delete cit->second._pPosCount;
	}
	if(!this->_set.size())
		return false;
	return true;
}

bool FragmentCollection::_RelationAnalysis(FragLevPos *pos, Fragment &fragment)
{		
	double cosine;
	bool res=false;
	for(int i=0;i<_logicLength;++i)
	{
		int count = pos->GetPosCount(i);
		if(count >= _minRelCount)
		{
			
			int leftCount = this->_preFragCol->_set[fragment._leftItem].SearchPosCount(i);
			int rightCount = this->_preFragCol->_set[fragment._rightItem].SearchPosCount(i+1);
			
			cosine = (double)count/sqrt(((double)leftCount * (double)rightCount));
			if(cosine < _cosine)
			{
				pos->ZeroPosCount(i);
			}
			else
				res = true;
		}
	}
	
}

const char* FragmentCollection::ToString(string &str) const
{
	char s[32];
	str.clear();
	sprintf(s,"Lev:%d; Size:%d\n",this->_level,_set.size());
	str = s;
	for(int i=0; i<(int)_set.size(); ++i)
	{
		string fragStr;
		_set[i].ToString(fragStr);
		str +=fragStr + "\n";
	}
	return str.c_str();
}

const char* FragAllLevCol::ToString(string &str)const{
	char cstr[256];
	str.clear();
	sprintf(cstr, "#Lev Size:%d; Logic Length:%d; Min Count:%d; Min Sup:%.3f; Scale:%.3f; Min Rel:%d; cosine:%.3f;Max offset:%d\n",
		this->_fragSet.size(), _logicLength, _minCount, _minSupport, _scale, _minRelCount, _cosine,_maxOffset);
	str += cstr;
	for(int i=0; i<(int)this->_fragSet.size(); ++i)
	{
		string fragStr;
		str += _fragSet[i]->ToString(fragStr);
	}
	return str.c_str();
}