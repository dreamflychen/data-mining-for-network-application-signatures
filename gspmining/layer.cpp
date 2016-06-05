#include "layer.h"

void LayerRoot::ClearAll()
{
	for(size_t i=0;i<_child.size(); ++i)
	{
		delete _child[i];
	}
	_child.clear();
	_unkVec.clear();
	_totalCount = 0;
}
const char *LayerRoot::ToString(string &str)
{
	str.clear();
	char cc[256];

	if(_classType == LAYER_CLASS)
	{
		str += "Class Type: Layer class; ";
	}
	else if(_classType == TREE_CLASS)
	{
		str += "Class Type: Tree class; ";
	}
	sprintf(cc,"Total instance counts: %d; Unknow Counts in total counts: %d; Min threshold: %f\n",
		_totalCount,_unkVec.size(),_minThreshold);
	str += cc;
	
	str +="Select layer signature:\n";
	for(size_t i=0;i<_child.size();++i)
	{
		if(!_child[i]->_bUnderThreshold)
		{
			string childStr;
			_child[i]->ToString(childStr);
			str += childStr;
			str += '\n';
		}
	}
	str +="Deleted layer signature:\n";
	
	for(size_t i=0;i<_child.size();++i)
	{
		if(_child[i]->_bUnderThreshold)
		{
			string childStr;
			_child[i]->ToString(childStr);
			str += childStr;
			str += '\n';
		}
	}
	str +="Hierarchical:\n";
	for(size_t i =0;i<_child.size();++i)
	{
		string childStr;
		str +="---";
		_child[i]->_pRR->ToString(childStr);
		str += childStr;
		if(_child[i]->_specialRuleSet.size())
		{
			for(size_t j=0;j<_child[i]->_specialRuleSet.size();++j)
			{
				str+="\n\t|---";
				_child[i]->_specialRuleSet[j]->ToString(childStr);
				str += childStr;
			}
		}
		str += '\n';
	}
	return str.c_str();
}

bool LayerRoot::IsAllUnderThreshold()
{
	bool res = true;
	for(size_t i=0;i<_child.size();++i)
	{
		if(!_child[i]->_bUnderThreshold)
		{
			res = false;
			return res;
		}
	}
	return res;
}


void LayerRoot::InitLayerRoot(ElementSequenceCol &esCol,RelatedRuleCollection &rrcol,int classType,double minThreshold)
{
	ClearAll();
	_classType = classType;
	_minThreshold = minThreshold;
	RelRulePtrVec rrp;
	for(FragRuleMapIt it = rrcol._frmap.begin(); it != rrcol._frmap.end(); ++it)
	{
		rrp.push_back(&(it->second));
	}
	sort(rrp.begin(),rrp.end(),RelatedRule::AscendRelatedRulePtrByLength);

	
	for(size_t i=0; i<rrp.size(); ++i)
	{
		_totalCount += rrp[i]->_count;
		_child.push_back(LayerElement::CreateLayerElement(rrp[i]));
	}
	_child.shrink_to_fit();
	
	for(size_t i=0;i<_child.size();++i)
	{
		for(size_t j=i+1;j<_child.size();++j)
		{
			if(classType == LAYER_CLASS)
			{
				if(_child[j]->_pRR->IsSpecialOfGiven(*(_child[i]->_pRR)))
				{
					_child[i]->_specialRuleSet.push_back(_child[j]->_pRR);
				}
			}
			else if(classType == TREE_CLASS)
			{
				if(_child[i]->_pRR->isPrefixOfGiven(*(_child[j]->_pRR)))
				{
					_child[i]->_specialRuleSet.push_back(_child[j]->_pRR);
				}
			}
		}
	}

	
	for(size_t i=0; i<_child.size(); ++i)
	{
		_child[i]->UpdateCount(_totalCount,minThreshold,esCol._size);
	}

	
	for(size_t i=0;i<_child.size(); ++i)
	{
		bool badd = true;
		if(_child[i]->_bUnderThreshold)
		{
			for(size_t j=0;j<_child.size();++j)
			{
				if(_child[j]->_bUnderThreshold == false)
				{
					if(classType == LAYER_CLASS)
					{
						if(_child[i]->_pRR->IsSpecialOfGiven(*_child[j]->_pRR))
						{
							badd=false;
							break;
						}					
					}
					else if(classType == TREE_CLASS)
					{
						if(_child[j]->_pRR->isPrefixOfGiven(*(_child[i]->_pRR)))
						{
							badd=false;
							break;
						}
					}
				}
			}
			if(badd)
			{
				ElementRuleVec &erv = _child[i]->_pRR->_emRuleVec;
				for(size_t k = 0; k<erv.size();++k)
				{
					_unkVec.push_back(erv[k]._index);
				}					
			}
		}
	}
	_unkVec.shrink_to_fit();
}

int LayerRoot::GetRuleSizeAboveThreshold()
{
	int size = 0;
	for(size_t i=0;i<_child.size(); ++i)
	{
		if(!_child[i]->_bUnderThreshold)
		{
			++size;
		}
	}
	return size;
}

bool FragNode::IsTerminal()
{
	return (_ruleSet.size()>0);
}

int FragNode::GetTerminalCount()
{
	int sum = 0;
	for(size_t i = 0; i<_ruleSet.size(); ++i)
	{
		sum += _ruleSet[i]->_count;
	}
	return sum;
}

double FragNode::GetTerminalSupportInSet()
{		
	return (double)GetTerminalCount()/(double)_pRoot->_totalSize;
}

double FragNode::GetTerminalSupportInEmCol()
{
	return (double)GetTerminalCount()/(double)_pRoot->_emSeqSize;
}
bool FragNode::AddChild(RelRulePtr pRule,size_t level)
{
	if(this->_frag.Merge(pRule->_set[level]))
	{
		++level;
		if(level<pRule->_set.size())
		{
			bool res = false;
			for(size_t i=0;i<_child.size();++i)
			{
				res = _child[i]->AddChild(pRule,level);
				if(res)
					return true;
			}
			if(!res)
			{
				FragNodePtr np = new FragNode(_pRoot);
				np->GenFollow(pRule,level);
				_child.push_back(np);
			}
		}			
		else
		{
			this->_ruleSet.push_back(pRule);
		}
		return true;
	}
	return false;
}



void FragNode::GenFollow(RelRulePtr pRule,size_t level)
{
	if(level >= pRule->_set.size())
		return;
	this->_frag = pRule->_set[level];
	++level;
	if(level < pRule->_set.size())
	{
		FragNodePtr np = new FragNode(_pRoot);
		np->GenFollow(pRule,level);
		_child.push_back(np);
	}
	else
	{
		this->_ruleSet.push_back(pRule);
	}		
}
int FragNode::UpdateCount()
{
	int delta = 0;
	for(size_t i=0;i<this->_child.size();++i)
	{
		delta += _child[i]->UpdateCount();
	}
	this->_count += delta;
	for(size_t j=0;j<this->_ruleSet.size();++j)
	{
		this->_count += _ruleSet[j]->_count;
	}
	this->_sup = (double)this->_count/(double)this->_pRoot->_totalSize;
	this->_allEmSeqSup = (double)this->_count/(double)this->_pRoot->_emSeqSize;
	return this->_count;
}
bool FragNode::Clip(FragNode *pParent)
{
	
	if(this->_sup < this->_pRoot->_minThreshold)
	{
		
		
		GetChildRuleSet(this->_ruleSet);
		if(!pParent)
		{
			for(size_t i=0; i<_ruleSet.size();++i)
			{
				ElementRuleVec &ev = _ruleSet[i]->_emRuleVec;
				for(size_t j=0; j<ev.size();++j)
					_pRoot->_unkVec.push_back(ev[j]._index);
			}
		}
		else
		{
			
			pParent->_ruleSet.insert(pParent->_ruleSet.end(),_ruleSet.begin(),_ruleSet.end());
		}		
		
		
		
		return true;
	}
	else
	{
		
		for(size_t i=0; i<this->_child.size(); ++i)
		{
			if(_child[i]->Clip(this))
				delete _child[i];
			else
				existSet.push_back(_child[i]);
		}
		_child = existSet;
		return false;
	}
}
void FragNode::GetChildRuleSet(RelRulePtrVec &ps)
{
	for(size_t i=0; i<this->_child.size(); ++i)
	{
		ps.insert(ps.end(),_child[i]->_ruleSet.begin(),_child[i]->_ruleSet.end());
		_child[i]->GetChildRuleSet(ps);
	}
}

const char* FragNode::ToString(string &str, int lev)
{
	str.clear();
	string blank;
	for(int i=0;i<lev;++i)
	{
		str+="\t";
		blank+="\t";
	}
	str+="|---";
	char cc[128];
	string terminalNodeStr="false";
	if(this->_ruleSet.size()>0)
		terminalNodeStr = "true";
	sprintf(cc,"Terminal Node:%s; count:%d; support in ack set:%.3f; support in all set:%.3f; rules in the node:%d; children: %d\n",
		terminalNodeStr.c_str(),this->_count,
		this->_sup,this->_allEmSeqSup,this->_ruleSet.size(), this->_child.size());
	str += cc;
	blank+="|   ";
	string fragStr;
	str += blank;
	str += this->_frag.ToString(fragStr);
	str += '\n';
	string childStr;
	for(size_t i = 0; i < _child.size(); ++i)
	{
		str += _child[i]->ToString(childStr,lev+1);
	}
	return str.c_str();
}