#ifndef GSPMINING_H
#define GSPMINING_H

#include "gspsystem.h"
#include "element.h"
#include "clocktime.h"
#include "audit.h"
#include "signature.h"
#include "fragment.h"
#include "report.h"
//Define interface of data mining
class CGspMining
{
	friend class CReport;
	bool _MiningAtSupport(int supIndex,SignatureCollection &sc);
	int _Mining(IntVector &unk,SignatureCollection &sc);
	int _FirstMining(IntVector &unk,SignatureCollection &sc);
	int _FollowMinging(IntVector &unk,SignatureCollection &sc);
	bool _evalue_execute();
public:

	CGspMining():_bLog(false),_pPfCol(0),_pEmSeqCol(0),_pAdSeq(0),
		_steps(0),_pAllCol(0),_bForceSupport(false){}
	//####public variables

	//write log or not
	bool	_bLog;

	bool	_bForceSupport;
	
	string _miningName;
	
	string _miningLog;
	
	CClockTime _time;

	
	PayloadFileCollection * _pPfCol;

	
	ElementSequenceCol * _pEmSeqCol;

	
	AuditSequence *_pAdSeq;

	
	FragAllLevCol *_pAllCol;

	
	int _steps;

	
	CClockTime _ioTime;
	CClockTime _cpuTime;

	
	SignatureCollection _sc;

	

	//Initialization, if pLog is empty, then the log recording is disabled, otherwise some functions are clled for log recording
	virtual bool Init(const char* miningName,PayloadFileCollection *pPfCol,
		ElementSequenceCol *psecol,AuditSequence *pAdSeq,FragAllLevCol *pAllCol, bool genReport=true);
		
	virtual bool execute();

	
	virtual bool Destroy();
private:
};

#endif