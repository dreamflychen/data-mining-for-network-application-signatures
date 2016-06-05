#ifndef LAYERMINING_H
#define LAYERMINING_H

#include <vector>
#include <string>
#include <hash_map>
#include <algorithm>
using namespace std;

#define TEXT_REWRITE				"wt+"
#define TEXT_APPEND					"at+"
#define TEXT_READ					"rt"

#define BINARY_REWRITE				"wb+"
#define BINARY_APPEND				"ab+"
#define BINARY_READ					"rb"

#define ORDERED_DIRECTION			0
#define INVERTED_DIRECTION			1

#define DEFAULT_LOG_FILE			"lmlog.txt"
#define PAYLOAD_EXTENSION			"payload"

#define DEFAULT_RESERVE_RATION		0.8
#define MAX_RESERVE_LEN				1000
#define DEFAULT_MIN_SUPPORT			0.2
#define DEFAULT_COSINE				0.0
#define DEFAULT_SCALE				0.8
#define DEFAULT_PRUNE_SUPPORT		0.382
#define DEFAULT_POS_INTERVAL		-1

#define LOGFILE_MODE				TEXT_APPEND

typedef unsigned int uint;
typedef unsigned char uchar;



typedef hash_map<string, FILE*> FileMap;
typedef FileMap::iterator	FileIt;



namespace GV
{
	extern int direction;
	extern FileMap fileMap;
	extern char *payloadFileExtension;
};


{
	string filename;				
	int length;						
	int id;							
};


typedef vector<RawNetflowAttribute> RawNetflowSet;
typedef RawNetflowSet::iterator RawNetflowAttrIt;


struct RawNetflowCollection
{
	string path;					
	int totalFileSize;				
	string keyword;					
	RawNetflowSet fileSet;			
	int reserveLen;					
	RawNetflowCollection()
	{
		init();
	}
	void init()
	{
		path = "";
		totalFileSize = 0;
		keyword = "";
		fileSet.clear();
		reserveLen = -1;
	}
	void toString(string &str)
	{
		char buf[32];
		str.clear();
		for(RawNetflowAttrIt it = fileSet.begin(); it != fileSet.end(); ++it)
		{
			str += "\t"+it->filename;
			str += "\t\t\t\t";
			str += itoa(it->id,buf,10);
			str += "\t\t\t\t";
			str += itoa(it->length,buf,10);
			str += '\n';
		}
	}
};




typedef vector<int> MapToken;		


typedef vector<MapToken*> ValueToTokenMap;


struct ElementMap
{
	MapToken token;					
	int value;						
};


typedef vector<ElementMap> TokenToValueMap;


struct ElementMapTable
{
	TokenToValueMap tvmap;			
	ValueToTokenMap vtmap;			
};





typedef vector<int> ElementSeries;
typedef ElementSeries::iterator EmSeriesIt;


struct ElementSequence
{
	ElementSeries series;			
	int id;							
	int rawFlowId;					
	double scoreEarned;				
};


typedef vector<ElementSequence> ElementSequenceSet;
typedef ElementSequenceSet::iterator EmSeqSetIt;


struct ElementSequenceCollection
{
	ElementSequenceSet	seqSet;		
	ElementMapTable	*pMapTable;		
};




size_t _elementSeriesHash(const ElementSeries &s);


struct HashFragCompare
{
	enum
	{
		bucket_size = 8,
		min_buckets = 128
	};
	size_t operator() (const ElementSeries &s) const
	{
		return _elementSeriesHash(s);
	}
	
	bool operator ()(const ElementSeries &s1,const ElementSeries &s2) const
	{
		if(s1.size()!=s2.size())
			return true;
		int len = s1.size();
		for(int i=0;i<len;++i)
		{
			if(s1[i] != s2[i])
				return true;
		}
		return false;
	}
};

struct OneCount
{
	int index;
	int count;
	OneCount(){
		index = -1;
		count = 0;
	}
};

typedef hash_map<ElementSeries, OneCount, HashFragCompare> AuditElementTable;
typedef AuditElementTable::iterator AdtEmTableIt;
typedef vector<double> DoubleVector;
typedef vector<int> IntVector;

struct AuditWindow
{
	int flowSize;					
	int startPos;					
	int totalCount;
	AuditElementTable auditTable;	
	double maxFreq;					
	
	double minFreq;					
	double midFreq;					
	double averageFreq;				
	double freqStandDeviation;		
	ElementSeries maxSer;
	AuditWindow()
	{
		flowSize = 0;
		startPos = 0;
		totalCount = 0;
		maxFreq = 0.0;
		minFreq = 0.0;
		midFreq = 0.0;
		averageFreq = 0.0;
		freqStandDeviation = 0.0;
	}
};


typedef vector<AuditWindow> AuditWindowSet;


struct AuditSequence
{
	AuditWindowSet winSet;			
	int halfWindowSize;					
	int shingleSize;				
	double lineSupport;
	int upperIndex;
	int lowerIndex;
	void ToMatLabType(string &str);
	AuditSequence()
	{
		halfWindowSize = 0;
		shingleSize = 0;
		lineSupport = 0.0;
		upperIndex = 0;
		lowerIndex = 0;
	}
};



struct FragmentInterval
{
	int start;	
	int end;	
	int count;	
	double support;
};

typedef vector<FragmentInterval> FragIntervalSet;


struct Fragment
{
	ElementSeries series;			
	int count;						
	double support;					
	bool isClosedItem;				
	int leftItem;					
	int rightItem;					
	FragIntervalSet iset;			
	Fragment()
	{
		count = 0;
		support = 0.0;
		isClosedItem = true;
		leftItem = -1;
		rightItem = -1;
	}
};


typedef vector<Fragment> FragmentSet;
typedef FragmentSet::iterator FragSetIt;


struct FragmentCollection
{
	FragmentSet set;				
	int fragSize;					
	FragmentCollection *preFragCol;	
};





struct CandidateFragmentAttr{
	int count;
	int leftItem;					
	int rightItem;					
	CandidateFragmentAttr():count(0),leftItem(-1),rightItem(-1){}
};




typedef hash_map<ElementSeries, CandidateFragmentAttr, HashFragCompare> CandidateFragmentSet;
typedef CandidateFragmentSet::iterator CandidateFragSetIt;


struct CandidateFragmentCollection{
	CandidateFragmentSet set;		
	int fragSize;					
};


typedef vector<FragmentCollection*> FragmentCollectionArray;


typedef vector<int> OffsetSeries;
typedef vector<OffsetSeries> OffsetSeriesSet;

typedef vector<int> ElementSeqIndexArray;

struct RelatedRule
{
	
	FragmentSet set;
	
	OffsetSeriesSet offsetSet;
	ElementSeqIndexArray emSeqIndexArray;
	int count;
	double support;
	RelatedRule()
	{
		count = 0;
		support = 0.0;
	}
};

typedef vector<RelatedRule> RelateRuleSet;




struct FragmentCombineItem
{
	Fragment frag;
	int pos;
	FragmentCombineItem()
	{		
		pos = 0;
	}
};
typedef vector<FragmentCombineItem> FragmentCombineSet;



struct FragUnit
{
	ElementSeries fragSr;
	int upperBound;
	int lowerBound;
	FragUnit()
	{
		upperBound = 0;
		lowerBound = 0;
	}
};

typedef vector<FragUnit> FragUnitArray;

struct FragUnitArrayCol
{
	FragUnitArray set;
	int count;
	double support;
	FragUnitArrayCol()
	{
		count = 0;
		support = 0.0;
	}
};

typedef vector<FragUnitArrayCol> FragUnitArrayColSet;







struct RuleNode;
typedef vector<RuleNode*> RuleChildNode;
struct RuleNode
{
	
	FragUnit fragUnit;
	int count;
	double support;
	ElementSeqIndexArray emSeqIndexSet;
	RuleChildNode childSet;
	bool isTerminal;
	int terminalCount;
	double terminalSupport;
	RuleNode()
	{
		
		count = 0;
		support = 0.0;
		isTerminal = false;
		terminalCount = 0;
		terminalSupport = 0.0;
	}
	~RuleNode()
	{
		int s = childSet.size();
		for(int i=0; i<s; ++i)
		{
			delete childSet[i];
		}
	}
};
struct RuleTreeRoot
{
	RuleChildNode nodeArray;
	ElementSeqIndexArray unknowEmSeqIndexSet;
	double recognizedRate;
	int	totalFlowSize;
	int recognizedFlowSize;
	int unknowFlowSize;
	RuleTreeRoot()
	{
		recognizedRate = 0.0;
		totalFlowSize = 0;
		recognizedFlowSize = 0;
		unknowFlowSize = 0;
	}
	~RuleTreeRoot()
	{
		int s = nodeArray.size();
		for(int i=0; i<s; ++i)
		{
			delete nodeArray[i];
		}
	}
};




struct RuleDescribeUnit
{
	string signature;				
	int upperBound;					
	int lowerBound;					
};

typedef vector<RuleDescribeUnit> RuleDescribeSequence;


typedef vector<RuleDescribeSequence> RuleDescribeSet;


bool initSystem();					
bool destroySystem();				

bool writelog(const char *file,const char*format, ...);

bool writedeflog(const char *format, ...);

bool closelog(const char *file);


class CLayerMining
{
public:
	
	double fcosine;

	
	
	
	double scale;
	
	
	int intervalLength;

	double minSupport;

	double treePruneThreshold;

	double reserveRation;

	int maxReserveLength;

	
	CLayerMining();

	
	
	
	virtual bool CreateOriginalNetflowSet(
		RawNetflowCollection &orawset, 
		char *path,
		char *keyword);

	
	
	virtual bool SetAlignLength (RawNetflowCollection &rcol);

	virtual double CalcAuditZeroPunish(int shingleSize);

	
	
	
	virtual bool CreateElemSeqColFromRawCol(
		ElementSequenceCollection &ecol,
		RawNetflowCollection &rcol);

	
	virtual bool GenerateMaxAuditSequence(
		AuditSequence  &seq,				
		ElementSequenceCollection &col,		
		int halfWindowSize,					
		int shingleSize);					

	
	
	virtual int CalcSignatureIntervalLength(AuditSequence &seq,RawNetflowCollection &rcol);

	
	virtual bool ClipElementSequenceCollection(ElementSequenceCollection &set, int length);

	
	virtual bool ClipAuditSequence(AuditSequence &seq,int length);

	
	virtual double CalcMinimumSupport(AuditSequence &seq, ElementSequenceCollection &set);


	
	virtual bool GenerateFrequentFragmentSet(FragmentCollection &fset,			
		ElementSequenceCollection &emCol,										
		FragmentCollection *pFragCol,											
		int len,																
		int maxGap																
		);		

	
	virtual bool StrategyRuleAnalysis(Fragment &frag,ElementSequenceCollection &emCol,FragmentCollection *pPreCol,int maxGap);


	
	bool GetCloseFragmentSet(FragmentSet &closeFragSet, FragmentCollectionArray &fragColArray);

	
	bool GenerateRelateRuleSet(RelateRuleSet &rset,ElementSeqIndexArray &unkSet, ElementSequenceCollection &ecol,FragmentSet &fset);

	
	virtual bool MinineFragmentPosition(Fragment &frag,ElementSequenceCollection &emCol);

	
	virtual bool GenerateRuleTree(RuleTreeRoot &root, RelateRuleSet &rset,ElementSeqIndexArray unkSet,int rawFlowSize);

	
	virtual bool GenerateRuleSet(FragUnitArrayColSet &fragUnitArrayColSet, RuleTreeRoot &root);

		
private:
	
};


class UtilityFunc
{
public:
	
	static char *GetCurrentDate();
	
	static int GetFragmentCountInElementSet(const ElementSeries &es, ElementSequenceCollection &emCol,int maxGap);
	static int GetFragmentCountInElementSet(const ElementSeries &es, ElementSequenceCollection &emCol,int beginPos, int endPos);
	
	static void GenerateVisiableString(string &outStr, const ElementSeries &sr);
	
	static int FindForwardElement(int start,int v, const ElementSeries &es);

	
	static bool SearchSeries(int start, const ElementSeries &frag, const ElementSeries &es);

	
	static bool GenerateFragmentSeriesFromElementSeries(FragmentCombineSet &cmSet,FragmentSet &fset, ElementSeries &ser);

	
	static bool GenerateFragmentSetFromCombineSet(FragmentSet &fset, FragmentCombineSet &cmSet);
	
	
	static int SearchFragmentSetInRelatedRuleSet(FragmentSet &fset,RelateRuleSet &rset);

	
	static bool CompareSeries(ElementSeries &one, ElementSeries &two);

	static bool UpdateFragmentInInterval(FragmentCombineSet &cmSet,Fragment &fset,ElementSeries &sr);

	static bool UpdateBetweenInterval(FragmentCombineSet &cmSet,Fragment &frag, ElementSeries &sr, int start, int end);

	
	static bool UpdateFragUnit(FragUnit &u,int index, RelatedRule &relatedRule);

	
	static int RecursiveAddRelatedRuleNode(RelatedRule &rule, int index, RuleNode &node, int rawFlowSize);

	
	static bool AddRelatedRuleToRuleTree(RuleTreeRoot &root, RelatedRule &relatedRule,int rawFlowSize);

	
	static int SearchChildNode(FragUnit &fu, RuleChildNode &c);
	
	static bool MergeIntersect(FragUnit &mergeTo, FragUnit &mergeFrom);
	
	static bool IsIntersect(FragUnit &one, FragUnit &two);

	
	static bool PruneRuleTree(RuleTreeRoot &root,RuleNode *pn, double treePruneThreshold,int rawFlowSize);

	
	static bool AddRawSetBelowNodeToUnk(RuleNode *pr,ElementSeqIndexArray &emIndexSet);

	
	static bool GenerateRuleDescriptUnit(FragUnitArrayColSet &fragUnitArraySet, RuleNode *pr, FragUnitArrayCol fragCol);

	static bool GenerateElementSeqenceFromUnknowSet(ElementSequenceCollection &newEmCol,
		ElementSequenceCollection &oldEmCol,
		ElementSeqIndexArray &emIndexSet);

	static bool GetElementSeries(ElementSeries &destSr, int startPos, int size, ElementSeries &orgSr);
	static bool IsAllZero(const ElementSeries &sr);
private:	
	
	static int FindBackwardElement(int start,int scale,int v,const ElementSeries &es);
	
	static bool FindSeqWithGap(int start,const ElementSeries &fragSr, const ElementSeries &emSr,int maxGap);	
};



class CExecutor
{
private:
	CLayerMining *pmine;
	RawNetflowCollection col;
	AuditSequence enSeq;
public:
	void Run();
	bool ExecMine(ElementSequenceCollection &nextCol, 
		FragUnitArrayColSet &fragUnitArraySet,
		ElementSequenceCollection &curEmCol);
};
#endif