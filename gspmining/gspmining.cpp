#include "gspmining.h"
#include "gspsystem.h"
#include "fragment.h"
#include "rule.h"
#include "layer.h"
#include "signature.h"

bool CGspMining::_MiningAtSupport(int supIndex,SignatureCollection &sc)
{
	
	int logicLength(0);
	if(!_bForceSupport)
	{
		logicLength = _pAdSeq->CalcIntervalLength(_pAdSeq->_supportVector[supIndex]);
		printf("@support %f,length is %d.\n",_pAdSeq->_supportVector[supIndex],logicLength);
	}
	else
	{
		logicLength = this->_pEmSeqCol->GetMaxElementSequenceLength();
	}
	
	_pAllCol->ClearAll();
	if(!_bForceSupport)
		_pAllCol->_minSupport=_pAdSeq->_supportVector[supIndex];
	if(!_pAllCol->GenerateAllLevel(_pEmSeqCol,logicLength))
	{
		printf("Generator cannot get close fragment this round\n");
		return false;
	}	
	CReport::PrintAllLevelFragment(this);	
	RelatedRuleCollection rrc;
	printf(".Gen rule Begin.");
	rrc.GenerateRelatedRuleCol(*_pAllCol, *_pEmSeqCol);
	CReport::PrintRelatedRule(this,&rrc);
	printf(".rule:%d; unk:%d.End.\n",rrc._frmap.size(),rrc._unkEmVec.size());		

	
	double factor;
	if( _bForceSupport || _pAdSeq->_supportVector.size()-1 == supIndex )
		factor =  _pAdSeq->_supportVector[supIndex]*FACTOR_SCALE;
	else
	{
		factor = 
			_pAdSeq->_supportVector[supIndex+1]+
			(_pAdSeq->_supportVector[supIndex]-_pAdSeq->_supportVector[supIndex+1])/2.0;
	}

	
	printf(".Gen layer signature Begin.");
	LayerRoot layerClass;
	layerClass.InitLayerRoot(*_pEmSeqCol,rrc,LAYER_CLASS);
	printf(".");
	SignatureCollection layerSigCol;
	layerSigCol.InitFromLayerClass(layerClass,factor);	
	layerSigCol.Update(*_pEmSeqCol);
	sc.Merge(layerSigCol);
	printf(".rule:%d.End.\n",layerSigCol._set.size());	

	
	printf(".Gen forest signature Begin.");
	RuleForest forest;
	forest.InitRuleForest(*_pEmSeqCol,rrc);
	printf(".");
	SignatureCollection treeSigCol;	
	treeSigCol.InitFromRuleForest(forest,factor);	
	treeSigCol.Update(*_pEmSeqCol);
	printf("Tree factor = %f\n",factor);
	if(_bLog)
	{
		string logstr;		
		layerSigCol.ToString(logstr);
		CGspSystem::WriteLog(this->_miningLog.c_str(), "%s\n",logstr.c_str());
		treeSigCol.ToString(logstr);
		CGspSystem::WriteLog(this->_miningLog.c_str(), "%s\n",logstr.c_str());
	}
	sc.Merge(treeSigCol);
	printf(".rule:%d.End.\n",treeSigCol._set.size());

	
	printf("Merged rule: %d\n",sc._set.size());

	
	
	/**

	struct xxx
	{
	   支持度
	   是否最优支持度
	   合并后的特诊集合
	   然后从中把最优支持度的选出，之后把不是最优支持度的里面最高识别率的选出
	}

	*/
	return true;
}

int CGspMining::_Mining(IntVector &unk,SignatureCollection &sc)
{
	
	if(_bForceSupport)
	{
		_MiningAtSupport(-1,sc);
		if(!sc._set.size())
			return 0;
		sc.Update(*_pEmSeqCol);
		unk = sc._unkSet;
		return _pEmSeqCol->_size - unk.size();
	}
	
	printf("Begin Generate Audit Sequence, half window size %d, shingle size %d...\n",
		_pAdSeq->_halfWindowSize,_pAdSeq->_shingleSize);
	_cpuTime.Begin();
#ifdef ENABLE_WINDOW_SIZE_TEST
	printf("Begin Testing window...\n");
	int hw = _pAdSeq->_halfWindowSize;
	size_t ws=1;
	for(ws=2;ws<1000;ws+=10)
	{
		printf("HF=%d ",ws);
		_pAdSeq->_halfWindowSize = (int)ws;
		if(!_pAdSeq->GenerateAuditSequence(*_pEmSeqCol))
		{
			printf("*Error: Generate Audit Sequence error.\n");
			break;
		}
		if(!_pAdSeq->CalcSupport())
		{
			break;
		}
	}
	CReport::PrintAllHalfWindowSizeSupport(this);
	printf("End Testing window. Final halfwindow = %d\n",ws);
	_pAdSeq->_halfWindowSize = hw;
#endif
#ifdef ENABLE_SHINGLE_SIZE_TEST
	printf("Begin Testing shingle...\n");
	int ss = _pAdSeq->_shingleSize;
	size_t s=1;
	for(s = 1;s <= 15;++s)
	{
		printf("shingle Size=%d ",s);
		_pAdSeq->_shingleSize = (int)s;
		if(!_pAdSeq->GenerateAuditSequence(*_pEmSeqCol))
		{
			printf("*Error: Generate Audit Sequence error.\n");
			break;
		}
		if(!_pAdSeq->CalcSupport())
		{
			printf("Below threshold\n");
		}
	}
	CReport::PrintAllShingleSizeSupport(this);
	printf("End Testing shingle. Final shingle = %d\n",s);
	_pAdSeq->_shingleSize = ss;
#endif
	if(!_pAdSeq->GenerateAuditSequence(*_pEmSeqCol))
	{
		printf("*Error: Generate Audit Sequence error.\n");
		return 0;
	}
	if(!_pAdSeq->CalcSupport())
	{
		printf("*Error: not specific signature, line support is %f.\n", _pAdSeq->_lineSupport);
		CReport::PrintAuditInfoInCGspMining(this);
		if(_bLog)
		{
			CGspSystem::WriteLog(this->_miningLog.c_str(), 
				"No signature. Cannot get support which greater than %f.\n", _pAdSeq->_minLineSupport);
		}
		return 0;
	}
	_cpuTime.End();	
	
	CReport::PrintAuditInfoInCGspMining(this);
	
	printf("Begin generate pos table...\n");
	_cpuTime.Begin();
	_pEmSeqCol->InitPosTable();
	_cpuTime.End();
	printf("Generate %d pos table costs %.2f seconds.\n",_pEmSeqCol->_size, _cpuTime.GetDurationSeconds());

	
	_cpuTime.Begin();
	SpecSigSet specSigSet;
	for(int i=0; i<(int)_pAdSeq->_supportVector.size(); ++i)
	{
		SpecificSignature specSig;		
		_MiningAtSupport(i,specSig._scol);
		if(specSig._scol._set.size()==0)
			continue;
		else
		{
			if(i == _pAdSeq->_supportVector.size()-1)
			{
				specSig._isBestSupport = true;
			}
			specSig._support = _pAdSeq->_supportVector[i];
			specSigSet.push_back(specSig);
		}
	}
	if(!specSigSet.size())
		return 0;
	printf("Update pattern regex Begin...");
	double maxSup=-1;
	int maxIndex=-1;
	for(size_t i=0; i < specSigSet.size(); ++i)
	{
		
		specSigSet[i]._scol.Update(*_pEmSeqCol);
		if(specSigSet[i]._scol._identifyRate>maxSup)
		{
			
			maxSup = specSigSet[i]._scol._identifyRate;
			maxIndex = i;			
		}
	}
	
	sc.Merge(specSigSet[specSigSet.size()-1]._scol);
	
	if(!specSigSet[maxIndex]._isBestSupport)
		sc.Merge(specSigSet[maxIndex]._scol);
	
	if(!sc._set.size())
		return 0;
	sc.Update(*_pEmSeqCol);
	unk = sc._unkSet;
	printf("End\n");
	_cpuTime.End();
	return _pEmSeqCol->_size - unk.size();
}


int CGspMining::_FirstMining(IntVector &unk,SignatureCollection &sc)
{
	return _Mining(unk,sc);
}

int CGspMining::_FollowMinging(IntVector &unk,SignatureCollection &sc)
{
	
	_pEmSeqCol->ReloadEmSeqFromUnkSet(unk);
	return _Mining(unk,sc);
}

bool CGspMining::Init(const char* miningName,PayloadFileCollection *pPfCol, ElementSequenceCol *psecol,
	AuditSequence *pAdSeq,FragAllLevCol *pAllCol,bool genReport)
{
	_time.Begin();

	if(pAllCol->_minSupport<0)
		_bForceSupport = false;
	else
		_bForceSupport = true;

	if(!miningName || !pPfCol || !psecol ||!pAdSeq)
		return false;
	_miningName = miningName;
	_pPfCol = pPfCol;
	_pEmSeqCol = psecol;
	_pAdSeq = pAdSeq;
	_pAllCol = pAllCol;
	this->_bLog = genReport;
	if(_bLog)
	{
		this->_miningLog = miningName;
		this->_miningLog += "_";
		string time;
		this->_miningLog += CClockTime::GetUnderlineDateAndTime(time);
		this->_miningLog += ".txt";		
	}
	CReport::PrintCGspMiningInit(this);
	return true;
}


bool CGspMining::execute()
{	
#ifdef ENABLE_POS_OFFSET_SCALE_COSINE_TEST
	return _evalue_execute();
#else
	int res = _pEmSeqCol->ReLoadEmSeqFromFileSet(*_pPfCol);
	if(!res)
	{
		printf("*Error: The Element Sequence Colleciton is empty after the file loading at CGspMining::execute.\n");
		return false;
	}
	double minRate = 0.1;
	int allSize = _pEmSeqCol->_size;
	IntVector unk;
	SignatureCollection firstSC;
	int rogSize = _FirstMining(unk,firstSC);
	double rate = (double)rogSize/(double)allSize;
	if(rate < minRate)
	{
		
		printf("Cannot find signature as first mining rate %f\n",rate);
		return false;
	}
	else
	{
		_sc.Merge(firstSC);
		firstSC.ClearAll();
	}
	if(!_bForceSupport)
	{
		while(rate >= minRate)
		{
			minRate = TERMINATE_RATE;
			SignatureCollection sc;
			rogSize = _FollowMinging(unk,sc);
			rate = (double)rogSize/(double)_pEmSeqCol->_size;
			if(rate >= minRate)
			{
				_sc.Merge(sc);
			}
		}
		printf("Final validation...\n");
		_pEmSeqCol->ReLoadEmSeqFromFileSet(*_pPfCol);
	}
	else
	{
		printf("Final validation...\n");
	}
	
	_sc.Update(*_pEmSeqCol);
	_sc.HeuristicCut();
	_sc.Update(*_pEmSeqCol);
	if(_bLog)
	{
		string strLog;
		CGspSystem::WriteLog(this->_miningLog.c_str(),_sc.ToString(strLog));
		CGspSystem::WriteLog(this->_miningLog.c_str(),_sc.ToRegexString(strLog,this->_miningName));
	}
	return true;
#endif
}



bool CGspMining::Destroy(){
	_time.End();
	if(_bLog)
	{
		string time;
		CGspSystem::WriteLog(this->_miningLog.c_str(),
		"#Mining stopped at %s\n#Total time costs %f seconds.\n",
		CClockTime::GetReadAbleDateAndTime(time), _time.GetDurationSeconds());
		CGspSystem::CloseLog(this->_miningLog.c_str());
	}
	return true;
}

bool CGspMining::_evalue_execute()
{
	int res = _pEmSeqCol->ReLoadEmSeqFromFileSet(*_pPfCol);
	if(!res)
	{
		printf("*Error: The Element Sequence Colleciton is empty after the file loading at CGspMining::execute.\n");
		return false;
	}
	if(!_pAdSeq->GenerateAuditSequence(*_pEmSeqCol))
	{
		printf("*Error: Generate Audit Sequence error.\n");
		return 0;
	}
	if(!_pAdSeq->CalcSupport())
	{
		printf("*Error: not specific signature, line support is %f.\n", _pAdSeq->_lineSupport);
		CReport::PrintAuditInfoInCGspMining(this);
		if(_bLog)
		{
			CGspSystem::WriteLog(this->_miningLog.c_str(), 
				"No signature. Cannot get support which greater than %f.\n", _pAdSeq->_minLineSupport);
		}
		return 0;
	}	
	_pEmSeqCol->InitPosTable();
	SignatureCollection firstSC;
	int logicLength = this->_pEmSeqCol->GetMaxElementSequenceLength();
	
	_pAllCol->_minSupport = _pAdSeq->_lineSupport;
	_pAllCol->_logicLength = _pAdSeq->CalcIntervalLength(_pAdSeq->_lineSupport);
	
	int oldMaxOffset = _pAllCol->_maxOffset;
	IntVector ivOffset;
	IntVector ivSize;
	DoubleVector dbAverageLen;
	for(int maxOffsetTest=0;maxOffsetTest<51;maxOffsetTest+=5)
	{
		printf("maxOffsetTest:%d\n",maxOffsetTest);
		ivOffset.push_back(maxOffsetTest);
		_pAllCol->ClearAll();	
		_pAllCol->_maxOffset = maxOffsetTest;
		if(!_pAllCol->GenerateAllLevel(_pEmSeqCol,logicLength))
		{
			ivSize.push_back(0);
			dbAverageLen.push_back(0);
		}
		else
		{
			ivSize.push_back(_pAllCol->getCloseFragSetSize());
			dbAverageLen.push_back(_pAllCol->getAverageCloseFragSize());
		}
	}	
	
	string logFileName = this->_miningLog.c_str();
	CGspSystem::WriteLog(logFileName.c_str(), "######Matlab position offset evalue:offset; total count; average length\n");
	for(int i=0;i<(int)ivOffset.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%d ",ivOffset[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	for(int i=0;i<(int)ivSize.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%d ",ivSize[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	for(int i=0;i<(int)dbAverageLen.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%f ",dbAverageLen[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	CGspSystem::WriteLog(logFileName.c_str(), "######~Matlab position offset evalue End\n");

	
	_pAllCol->_maxOffset = oldMaxOffset;

	
	DoubleVector dbVec;
	ivSize.clear();
	dbAverageLen.clear();
	double oldScale=_pAllCol->_scale;
	for(int i=1;i<11;i+=1)
	{
		double scale = i*0.1;
		printf("scale:%f\n",scale);
		dbVec.push_back(scale);
		_pAllCol->ClearAll();	
		_pAllCol->_scale = scale;
		if(!_pAllCol->GenerateAllLevel(_pEmSeqCol,logicLength))
		{
			ivSize.push_back(0);
			dbAverageLen.push_back(0);
		}
		else
		{
			ivSize.push_back(_pAllCol->getCloseFragSetSize());
			dbAverageLen.push_back(_pAllCol->getAverageCloseFragSize());
		}
	}
	
	CGspSystem::WriteLog(logFileName.c_str(), "######Matlab position scale support evalue:scale; total count; average length\n");
	for(int i=0;i<(int)dbVec.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%f ",dbVec[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	for(int i=0;i<(int)ivSize.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%d ",ivSize[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	for(int i=0;i<(int)dbAverageLen.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%f ",dbAverageLen[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	CGspSystem::WriteLog(logFileName.c_str(), "######~Matlab position scale support evalue End\n");
	
	_pAllCol->_scale = oldScale;

	
	dbVec.clear();
	ivSize.clear();
	dbAverageLen.clear();
	double oldcos=_pAllCol->_cosine;
	for(int i=0;i<11;i+=1)
	{
		double cos=i*0.1;
		printf("cos:%f\n",cos);
		dbVec.push_back(cos);
		_pAllCol->ClearAll();	
		_pAllCol->_cosine = cos;
		if(!_pAllCol->GenerateAllLevel(_pEmSeqCol,logicLength))
		{
			ivSize.push_back(0);
			dbAverageLen.push_back(0);
		}
		else
		{
			ivSize.push_back(_pAllCol->getCloseFragSetSize());
			dbAverageLen.push_back(_pAllCol->getAverageCloseFragSize());
		}
	}
	
	CGspSystem::WriteLog(logFileName.c_str(), "######Matlab cosine evalue:cosine; total count; average length\n");
	for(int i=0;i<(int)dbVec.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%f ",dbVec[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	for(int i=0;i<(int)ivSize.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%d ",ivSize[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	for(int i=0;i<(int)dbAverageLen.size(); ++i)
	{
		CGspSystem::WriteLog(logFileName.c_str(),"%f ",dbAverageLen[i]);
	}
	CGspSystem::WriteLog(logFileName.c_str(),"\n");
	CGspSystem::WriteLog(logFileName.c_str(), "######~Matlab cosine evalue End\n");
	
	return true;
}
