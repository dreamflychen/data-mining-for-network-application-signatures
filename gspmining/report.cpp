#include "report.h"
#include "gspmining.h"
#include "rule.h"

void CReport::PrintCGspMiningInit(CGspMining *pGspMining)
{
	string str;
	printf("Gsp Mining Initial parameters:\n");
	printf("\t Tag: %s; Half Window Size: %d; Shingle Size: %d;\n\t Minimum Support Threshold: %f\n",
		pGspMining->_miningName.c_str(),pGspMining->_pAdSeq->_halfWindowSize,
		pGspMining->_pAdSeq->_shingleSize, pGspMining->_pAdSeq->_minLineSupport);
	printf("\t Is self-adaption mode? ");
	if(pGspMining->_pAllCol->_minSupport < 0)
		printf("Yes.\n");
	else
	{
		printf("No. Minimum Support: %f.\n", pGspMining->_pAllCol->_minSupport);
	}
	printf("\t Cosine: %f; Scale: %f; Offset: %d\n", 
		pGspMining->_pAllCol->_cosine, pGspMining->_pAllCol->_scale,
		pGspMining->_pAllCol->_maxOffset);
	if(pGspMining->_bLog)
	{	
		printf("\t Log enable. Log file name: %s\n",pGspMining->_miningLog.c_str());

		string str;
		string filename = pGspMining->_miningLog.c_str();
		CGspSystem::WriteLog(filename.c_str(), 
			"#Powered by Wynter Han\n#Date time: %s", 
			CClockTime::GetReadAbleDateAndTime(str));

		CGspSystem::WriteLog(filename.c_str(),"Gsp Mining Initial parameters:\n");
		CGspSystem::WriteLog(filename.c_str(),
			"\t Tag: %s; Half Window Size: %d; Shingle Size: %d; Minimum Support Threshold: %f\n",
			pGspMining->_miningName.c_str(),pGspMining->_pAdSeq->_halfWindowSize,
			pGspMining->_pAdSeq->_shingleSize, pGspMining->_pAdSeq->_minLineSupport);
		CGspSystem::WriteLog(filename.c_str(),"\t Is self-adaption mode? ");
		if(pGspMining->_pAllCol->_minSupport < 0)
			CGspSystem::WriteLog(filename.c_str(),"Yes.\n");
		else
		{
			CGspSystem::WriteLog(filename.c_str(),"No. Minimum Support: %f.\n", pGspMining->_pAllCol->_minSupport);
		}
		CGspSystem::WriteLog(filename.c_str(),
			"\t Cosine: %f; Scale: %f; Offset: %d\n", 
			pGspMining->_pAllCol->_cosine, pGspMining->_pAllCol->_scale,
			pGspMining->_pAllCol->_maxOffset);
	}

}

void CReport::PrintAuditInfoInCGspMining(CGspMining *pGspMining)
{
	printf("Audit %d windows generation costs %.2f seconds.\n",
		pGspMining->_pAdSeq->_winSet.size(),pGspMining->_cpuTime.GetDurationSeconds());
	printf("All support(%d): ",pGspMining->_pAdSeq->_supportVector.size());
	
	{
		if(i == pGspMining->_pAdSeq->_supportVector.size()-1)
		{
			printf("*%f*\n",pGspMining->_pAdSeq->_supportVector[i]);
		}
		else
			printf("%f ",pGspMining->_pAdSeq->_supportVector[i]);
	}

	if(pGspMining->_bLog)
	{
		string logstr;
		pGspMining->_pAdSeq->ToSummaryString(logstr);
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(), "%s\n",logstr.c_str());
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(),"#Matlab matrix(Index, StartPos, FlowSize, KindSize, MaxFreq, MinFreq, lineSupport, Support Array)\n");
		pGspMining->_pAdSeq->ToMatLabTxtMatrix(logstr);
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(),"%s\n",logstr.c_str());
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(),"#~End matrix\n");
	}
}

void CReport::PrintAllLevelFragment(CGspMining *pGspMining)
{
	if(pGspMining->_bLog)
	{
		string logstr;		
		pGspMining->_pAllCol->ToString(logstr);
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(), "%s\n",logstr.c_str());
	}
}

void CReport::PrintRelatedRule(CGspMining *pGspMining,RelatedRuleCollection *_prr)
{
	RelatedRuleCollection &rr = *_prr;
	if(pGspMining->_bLog)
	{
		string logstr;		
		rr.ToString(logstr);
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(), "######RelatedRuleCollection\n");
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(), "%s\n",logstr.c_str());
		CGspSystem::WriteLog(pGspMining->_miningLog.c_str(), "######~End\n");
	}
	
}


void CReport::PrintAllHalfWindowSizeSupport(CGspMining *pGspMining)
{
#ifdef ENABLE_WINDOW_SIZE_TEST
	if(pGspMining->_bLog)
	{
		string logstr;		
		string logFileName = pGspMining->_miningLog.c_str();
		CGspSystem::WriteLog(logFileName.c_str(), "######Window Support Vector: half window, min support\n");
		size_t size = pGspMining->_pAdSeq->_windowSupportVector.size();
		for(size_t i=0; i<size; ++i)
		{
			CGspSystem::WriteLog(logFileName.c_str(),"%d ",2+i*10);
		}
		CGspSystem::WriteLog(logFileName.c_str(), "\n");
		for(size_t i = 0; i < pGspMining->_pAdSeq->_windowSupportVector.size(); ++i)
		{
			CGspSystem::WriteLog(logFileName.c_str(), "%f ",pGspMining->_pAdSeq->_windowSupportVector[i]);
		}				
		CGspSystem::WriteLog(logFileName.c_str(), "\n######~End\n");
		CGspSystem::FlushAllFile();
	}
#endif
}

void CReport::PrintAllShingleSizeSupport(CGspMining *pGspMining)
{
#ifdef ENABLE_SHINGLE_SIZE_TEST
	if(pGspMining->_bLog)
	{
		string logstr;		
		string logFileName = pGspMining->_miningLog.c_str();
		CGspSystem::WriteLog(logFileName.c_str(), "######Shingle Support Vector: shingle size, min support\n");
		size_t size = pGspMining->_pAdSeq->_windowSupportVector.size();
		for(size_t i=0; i<size; ++i)
		{
			CGspSystem::WriteLog(logFileName.c_str(),"%d ",1+i);
		}
		CGspSystem::WriteLog(logFileName.c_str(), "\n");
		for(size_t i = 0; i < pGspMining->_pAdSeq->_windowSupportVector.size(); ++i)
		{
			CGspSystem::WriteLog(logFileName.c_str(), "%f ",pGspMining->_pAdSeq->_windowSupportVector[i]);
		}				
		CGspSystem::WriteLog(logFileName.c_str(), "\n######~End\n");
		CGspSystem::FlushAllFile();
	}
#endif
}