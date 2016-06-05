#ifndef LAYER_H
#define LAYER_H
#include "rule.h"

struct LayerElement;
typedef LayerElement *LyEmPtr;
typedef vector<LyEmPtr> LyEmPtrVec;

struct LayerElement
{
	LayerElement():_lyCount(0),_lySup(0),_allEmSeqSup(0),_pRR(0),_bUnderThreshold(false)
	{
	}
	~LayerElement()
	{
		
	}

	const char* ToString(string &str)
	{
		str.clear();
		string rrStr;
		_pRR->ToString(rrStr);
		str += rrStr;
		char cc[256];
		sprintf(cc,"(Layer Counts:%d, Layer Sup:%f, Element Collection Support:%f, Specific Size:%d)",
			_lyCount,_lySup,_allEmSeqSup,_specialRuleSet.size());
		str += cc;
		return str.c_str();
	}

	
	RelRulePtrVec	_specialRuleSet;

	
	int _lyCount;
	
	double _lySup;
	
	double _allEmSeqSup;
	
	RelatedRule *_pRR;
	
	bool _bUnderThreshold;
	

	void UpdateCount(int totalCount,double minSup,int emSeqCounts)
	{
		_lyCount = _pRR->_count;
		for(size_t i = 0; i != _specialRuleSet.size(); ++i)
		{
			RelatedRule *_cpr = _specialRuleSet[i];
			_lyCount += _cpr->_count;
		}
		_lySup = (double)_lyCount/(double)totalCount;
		_allEmSeqSup = (double)_lyCount/emSeqCounts;
		if(_lySup < minSup)
			_bUnderThreshold = true;
		else
			_bUnderThreshold =false;
		_specialRuleSet.shrink_to_fit();
	}

	static LyEmPtr CreateLayerElement(RelRulePtr rrp)
	{
		LyEmPtr p = new LayerElement;
		p->_pRR = rrp;
		return p;
	}
};

struct LayerRoot
{
	LayerRoot():_totalCount(0),_minThreshold(0)
	{
	}
	~LayerRoot()
	{
		ClearAll();
	}

	
	LyEmPtrVec _child;
	
	IntVector _unkVec;
	
	int _totalCount;
	
	
	int _classType;
	
	double _minThreshold;

	
	void ClearAll();
	
	const char *ToString(string &str);
	
	bool IsAllUnderThreshold();
	
	void InitLayerRoot(ElementSequenceCol &esCol,RelatedRuleCollection &rrcol,int classType,double minThreshold=MIN_LAYER_THRESHOLD);
	
	int GetRuleSizeAboveThreshold();
};


struct RuleForest;
typedef RuleForest* RuleForestPtr;
struct FragNode;
typedef FragNode * FragNodePtr;
typedef vector<FragNodePtr> FragNodePtrSet;
struct FragNode
{
	FragNode(RuleForest *root):_pRoot(root),_count(0),_sup(0),_allEmSeqSup(0){}
	~FragNode(){
		for(size_t i=0; i<_child.size();++i)
		{
			delete _child[i];
		}
	}

	

	
	FragInterval	_frag;
	
	int	_count;
	
	double _sup;
	
	double _allEmSeqSup;
	
	RuleForest *_pRoot;
	
	RelRulePtrVec	_ruleSet;
	
	FragNodePtrSet _child;

	

	
	bool IsTerminal();
	
	int GetTerminalCount();
	
	double GetTerminalSupportInSet();
	
	double GetTerminalSupportInEmCol();
	bool AddChild(RelRulePtr pRule,size_t level);
	
	
	
	void GenFollow(RelRulePtr pRule,size_t level);
	
	int UpdateCount();
	
	bool Clip(FragNode *pParent);
	
	void GetChildRuleSet(RelRulePtrVec &ps);
	const char* ToString(string &str, int lev);
};

struct RuleForest
{
private:
	int _GetNodeRuleSize(FragNodePtr ptr)
	{
		if(ptr->_child.size() == 0)
			return 1;
		int sum = 0;
		for(size_t i=0; i<ptr->_child.size();++i)
		{
			sum += _GetNodeRuleSize(ptr->_child[i]);
		}
		if(ptr->_ruleSet.size()>0)
			++sum;
		return sum;
	}
public:
	RuleForest():_emSeqSize(0),_totalSize(0),_minThreshold(0){
	}
	~RuleForest(){
		for(size_t i=0; i<_treeEntrySet.size();++i)
		{
			delete _treeEntrySet[i];
		}
	}
	
	int _emSeqSize;
	
	int _totalSize;
	
	double _minThreshold;
	
	FragNodePtrSet _treeEntrySet;
	
	IntVector _unkVec;
	void ClearAll()
	{
		for(size_t i=0; i<_treeEntrySet.size();++i)
		{
			delete _treeEntrySet[i];
		}
		_treeEntrySet.clear();
	}
	void InitRuleForest(ElementSequenceCol &esCol,RelatedRuleCollection &rrcol,double minThreshold=MIN_LAYER_THRESHOLD)
	{
		ClearAll();
		_emSeqSize = esCol._size;
		_totalSize = rrcol.GetTotalRecognizedSize();

		_minThreshold = minThreshold;
		RelRulePtrVec rrp;
		for(FragRuleMapIt it = rrcol._frmap.begin(); it != rrcol._frmap.end(); ++it)
		{
			rrp.push_back(&(it->second));
		}
		sort(rrp.begin(),rrp.end(),RelatedRule::AscendRelatedRulePtrByLength);
		for(size_t i=0;i<rrp.size();++i)
		{
			AddToTree(rrp[i]);
		}
		UpdateTree();
		ClipTree();
	}
	void AddToTree(RelRulePtr pRule)
	{
		bool res = false;
		for(size_t i=0; i<_treeEntrySet.size();++i)
		{
			res = _treeEntrySet[i]->AddChild(pRule,0);
			if(res)
				return;
		}
		if(!res)
		{
			FragNodePtr np = new FragNode(this);
			np->GenFollow(pRule,0);
			_treeEntrySet.push_back(np);
		}
	}
	
	{
		for(size_t i=0; i<_treeEntrySet.size();++i)
		{
			_treeEntrySet[i]->UpdateCount();
		}
	}
	void ClipTree()
	{
		FragNodePtrSet existSet;
		for(size_t i=0; i<_treeEntrySet.size();++i)
		{
			if(_treeEntrySet[i]->Clip(0))
				delete _treeEntrySet[i];
			else
				existSet.push_back(_treeEntrySet[i]);
		}
		_treeEntrySet = existSet;
	}

	int GetRuleCounts()
	{
		int sum = 0;
		for(size_t i=0; i<this->_treeEntrySet.size(); ++i)
		{ 
			sum +=_GetNodeRuleSize(_treeEntrySet[i]);
		}
		return sum;
	}

	const char* ToString(string &str)
	{
		char cc[128];
		str.clear();
		sprintf(cc, "Prefix tree: rule size:%d; root size:%d; unk flow:%d; min threshold: %f\n",
			this->GetRuleCounts(),_treeEntrySet.size(), this->_unkVec.size(), _minThreshold);
		str += cc;
		for(size_t i=0; i<this->_treeEntrySet.size();++i)
		{
			string temp;
			str += _treeEntrySet[i]->ToString(temp,0);
		}
		return str.c_str();
	}
};

#endif 