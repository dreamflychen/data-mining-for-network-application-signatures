#ifndef EVALUATE_H
#define EVALUATE_H

#include "stdcommonheader.h"
#include "constdefine.h"
#include "payload.h"
#include "utilityfunc.h"
#include "gspsystem.h"


typedef map<string,int> TagCountMap;
typedef TagCountMap::iterator TagCountMapIt;


struct RegexRuleCollection;
struct RegexRule
{	
	double _weight;
	string _regexStr;
	wregex _regex;
	string _tag;
	TagCountMap _errorIdMap;
	RegexRule():_weight(0){}
};

typedef vector<RegexRule> RegexRuleSet;


struct TP
{
	//The sample classified as the protocol: corrected recognized size/ being classified as the protocol size
	double _tp;
	unsigned int _totalAppCount;
	unsigned int _trueCount;
	TP():_tp(0),_totalAppCount(0),_trueCount(0){}
};

struct FP
{
	//1-TP
	double _fp;
	unsigned int _totalRecogCount;
	unsigned int _falseCount;
	FP():_fp(0),_totalRecogCount(0),_falseCount(0){}
};


struct RegexRuleCol
{
	string _tag;
	IntVector _regexRuleIndex;
	TP _appTP;
	FP _appFP;
	TP _priAppTP;
	FP _priAppFP;
	//TagCountMap _errorIdMap;
};

typedef unordered_map<string, RegexRuleCol> ProtoRegexMap;
typedef ProtoRegexMap::iterator RegexMapIt;

class CEvaluate
{
	ProtoRegexMap _proMap;
	FILE *_regexFile;
	FILE *_packIndexFile;
	FILE *_packFile;
	UCharVector _ucv;
	WCharVector _wucv;
	RegexRuleSet _regRuleSet;
	void _TestInstance(int size,const char* tag);
	void _PriorityTestInstance(string &tag);
	void _GlobalTestInstance(string &tag);
	string _regexFileName;
public:
	bool Init(const char* packFilePath,const char* regexFilePath);
	void Evaluate();
	void myrun();
};


#endif