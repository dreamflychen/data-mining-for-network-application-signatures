#include "genhandle.h"
#include "getopt.h"
#include "gensys.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

int get_gen_input(int argc, char **argv)
{
	char *pszParam;
	int result;
	char *str_opt = "s:d:l:o:t:u:i:";
	int is_valid = true;
	int valid_count = 2;
	FILE *fp = 0;

	result = GetOption(argc, argv, str_opt, &pszParam);
	while(result != 0)
	{
		switch(result)
		{
		case 's':
			fp=fopen(pszParam,"r");
			if(!fp)
			{
				printf("The file \"%s\" is not exists\n", pszParam);
				is_valid = false;
			}
			else
			{
				g_gensys_param.source_file_path = pszParam;
				fclose(fp);
			}
			--valid_count;
			break;
		case 'd':
			if(!PathIsDirectory(pszParam))
			{
				printf("The path \"%s\" is not valid\n", pszParam);
				is_valid = false;
			}
			else
				g_gensys_param.dest_dir_path = pszParam;
			--valid_count;
			break;
		case 'l':
			g_gensys_param.length = atoi(pszParam);
			if(g_gensys_param.length < INFINIT_NUM)
			{
				printf("The length \"%d\" is not valid\n", g_gensys_param.length);
				is_valid = false;
			}
			g_gensys_param.buffer_size = g_gensys_param.length;
			break;
		case 'o':
			g_gensys_param.time_out = atoi(pszParam);
			if(g_gensys_param.time_out < INFINIT_NUM)
			{
				printf("The timeout \"%d\" is not valid\n", g_gensys_param.time_out);
				is_valid = false;
			}
			break;
		case 't':
			g_gensys_param.tcp_option = atoi(pszParam);
			if(g_gensys_param.tcp_option < 0 || g_gensys_param.tcp_option > 3)
			{
				printf("The tcp mode \"%d\" is not valid\n", g_gensys_param.tcp_option);
				is_valid = false;
			}
			break;
		case 'u':
			g_gensys_param.udp_option = atoi(pszParam);
			if(g_gensys_param.udp_option < 0 || g_gensys_param.udp_option > 3)
			{
				printf("The udp mode \"%d\" is not valid\n", g_gensys_param.udp_option);
				is_valid = false;
			}
			break;
		case 'i':
			g_gensys_param.host_ip = inet_addr(pszParam);
			if(g_gensys_param.host_ip == INADDR_NONE)
			{
				printf("The host ip is not valid\n");
				is_valid = false;
			}
			break;
		case 1:
			printf("\"%s\" is an invalid parameter\n", pszParam);
			is_valid = false;
			break;
		case -1:
			printf("\"%s\" is an invalid option\n", pszParam);
			is_valid = false;
			break;
		}
		result=GetOption(argc, argv, str_opt, &pszParam);
	}
	return (is_valid && !valid_count);
}

void gen_show_usage()
{
	printf("======== Powered by HanDong. 2010-03-27 :)========\n");
	printf("A tool to generate flows and streams from pcap format files.\n");
	printf("Usage:\n");
	printf("\t -s [source file path] Set the trace file path.\n");
	printf("\t -d [destination path] Set the destination folder path.\n");
	
	printf("\t -l [len] Set the truncated flow or stream length.\n");
	printf("\t\t The default value is -1 means no limit.\n");
	
	printf("\t -o [timeout] Set the timeout for udp flow mode.\n");
	printf("\t\t The default value is -1 means no limit.\n");
	
	printf("\t -t [0|1|2|3] Set the tcp mode.\n");
	printf("\t\t 0 means get rid of tcp packets.\n");
	printf("\t\t 1 means collect tcp packets as flow but not reassemble.\n");
	printf("\t\t 2 means collect tcp packets as stream and reassembled.\n");
	printf("\t\t 3 means collect tcp packets as flow and stream\n");
	printf("\t\t The default value is 3.\n");

	printf("\t -u [0|1|2|3] Set the udp mode.\n");
	printf("\t\t 0 means get rid of udp packets.\n");
	printf("\t\t 1 means collect udp packets as flow concerned with timeout.\n");
	printf("\t\t 2 means collect udp packets as stream just one udp packet.\n");
	printf("\t\t 3 means collect udp packets as flow and stream\n");
	printf("\t\t The default value is 3.\n");
}

int gen_input_mgr(int argc, char **argv)
{
	if(!get_gen_input(argc,argv))
	{
		gen_show_usage();
		return false;
	}
	return true;
}