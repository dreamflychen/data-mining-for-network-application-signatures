#ifndef _GENSYS_H_
#define _GENSYS_H_
#define _CRT_SECURE_NO_WARNINGS

#include <string>
using namespace std;

#define INFINIT_NUM		-1

//tcp udp , flow collection mode
#define REJECT_MODE		0x0
#define FLOW_MODE		0x1
#define STREAM_MODE		0x2
#define BOTH_MODE		(0x1 | 0x2)

struct gensys_prm
{
	char* source_file_path; //source file paht
	char* dest_dir_path;    //destination folder path
	int length;              //truncated length
	int time_out;
	int tcp_option;
	int udp_option;
	unsigned long host_ip;
	size_t buffer_size;
	gensys_prm():source_file_path(0), dest_dir_path(0), 
		length(INFINIT_NUM), time_out(INFINIT_NUM),
		tcp_option(BOTH_MODE), udp_option(BOTH_MODE), host_ip(0xffffffff),
		buffer_size(2048){}
};

extern gensys_prm g_gensys_param;
int gensys_start();

#endif