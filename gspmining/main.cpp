#include <omp.h>

#include "gspmheader.h"
#include <unordered_map>
int main(int argc, char** argv)
{

	
	if(!opt.GenInputMgr(argc,argv))
		return false;
	CGspSystem::InitSystem();
	if(opt._c == 0 || opt._c == 1)
	{
		omp_set_num_threads(opt._t);
		PayloadFileCollection *pyCol;
		if(opt._c == 0)
		{
			pyCol = new PayloadFileCollection();
		}
		else
		{
			pyCol = new PayloadPackFileCollection();
		}
		if(!pyCol->LoadCollectionFromPath(opt._s.c_str(),opt._k.size()>0?opt._k.c_str():0))
		{
			opt.GenShowUsage();
			printf("Loading file path %s failed!",opt._s.c_str());
			if(opt._k.size()>0)
				printf("Key stirng is %s",opt._k.c_str());
			printf("\n");
			return 0;
		}

		ElementSequenceCol seqCol;
		seqCol.setParam(opt._i,opt._x,opt._r == -1?RETAIN_LENGTH:opt._r);		
		AuditSequence adMap;
		adMap.setParam(opt._w,opt._n,opt._p);
		FragAllLevCol fragCol;
		fragCol.setParam(opt._f,opt._o,opt._l,opt._z);
		CGspMining mine;
		if(!mine.Init(opt._a.c_str(), pyCol, &seqCol,&adMap,&fragCol))
		{
			printf("Mining init failed!\n");
		}
		else
		{
			mine.execute();
			mine.Destroy();
		}
		delete pyCol;
	}
	else if(opt._c == 2)
	{		
		printf("Begin packing file...\n");
		printf("\tPacking information:\n");
		printf("\tSource index file path:%s\n",opt._s.c_str());
		printf("\tDestination folder path:%s\n",opt._d.c_str());
		printf("\tRetain parameter:%d\n",opt._r);
		CPack pack;
		if(!pack.InitPack(opt._s.c_str(),opt._d.c_str(),opt._r))
		{
			printf("To packing a set, init index file %s and dest path %s are failed\n",
				opt._s.c_str(),opt._d.c_str());
		}
		else
		{
			pack.Execute();
			printf("Packing end.\n");
		}
	}
	else if(opt._c == 3)
	{
		CEvaluate eva;
		if(!eva.Init(opt._s.c_str(), opt._e.c_str()))
		{
			printf("Evaluate failed!\n");
		}
		eva.Evaluate();
	}
	CGspSystem::DestroySystem();
	printf("Total program costs %f seconds.\n", CClockTime::GetTimeCostFromStartUp());
	MemoryState state;
	GetMemoryUsage(state);
	printf("Peak memory usage: %f MB\n", state._peakWorkingSetSize);
	return 0;
}