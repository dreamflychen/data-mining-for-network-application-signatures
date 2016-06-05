#include "clocktime.h"

const char* CClockTime::GetReadAbleDateAndTime(string &str)
{
	str.clear();
	time_t t=time(NULL);
	str = ctime(&t);
	return str.c_str();
}

const char* CClockTime::GetUnderlineDateAndTime(string &str)
{
	char timestr[32];
	time_t timet = time(NULL);
	struct tm *ptime = localtime(&timet);
	strftime(timestr, sizeof(timestr)-1,
		"%Y%m%d_%H-%M-%S", ptime);
	str.clear();
	str = timestr;
	return str.c_str();
}


double CClockTime::EstimateTime(double secondsUsed,int finished,int left)
{
	return (secondsUsed/(double)finished)*(double)left;
}

double CClockTime::GetTimeCostFromStartUp()
{
	return (double)clock()/(double)CLOCKS_PER_SEC;
}


void CClockTime::Begin(){
	this->_state = CLOCK_RUN;
	this->_begin = clock();
}


void CClockTime::End(){
	this->_state = CLOCK_STOP;
	this->_end = clock();
	this->_saved += GetDurationSeconds();
}


	if(this->_state == CLOCK_RUN)
	{
		return (double)(clock()-_begin)/(double)CLOCKS_PER_SEC;
	}
	else
	{
		return (double)(_end-_begin)/(double)CLOCKS_PER_SEC;
	}
}

