#include "test.h"
#include "LayerMining.h"




bool test_init_destroy()
{
	if(!initSystem())
		return false;
	
	writedeflog("%s %d\n","helloworld",1989);
	
	writelog("test.log","%s %d\n","custom helloworld", 2011);
	
	writelog("test.log","hi this is data mining area\n");
	
	writedeflog("%s %d\n","helloworld two",2012);
	
	writelog("e:\\temp\\test2.log","Hi, write to a different path\n");
	writelog("e:\\temp\\test2.log","Hello, write to a different path two");
	if(!destroySystem())
		return false;
	return true;
}





