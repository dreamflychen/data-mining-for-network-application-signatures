#include "utilityfunc.h"


{
	res.clear();
	sort(one.begin(),one.end());
	sort(two.begin(),two.end());
	res.resize(one.size()+two.size());
	IvIt it = set_intersection(one.begin(),one.end(),two.begin(),two.end(),res.begin());
	res.resize(it-res.begin());
}

void UtilityFunc::UCharVectorToWCharVector(WCharVector &wv, UCharVector &uv)
{
	wv.clear();
	for(size_t i = 0; i<uv.size();++i)
	{
		wv.push_back(uv[i]);
	}
}

void UtilityFunc::MultiCharToUnicodeChar(wchar_t *pDest,const char *pSource)
{
	size_t s = strlen(pSource);
	memset(pDest,0,sizeof(wchar_t)*(s+1));
	for(size_t i=0;i<s;++i)
	{
		pDest[i] = pSource[i];
	}
}

void UtilityFunc::MultiCharToUnicodeChar(wstring &destStr,const char *pSource)
{
	destStr.clear();
	size_t s = strlen(pSource);
	for(size_t i=0;i<s;++i)
	{
		destStr += pSource[i];
	}
}

const char* UtilityFunc::UnicodeCharToMultiChar(string &str,const wchar_t *pSource)
{
	str.clear();
	size_t s = wcslen(pSource);
	for(size_t i=0; i<s; ++i)
	{
		str += (char)(pSource[i]&0x00FF);
	}
	return str.c_str();
}

bool UtilityFunc::IsTwoUCharVectorEqual(const UCharVector &left,const UCharVector &right)
{
	if(left.size()!=right.size())
		return false;
	return (memcmp(&left[0],&right[0],left.size())==0);
}
bool UtilityFunc::IsEqualAtPos(UCharVector &ucv, int startpos, UCharVector &pattern)
{
	if(startpos >= (int)ucv.size() || 
		startpos+pattern.size()-1 >= ucv.size())
		return false;
	return (memcmp(&ucv[startpos],&pattern[0],pattern.size())==0);
}
const wchar_t* UtilityFunc::UCharVectorToRegexString(wstring &str,const UCharVector &ucv)
{
	str.clear();
	char cc[16];
	wchar_t wcc[16];
	for(size_t i=0; i<ucv.size(); ++i)
	{
		wchar_t c = ucv[i];
		switch(c)
		{
		case '\n':
			str += L"\\n";
			break;
		case '\r':
			str += L"\\r";
			break;
		case '\t':
			str += L"\\t";
			break;
		case '(':
			str += L"\\(";
			break;
		case ')':
			str += L"\\)";
			break;
		case '.':
			str += L"\\.";
			break;
		case '|':
			str += L"\\|";
			break;
		case '\\':
			str += L"\\\\";
			break;
		default:
			if(c>='a' && c<='z'
				||
				c>='A' && c<='Z'
				||
				c>='0' && c<='9' || c==':' || c=='='|| c=='_' || c=='=' || c =='-')
			{
				str += c;
			}
			else
			{
				sprintf(cc, "\\u%04x",c);
				UtilityFunc::MultiCharToUnicodeChar(wcc,cc);
				str += wcc;
			}
		}
	}
	return str.c_str();
}
const char* UtilityFunc::UCharVectorToString(string &str,const UCharVector &ucv)
{
	char hexStr[6];
	int len = ucv.size();
	str.clear();
	int v(0);
	for(int i=0; i<len ;++i)
	{
		if(i > 0)
			str += " "; 
		v = ucv[i];
		if( v > 0x20 && v< 0x7F) 
		{			
			str += (char)v;
		}
		else if(v == 0x09)
		{
			str += "[tab]";
		}
		else if(v == 0x20)
		{
			str += "[sp]";
		}
		else if(v == 0x0A)
		{
			str += "[nl]";
		}
		else if(v == 0x0D)
		{
			str += "[er]";
		}
		else 
		{
			sprintf(hexStr,"0x%02x",v);
			str += hexStr;
		}
	}
	return str.c_str();
}

const char* UtilityFunc::InputValueInArray(string &str,double value, int arraySize)
{
	str.clear();
	char cc[32];
	sprintf(cc,"%f ", value);
	str+=cc;
	sprintf(cc,"0 ");
	for(int i=1; i<arraySize; ++i)
	{
		str+=cc;
	}
	return str.c_str();
}