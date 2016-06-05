#ifndef CLOCKTIME_H
#define CLOCKTIME_H
#include "stdcommonheader.h"
#include "constdefine.h"

class CClockTime
{
private:
	//begin time
	clock_t _begin;
	//end time
	clock_t _end;
	//present status, can be in running, or stopped
	int		_state;
	double _saved;
public:
	CClockTime():_begin(0),_end(0),_state(CLOCK_STOP),_saved(0){}

	//####static member function

	//get time like Sat Feb 19 00:37:46 2011
	static const char* GetReadAbleDateAndTime(string &str);
	//get time like 20110217_15-57-24
	static const char* GetUnderlineDateAndTime(string &str);
	//estimate the remaining time
	static double EstimateTime(double secondsUsed,int finished,int left);
	//Time cost for running the whole program
	static double GetTimeCostFromStartUp();

	//begin time
	void Begin();
	//end time
	void End();
	//Get up to date consumed time
	double GetDurationSeconds();
	double GetAccumulateSeconds(){return _saved;}
};

#endif