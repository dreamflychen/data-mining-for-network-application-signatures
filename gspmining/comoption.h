#ifndef COMOPTION_H
#define COMOPTION_H
#include "stdcommonheader.h"
#include "constdefine.h"
#include "getopt.h"

class CComOption
{
public:
	CComOption():_c(0),_t(1),_r(-1),_i(-1),_x(-1),
		_w(HALF_WINDOW_SIZE),_n(SHINGLE_SIZE),_a(DEFAULT_PROTOCOL_TAG),
		_o(COSINE),_l(SCALE),_p(MIN_SUPPORT_THRESHOLD),_f(-1),_z(MAX_OFFSET){}
	
	char _commandParam[PARAM_BUFFER_SIZE];//for temporary parsing
	int _c;//execution type
	string _s;//source
	string _d;//destination
	int _t;//number of threads
	int _r;//reversed length
	string _k;//keywords for filter
	int _i;//min value
	int _x;//max value
	string _a;//protocal tag
	string _e;//file path for regular expression
	int _w;//half window size
	int _n;//shingle length
	double _o; //cosine
	double _l; //scale
	double _p; //min support threshold
	double _f;//force support
	int _z;//offset
private:


	bool GetGenInput(int argc, char **argv,string &errStr);

public:
	void GenShowUsage();
	bool GenInputMgr(int argc, char **argv);
};
#endif