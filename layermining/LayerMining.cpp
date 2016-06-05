#include <limits>
#include "LayerMining.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include "osbase.h"

namespace GV
{
	int direction = ORDERED_DIRECTION;
	FileMap fileMap;
	char logfile[]= DEFAULT_LOG_FILE;
	char *payloadFileExtension = PAYLOAD_EXTENSION;
};


//		hash table for candidate fragments
typedef vector<int> IndexArray;
typedef vector<int>::iterator IndexArrayIt;
typedef hash_map<ElementSeries, IndexArray, HashFragCompare> PartFragTable; // record these have the same fragment part
typedef PartFragTable::iterator PartFragTableIt;
//		mining data reated to positions
typedef vector<int*> PosLayerArray;



size_t _elementSeriesHash(const ElementSeries &s)
{
	register unsigned int nr=1, nr2=4;
	int length = s.size();
	int i=0;
	while (length--)
	{
		nr ^= (((nr & 63)+nr2)*((unsigned int) (unsigned char) s[i++]))+ (nr << 8);
		nr2 += 3;
	}
	return((unsigned int) nr);	
}



bool ascendRawNetflowAttribute(RawNetflowAttribute &i,RawNetflowAttribute &j) 
{ 
	return (i.length<j.length); 
}

bool ascendElementSequence(ElementSequence &i,ElementSequence &j)
{
	return (i.series.size() < j.series.size());
}

bool ascendFragmentCombineItem(FragmentCombineItem &i,FragmentCombineItem &j)
{
	return ( i.pos < j.pos );
}

bool ascendDouble(double &i,double &j)
{
	return (i < j);
}

struct DescendAuditWindowSet
{
	AuditWindowSet *pWinSet;
	bool operator() (int &i, int &j)
	{
		AuditWindow &ai = (*pWinSet)[i];
		AuditWindow &aj = (*pWinSet)[j];
		return (ai.maxFreq > aj.maxFreq);
	}
};
bool descendAuditWindowSet(AuditWindow &i, AuditWindow &j)
{
	return (i.maxFreq > j.maxFreq);
}

bool initSystem()
{
#ifdef _DEBUG
	printf("=====init system=====\n");
#endif
	FILE *fp = fopen(GV::logfile, LOGFILE_MODE);
	if(fp)
		GV::fileMap[GV::logfile] = fp;
	else
	{
		printf("Open file %s error\n", GV::logfile);
		return false;
	}
	writedeflog(":Init System at %s", UtilityFunc::GetCurrentDate());
	return true;
}
bool destroySystem()
{

	writedeflog(":Destroy System at %s", UtilityFunc::GetCurrentDate());
	FILE *fp;
	for(FileIt it = GV::fileMap.begin(); it != GV::fileMap.end(); ++it)
	{
		printf("Close file: %s\n", it->first.c_str());
		fp = it->second;
		if(fp)
		{
			fclose(fp);
			it->second = 0;
		}
		else
			printf("Invalid file handle");
	}
	GV::fileMap.clear();
#ifdef _DEBUG
	printf("-----destroy system-----\n");
#endif
	return true;
}



bool _writeFileLog(const char *file,const char*format, va_list ap)
{
	FileIt it = GV::fileMap.find(file);
	if(it != GV::fileMap.end())
	{
		FILE *fp = it->second;
		if(!fp)
			return false;
		vfprintf(fp,format,ap);
	}
	else
		return false;
	return true;	
}

bool writelog(const char *file,const char* format, ...)
{
	bool res;
	FILE *fp;
	FileIt it;	
	va_list ap;
	it =  GV::fileMap.find(file);
	if(it == GV::fileMap.end())
	{
		fp = fopen(file,LOGFILE_MODE);
		if(!fp)
			return false;
		GV::fileMap[file] = fp;
	}

	va_start(ap, format);
	res = _writeFileLog(file, format,ap);
	va_end(ap);
	return res;
}

bool closelog(const char *file)
{
	bool res;
	FILE *fp;
	FileIt it;	
	it =  GV::fileMap.find(file);
	if(it == GV::fileMap.end())
	{
		return true;
	}
	fclose(it->second);
	GV::fileMap.erase(it->first);
	return true;
}


bool writedeflog(const char *format, ...)
{
	bool res;
	va_list ap;
	va_start(ap, format);
	res =  _writeFileLog(GV::logfile, format,ap);
	va_end(ap);
	return res;
}

/////////////////
CLayerMining::CLayerMining()
{
	fcosine = DEFAULT_COSINE;
	scale = 0.5f;
	intervalLength = DEFAULT_POS_INTERVAL;
	treePruneThreshold = DEFAULT_PRUNE_SUPPORT;
	reserveRation = DEFAULT_RESERVE_RATION;
	maxReserveLength = MAX_RESERVE_LEN;
}

bool CLayerMining::CreateOriginalNetflowSet(
		RawNetflowCollection &orawset, 
		char *path,
		char *keyword)
{
	writedeflog(">Enter CLayerMining::CreateOriginalNetflowSet\n");
	orawset.init();
	if(keyword && strlen(keyword))
		orawset.keyword = keyword;
	else
	{
		orawset.keyword = "";
		keyword = 0;
	}
	if(path && strlen(path))
		orawset.path = path;
	else
	{
		orawset.path = "";
		writedeflog("The path is empty\n");
		writedeflog(">Leave CLayerMining::CreateOriginalNetflowSet\n");
		return false;
	}
	bool res(true);	
	if(!findAllFiles(orawset.fileSet, orawset.path, keyword?orawset.keyword.c_str():0))
		res = false;
	else if(!orawset.fileSet.size())
	{
		writedeflog("The fileSet is empty\n");
		res = false;
	}
	else
	{

		sort(orawset.fileSet.begin(),orawset.fileSet.end(),ascendRawNetflowAttribute);
	}

	orawset.totalFileSize = orawset.fileSet.size();
	writedeflog("Folder path is %s;  keyword is %s;  file set size is %d\n"
		, orawset.path.c_str(), orawset.keyword.c_str(), orawset.totalFileSize);
	writedeflog(">Leave CLayerMining::CreateOriginalNetflowSet\n");
	return res;
}

bool CLayerMining::SetAlignLength (RawNetflowCollection &rcol)
{
	writedeflog(">Enter CLayerMining::SetAlignLength\n");
	int clipSize =(int)( (1.0-reserveRation)*(double)rcol.totalFileSize);
	rcol.reserveLen = rcol.fileSet[clipSize].length;
	if(rcol.reserveLen > maxReserveLength)
	{
		rcol.reserveLen = maxReserveLength;
	}
	writedeflog("RawNetflowCollection reserveLength is %d\n",rcol.reserveLen);
	writedeflog(">Leave CLayerMining::SetAlignLength\n");
	return true;
}


bool CLayerMining::CreateElemSeqColFromRawCol(
	ElementSequenceCollection &ecol,
	RawNetflowCollection &rcol)
{
	writedeflog(">Enter CLayerMining::CreateElemSeqColFromRawCol\n");
	int id(0);
	uchar *pbuf(0);
	string path;
	
	if(rcol.reserveLen != -1)
	{
		pbuf = new uchar[rcol.reserveLen];
	}
	else
		pbuf = new uchar[rcol.fileSet.back().length];
	
	if(!pbuf) {
		writedeflog(">Leave CLayerMining::CreateElemSeqColFromRawCol\n");
		return false;
	}

	FILE *fp;
	int size;
	int index(0);
	for(RawNetflowAttrIt it = rcol.fileSet.begin(); 
		it != rcol.fileSet.end(); ++it)
	{
		ElementSequence seq;
		//including clipping
		if((rcol.reserveLen != -1 && it->length >= rcol.reserveLen) || rcol.reserveLen == -1)
		{
			//satisfied flows
			path = rcol.path+it->filename;
			fp = fopen(path.c_str(), BINARY_READ);
			if(!fp) {
				delete [] pbuf;
				writedeflog(">Leave CLayerMining::CreateElemSeqColFromRawCol\n");
				return false;
			}
			if(rcol.reserveLen == -1)
			{//read whole flow
				size = fread(pbuf,sizeof(uchar),it->length,fp);
			}
			else
			{//read truncated flow
				size = fread(pbuf, sizeof(uchar), rcol.reserveLen, fp);
			}
			fclose(fp);
		
			for(int i=0; i<size; ++i)
			{
				seq.series.push_back(pbuf[i]);
			}
			seq.rawFlowId = index++;
			seq.scoreEarned = 0.0;
			seq.id = id++;
			ecol.pMapTable = 0;
			ecol.seqSet.push_back(seq);
		}

	}
	sort(ecol.seqSet.begin(),ecol.seqSet.end(),ascendElementSequence);
	delete [] pbuf;
	writedeflog("Element Sequence Set Size: %d\n",ecol.seqSet.size());
	writedeflog(">Leave CLayerMining::CreateElemSeqColFromRawCol\n");
	return true;
}

double CLayerMining::CalcAuditZeroPunish(int shingleSize)
{
	if(shingleSize <=1)
		return 0.5;
	else if(shingleSize == 2)
		return 0.6;
	else if(shingleSize == 3)
		return 0.8;
	else if(shingleSize == 4)
		return 0.7;
	else if(shingleSize >= 5)
		return 0.8;
	return 0.8;
}

bool CLayerMining::GenerateMaxAuditSequence(
		AuditSequence  &seq,				//entropy
		ElementSequenceCollection &col,		//sequential set
		int halfWindowSize,					//half window size
		int shingleSize)
{
	seq.winSet.clear();
	seq.halfWindowSize = halfWindowSize;
	seq.shingleSize = shingleSize;
	int maxlen = col.seqSet.back().series.size();
	int windowCount = maxlen/halfWindowSize;

	for(int i=0; i<windowCount; ++i)
	{
		AuditWindow emptyaw;
		emptyaw.startPos = i*halfWindowSize;
		seq.winSet.push_back(emptyaw);
	}
	int sizeLimit;
	int seqSize = col.seqSet.size();
	int spos;
	ElementSeries ds;
	AdtEmTableIt adit;
	int windowSize = halfWindowSize*2;
	//process each flow
	for(int i=0; i < seqSize; ++i)
	{
		ElementSeries &ts = col.seqSet[i].series;
		for(int j=0;j<windowCount; ++j)			
		{
			AuditWindow &aw = seq.winSet[j];				
			
			spos = aw.startPos;								
			sizeLimit = ts.size();							
			if(spos+shingleSize > sizeLimit)				
				break;
			else
				
				++aw.flowSize;
			for(int k=0; k<windowSize;++k)
			{
				if(UtilityFunc::GetElementSeries(ds,spos+k,shingleSize,ts))
				{
					adit = aw.auditTable.find(ds);
					if(adit == aw.auditTable.end())
					{
						
						aw.auditTable[ds].count = 1;
						aw.auditTable[ds].index = i;
					}
					else
					{
						if(aw.auditTable[ds].index != i)
						{
							aw.auditTable[ds].index = i;
							++aw.auditTable[ds].count;
						}
					}
					++aw.totalCount;
				}
				else
					break;
			}
		}
	}

	
	for(int i=windowCount-1; i>=0; --i)
	{
		AuditWindow &aw = seq.winSet[i];	
		if(aw.auditTable.size()==0)
			seq.winSet.pop_back();
	}
	windowCount = seq.winSet.size();
	
	DoubleVector dv;
	double dsum;
	double dem;
	int dvSize;
	double sdd;
	for(int i=0; i<windowCount; ++i)
	{
		AuditWindow &aw = seq.winSet[i];	
		if(aw.auditTable.size()==0)
			break;
		dv.clear();
		dsum = 0.0;
		dem = 0.0;
		double max=-1;
		for(adit = aw.auditTable.begin(); adit!=aw.auditTable.end(); ++adit)
		{
			dem = (double)adit->second.count/(double)aw.flowSize;
			//if(UtilityFunc::IsAllZero(adit->first))
			//	dem *= CalcAuditZeroPunish(shingleSize);
			//if(dem > 1.0)
			//	dem = 1.0;
			if(dem>=max)
			{
				max = dem;
				aw.maxSer = adit->first;
			}

			dsum += dem;
			dv.push_back(dem);
		}
		if(dv.size() == 1)
		{
			aw.maxFreq = aw.minFreq = aw.midFreq = aw.averageFreq = dv[0];
			aw.freqStandDeviation = 0.0;
		}
		else
		{
			aw.averageFreq = dsum/(double)aw.auditTable.size();
			sort(dv.begin(),dv.end(),ascendDouble);
			aw.minFreq = dv[0];
			aw.maxFreq = dv[dv.size()-1];
			dvSize = dv.size();
			if(dvSize%2 == 0)
				aw.midFreq = (dv[dvSize/2] + dv[dvSize/2 -1])/2.0;
			else
				aw.midFreq = dv[dvSize/2];
			dsum = 0;
			for(int j=0;j<dvSize;++j)
			{
				sdd = dv[j] - aw.averageFreq;
				dsum += sdd * sdd;
			}
			dsum /= (double)(dv.size());
			aw.freqStandDeviation = sqrt(dsum);
		}
	}
	
	IntVector iv;
	int winSetSize = seq.winSet.size();
	for(int i=0; i<winSetSize; ++i)
	{
		iv.push_back(i);
	}
	DescendAuditWindowSet desCompare;
	desCompare.pWinSet = &seq.winSet;
	sort(iv.begin(),iv.end(),desCompare);
	
	int offsetIndex;
	double offsetMaxSup = -1.0;
	double tempOffsetSup;
	for(int i=0;i<winSetSize-1;++i)
	{
		tempOffsetSup = seq.winSet[i].maxFreq - seq.winSet[i+1].maxFreq;
		if(offsetMaxSup <= tempOffsetSup)
		{
			offsetMaxSup = tempOffsetSup;
			offsetIndex = i;
		}
	}
	seq.upperIndex = offsetIndex;
	seq.lowerIndex = offsetIndex+1;
	seq.lineSupport = seq.winSet[offsetIndex+1].maxFreq + offsetMaxSup/2.0;
	return true;
}

int CLayerMining::CalcSignatureIntervalLength(AuditSequence &seq,RawNetflowCollection &rcol)
{
	return rcol.reserveLen;
}

bool CLayerMining::ClipElementSequenceCollection(ElementSequenceCollection &col, int length)
{
	for(EmSeqSetIt it = col.seqSet.begin(); 
		it != col.seqSet.end(); ++it)
	{
		if(it->series.size() > (unsigned int)length)
		{
			it->series.erase(it->series.begin()+ length,it->series.end());
		}
	}
	return true;
}


bool CLayerMining::ClipAuditSequence(AuditSequence &seq,int length)
{
	return true;
}

double CLayerMining::CalcMinimumSupport(AuditSequence &seq, ElementSequenceCollection &set)
{
	return DEFAULT_MIN_SUPPORT;
}


bool CLayerMining::GenerateFrequentFragmentSet(FragmentCollection &fset,			
											ElementSequenceCollection &emCol,	
											FragmentCollection *pFragCol,			
											int len,							
											int maxGap)							
											
{
	CandidateFragmentCollection candidate;
	candidate.fragSize = len;
	fset.set.clear();
	
	if(!pFragCol)
	{
		
		if(len!=1)
			return false;
		
		for(EmSeqSetIt seqIt = emCol.seqSet.begin();
			seqIt != emCol.seqSet.end(); ++seqIt)
		{
			
			for(EmSeriesIt emSerIt = seqIt->series.begin();
				emSerIt != seqIt->series.end(); ++ emSerIt)
			{
				ElementSeries tseries(emSerIt, emSerIt+1);
				if(candidate.set.find(tseries) == candidate.set.end())
				{
					CandidateFragmentAttr attr;
					candidate.set[tseries]=attr;
				}
			}
		}
	}
	else
	{
		if(pFragCol->fragSize + 1 != len)
			return false;
		if(len == 2) 
		{
			int i,j;
			i=0;
					
			for(FragSetIt it_i = pFragCol->set.begin(); 
				it_i != pFragCol->set.end(); ++it_i)
			{
				j=0;
				for(FragSetIt it_j = pFragCol->set.begin();
					it_j != pFragCol->set.end(); ++it_j)
				{
					ElementSeries tseries(it_i->series);
					tseries.insert(tseries.end(),it_j->series.begin(),it_j->series.end()); 

					CandidateFragmentAttr attr;
					attr.leftItem = i;
					attr.rightItem = j;
					candidate.set[tseries] = attr;
					++j;
				}
				++i;
			}
		}
		else if(len >= 3) 
		{
			
			PartFragTable partTable;
			int index = 0;
			for(FragSetIt it_i = pFragCol->set.begin(); 
				it_i != pFragCol->set.end(); ++it_i)
			{
				ElementSeries tseries(it_i->series.begin()+1, it_i->series.end());
				PartFragTableIt fit = partTable.find(tseries);
				if(fit == partTable.end())
				{
					IndexArray a;
					a.push_back(index);
					partTable[tseries] = a;
				}
				else
				{
					fit->second.push_back(index);
				}
				++index;
			}
			
			index = 0;
			for(FragSetIt it_i = pFragCol->set.begin(); 
				it_i != pFragCol->set.end(); ++it_i)
			{
				ElementSeries tseries( it_i->series.begin(), 
					--it_i->series.end() );
				PartFragTableIt fit = partTable.find(tseries); 
				if(fit != partTable.end())
				{		
					
					for(IndexArrayIt it_a = fit->second.begin(); it_a != fit->second.end(); ++it_a)
					{
						
						ElementSeries candidateSeries(pFragCol->set[*it_a].series);
						
						candidateSeries.push_back(it_i->series.back());
						
						CandidateFragmentAttr attr;
						attr.leftItem = *it_a;
						attr.rightItem = index;
						candidate.set[candidateSeries] = attr;
					}
					
				}
				++index;
			}
		}
	}
	//2.matching each candidate set, increasing counter
	int count(0);
	for(CandidateFragSetIt cit = candidate.set.begin(); cit != candidate.set.end(); ++cit)
	{
		count = UtilityFunc::GetFragmentCountInElementSet(cit->first,emCol,maxGap);		
		cit->second.count = count;
	}
	//3. remove these do not satisfy the minimal support
	for(CandidateFragSetIt cit = candidate.set.begin(); cit != candidate.set.end(); )
	{
		if((double)(cit->second.count)/(double)(emCol.seqSet.size()) < minSupport)
		{
			candidate.set.erase(cit++);
		}				
		else
			++cit;
	}
	//4. remove these do not satisfy the position, or constraints, generage signature sets
	fset.fragSize = len;
	fset.preFragCol = pFragCol;	
	for(CandidateFragSetIt cit = candidate.set.begin(); cit != candidate.set.end(); )
	{
		Fragment frag;
		frag.count = cit->second.count;
		frag.leftItem = cit->second.leftItem;
		frag.rightItem = cit->second.rightItem;
		frag.series = cit->first;
		frag.support = (double)frag.count/(double)emCol.seqSet.size();
		
		if(!MinineFragmentPosition(frag,emCol))
		{
			candidate.set.erase(cit++);
		}
		else
		{
			
			if( !StrategyRuleAnalysis(frag, emCol,pFragCol,maxGap) )
			{
				candidate.set.erase(cit++);
			}
			else
			{
				
				fset.set.push_back(frag);
				if(len >1)
				{
					pFragCol->set[frag.leftItem].isClosedItem = false;
					pFragCol->set[frag.rightItem].isClosedItem = false;
				}
				++cit;
			}
		}
	}	
	if(fset.set.size() == 0)
		return false;
	return true;
}

bool CLayerMining::StrategyRuleAnalysis(Fragment &frag,ElementSequenceCollection &emCol,FragmentCollection *pPreCol,int maxGap)
{	

	double cosine(0);
	if(frag.series.size()>=2 && pPreCol)
	{
		FragmentCollection &preCol = *pPreCol;
		//calcuate the appeared times of the left short sequence
		Fragment &fl=preCol.set[frag.leftItem];
		
		Fragment &fr=preCol.set[frag.rightItem];
		int cl(0);
		int cr(0);
		int isize(0);
		if( maxGap==0 )
		{
			isize = frag.iset.size();
			for(int i=0;i<isize;++i)
			{
				cl = UtilityFunc::GetFragmentCountInElementSet(fl.series,emCol,frag.iset[i].start,frag.iset[i].end+frag.series.size()-1);
				cr = UtilityFunc::GetFragmentCountInElementSet(fr.series,emCol,frag.iset[i].start,frag.iset[i].end+frag.series.size()-1);
				cosine = (double)frag.count / sqrt((double)(cl * cr));
				if(cosine < fcosine)
					return false;
			}
		}
		else
		{
			cl = UtilityFunc::GetFragmentCountInElementSet(fl.series,emCol,maxGap);
			cr = UtilityFunc::GetFragmentCountInElementSet(fr.series,emCol,maxGap);
			//cosine(A,B)=P(AUB)/sqrt(P(A) X P(B))
			cosine = (double)frag.count / sqrt((double)(cl * cr)); 
			if(cosine < fcosine)
				return false;
		}
	}
	return true;
}


bool CLayerMining::MinineFragmentPosition(Fragment &frag,ElementSequenceCollection &emCol)
{	
	if(intervalLength == -1 || frag.series.size()==0)
		return true;
	PosLayerArray pslyArray; 
	int *ppos; 
	int *pzero; 
	int allocSize = emCol.seqSet.back().series.size();
	int emSetSize = emCol.seqSet.size(); 
	int layer = 0; 
	bool retRes(false); 
	int supSize =(int)( minSupport*scale*emCol.seqSet.size()); 
	int maxCount(0); 
	
	while(true)
	{
		ppos = new int[allocSize];
		memset(ppos, 0, allocSize*sizeof(int));
		pslyArray.push_back(ppos);
		maxCount = 0;
		if(layer==0)
		{
			pzero = pslyArray[0];
			bool res = false;
			for(int emSetCount=0; emSetCount<emSetSize; ++emSetCount)
			{
				int start(-1);				
				do
				{
					ElementSeries &fragSer = frag.series;
					ElementSeries &ser = emCol.seqSet[emSetCount].series;
					start = UtilityFunc::FindForwardElement(start+1, frag.series[0],ser);
					res = UtilityFunc::SearchSeries(start, fragSer, ser);
					if(res)
					{
						
						ppos[start]+=1;
						if(ppos[start]>maxCount)
							maxCount = ppos[start];
					}
				}while(start!=-1);
			}
		}
		else
		{
			
			int nextpos(0);
			int *pre = pslyArray[layer-1];		 
			for(int i=0;i<allocSize;++i)
			{
				nextpos = i+layer;
				if(nextpos >= allocSize)
				{
					
					ppos[i] = pre[i];
				}
				else
				{
					
					ppos[i] = pre[i] + pzero[nextpos];
				}
				if( ppos[i] >= maxCount )
					maxCount = ppos[i];
			}
		}	
		
		
		if( maxCount >= supSize )
		{
			
			
			for(int j=0;j<allocSize;++j)
			{
				if(ppos[j] == maxCount)
				{
					FragmentInterval fit;
					fit.count = maxCount;
					fit.start = j;
					fit.end = j+layer;
					fit.support = (double)maxCount/(double)emSetSize;
					frag.iset.push_back(fit);
				}
			}
			retRes = true;
			break;
		}
		
		++layer;
		
		if(layer >= this->intervalLength)
		{
			retRes = false;
			break;
		}
		
	}	
	
	int layerSize = pslyArray.size();
	for(int i=0;i<layerSize;++i)
	{
		delete []pslyArray[i];
	}
	return retRes;
}



bool CLayerMining::GenerateRelateRuleSet(RelateRuleSet &rset,ElementSeqIndexArray &unkSet, ElementSequenceCollection &ecol,FragmentSet &fset)
{
	rset.clear();
	unkSet.clear();
	int ecolSize = ecol.seqSet.size();
	int fragSize = 0;
	for(int i=0; i<ecolSize; ++i)
	{
		ElementSeries &sr = ecol.seqSet[i].series;
		FragmentCombineSet cmSet;
		if(UtilityFunc::GenerateFragmentSeriesFromElementSeries(cmSet, fset, sr))
		{
			FragmentSet sigSet;
			UtilityFunc::GenerateFragmentSetFromCombineSet(sigSet,cmSet);
			int index = UtilityFunc::SearchFragmentSetInRelatedRuleSet(sigSet,rset);
			if(index == -1)
			{	
				
				RelatedRule r;
				r.set = sigSet;
				index = rset.size();
				rset.push_back(r);
			}
			RelatedRule &r = rset[index];
			++r.count;
			r.support = (double)r.count/(double)ecolSize;
			r.emSeqIndexArray.push_back(i); 
			fragSize = sigSet.size();
			OffsetSeries osr;
			for(int j=0; j<fragSize;++j){
				osr.push_back(cmSet[j].pos);
			}			
			r.offsetSet.push_back(osr);
		}
		else
			unkSet.push_back(i);
	}
	return true;
}

bool CLayerMining::GetCloseFragmentSet(FragmentSet &closeFragSet, FragmentCollectionArray &fragColArray)
{
	closeFragSet.clear();
	int s = fragColArray.size();	
	for( int i=0; i<s; ++i)
	{
		FragmentSet &fset = fragColArray[i]->set;
		int t = fset.size();
		for(int j=0; j<t; ++j)
		{
			Fragment &f = fset[j];
			if(f.isClosedItem)
				closeFragSet.push_back(f);
		}
	}
	return true;
}

bool CLayerMining::GenerateRuleTree(RuleTreeRoot &root, RelateRuleSet &rset,ElementSeqIndexArray unkSet,int rawFlowSize)
{
	root.unknowEmSeqIndexSet = unkSet;
	root.nodeArray.clear();
	
	int rsetSize = rset.size();
	for(int i=0; i< rsetSize; ++i)
	{
		UtilityFunc::AddRelatedRuleToRuleTree(root, rset[i],rawFlowSize);
	}
	
	int rootChildSize = root.nodeArray.size();
	RuleChildNode tempNode;
	RuleNode *prn;
	for(int i=0; i < rootChildSize; ++i)
	{
		prn = root.nodeArray[i];
		if(prn->support<treePruneThreshold)
		{
			UtilityFunc::AddRawSetBelowNodeToUnk(prn,root.unknowEmSeqIndexSet);
			delete prn;
		}
		else
		{
			tempNode.push_back(prn);
			
			UtilityFunc::PruneRuleTree(root,prn,this->treePruneThreshold,rawFlowSize);
		}
	}
	root.nodeArray = tempNode;
	root.unknowFlowSize = root.unknowEmSeqIndexSet.size();
	root.totalFlowSize = rawFlowSize;
	root.recognizedFlowSize = rawFlowSize - root.unknowFlowSize;
	root.recognizedRate = (double)root.recognizedFlowSize / (double)rawFlowSize;
	return true;
}

bool CLayerMining::GenerateRuleSet(FragUnitArrayColSet &fragUnitArrayColSet, RuleTreeRoot &root)
{
	
	int s = root.nodeArray.size();
	for(int i=0; i<s; ++i)
	{
		FragUnitArrayCol col;		
		UtilityFunc::GenerateRuleDescriptUnit(fragUnitArrayColSet,root.nodeArray[i], col);
	}
	return true;
}



char * UtilityFunc::GetCurrentDate()
{
	time_t t=time(NULL);
	return ctime(&t);
}

bool UtilityFunc::GenerateRuleDescriptUnit(FragUnitArrayColSet &fragUnitArraySet, RuleNode *pr, FragUnitArrayCol fragCol)
{
	FragUnit unit;
	unit = pr->fragUnit;
	
	int curIndex = fragCol.set.size();
	if(curIndex>0 && unit.lowerBound == unit.upperBound && unit.lowerBound == 0)
	{
		ElementSeries &sr = fragCol.set[curIndex-1].fragSr;
		sr.insert(sr.end(), unit.fragSr.begin(), unit.fragSr.end());
	}
	else
	{
		
		fragCol.set.push_back(unit);
	}
	if(pr->isTerminal)
	{
		fragCol.count = pr->terminalCount;
		fragCol.support = pr->terminalSupport;
		fragUnitArraySet.push_back(fragCol);
	}

	int s=pr->childSet.size();
	for(int i=0; i<s; ++i)
	{
		UtilityFunc::GenerateRuleDescriptUnit(fragUnitArraySet,pr->childSet[i], fragCol);
	}
	return true;
}

bool UtilityFunc::PruneRuleTree(RuleTreeRoot &root,RuleNode *pn, double treePruneThreshold,int rawFlowSize)
{
	int s = pn->childSet.size();
	if(!s)
		return true;
	RuleChildNode tempNode;
	RuleNode *tp;
	for(int i=0;i<s;++i)
	{
		tp = pn->childSet[i];
		if(tp->support<treePruneThreshold)
		{
			
			pn->isTerminal = true;
			pn->terminalCount+=tp->count;
			pn->terminalSupport = (double)pn->terminalCount/(double)rawFlowSize;
			AddRawSetBelowNodeToUnk(tp,pn->emSeqIndexSet);
			delete tp;
		}
		else
		{
			tempNode.push_back(tp);
			PruneRuleTree(root,tp,treePruneThreshold,rawFlowSize);
		}
	}
	pn->childSet = tempNode;
	return true;
}


bool UtilityFunc::AddRawSetBelowNodeToUnk(RuleNode *pr,ElementSeqIndexArray &rawflowArray)
{
	if(pr->isTerminal)
		rawflowArray.insert(rawflowArray.end(), pr->emSeqIndexSet.begin(),pr->emSeqIndexSet.end());

	int s=pr->childSet.size();
	for(int i=0;i<s;++i)
	{
		AddRawSetBelowNodeToUnk(pr->childSet[i],rawflowArray);
	}

	return true;
}


bool UtilityFunc::IsIntersect(FragUnit &one, FragUnit &two)
{
	int d = one.lowerBound;
	if(one.lowerBound >= two.lowerBound && one.lowerBound <= two.upperBound
		||
		one.upperBound >= two.lowerBound && one.upperBound <= two.upperBound)
	{
		return true;
	}
	return false;
}

bool UtilityFunc::MergeIntersect(FragUnit &mergeTo, FragUnit &mergeFrom)
{
	if(mergeTo.lowerBound > mergeFrom.lowerBound)
	{
		mergeTo.lowerBound = mergeFrom.lowerBound;
	}
	if(mergeTo.upperBound < mergeFrom.upperBound)
	{
		mergeTo.upperBound = mergeFrom.upperBound;
	}
	return true;
}

int UtilityFunc::SearchChildNode(FragUnit &fu, RuleChildNode &c)
{
	int s = c.size();
	if(!s)
		return -1;
	RuleNode *pr;
	for(int i=0;i<s;++i)
	{
		pr = c[i];
		FragUnit &fu2 = pr->fragUnit;
		if(UtilityFunc::CompareSeries(fu.fragSr,fu2.fragSr)){
			
			if(IsIntersect(fu,fu2)){
				return i;
			}
		}
	}
	return -1;
}

bool UtilityFunc::AddRelatedRuleToRuleTree(RuleTreeRoot &root, RelatedRule &relatedRule,int rawFlowSize)
{
	FragUnit fu;
	UpdateFragUnit(fu,0,relatedRule);
	int index = SearchChildNode(fu, root.nodeArray);
	RuleNode *prn;
	if(index!=-1)
	{
		prn = root.nodeArray[index];
	}
	else
	{
		prn = new RuleNode;
		prn->fragUnit = fu;
		root.nodeArray.push_back(prn);		
	}
	prn->count += RecursiveAddRelatedRuleNode(relatedRule,1,*prn, rawFlowSize);
	prn->support = (double)prn->count/(double)rawFlowSize;
	return true;
}

bool UtilityFunc::UpdateFragUnit(FragUnit &u,int index, RelatedRule &relatedRule)
{
	int min(0),max(0);
	
	OffsetSeriesSet &rarray = relatedRule.offsetSet;		
	int count = rarray.size();						
	int v;

	if(index == 0)
	{
		for(int i=0;i<count;++i)
		{		
			v = rarray[i][index];
			if(i==0)
			{
				min = v;
				max = v;
			}
			else
			{
				if(min > v)
					min = v;
				if(max < v)
					max = v;
			}
		}
	}
	else
	{
		for(int i=0;i<count;++i)
		{		
			v = rarray[i][index]-(rarray[i][index-1]+relatedRule.set[index-1].series.size());
			if(i==0)
			{
				min = v;
				max = v;
			}
			else
			{
				if(min > v)
					min = v;
				if(max < v)
					max = v;
			}
		}
	}
	u.fragSr = relatedRule.set[index].series;
	u.lowerBound=min;
	u.upperBound=max;
	return true;
}

int UtilityFunc::RecursiveAddRelatedRuleNode(RelatedRule &rule, int index, RuleNode &node, int rawFlowSize)
{
	FragmentSet fs = rule.set;
	
	RuleNode *pn;
	if(fs.size() <= (unsigned int)index)
	{
		node.isTerminal = true;
		node.terminalCount = rule.count;
		node.terminalSupport = (double)node.terminalCount/(double)rawFlowSize;
		node.emSeqIndexSet.insert(node.emSeqIndexSet.end(), rule.emSeqIndexArray.begin(),rule.emSeqIndexArray.end());
		return node.terminalCount;
	}
	else
	{		
		FragUnit fu;
		UpdateFragUnit(fu,index,rule);
		int index = SearchChildNode(fu, node.childSet);
		if(index == -1)
		{
			pn = new RuleNode;
			node.childSet.push_back(pn);
		}
		else
		{
			pn = node.childSet[index];
		}	
		pn->fragUnit = fu;
	}
	int deltaCount = RecursiveAddRelatedRuleNode(rule,index+1,*pn, rawFlowSize);
	pn->count += deltaCount;
	pn->support = (double)pn->count/(double)rawFlowSize;
	return deltaCount;
}

bool UtilityFunc::GenerateFragmentSetFromCombineSet(FragmentSet &fset, FragmentCombineSet &cmSet)
{
	int setSize = cmSet.size();
	for(int i=0; i<setSize; ++i)
	{
		fset.push_back(cmSet[i].frag);
	}
	return true;
}

bool UtilityFunc::GenerateFragmentSeriesFromElementSeries(FragmentCombineSet &cmSet,FragmentSet &fset, ElementSeries &ser)
{
	int fsetSize = fset.size();
	ElementSeries tser = ser;
	
	cmSet.clear();
	for(int i=fsetSize-1; i>=0; --i)
	{
		Fragment &frag = fset[i];
		UtilityFunc::UpdateFragmentInInterval(cmSet, frag, tser);		
	}
	if(cmSet.size()>0)
	{
		sort(cmSet.begin(), cmSet.end(), ascendFragmentCombineItem);
		return true;
	}
	return false;
}

bool UtilityFunc::UpdateFragmentInInterval(FragmentCombineSet &cmSet,Fragment &fset,ElementSeries &sr)
{
	
	int iSize = fset.iset.size();
	int start;
	for(int i=0; i<iSize; ++i)
	{
		FragmentInterval iv = fset.iset[i];
		start = UpdateBetweenInterval(cmSet, fset,sr,iv.start,iv.end);
	}
	return true;
}

bool UtilityFunc::UpdateBetweenInterval(FragmentCombineSet &cmSet,Fragment &frag, ElementSeries &sr, int start, int end)
{
	start = start-1;
	int res(false);
	int upSize = frag.series.size();
	ElementSeries &fsr = frag.series;
	while(true)
	{
		start = UtilityFunc::FindForwardElement(start+1,fsr[0],sr);
		if(start == -1 || start > end)
			return true;
		res = UtilityFunc::SearchSeries(start,fsr,sr);
		if(res)
		{
			FragmentCombineItem c;
			c.frag = frag;
			c.pos = start;
			cmSet.push_back(c);
			
			upSize = fsr.size() + start;
			for(int i=start; i<upSize; ++i)
			{
				sr[i] = -1;
			}
		}
	}
	return true;
}

int UtilityFunc::SearchFragmentSetInRelatedRuleSet(FragmentSet &fset,RelateRuleSet &rset)
{
	int rsetSize = rset.size();
	for(int i=0; i < rsetSize; ++i)
	{
		RelatedRule &r = rset[i];

		if(fset.size()!=r.set.size())
			continue;

		int setSize = r.set.size();
		int j(0);
		for(; j < setSize; ++j)
		{
			if(!CompareSeries(fset[j].series, r.set[j].series))
				break;
		}
		if( j == setSize)
		{
			return i;
		}
	}
	return -1;
}

int UtilityFunc::GetFragmentCountInElementSet(const ElementSeries &es, ElementSequenceCollection &emCol,int beginPos, int endPos)
{
	int count(0);
	EmSeriesIt resit;
	EmSeriesIt last1;
	int size = emCol.seqSet.size();  
	for(int i=0 ; i < size; ++i)
	{
		ElementSequence &seq = emCol.seqSet[i];
		EmSeriesIt resit = seq.series.end();
		last1 = seq.series.begin() + endPos +1;
		resit = search(seq.series.begin()+beginPos,seq.series.begin()+endPos+1,es.begin(),es.end());
		if(resit != last1){
				++count;
		}
	}
	return count;
}

int UtilityFunc::GetFragmentCountInElementSet(const ElementSeries &es, ElementSequenceCollection &emCol,int maxGap)
{
	int count(0);
	int size = emCol.seqSet.size();   
	EmSeriesIt resit;
	for(int i=0 ; i < size; ++i)
	{
		ElementSequence &seq = emCol.seqSet[i];
		EmSeriesIt resit = seq.series.end();
		if(maxGap == 0)
		{
			resit = search(seq.series.begin(),seq.series.end(), es.begin(), es.end());
			if(resit != seq.series.end()){
				++count;
			}
		}
		else
		{
			
			int start(0);
			start = FindForwardElement(start, es[0],seq.series);
			bool res(false);
			while(start != -1)
			{
				res = FindSeqWithGap(start, es, seq.series, maxGap);
				if(res)
				{
					++count;
					break;
				}
				start = FindForwardElement(start+1, es[0],seq.series);
			}
		}
	}
	return count;	
}

int UtilityFunc::FindForwardElement(int start, int v,const ElementSeries &es)
{
	int len = es.size();
	if(start>=len)
		return -1;
	for(int i=start; i<len; ++i)
	{
		if(es[i] == v)
			return (i);
	}
	return -1;
}

int UtilityFunc::FindBackwardElement(int start,int scale, int v,const ElementSeries &es)
{
	int i=start-scale;
	if(i<0)
		i = 0;
	for(;i<=start;++i)
	{
		if(es[i] == v)
			return (i);
	}
	return -1;
}

bool UtilityFunc::SearchSeries(int start, const ElementSeries &frag, const ElementSeries &es)
{
	
	if(start == -1)
		return false;
	
	
	int fragLen = frag.size();
	int esLen = es.size();
	int esLeftLen = esLen-start;
	if(esLeftLen < fragLen)
		return false;

	
	for(int i=0;i<fragLen; ++i)
	{
		if(frag[i] != es[i+start])
		{
			return false;
		}
	}
	return true;
}

bool UtilityFunc::FindSeqWithGap(int start,const ElementSeries &fragSr, const ElementSeries &emSr,int maxGap)
{
	int start_pos(start);
	int end_pos(start);
	bool forward=true;
	int esSize=fragSr.size();
	int sPos = 1;

	
	int o_end_Pos = 0;
	int o_sPos = 0;
	while(sPos!=esSize)
	{
		if(forward)
		{			
			
			end_pos = FindForwardElement(end_pos+1,fragSr[sPos],emSr);
			if(end_pos == -1)
				break;			
			if(end_pos-start_pos-1<=maxGap)
			{
				++sPos;
				start_pos = end_pos;
			}
			else
			{
				forward = false;
				o_sPos = sPos;
				o_end_Pos = end_pos;
				--sPos;
			}
		}
		else
		{
			start_pos = FindBackwardElement(end_pos-1,maxGap,fragSr[sPos],emSr);
			if(start_pos == -1)
			{
				return false;
			}
			else
			{
				--sPos;
				if(sPos<0)
				{
					
					
					
					forward = true;
					start_pos = o_end_Pos;
					end_pos = o_end_Pos;
					sPos = ++o_sPos;                                                                                                                                                                                                                                               					
				}
				else
				{  
					end_pos = start_pos;
				}
			}
		}
	}
	if(end_pos == -1)
		return false;
	return true;
}

void UtilityFunc::GenerateVisiableString(string &outStr, const ElementSeries &sr)
{
	char hexStr[6];
	int len = sr.size();
	outStr.clear();
	int v(0);
	for(int i=0; i<len ;++i)
	{
		if(i > 0)
				outStr += " "; 
		v = sr[i];
		if( v > 0x20 && v< 0x7F) 
		{			
			outStr += (char)v;
		}
		else if(v == 0x09)
		{
			outStr += "[tab]";
		}
		else if(v == 0x20)
		{
			outStr += "[sp]";
		}
		else if(v == 0x0A)
		{
			outStr += "[nl]";
		}
		else if(v == 0x0D)
		{
			outStr += "[er]";
		}
		else 
		{
			outStr += "0x";
			itoa(v,hexStr,16);
			if(hexStr[1]==0)
			{
				outStr +="0";
			}
			outStr += hexStr;
		}
	}
}

bool UtilityFunc::CompareSeries(ElementSeries &one, ElementSeries &two)
{
	if(one.size()!=two.size())
		return false;
	int s = one.size();
	for(int i=0; i < s; ++i)
	{
		if(one[i] != two[i])
			return false;
	}
	return true;
}

bool UtilityFunc::GenerateElementSeqenceFromUnknowSet(ElementSequenceCollection &newEmCol,
		ElementSequenceCollection &oldEmCol,
		ElementSeqIndexArray &emIndexSet)
{
	newEmCol.seqSet.clear();
	newEmCol.pMapTable = oldEmCol.pMapTable;
	int len = emIndexSet.size();
	int index(0);
	for(int i=0; i<len; ++i)
	{
		index = emIndexSet[i];
		newEmCol.seqSet.push_back(oldEmCol.seqSet[index]);
	}
	return true;
}

bool UtilityFunc::GetElementSeries(ElementSeries &destSr, int startPos, int size, ElementSeries &orgSr)
{
	int orgSize = orgSr.size();
	int endPos = startPos+size;
	destSr.clear();
	if(startPos>=orgSize || endPos >orgSize)
		return false;
	destSr.insert(destSr.begin(),orgSr.begin()+startPos,orgSr.begin()+endPos);
	return true;
}

bool UtilityFunc::IsAllZero(const ElementSeries &sr)
{
	int s = sr.size();
	for(int i=0;i<s;++i)
	{
		if(sr[i])
			return false;
	}
	return true;
}

void AuditSequence::ToMatLabType(string &str)
{
	char tstr[128];
	str.clear();
	
	int len = this->winSet.size();
	for(int i=0;i<len;++i)
	{
		sprintf(tstr,"%d",winSet[i].startPos);
		str += tstr;
		str +=" ";
	}
	str += "\n";
	
	for(int i=0;i<len;++i)
	{
		sprintf(tstr,"%f",this->winSet[i].maxFreq);
		str +=tstr;
		str +=" ";
	}
	str += "\n";
	
	for(int i=0;i<len;++i)
	{
		sprintf(tstr,"%f",this->winSet[i].midFreq);
		str +=tstr;
		str +=" ";
	}
	str += "\n";
	
	for(int i=0; i<len; ++i)
	{
		sprintf(tstr,"%f",this->winSet[i].minFreq);
		str +=tstr;
		str +=" ";
	}
	str += "\n";
	
	for(int i=0; i<len; ++i)
	{
		sprintf(tstr,"%f",this->winSet[i].averageFreq);
		str +=tstr;
		str +=" ";
	}
	str += "\n";
	
	for(int i=0; i<len; ++i)
	{
		sprintf(tstr,"%f",this->winSet[i].freqStandDeviation);
		str +=tstr;
		str +=" ";
	}
	str +="\n";
	string outStr;
	for(int i=0; i<len; ++i)
	{		
		sprintf(tstr,"@%d: ",i);
		str +=tstr;
		UtilityFunc::GenerateVisiableString(outStr,winSet[i].maxSer);
		str +=outStr;
	}
	str +="\n";
}


bool CExecutor::ExecMine(ElementSequenceCollection &nextCol, 
	FragUnitArrayColSet &fragUnitArraySet,
	ElementSequenceCollection &curEmCol)
{
	nextCol.seqSet.clear();
	nextCol.pMapTable = 0;
	fragUnitArraySet.clear();

	bool res = false;
	FragmentCollection *preFragCol = 0;
	int len = 1;
	FragmentCollectionArray fragColArray;
	while(true)
	{
		
		printf("Is executing %d layer mining...\n", len);
		FragmentCollection *fragCol=new FragmentCollection();
		res = pmine->GenerateFrequentFragmentSet(*fragCol,curEmCol,preFragCol,len,0);
		preFragCol = fragCol;
		if(!res)
		{
			printf("Mining is finished at %d layer.\n", len);
			if(len == 0)
				return false;
			break;
		}
		else
		{
			++len;
			fragColArray.push_back(fragCol);
		}
	}
	
	FragmentSet closeSet;
	pmine->GetCloseFragmentSet(closeSet, fragColArray);

	
	RelateRuleSet ruleSet;
	ElementSeqIndexArray rfarray;
	pmine->GenerateRelateRuleSet(ruleSet,rfarray, curEmCol, closeSet);


	int ruleLen = ruleSet.size();
	string srStr;
	for(int i=0;i<ruleLen; ++i)
	{
		writedeflog("#####Signatures####\n");
		int fragLen = ruleSet[i].set.size();
		for(int j=0;j<fragLen;++j)
		{
			UtilityFunc::GenerateVisiableString(srStr,ruleSet[i].set[j].series);
			writedeflog("%s |",srStr.c_str());
		}
		writedeflog(" @ support: %f; count: %d\n",ruleSet[i].support,ruleSet[i].count);

	}
	writedeflog("**unkcount: %d\n",rfarray.size());
	
	
	RuleTreeRoot treeRoot;
	pmine->GenerateRuleTree(treeRoot, ruleSet,rfarray, curEmCol.seqSet.size());
	
	
	pmine->GenerateRuleSet(fragUnitArraySet,treeRoot);
	writedeflog("info total:%d; recognized size:%d, recognized rate:%f, unknow size:%d\n",treeRoot.totalFlowSize,
		treeRoot.recognizedFlowSize,treeRoot.recognizedRate,
		treeRoot.unknowFlowSize);

	if(	treeRoot.recognizedRate<0.382)
		return false;
	
	UtilityFunc::GenerateElementSeqenceFromUnknowSet(nextCol,curEmCol,treeRoot.unknowEmSeqIndexSet);

	return true;
}

typedef vector<FragUnitArrayColSet> FragUnitArrayColSetArray;

void CExecutor::Run()
{
	
	initSystem();
	pmine = new CLayerMining;
	
	pmine->fcosine = 0.618;
	pmine->scale = 0.8;
	pmine->minSupport = 0.7;
	pmine->treePruneThreshold = 0.382;
	pmine->intervalLength = 5;
	pmine->reserveRation = 0.8;
	pmine->maxReserveLength = 1000;

	
	ElementSequenceCollection emCol;
	ElementSequenceCollection newEmCol;

	pmine->CreateOriginalNetflowSet(col,"E:\\temp\\packet_col\\xunlei\\xunlei_02_16_1645\\wan_to_lan","");
	pmine->SetAlignLength(col);
	printf("align length: %d, total file: %d\n",col.reserveLen, col.fileSet.size());
	
	pmine->CreateElemSeqColFromRawCol(emCol,col);
	
	
	printf("element size: %d\n",emCol.seqSet.size());
	
	double endRate = 0.1;
	int sumSize = emCol.seqSet.size();

	bool res(false);
	int times(0);
	FragUnitArrayColSetArray farray;
	while(true)
	{

		AuditSequence ausq;
		clock_t t = clock();
		pmine->GenerateMaxAuditSequence(ausq,emCol,100,3);
		t = clock() - t;
		printf("GenerateMaxAuditSequence cost time %f seconds\n",(double)t/(double)CLOCKS_PER_SEC);
		string matlabStr;
		ausq.ToMatLabType(matlabStr);
		writelog("auditMatrix.txt","%s",matlabStr.c_str());
		closelog("auditMatrix.txt");
		pmine->minSupport = ausq.lineSupport;

		writedeflog("Mining at support: %f\n",pmine->minSupport);
		printf("Begin mining...at support is:%f\n",pmine->minSupport);
		if(pmine->minSupport < 0.1)
			break;
		FragUnitArrayColSet fragUnitArraySet;
		res = ExecMine(newEmCol, fragUnitArraySet,emCol);
		if(res)
		{
			farray.push_back(fragUnitArraySet);
			emCol = newEmCol;
			++times;
			int newEmColSize = newEmCol.seqSet.size();
			double newEndRate = (double)newEmColSize / (double)sumSize;
			printf("Left ratio: %f\n",newEndRate);


			for(int i=0;i<newEmColSize; ++i)
			{
				writedeflog("#%d: %s\n",i+1, col.fileSet[newEmCol.seqSet[i].rawFlowId].filename.c_str());
			}


			if(newEndRate < endRate)
			{
				break;
			}
		}
		else
		{
			if(times == 0)
				printf("Cannot find signature\n");
			printf("End mining.\n");
			break;
		}
	}

	for(int i=0; i<farray.size();++i)
	{
		writedeflog("$$$$ Level %d $$$$\n",i);
		for(int j=0; j<farray[i].size(); ++j)
		{
			writedeflog("%%%%%%%%%%signature%%%%%%%%%%\n");
			for(int k=0; k<farray[i][j].set.size(); ++k)
			{
				FragUnit &u = farray[i][j].set[k];
				string str;
				UtilityFunc::GenerateVisiableString(str,u.fragSr);
				writedeflog("(%d,%d) %s | ",u.lowerBound,u.upperBound,str.c_str());
			}
			writedeflog(" support: %f; count: %d\n",farray[i][j].support,farray[i][j].count);
		}
	}


	delete pmine;
	destroySystem();
	
	
	
	
}

