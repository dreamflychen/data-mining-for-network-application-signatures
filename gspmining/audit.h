#ifndef AUDIT_H
#define AUDIT_H

#include "stdcommonheader.h"
#include "constdefine.h"
#include "element.h"
#include "utilityfunc.h"


struct FragLevPos;
struct FrequentCount
{
	
	union{
	int _index;
	FragLevPos *_pLevPosCount;
	};
	
	unsigned int _count;

	FrequentCount():_index(-1),_count(0){}
};


struct FragmentReference
{
	
	unsigned char *_pv;
	
	int _length;
	FragmentReference():_pv(0),_length(0){}
	void InitFromUCharVector(int pos,int len, UCharVector &v){
		this->_pv = &v[pos];
		this->_length = len;
	}
};

typedef FragmentReference* FragRefPtr;


struct FragmentReferenceHashAndEq
{
	
	size_t operator()(const FragmentReference &ptrRef) const
	{
		register unsigned int nr=1, nr2=4;
		const unsigned char *s = ptrRef._pv;
		int length = ptrRef._length;
		int i=0;
		while (length--)
		{
			nr ^= (((nr & 63)+nr2)*((unsigned int) (unsigned char) s[i++]))+ (nr << 8);
			nr2 += 3;
		}
		return(nr);	
	}

	bool operator()(const FragmentReference& left, const FragmentReference& right) const
	{	
		
		register int size = left._length;
		if(size != right._length)
			return false;
		return (memcmp(left._pv,right._pv,size)==0);
	}
};

typedef unordered_map<FragmentReference, FrequentCount,FragmentReferenceHashAndEq,FragmentReferenceHashAndEq> AuditElementMap;
typedef AuditElementMap::iterator AdtEmMapIt;


struct AuditWindow
{
	int _flowSize;					
	int _startPos;					
	int _kindSize;				
	
	double _maxFreq;					
	double _minFreq;					
	ElementSeries _maxSer;		
	AuditWindow():
		_flowSize(0),_startPos(0),_kindSize(0),
		_maxFreq(0),_minFreq(0){}

	
	static bool DescendByMaxFreq(const AuditWindow* left,const AuditWindow* right)
	{
		return ( left->_maxFreq > right->_maxFreq );
	}
};


typedef vector<AuditWindow> AuditWindowSet;
typedef AuditWindow* AdWinPtr;
typedef vector<AdWinPtr> AdWinPtrSet;


struct AuditSequence
{
private:
	

	
	void _CountShingleInSequence(AuditElementMap &auMap,UCharVector &ucv, int start,int index);
	
	bool _BuildWindow(AuditWindow &aw);
	
	void _StatisticsWindows(AuditWindow &aw,AuditElementMap &auMap);
	
public:
	AuditWindowSet _winSet;			
	ElementSequenceCol *pEmCol;		
	int _halfWindowSize;			
	int _shingleSize;				
	double _lineSupport;			
	double _minLineSupport;			
	DoubleVector _supportVector;	

	DoubleVector _windowSupportVector;

	const char * ToMatLabTxtMatrix(string &str);
	const char * ToSummaryString(string &str);
	
	AuditSequence():pEmCol(0),_halfWindowSize(HALF_WINDOW_SIZE),_shingleSize(SHINGLE_SIZE),
		_lineSupport(0),_minLineSupport(MIN_SUPPORT_THRESHOLD)
	{
		
	}
	

	
	void setParam(int halfWindowSize=HALF_WINDOW_SIZE,int shingleSize=SHINGLE_SIZE,double minLineSupport=MIN_SUPPORT_THRESHOLD)
	{
		_halfWindowSize = halfWindowSize;
		_shingleSize = shingleSize;
		_minLineSupport = minLineSupport;
	}

	virtual bool GenerateAuditSequence(
		ElementSequenceCol &col		
		);
	
	
	
	
	virtual bool CalcSupport();

	
	
	
	int CalcIntervalLength(double support);

	
	{
		return _lineSupport>=_minLineSupport;
	}
};

#endif