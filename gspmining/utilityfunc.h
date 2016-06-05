#ifndef UTILITYFUNC_H
#define UTILITYFUNC_H

#include "stdcommonheader.h"

typedef vector<unsigned char> UCharVector;

typedef vector<int> IntVector;
typedef IntVector::iterator IvIt;
typedef IntVector * IntVecPtr;

typedef vector<double> DoubleVector;

typedef vector<unsigned short int> USIntVector;

typedef vector<bool> BoolVector;
typedef vector<wchar_t> WCharVector;


class UtilityFunc
{
public:
	
	static void IntVectorIntersect(IntVector &res,IntVector &one,IntVector &two);
	static void UCharVectorToWCharVector(WCharVector &wv, UCharVector &uv);
	static void MultiCharToUnicodeChar(wchar_t *pDest,const char *pSource);
	static void UtilityFunc::MultiCharToUnicodeChar(wstring &destStr,const char *pSource);
	static const char* UnicodeCharToMultiChar(string &str,const wchar_t *pSource);
	static bool IsTwoUCharVectorEqual(const UCharVector &left,const UCharVector &right);
	static bool IsEqualAtPos(UCharVector &ucv, int startpos, UCharVector &pattern);
	static const wchar_t* UCharVectorToRegexString(wstring &str,const UCharVector &ucv);
	static const char* UCharVectorToString(string &str,const UCharVector &ucv);
	static const char* InputValueInArray(string &str,double value, int arraySize);
};

struct UCharVectorHashAndEq
{
	
	size_t operator()(const UCharVector &ptrRef) const
	{
		register unsigned int nr=1, nr2=4;
		const unsigned char *s = &ptrRef[0];
		int length = ptrRef.size();
		int i=0;
		while (length--)
		{
			nr ^= (((nr & 63)+nr2)*((unsigned int) (unsigned char) s[i++]))+ (nr << 8);
			nr2 += 3;
		}
		return(nr);	
	}

	bool operator()(const UCharVector& left, const UCharVector& right) const
	{	
		// apply operator== to operands
		register int size = left.size();
		if(size != right.size())
			return false;
		return (memcmp(&left[0],&right[0],size)==0);
	}
};

#endif