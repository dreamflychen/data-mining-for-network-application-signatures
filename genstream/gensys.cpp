#include <nids.h>
#include <shlwapi.h>
#include <imagehlp.h>
#include <time.h>
#include <string>
#include <hash_map>
using namespace std;
#include "ip.h"
#include "tcp.h"
#include "gensys.h"
#include "filepool.h"

#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"imagehlp.lib.")
#pragma comment(lib,"wpcap.lib")
#pragma comment(lib,"wsock32.lib")
#ifdef _DEBUG
#pragma comment(lib,"libnidsdbg.lib")
#else
#pragma comment(lib,"libnids.lib")
#endif

#define BUFFER_SIZE 2048
#define MAX_TCP_STREAM 10000

#define INIT_UDP_FLOW		0
#define DATA_UDP_FLOW		1
#define TIMEOUT_UDP_FLOW	2
#define EXIT_UPD_FLOW		3
#define INIT_UDP_FLAG		0
#define RESP_UDP_FLAG		1

#define INIT_TCP_FLOW		0
#define DATA_TCP_FLOW		1
#define TIMEOUT_TCP_FLOW	2
#define EXIT_TCP_FLOW		3
#define FIN_I_TCP_FLOW		4
#define FIN_R_TCP_FLOW		5
#define INIT_TCP_FLAG		0
#define RESP_TCP_FLAG		1

#define DELTA_FILE_NUMBER 1000 //show the step between status files

char *combine_name(int flow_type, char *proto, unsigned int id, 
				   struct timeval time_start, struct timeval time_end,
				   int actual_size, int write_size, unsigned int init_ip,
				   unsigned short init_port, char direction, 
				   unsigned int resp_ip, unsigned short resp_port);
unsigned int calc_hashnr(const byte *key,unsigned int length);
char *get_temp_file_path(unsigned int id, char c);
char *get_permanent_file_path(char *filename);
struct timeval *get_duration(struct timeval *time_start, 
							 struct timeval *time_end,
							 struct timeval *result);
int is_over_time(struct timeval *time_start);
struct tuple4 *swap_tuple(struct tuple4 *dest, struct tuple4 *source);

void ip_frag_func(struct ip * a_packet, int len);
void udp_flow_end();
void udp_flow_callback(struct tuple4 * addr, char * buf, 
					   int len, struct ip *iph);
void udp_stream_callback(struct tuple4 * addr, char * buf, 
						 int len, struct ip *iph);
void tcp_callback (struct tcp_stream *a_tcp, void ** this_time_not_needed);
void ip_callback(struct ip * a_packet, int len);
void tcp_flow_end();

gensys_prm g_gensys_param; //全局系统变量

struct gensys_env
{
	char filter_str[512];
	char log_file_str[MAX_PATH];
	char data_dir_str[MAX_PATH];
	FILE *log_fp;

	gensys_env():log_fp(0){}
} env;

struct tcp_stream_info
{
	unsigned int r_id;
	unsigned int i_id;
	string i_path;
	string r_path;
	//FILE *i_fp;
	//FILE *r_fp;
	HBUF i_fp;
	HBUF r_fp;
	int r_write_size;
	int r_actual_size;
	int i_write_size;
	int i_actual_size;
	int mode;
	char *proto;
	struct tuple4 tuple;
	struct timeval time_start;
	struct timeval time_end;
	tcp_stream_info()
	{
		memset(this,0,sizeof(tcp_stream_info));
	}
};

struct flow
{
	unsigned int id;
	//FILE *fp;
	HBUF fp;
	string path;
	int write_size;
	int actual_size;
	flow():id(0), fp(0), write_size(0), actual_size(0){}
};

struct tcp_flow_info
{
	flow client;
	flow server;
	int state;
	struct timeval time_start;
	struct timeval time_end;
	struct tuple4 tuple;
	char *proto;
	tcp_flow_info():proto("tcp"), state(0) {}
};

struct udp_flow_info
{
	char *proto;
	struct tuple4 tuple;
	struct timeval time_start;
	struct timeval time_end;
	flow client;
	flow server;
	udp_flow_info():proto("udp") {}
};

/*//no need in vs 2010
struct eq_tuple
{
	bool operator ()(const struct tuple4 &s1,const struct tuple4 &s2) const
	{
		return (memcmp(&s1, &s2, sizeof(struct tuple4)) == 0);
	}
};*/

struct hash_tuple
{
	size_t operator() (const struct tuple4 &s) const
	{
		struct tuple4 tuple = s;
		return calc_hashnr((unsigned char*)&tuple, sizeof(struct tuple4));
	}
	//needed in vs 2010
	bool operator ()(const struct tuple4 &s1,const struct tuple4 &s2) const
	{
		return (memcmp(&s1, &s2, sizeof(struct tuple4)) != 0);
	}
	enum
	{
		bucket_size = 4
	};
};

typedef hash_map<struct tuple4, udp_flow_info*, 
					hash_tuple> hash_udp;
typedef hash_udp::iterator udp_iterator;
typedef hash_map<struct tuple4, tcp_flow_info*,
					hash_tuple> hash_tcp;
typedef hash_tcp::iterator tcp_iterator;

struct id_statistics
{
	unsigned int num_udp_flow;
	unsigned int num_udp_packet;
	unsigned int num_tcp_flow;
	unsigned int num_tcp_stream;
	unsigned int id;
	unsigned int file_num;
	hash_udp udp_flow_table;
	hash_tcp tcp_flow_table;
	struct timeval last_packet_time;
	id_statistics():num_udp_flow(0), num_udp_packet(0),
		num_tcp_flow(0), num_tcp_stream(0), id(0),file_num(0){}
} statistics;

int init_filter()
{
	if(g_gensys_param.tcp_option == REJECT_MODE &&
		g_gensys_param.udp_option == REJECT_MODE)
	{
		return false;
	}
	else if(g_gensys_param.tcp_option == REJECT_MODE &&
		!(g_gensys_param.udp_option == REJECT_MODE))
	{
		strcpy(env.filter_str, "ip and udp");
	}
	else if(!(g_gensys_param.tcp_option == REJECT_MODE) &&
		g_gensys_param.udp_option == REJECT_MODE)
	{
		strcpy(env.filter_str, "ip and tcp");
	}
	else if(!(g_gensys_param.tcp_option == REJECT_MODE) &&
		!(g_gensys_param.udp_option == REJECT_MODE))
	{
		strcpy(env.filter_str, "ip and (tcp or udp)");
	}
	else 
		return false;
	return true;
}



int gensys_start()
{
	const char *pure_file_start = 0;
	const char *pure_file_ext = 0;
	char *file_copy = 0;
	char log_str[MAX_PATH];
	nids_params.filename = g_gensys_param.source_file_path;
	nids_params.scan_num_hosts = 0;
	nids_params.n_tcp_streams = MAX_TCP_STREAM;
	if(!init_filter())
		return false;
	nids_params.pcap_filter=env.filter_str;
	if(!nids_init())
	{
		printf("Init nids filter failed!\n");
		return false;
	}
	nids_register_ip_frag(ip_frag_func);
	if(g_gensys_param.tcp_option & FLOW_MODE)
		nids_register_ip(ip_callback);
	if(g_gensys_param.tcp_option & STREAM_MODE)
		nids_register_tcp(tcp_callback);
	if(g_gensys_param.udp_option & FLOW_MODE) 
		nids_register_udp(udp_flow_callback);
	if(g_gensys_param.udp_option & STREAM_MODE)
		nids_register_udp(udp_stream_callback);
	//get file name
	string pure_file_str = PathFindFileName(g_gensys_param.source_file_path);
	pure_file_start = pure_file_str.c_str();
	pure_file_ext = PathFindExtension(pure_file_start);
	if(pure_file_ext != NULL )
		pure_file_str = pure_file_str.substr(0, pure_file_ext - pure_file_start);
	//merge fiel path
	if(pure_file_str.length() > MAX_PATH - 10)
		return false;
	strcpy(log_str, "log_");
	strcat(log_str, pure_file_str.c_str());
	strcat(log_str, ".txt");
	PathCombine(env.log_file_str, g_gensys_param.dest_dir_path, log_str);
	//create directory of storage stream
	if( strlen(g_gensys_param.dest_dir_path) + 
		pure_file_str.length() > MAX_PATH -3 )
		return false;
	PathCombine(env.data_dir_str, 
		g_gensys_param.dest_dir_path, pure_file_str.c_str());
	strcat(env.data_dir_str, "\\");
	MakeSureDirectoryPathExists(env.data_dir_str);

	env.log_fp = fopen(env.log_file_str, "wt");
	if(!env.log_fp)
	{
		printf("Create log file %s failed!", env.log_file_str);
		return false;
	}
	time_t lt = time(NULL);
	char *now = ctime(&lt);
	fprintf(env.log_fp, "#Log start at %s", now);
	printf("Running start.\nLog file is: %s\n", env.log_file_str);
	printf("Dictionary to save payload is: %s\n", env.data_dir_str);
	nids_run();
	nids_exit();
	//to end udp flow
	udp_flow_end();
	//to end tcp flow
	tcp_flow_end();
	fprintf(env.log_fp, 
		"#The Whole program elapsed time is: %u secs.\n",
		clock()/CLOCKS_PER_SEC);
	fclose(env.log_fp);
	env.log_fp = 0;
	printf("Done.\n");
	return true;
}

void ip_frag_func(struct ip * a_packet, int len)
{
	static size_t _packNumber = 0;
	statistics.last_packet_time = nids_last_pcap_header->ts;
	++_packNumber;
	if( (_packNumber % DELTA_FILE_NUMBER) == 0)
	{
		printf(".");
	}
	if((_packNumber % (DELTA_FILE_NUMBER*10)) ==0)
	{
		printf("%d(%d)",_packNumber,statistics.file_num);
	}
}
void udp_flow_work(const struct tuple4 * addr, char * buf, udp_flow_info *pinfo,
					   int len, int flag, int side)
{
	char *file_path;
	if(flag == INIT_UDP_FLOW)//init udp
	{		
		pinfo->server.id = (++statistics.id);
		++statistics.num_udp_flow;
		file_path = get_temp_file_path(pinfo->server.id, 'i');
		pinfo->server.path = file_path;
		pinfo->server.fp = openbuffer(file_path,g_gensys_param.buffer_size);
		if(!pinfo->server.fp)
		{
			printf("Open file \"%s\" failed.\n", file_path);
			exit(-1);
		}

		pinfo->client.id = (++statistics.id);
		++statistics.num_udp_flow;
		file_path = get_temp_file_path(pinfo->client.id, 'r');
		pinfo->client.path = file_path;
		pinfo->client.fp = openbuffer(file_path,g_gensys_param.buffer_size);
		if(!pinfo->server.fp)
		{
			printf("Open file \"%s\" failed.\n", file_path);
			exit(-1);
		}
		//reset status, add data
		flag = DATA_UDP_FLOW;
	}
	if(flag == DATA_UDP_FLOW)//追加udp
	{
		int write_size = 0;
		int already_write_size = 0;
		flow *pflow;
		if(side == INIT_UDP_FLOW)
			pflow = &pinfo->server;
		else
			pflow = &pinfo->client;
		
		pflow->actual_size += len;
		if(g_gensys_param.length == INFINIT_NUM)
			write_size = len;
		else if(pflow->write_size < g_gensys_param.length)
		{
			//the length of rest data
			write_size = g_gensys_param.length - pflow->write_size;
			if(write_size >= len)
				 write_size = len;
		}
		if(write_size != 0)
		{
			writebuffer(buf, write_size, pflow->fp);
			pflow->write_size += write_size;
		}
	}
	else//timeout udp
	{
		closebuffer(pinfo->client.fp);
		closebuffer(pinfo->server.fp);
		if(!pinfo->client.write_size)
			remove(pinfo->client.path.c_str());
		else
		{
			file_path = combine_name(0, pinfo->proto, pinfo->client.id,
				pinfo->time_start, pinfo->time_end,
				pinfo->client.actual_size, pinfo->client.write_size,
				pinfo->tuple.saddr, pinfo->tuple.source,
				'r', pinfo->tuple.daddr, pinfo->tuple.dest);
			file_path = get_permanent_file_path(file_path);
			rename(pinfo->client.path.c_str(), file_path);
			++statistics.file_num;
		}
		if(!pinfo->server.write_size)
			remove(pinfo->server.path.c_str());
		else
		{
			file_path = combine_name(0, pinfo->proto, pinfo->server.id,
				pinfo->time_start, pinfo->time_end,
				pinfo->server.actual_size, pinfo->server.write_size,
				pinfo->tuple.saddr, pinfo->tuple.source,
				'i', pinfo->tuple.daddr, pinfo->tuple.dest);
			file_path = get_permanent_file_path(file_path);
			rename(pinfo->server.path.c_str(), file_path);
			++statistics.file_num;
		}
	}
}

void udp_over_time_detect()
{
	if(g_gensys_param.time_out == INFINIT_NUM)
		return;
	udp_iterator iter;
	size_t i;
	vector<struct tuple4> time_out_array;
	struct tuple4 addr;
	udp_flow_info *pinfo;
	for(iter = statistics.udp_flow_table.begin();
			iter != statistics.udp_flow_table.end(); ++iter)
				if(is_over_time(&iter->second->time_end))
					time_out_array.push_back(iter->first);
	//delete timeout
	for(i = 0; i < time_out_array.size(); ++i)
	{
		addr = time_out_array[i];
		pinfo = statistics.udp_flow_table[addr];
		udp_flow_work(&addr, 0, pinfo,
					   0, TIMEOUT_UDP_FLOW, INIT_UDP_FLAG);//callback notification
		delete pinfo;//recover memory
		statistics.udp_flow_table.erase(addr);//remove from item
	}
}

void udp_flow_end()
{
	udp_over_time_detect();
	udp_iterator iter;
	for(iter = statistics.udp_flow_table.begin();
		iter != statistics.udp_flow_table.end(); ++iter)
	{
			udp_flow_work(&iter->first, 0, iter->second,
					   0, EXIT_UPD_FLOW, INIT_UDP_FLAG);//callback notification
			delete iter->second;//recover memory
	}
	statistics.udp_flow_table.clear();
}

void udp_flow_callback(struct tuple4 * addr, char * buf, 
					   int len, struct ip *iph)
{
	udp_iterator iter;
	struct tuple4 swap_addr;
	udp_flow_info *pinfo;
	udp_over_time_detect();//clean timeout
	iter = statistics.udp_flow_table.find(*addr);
	if(iter == statistics.udp_flow_table.end())
	{
		swap_tuple(&swap_addr, addr);
		iter = statistics.udp_flow_table.find(swap_addr);
		if(iter == statistics.udp_flow_table.end())
		{
			pinfo = new udp_flow_info;
			statistics.udp_flow_table[*addr] = pinfo;
			pinfo->time_start = statistics.last_packet_time;
			pinfo->time_end = statistics.last_packet_time;
			pinfo->tuple = *addr;		
			udp_flow_work(addr, buf, pinfo, len, INIT_UDP_FLOW, INIT_UDP_FLAG);
		}
		else
		{
			//find from reversed direction
			pinfo = iter->second;
			pinfo->time_end = statistics.last_packet_time;
			udp_flow_work(addr, buf, pinfo, len, DATA_UDP_FLOW, RESP_UDP_FLAG);
		}
	}
	else
	{
		//find directly
		pinfo = iter->second;
		pinfo->time_end = statistics.last_packet_time;
		udp_flow_work(addr, buf, pinfo, len, DATA_UDP_FLOW, INIT_UDP_FLAG);
	}
}

void udp_stream_callback(struct tuple4 * addr, char * buf, 
						 int len, struct ip *iph)
{
	char direction = 'u';
	int write_size = len;
	unsigned int init_ip = g_gensys_param.host_ip;
	unsigned int resp_ip;
	unsigned short init_port;
	unsigned short resp_port;
	char *filename;
	FILE *fp;

	++statistics.id;
	++statistics.num_udp_packet;
	
	if(len == 0)
		return;

	if(addr->saddr == g_gensys_param.host_ip)
	{
		direction = 'i';
		resp_ip = addr->daddr;
		init_port = addr->source;
		resp_port = addr->dest;
	}
	else if(addr->daddr == g_gensys_param.host_ip)
	{
		direction = 'r';
		resp_ip = addr->saddr;
		init_port = addr->dest;
		resp_port = addr->source;
	}
	else
	{
		init_ip = addr->saddr;
		init_port = addr->source;
		resp_ip = addr->daddr;
		resp_port = addr->dest;
	}
	if(g_gensys_param.length != INFINIT_NUM 
		&& len > g_gensys_param.length)
		write_size = g_gensys_param.length;
	
	filename = combine_name(1, "udp", statistics.id, statistics.last_packet_time,
		statistics.last_packet_time, len, write_size, init_ip, init_port, direction,
		resp_ip, resp_port);
	filename = get_permanent_file_path(filename);
	//remove(filename);
	fp = fopen(filename, "wb");
	if(!fp)
	{
		printf("Open file \"%s\" failed.\n", filename);
		exit(-1);
	}
	fwrite(buf, sizeof(char), write_size, fp);
	fclose(fp);
	++statistics.file_num;
}

void tcp_callback (struct tcp_stream *a_tcp, void ** this_time_not_needed)
{
	char *file_path;
	tcp_stream_info *pinfo=(tcp_stream_info *)a_tcp->user;
	if(a_tcp->nids_state == NIDS_JUST_EST)
	{
		a_tcp->client.collect++;
		a_tcp->server.collect++;

		pinfo = new tcp_stream_info;		
		pinfo->time_start = statistics.last_packet_time;
		pinfo->time_end = statistics.last_packet_time;
		pinfo->proto = "tcp";
		pinfo->mode = 1;
		pinfo->tuple = a_tcp->addr;

		pinfo->i_id = (++statistics.id);
		++statistics.num_tcp_stream;
		file_path = get_temp_file_path(pinfo->i_id, 'i');		
		pinfo->i_path = file_path;
		//remove(file_path);
		pinfo->i_fp = openbuffer(file_path, g_gensys_param.buffer_size);
		if(!pinfo->i_fp)
		{
			printf("Open file \"%s\" failed.\n", file_path);
			exit(-1);
		}
		pinfo->r_id = (++statistics.id);
		++statistics.num_tcp_stream;
		file_path = get_temp_file_path(pinfo->r_id, 'r');
		pinfo->r_path = file_path;
		//remove(file_path);
		pinfo->r_fp = openbuffer(file_path, g_gensys_param.buffer_size);
		if(!pinfo->r_fp)
		{
			printf("Open file \"%s\" failed.\n", file_path);
			exit(-1);
		}
		a_tcp->user = pinfo;		
	}
	else if(a_tcp->nids_state == NIDS_DATA)
	{		
		int write_size = 0;
		int count_writed = 0;
		int remain_write = 0;
		HBUF fp;
		char *buffer;
		if(a_tcp->server.count_new)
		{
			if(g_gensys_param.length == INFINIT_NUM)
				write_size = a_tcp->server.count_new;
			else
			{
				count_writed = a_tcp->server.count - a_tcp->server.count_new;
				if(count_writed < g_gensys_param.length)
				{
					remain_write = g_gensys_param.length - count_writed;
					if(a_tcp->server.count_new <= remain_write)
						write_size = a_tcp->server.count_new;
					else
						write_size = remain_write;
				}
			}
			fp = pinfo->i_fp;
			buffer = a_tcp->server.data;
			pinfo->i_write_size += write_size;
		}
		else if(a_tcp->client.count_new)
		{
			if(g_gensys_param.length == INFINIT_NUM)
				write_size = a_tcp->client.count_new;
			else
			{
				count_writed = a_tcp->client.count - a_tcp->client.count_new;
				if(count_writed < g_gensys_param.length)
				{
					//not full, write more
					remain_write = g_gensys_param.length - count_writed;
					if(a_tcp->client.count_new <= remain_write)
						write_size = a_tcp->client.count_new;
					else
						write_size = remain_write;
				}
			}
			fp = pinfo->r_fp;
			buffer = a_tcp->client.data;
			pinfo->r_write_size += write_size;
		}

		if(write_size)
			writebuffer(buffer, write_size, fp);
	}
	else
	{
		pinfo->time_end = statistics.last_packet_time;
		pinfo->i_actual_size = a_tcp->server.count;	
		pinfo->r_actual_size = a_tcp->client.count;
		closebuffer(pinfo->i_fp);
		closebuffer(pinfo->r_fp);
		//delete 0 length file
		if(pinfo->i_actual_size == 0)
			remove(pinfo->i_path.c_str());
		else
		{
			// rename
			file_path = combine_name(1, pinfo->proto, pinfo->i_id, 
				pinfo->time_start, pinfo->time_end,
				pinfo->i_actual_size, pinfo->i_write_size,  
				pinfo->tuple.saddr, pinfo->tuple.source, 'i', 
				pinfo->tuple.daddr, pinfo->tuple.dest);
			file_path = get_permanent_file_path(file_path);
			//remove(file_path);
			if(rename(pinfo->i_path.c_str(), file_path))
			{
				printf("rename error\n");
				exit(-1);
			}
			++statistics.file_num;

		}
		if(pinfo->r_actual_size == 0)
			remove(pinfo->r_path.c_str());
		else
		{
			// rename
			file_path = combine_name(1, pinfo->proto, pinfo->r_id, 
				pinfo->time_start, pinfo->time_end,
				pinfo->r_actual_size, pinfo->r_write_size,  
				pinfo->tuple.saddr, pinfo->tuple.source, 'r', 
				pinfo->tuple.daddr, pinfo->tuple.dest);
			file_path = get_permanent_file_path(file_path);
			//remove(file_path);
			rename(pinfo->r_path.c_str(), file_path);
			++statistics.file_num;
		}
		delete pinfo;
		a_tcp->user=0;
	}
}

void tcp_flow_work(char * buf, int len, tcp_flow_info *pinfo,
					    int flowtype, int side)
{
	int write_size = 0;
	if( flowtype == INIT_TCP_FLOW )
	{
		//initialization
		pinfo->server.id = ++statistics.id;
		++statistics.num_tcp_flow;
		pinfo->server.path = get_temp_file_path(pinfo->server.id, 'i');
		pinfo->server.fp = openbuffer(pinfo->server.path.c_str(), g_gensys_param.buffer_size);
		if(!pinfo->server.fp)
		{
			printf("Open file %s failed\n", pinfo->server.path.c_str());
			exit(-1);
		}
		pinfo->client.id = ++statistics.id;
		++statistics.num_tcp_flow;
		pinfo->client.path = get_temp_file_path(pinfo->client.id, 'r');
		pinfo->client.fp = openbuffer(pinfo->client.path.c_str(), g_gensys_param.buffer_size);
		if(!pinfo->client.fp)
		{
			printf("Open file %s failed\n", pinfo->client.path.c_str());
			exit(-1);
		}
		if(len && buf)
		{
			if(g_gensys_param.length == INFINIT_NUM ||
				len <= g_gensys_param.length)
				write_size = len;
			else
				write_size = g_gensys_param.length;
			writebuffer(buf, write_size, pinfo->server.fp);
			pinfo->server.actual_size += len;
			pinfo->server.write_size += write_size;
		}		
	}
	else
	{		
		flow *pflow;
		if( side == INIT_TCP_FLAG)
			pflow = &pinfo->server;
		else
			pflow = &pinfo->client;
		if(len && buf)
		{
			write_size = len;
			pflow->actual_size += len;
			if(g_gensys_param.length != INFINIT_NUM )
			{
				write_size = g_gensys_param.length - pflow->write_size;
				if(len < write_size)
					write_size = len;
			}
			if(write_size)
			{
				writebuffer(buf, write_size, pflow->fp);
				pflow->write_size += write_size;
			}
		}

		if( flowtype!= DATA_TCP_FLOW )
		{
			//exit
			char *filename;
			closebuffer(pinfo->server.fp);
			closebuffer(pinfo->client.fp);
			if(pinfo->server.write_size == 0)
				remove(pinfo->server.path.c_str());
			else
			{				
				filename = combine_name(0, pinfo->proto, pinfo->server.id, 
					pinfo->time_start,
					pinfo->time_end, pinfo->server.actual_size,
					pinfo->server.write_size, pinfo->tuple.saddr,
					pinfo->tuple.source, 'i', pinfo->tuple.daddr,
					pinfo->tuple.dest);
				rename(pinfo->server.path.c_str(), 
					get_permanent_file_path(filename));
				++statistics.file_num;
			}

			if(pinfo->client.write_size == 0)
				remove(pinfo->client.path.c_str());
			else
			{
				filename = combine_name(0, pinfo->proto, pinfo->client.id, 
					pinfo->time_start,
					pinfo->time_end, pinfo->client.actual_size,
					pinfo->client.write_size, pinfo->tuple.saddr,
					pinfo->tuple.source, 'r', pinfo->tuple.daddr,
					pinfo->tuple.dest);
				rename(pinfo->client.path.c_str(), 
					get_permanent_file_path(filename));
				++statistics.file_num;
			}
		}
		
	}
}

void tcp_over_time_detect()
{
	if(g_gensys_param.time_out == INFINIT_NUM)
		return;
	tcp_iterator iter;
	size_t i;
	vector<struct tuple4> time_out_array;
	struct tuple4 addr;
	tcp_flow_info *pinfo;
	for(iter = statistics.tcp_flow_table.begin();
			iter != statistics.tcp_flow_table.end(); ++iter)
				if(is_over_time(&iter->second->time_end))
					time_out_array.push_back(iter->first);
	//delete timeout
	for(i = 0; i < time_out_array.size(); ++i)
	{
		addr = time_out_array[i];
		pinfo = statistics.tcp_flow_table[addr];
		//callback notification
		tcp_flow_work(0, 0, pinfo, TIMEOUT_TCP_FLOW, 0);
		delete pinfo;
		statistics.udp_flow_table.erase(addr);
	}
	
}

void tcp_flow_end()
{
	tcp_over_time_detect();
	tcp_iterator iter;
	for(iter = statistics.tcp_flow_table.begin();
		iter != statistics.tcp_flow_table.end(); ++iter)
	{
			tcp_flow_work(0, 0, iter->second,
				EXIT_TCP_FLOW, 0);
			delete iter->second;
	}
	statistics.tcp_flow_table.clear();
}

void ip_callback(struct ip * a_packet, int len)
{
	if(a_packet->ip_p != IPPROTO_TCP)
		return;
	//processing tcp
	tcp_over_time_detect();
	tcp_flow_info *pinfo = 0;
	int total_len = ntohs(a_packet->ip_len);
	int ip_len = a_packet->ip_hl * 4;
	char *p_buf = (char*)a_packet;
	struct tcphdr *p_tcp = (struct tcphdr*)(p_buf + ip_len);
	int tcp_len = p_tcp->th_off * 4;
	int payload_len = total_len - ip_len - tcp_len;
	char *payload = p_buf + ip_len + tcp_len;
	struct tuple4 tuple, swapt;
	int side = 0;
	
	tuple.saddr = a_packet->ip_src.s_addr;
	tuple.daddr = a_packet->ip_dst.s_addr;
	tuple.source = ntohs(p_tcp->th_sport);
	tuple.dest = ntohs(p_tcp->th_dport);
	tcp_iterator iter, swap_iter;
	iter = statistics.tcp_flow_table.find(tuple);
	swap_tuple(&swapt, &tuple);
	swap_iter = statistics.tcp_flow_table.find(swapt);

	if(iter != statistics.tcp_flow_table.end())
	{
		pinfo = iter->second;
		side = INIT_TCP_FLAG;
	}
	else if(swap_iter != statistics.tcp_flow_table.end())
	{
		pinfo = swap_iter->second;
		side = RESP_TCP_FLAG;
	}

	if(iter == statistics.tcp_flow_table.end()
		&& swap_iter == statistics.tcp_flow_table.end())
	{
		int syn = p_tcp->th_flags & TH_SYN;
		int ack = p_tcp->th_flags & TH_ACK;
		if( syn && !ack)
		{
			//get syn first time
			//create new object
			pinfo = new tcp_flow_info;
			pinfo->state = 1;
			pinfo->tuple = tuple;
			pinfo->time_start = statistics.last_packet_time;
			pinfo->time_end = statistics.last_packet_time;
			statistics.tcp_flow_table[tuple] = pinfo;
			tcp_flow_work(payload, payload_len, pinfo, INIT_TCP_FLOW, INIT_TCP_FLAG);
		}
	}
	else
	{		
		int rst = p_tcp->th_flags & TH_RST;
		int fin = p_tcp->th_flags & TH_FIN;
		pinfo->time_end = statistics.last_packet_time;
		if(rst)//end directly
		{
			tcp_flow_work(payload, payload_len, pinfo, EXIT_TCP_FLOW, side);
			if(iter!=statistics.tcp_flow_table.end())
				delete iter->second;
			if(swap_iter != statistics.tcp_flow_table.end())
				delete swap_iter->second;
			statistics.tcp_flow_table.erase(tuple);
			statistics.tcp_flow_table.erase(swapt);
		}
		else if(fin)
		{
			if(pinfo->state == 1 && 
				side == INIT_TCP_FLAG)
			{
				pinfo->state = 2;//end init
				tcp_flow_work(payload, payload_len, pinfo, DATA_TCP_FLOW, side);
			}
			else if(pinfo->state == 1 &&
				side == RESP_TCP_FLAG)
			{
				pinfo->state = 3;//end resp
				tcp_flow_work(payload, payload_len, pinfo, DATA_TCP_FLOW, side);
			}
			else if( (pinfo ->state == 2 && side == RESP_TCP_FLAG) 
				|| (pinfo ->state == 3 && side == INIT_TCP_FLAG) )
			{
				//end all
				tcp_flow_work(payload, payload_len, pinfo, EXIT_TCP_FLOW, side);
				if(iter!=statistics.tcp_flow_table.end())
					delete iter->second;
				if(swap_iter != statistics.tcp_flow_table.end())
					delete swap_iter->second;
				statistics.tcp_flow_table.erase(tuple);
				statistics.tcp_flow_table.erase(swapt);
			}
		}
		else
			tcp_flow_work(payload, payload_len, pinfo, DATA_TCP_FLOW, side);
	}
	
}
/////////////////////////////////////////////////////////////
struct tuple4 *swap_tuple(struct tuple4 *dest, struct tuple4 *source)
{
	dest->daddr = source->saddr;
	dest->dest = source->source;
	dest->saddr = source->daddr;
	dest->source = source->dest;
	return dest;
}

int is_over_time(struct timeval *time_start)
{
	struct timeval d;
	get_duration(time_start, &statistics.last_packet_time, &d);
	if( d.tv_sec >= (long)g_gensys_param.time_out)
	{
		return 1;
	}
	return 0;
}

char *get_temp_file_path(unsigned int id, char c)
{
	static char filefullpath[MAX_PATH];
	char filename[20];
	sprintf(filename, "%c%u.payload", c, id);
	if(strlen(g_gensys_param.dest_dir_path) + strlen(filename) > MAX_PATH-1)
	{
		printf("File name %s is too long.\n", filename);
		exit(-1);
	}
	PathCombine(filefullpath, env.data_dir_str, filename);
	return filefullpath;
}

char *get_permanent_file_path(char *filename)
{
	static char filefullpath[MAX_PATH];
	if(strlen(g_gensys_param.dest_dir_path) + strlen(filename) > MAX_PATH-1)
	{
		printf("File name %s is too long.\n", filename);
		exit(-1);
	}
	PathCombine(filefullpath, env.data_dir_str, filename);
	return filefullpath;
}

struct timeval *get_duration(struct timeval *time_start, 
							 struct timeval *time_end,
							 struct timeval *result)
{
	if(!result)
		return 0;
	if(time_end->tv_usec < time_start->tv_usec)
	{
		//append L for long type
		result->tv_usec = 1000000L + time_end->tv_usec - time_start->tv_usec; 
		result->tv_sec = time_end->tv_sec - 1L - time_start->tv_sec; 
	}
	else
	{
		result->tv_sec = time_end->tv_sec - time_start->tv_sec;
		result->tv_usec = time_end->tv_usec - time_start->tv_usec;
	}
	return result;
}

char *combine_name(int flow_type, char *proto, unsigned int id, 
				   struct timeval time_start, struct timeval time_end,
				   int actual_size, int write_size, unsigned int init_ip,
				   unsigned short init_port, char direction, 
				   unsigned int resp_ip, unsigned short resp_port)
{
	static char filename[MAX_PATH];
	char start_time_str[30];
	char end_time_str[30];
	char i_ip_str[16];
	char r_ip_str[16];
	struct timeval duration;
	time_t timet = time_start.tv_sec;
	struct tm *ptime = localtime(&timet);
	strftime(start_time_str, sizeof(start_time_str)-1,
		"%Y%m%d@%H#%M#%S", ptime);
	timet = time_end.tv_sec;
	ptime = localtime(&timet);
	strftime(end_time_str, sizeof(end_time_str)-1,
		"%Y%m%d@%H#%M#%S", ptime);

	get_duration(&time_start, &time_end, &duration);

	struct in_addr i_addr;
	struct in_addr r_addr;
	i_addr.S_un.S_addr = init_ip;//network order
	strcpy(i_ip_str, inet_ntoa(i_addr));
	r_addr.S_un.S_addr = resp_ip;
	strcpy(r_ip_str, inet_ntoa(r_addr));
	
	sprintf(filename,
		"%d-%s-%u-%s.%ld-%s.%ld-%ld.%ld-%d-%d-%s@%u#%c#%s@%u.payload",
		flow_type, proto, id, start_time_str, time_start.tv_usec,
		end_time_str, time_end.tv_usec, 
		duration.tv_sec, duration.tv_usec, 
		actual_size, write_size,
		i_ip_str, init_port, direction, 
		r_ip_str, resp_port);	
	return filename;
}

unsigned int calc_hashnr(const byte *key,unsigned int length)
{
  register unsigned int nr=1, nr2=4;
  while (length--)
  {
    nr ^= (((nr & 63)+nr2)*((unsigned int) (unsigned char) *key++))+ (nr << 8);
    nr2 += 3;
  }
  return((unsigned int) nr);
}
