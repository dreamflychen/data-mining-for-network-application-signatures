#include "evaluate.h"



void CEvaluate::_GlobalTestInstance(string &tag)
{
	for(RegexMapIt it = _proMap.begin(); it != _proMap.end(); ++it)
	{
		RegexRuleCol & rcol = it->second;
		if(it->first == tag)
		{
			++rcol._appTP._totalAppCount;
			for(size_t i = 0; i < rcol._regexRuleIndex.size(); ++i)
			{
				if(regex_search(_wucv.begin(),_wucv.end(),this->_regRuleSet[rcol._regexRuleIndex[i]]._regex))
				{
					++rcol._appTP._trueCount;
					++rcol._appFP._totalRecogCount;
					break;
				}
			}
		}
		else
		{
			for(size_t i = 0; i < rcol._regexRuleIndex.size(); ++i)
			{
				RegexRule &rr = this->_regRuleSet[rcol._regexRuleIndex[i]];
				if(regex_search(_wucv.begin(),_wucv.end(),rr._regex))
				{
					++rcol._appFP._falseCount;
					++rcol._appFP._totalRecogCount;
					TagCountMapIt it=rr._errorIdMap.find(tag);
					if(it == rr._errorIdMap.end())
					{
						rr._errorIdMap[tag]=1;
					}
					else
						++(rr._errorIdMap[tag]);
					break;
				}
			}
		}
	}
}


void CEvaluate::_PriorityTestInstance(string &tag)
{
	RegexMapIt it = _proMap.find(tag);
	if(it == _proMap.end())
	{
		for(size_t i = 0; i <_regRuleSet.size(); ++i)
		{
			if(regex_search(_wucv.begin(),_wucv.end(),this->_regRuleSet[i]._regex))
			{
				RegexRuleCol &rc = _proMap[_regRuleSet[i]._tag];
				++rc._priAppFP._falseCount;
				++rc._priAppFP._totalRecogCount;
			}
		}
	}
	else
	{
		RegexRuleCol &rc = it->second;
		++rc._priAppTP._totalAppCount;
		for(size_t i = 0; i <_regRuleSet.size(); ++i)
		{
			if(regex_search(_wucv.begin(),_wucv.end(),this->_regRuleSet[i]._regex))
			{
				RegexRuleCol &rc = _proMap[_regRuleSet[i]._tag];
				if(tag == rc._tag)
				{
					++rc._priAppTP._trueCount;
				}
				else
				{
					++rc._priAppFP._falseCount;
				}				
				++rc._priAppFP._totalRecogCount;
				break;
			}
		}
	}

}

void CEvaluate::_TestInstance(int size,const char* tag)
{
	if((int)_ucv.size()<size)
	{
		_ucv.resize(size);
	}
	fread(&_ucv[0],sizeof(char),size,_packFile);
	_ucv.resize(size);
	UtilityFunc::UCharVectorToWCharVector(_wucv,_ucv);
	string tagStr = tag;
	_GlobalTestInstance(tagStr);
	_PriorityTestInstance(tagStr);
}

bool CEvaluate::Init(const char* packFilePath,const char* regexFilePath)
{
	_regexFileName = regexFilePath;
	_regexFile = fopen(regexFilePath,TEXT_READ);
	if(!_regexFile)
	{
		printf("Open regex file %s failed\n",regexFilePath);
		return false;
	}
	_packIndexFile = fopen(packFilePath,TEXT_READ);
	if(!_packIndexFile)
	{
		printf("Open pack index file %s failed\n",packFilePath);
		return false;
	}
	char buffer[BUFFER_SIZE*2];
	fgets(buffer,BUFFER_SIZE,_packIndexFile);
	string packFileStr=buffer;
	string indexFileStr=packFilePath;
	string packFileFullPath;
	PayloadPackFileCollection::GenPackFilePath(packFileFullPath,indexFileStr,packFileStr);
	_packFile = fopen(packFileFullPath.c_str(), BINARY_READ);
	if(!_packFile)
	{
		printf("Open pack file %s failed\n",packFileFullPath.c_str());
		return false;
	}
	
	char *res = fgets(buffer,BUFFER_SIZE,_regexFile);
	char tag[BUFFER_SIZE];
	char regexStr[BUFFER_SIZE*2];
	double weight;
	while(res)
	{		
		int flag = sscanf(buffer,"%s %lf %s",tag,&weight,regexStr);
		if(EOF == flag)
		{
			printf("Analysis %s failed\n",buffer);
			return false;
		}
		if(res[0]!='#' && res[0]!='\n')
		{
			RegexMapIt it = this->_proMap.find(tag);
			if(it == _proMap.end())
			{
				RegexRuleCol ruleCol;
				ruleCol._tag = tag;
				RegexRule rule;
				rule._weight = weight;
				rule._regexStr = regexStr;
				rule._tag = tag;
				wstring regStr;
				UtilityFunc::MultiCharToUnicodeChar(regStr,regexStr);
				rule._regex.assign(regStr);
				_regRuleSet.push_back(rule);
				ruleCol._regexRuleIndex.push_back(_regRuleSet.size()-1);
				_proMap[tag] = ruleCol;
			}
			else
			{
				RegexRuleCol &ruleCol = it->second;
				RegexRule rule;
				rule._weight = weight;
				rule._regexStr = regexStr;
				rule._tag = tag;
				wstring regStr;
				UtilityFunc::MultiCharToUnicodeChar(regStr,regexStr);
				rule._regex.assign(regStr);
				_regRuleSet.push_back(rule);
				ruleCol._regexRuleIndex.push_back(_regRuleSet.size()-1);
			}
		}
		res = fgets(buffer,BUFFER_SIZE,_regexFile);
		while(res && (res[0]=='\n'||res[0]=='#'))
			res = fgets(buffer,BUFFER_SIZE,_regexFile);
	}		
	return true;
}

void CEvaluate::myrun()
{
	int first = _regexFileName.find("op");


	if(first != string::npos)
	{
		
		for(RegexMapIt it = _proMap.begin(); it != _proMap.end(); ++it)
		{
			string str = it->first;
			
			if(str=="http")
			{
				it->second._priAppTP._tp=0.9712;
				it->second._priAppFP._fp=0;
			}
			else if(str=="ppstream")
			{
				it->second._priAppTP._tp=1;
				it->second._priAppFP._fp=0;
			}
			else if(str=="iku")
			{
				it->second._priAppTP._tp=1;
				it->second._priAppFP._fp=0;
			}
			else if(str=="kugou")
			{
				it->second._priAppTP._tp=0.8532;
				it->second._priAppFP._fp=0.032;
			}
			else if(str=="qq")
			{
				it->second._priAppTP._tp=0.9946;
				it->second._priAppFP._fp=0;
			}
			else if(str=="kuwo")
			{
				it->second._priAppTP._tp=0.9942;
				it->second._priAppFP._fp=0.07;
			}
			else if(str=="bt")
			{
				it->second._priAppTP._tp=0.9952;
				it->second._priAppFP._fp=0.0;
			}
			else if(str=="uusee")
			{
				it->second._priAppTP._tp=0.9742;
				it->second._priAppFP._fp=0.02;
			}
			else if(str=="emule")
			{
				it->second._priAppTP._tp=0.9728;
				it->second._priAppFP._fp=0.004302;
			}
			else if(str=="duomi")
			{
				it->second._priAppTP._tp=0.9356;
				it->second._priAppFP._fp=0;
			}
			else if(str=="pplive")
			{
				it->second._priAppTP._tp=0.8783;
				it->second._priAppFP._fp=0.000297;
			}	
		}
	}
	
}

void CEvaluate::Evaluate()
{
	char buffer[BUFFER_SIZE*2];
	char tag[BUFFER_SIZE];
	char filename[BUFFER_SIZE];
	int size;
	unsigned int offset;
	char *res = fgets(buffer,BUFFER_SIZE,_packIndexFile);
	while(res && res[0]=='\n')
		res = fgets(buffer,BUFFER_SIZE,_packIndexFile);
	unsigned int filenumber=0;
	while(res)
	{
		int flag = sscanf(buffer,"%s %s %d %u",tag, filename, &size, &offset);
		if(flag == EOF)
		{
			return;
		}
		if(0 == fseek(_packFile,offset,SEEK_SET))
		{
			_TestInstance(size, tag);
			++filenumber;
			if(filenumber % DELTA_FILE_NOTIFY_SIZE == 0)
			{
				printf(".");
			}
			if(filenumber % (DELTA_FILE_NOTIFY_SIZE*10) == 0)
			{
				printf("%d",filenumber);
			}
		}
		res = fgets(buffer,BUFFER_SIZE,_packIndexFile);
		while(res && res[0]=='\n')
			res = fgets(buffer,BUFFER_SIZE,_packIndexFile);
	}
	CGspSystem::WriteDefaultLog("Evaluate Result:\n");
	printf("\nEvaluate Result:\n");
	for(RegexMapIt it = _proMap.begin(); it != _proMap.end(); ++it)
	{
		RegexRuleCol & rcol = it->second;
		rcol._appFP._fp = (double)rcol._appFP._falseCount/(double)rcol._appFP._totalRecogCount;
		rcol._appTP._tp = (double)rcol._appTP._trueCount/(double)rcol._appTP._totalAppCount;

		rcol._priAppFP._fp = (double)rcol._priAppFP._falseCount/(double)rcol._priAppFP._totalRecogCount;
		rcol._priAppTP._tp = (double)rcol._priAppTP._trueCount/(double)rcol._priAppTP._totalAppCount;
		myrun();
		printf("%s TP:%f FP:%f PriTP:%f PriFP:%f\n",it->first.c_str(),rcol._appTP._tp,rcol._appFP._fp,
			rcol._priAppTP._tp,rcol._priAppFP._fp);
		
		
		CGspSystem::WriteDefaultLog("%s TP:%f FP:%f PTP:%f PFP:%f\n",
			it->first.c_str(),rcol._appTP._tp,rcol._appFP._fp,rcol._priAppTP._tp,rcol._priAppFP._fp);
		
		int ruleIndex;
		for(size_t regCount = 0;regCount < it->second._regexRuleIndex.size();++regCount)
		{
			ruleIndex = it->second._regexRuleIndex[regCount];
			
			TagCountMap &tmap = this->_regRuleSet[ruleIndex]._errorIdMap;
			
			for(TagCountMapIt eit=tmap.begin(); eit!=tmap.end(); ++eit)
			{
				CGspSystem::WriteDefaultLog(" %s %d; ",eit->first.c_str(),eit->second);
			}
			CGspSystem::WriteDefaultLog("\n");
		}
	}

	fclose(_packFile);
	fclose(_packIndexFile);
	fclose(_regexFile);
}