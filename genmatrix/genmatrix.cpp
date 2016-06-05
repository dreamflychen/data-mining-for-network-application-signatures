/* 
  Program features
  Input:
	(default size 32) half window size -wb
	(default 0.8f) threshold size -t\t
	*(required) input path -i
	*(required) output path -o
	
  Process:
	Split the "*.payload" file by half window.
	The split will be ended if the number of
	files has the length is less than the threshold.
	For each window, calculate similarity matrix,
	the result is written to log file.
  
  Output:
	Log file: date_log.txt
	Similarity Matrix: Index of window_half window size_actual threhold_predefined threshold.txt
	Output the row number e.g. i-100
	the float type can output 23170 elements in Matrix.
*/
#include <vector>
#include <string>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

#include "getopt.h"
#define HALF_WINDOW_VALUE 32
#define THRESHOLD_VALUE 0.8f
#define MAX_LEN			1024
#define LEFT_UP			1
#define UP				2
#define LEFT			3

struct argument
{
	int half_window;
	int max_len;
	float threshold;
	char *input_path;
	char *output_path;
	char *filter;
	float alpha;
	int continue_count;
	argument():half_window(HALF_WINDOW_VALUE), 
		max_len(MAX_LEN),threshold(THRESHOLD_VALUE), 
		input_path(0), output_path(0), filter(0),
		alpha(0.5f), continue_count(3){}
} argu;

struct payload
{
	char *buffer;
	int size;
	payload():buffer(0), size(0){}
};

struct dtable
{
	int **c;
	char **b;
};

struct global
{
	vector<payload> payload_list;
	vector<float> threshold_list;
	dtable table;
	float **matrix;
	vector<payload> window_list;
	vector<string> filter_list;
	FILE *fp_log;
} env;

void show_usage()
{
	printf("======== Powered by HanDong. 2010-4-4 :) ========\n");
	printf("A tool to read payload files and output matrix.\n");
	printf("Usage:\n");
	printf("\t -w [integer value] Set the half window size.\n");
	printf("\t\t The default value of option w is %d\n", HALF_WINDOW_VALUE);
	printf("\t -t [threshold value between 0 to 1] Set threshold value.\n");
	printf("\t\t The default value of option t is %.2f .\n", THRESHOLD_VALUE);
	printf("\t -l [the length will be read from payload file]\n");
	printf("\t\t The default value of option l is %d\n", MAX_LEN);
	printf("\t -i [path] Set the path of input file directory.\n");
	printf("\t -o [path] Set the path of output file directory.\n");
	printf("\t -f [string] Set the filter of filename. \n");
	printf("\t\t The default don't set filter\n");
	printf("\t\t The filter string can be divided by ','. \n");
	printf("\t -a [float] Set the alpha value.\n");
	printf("\t\t The default value is 0.5 as factor=seq_count/(seq_count+alpha)\n");
	printf("\t -c [integer value] Set continue threshold\n");
	printf("\t\t The default value is 3 as seq_count>= 3 factor=1.0\n");
}

int get_input(int argc, char **argv)
{
	char *pszParam;
	int result;
	char *str_opt = "w:t:i:o:l:f:a:c:";
	int is_valid = true;
	int valid_count = 2;
	FILE *fp = 0;
	result = GetOption(argc, argv, str_opt, &pszParam);
	int half_window = 0;
	int max_len = 0;
	double threshold = 0.0;
	double alpha = 0.0;
	while( result !=0 )
	{
		switch(result)
		{
		case 'l':
			max_len = atoi(pszParam);
			if(!max_len)
			{
				printf("The max length value %s should not be zero.\n", pszParam);
			}
			else
				argu.max_len = max_len;
			break;
		case 'w':
			half_window = atoi(pszParam);
			if(!half_window)
			{
				printf("The half window value %s should larger than zero.\n", pszParam);
				is_valid = false;
			}
			else
				argu.half_window = half_window;
			break;
		case 't':
			threshold = atof(pszParam);
			if( threshold == 0.0 )
			{
				printf("The threshold %s should larger than zero\n", pszParam);
				is_valid = false;
			}
			else
				argu.threshold = (float)threshold;
			break;
		case 'i':
			if(!PathIsDirectory(pszParam))
			{
				printf("The path \"%s\" is not valid\n", pszParam);
				is_valid = false;
			}
			else
				argu.input_path = pszParam;
			--valid_count;
			break;
		case 'o':
			if(!PathIsDirectory(pszParam))
			{
				printf("The path \"%s\" is not valid\n", pszParam);
				is_valid = false;
			}
			else
				argu.output_path = pszParam;
			--valid_count;
			break;
		case 'f':
			argu.filter = pszParam;
			break;
		case 'a':
			alpha = atof(pszParam);
			if( alpha == 0.0 )
			{
				printf("The alpha %s should larger than zero\n", pszParam);
				is_valid = false;
			}
			else
				argu.alpha = (float)threshold;
			break;
		case 'c':
			argu.continue_count = atoi(pszParam);
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
	if( !is_valid || valid_count)
	{
		show_usage();
		return 0;
	}
	return 1;
}

int read_window(int window_index)
{
	size_t i;
	int sum = 0;
	int offset = window_index * argu.half_window;
	int wsize = argu.half_window * 2;
	for(i = 0; i < env.payload_list.size(); ++i)
	{
		memset(env.window_list[i].buffer, 0, wsize);
		env.window_list[i].size = 0;
		if(offset < env.payload_list[i].size)
		{
			int length(argu.half_window * 2);
			if( offset + length > env.payload_list[i].size)
				length = env.payload_list[i].size - offset;
			memcpy(env.window_list[i].buffer, env.payload_list[i].buffer + offset, length);
			env.window_list[i].size = length;
			++sum;
		}
	}
	float threshold =  (float)sum/(float)env.payload_list.size();
	if(threshold < argu.threshold)
		return 0;
	env.threshold_list.push_back(threshold);
	return 1;
}

float calc_similar(vector<int> &seq, int maxstrlen)
{
	size_t i;
	float sub_sim = 0.0f;
	float sum_sim = 0.0f;
	float factor = 0.0f; //the value of factor is smaller than 1, and is portion to the strlen[i];
	for(i = 0; i < seq.size(); ++i)
	{
		if(seq[i] < argu.continue_count)
			factor = seq[i] / (seq[i] + argu.alpha);
		else//set factor to 1, if there are no less than 3 bytes are identical.
			factor = 1.0f;
		sub_sim = seq[i] * factor;
		sum_sim += sub_sim;
	}
	return (sum_sim / maxstrlen);//get similarity
}

int calc_continue(vector<int> &seq, vector<int> &seq_c)
{
	size_t i;
	int count = 0;
	int pos = -1;
	seq_c.clear();
	int c = 0;
	for(i = 0; i < seq.size(); ++i)
	{
		if(pos == -1)
		{
			++count;// begin the initial segment
			c = 1;
			seq_c.push_back(c);
		}
		else if(seq[i] - pos > 1)
		{
			pos = seq[i];// begin
			++count;
			c = 1;
			seq_c.push_back(c);// join the counter
		}
		else
		{			
			++c;
			seq_c[count - 1] = c;
		}
		pos = seq[i];
	}
	return count;
}

void print_asc(char *c, int size)
{
	unsigned int ivalue;
	for(int i=0; i < size; ++i)
	{
		ivalue = c[i];
		if(ivalue <= 0x20 || ivalue > 0x7f)
			printf("\\x%02X", ivalue);
		else
			printf("%c",c[i]);
	}
	printf("\n");
}

void print_b()
{
	int wl = argu.half_window * 2 + 1;
	int i, j;
	printf("\n");
	for(i = 0; i < wl; ++i)//empty the table of dynamic programming
	{
		for(j = 0; j < wl; ++j)
		{
			if(env.table.b[i][j] == 0)
				printf("0");
			else if(env.table.b[i][j] == LEFT_UP)
				printf("\\");
			else if(env.table.b[i][j] == LEFT)
				printf("-");
			else if(env.table.b[i][j] == UP)
				printf("|");
		}
		printf("\n");
	}
}

float get_similar(int x, int y)
{
	int m = env.window_list[x].size;
	int n = env.window_list[y].size;
	char *px = env.window_list[x].buffer;
	char *py = env.window_list[y].buffer;
	int **c = env.table.c;
	char **b = env.table.b;
	int i = 0;
	int j = 0;
	int wl = argu.half_window * 2 + 1;
	for(i = 0; i < wl; ++i)//empty the table of dynamic programming
	{
		memset(c[i], 0, wl * sizeof(int));
		memset(b[i], 0, wl);
	}
	for(i = 1; i <= m; ++i)
	{
		for(j = 1; j <= n; ++j)
		{
			if(px[i - 1] == py[j - 1])
			{
				c[i][j] = c[i - 1][j - 1] + 1;
				b[i][j] = LEFT_UP;
			}
			else if(c[i - 1][j] >= c[i][j - 1])
			{
				c[i][j] = c[i - 1][j];
				b[i][j] = UP;
			}
			else
			{
				c[i][j] = c[i][j - 1];
				b[i][j] = LEFT;
			}
		}
	}
	//print_asc(px, m);
	//print_asc(py, n);
	//print_b();
	i = m;
	j = n;
	int s = 0;
	vector<int> seq_m;
	vector<int> seq_n;
	vector<int> count_m;
	vector<int> count_n;
	while(i >= 0 && j >= 0)
	{
		if(i == 0 || j ==0 )
		{
			break;
		}
		if(b[i][j] == LEFT_UP)
		{
			seq_m.insert(seq_m.begin(), i);
			seq_n.insert(seq_n.begin(), j);
			--i;
			--j;
			++s;
		}
		else if(b[i][j] == UP)
		{
			--i;
		}
		else
		{
			--j;
		}
	}
	i = calc_continue(seq_m, count_m);
	j = calc_continue(seq_n, count_n);
	float sim;
	int len = m > n? m : n;
	if ( i < j )
		sim = calc_similar(count_m, len);
	else
		sim = calc_similar(count_n, len);
	return sim;
}

int create_matrix(int window_index)
{
	//compare each pair, create similarity matrix
	int i = 0;
	int j = 0;
	float sim = 0.0f;
	int tsize = env.window_list.size();
	printf("calc matrix\n");
	for(i = 0; i < tsize; ++i)
	{
		env.matrix[i][i] = 1.0f;
		for( j = i + 1; j < tsize; ++j)
		{
			if(env.window_list[i].size == 0 ||
				env.window_list[j].size ==0)
				sim = 0.0f;
			else
				sim = get_similar(i, j);
			env.matrix[i][j] = sim;
			env.matrix[j][i] = sim;
		}
		printf("finish %d line\n",i);
	}
	printf("end calc matrix\n");
	return 1;
}

int init_logfile()
{
	string path(argu.output_path);
		char c = path[path.length() -1];
	if(c != '\\' && c != '/')
	{
		path += "\\";
	}
	path += "log.txt";
	env.fp_log = fopen(path.c_str(), "wt");
	if(!env.fp_log)
		return 0;
	return 1;
}

void string_split(string str, char delim, vector<string> &results)
{
	int cur_at;
	while( (cur_at = str.find_first_of(delim)) != str.npos )
	{
		if(cur_at > 0)
			results.push_back(str.substr(0, cur_at));
		str = str.substr(cur_at + 1);
	}
	if(str.length() > 0)
		results.push_back(str);
}

void init_filter()
{
	if(argu.filter == 0)
		return;
	string_split(argu.filter, ',', env.filter_list);
}

int filter_select(const char* file)
{
	if(argu.filter == 0)
		return 1;//always true
	else
	{
		for(size_t i = 0; i < env.filter_list.size(); ++i)
		{
			if(!strstr(file, env.filter_list[i].c_str()))
				return 0;
		}
	}
	return 1;
}

int init()
{
	//find all files
	string search_path = argu.input_path;
	string path(search_path);
	string full_path;
	FILE *fp;
	size_t i;
	payload p;
	char c = search_path[search_path.length() -1];
	init_filter();//init file, filter keywords
	init_logfile();
	//log
	fprintf(env.fp_log, "half_window:%d, threshold:%.2f, max_len:%d, ", 
		argu.half_window, argu.threshold, argu.max_len);
	fprintf(env.fp_log, "alpha:%.2f, continue:%d, filter:%s\n", 
		argu.alpha, argu.continue_count, argu.filter?argu.filter:"none");
	//~log
	
	if(c == '\\' || c == '/')
		search_path += "*.payload";
	else
	{
		search_path += "\\";
		path += "\\";
		search_path += "*.payload";
	}
	WIN32_FIND_DATA fdata;
	HANDLE find = FindFirstFile(search_path.c_str(), &fdata);
	if(find == INVALID_HANDLE_VALUE)
	{
		printf("Can not find any file.\n");
		return 0;
	}
	do
	{		
		//eliminate the file without keywords
		if(argu.filter && !filter_select(fdata.cFileName))
			continue;
		full_path = path;
		full_path += fdata.cFileName;
		//log
		fprintf(env.fp_log, "%s\n", fdata.cFileName);
		//~log
		fp = fopen(full_path.c_str(), "rb");
		if(!fp)
		{
			printf("Can no open file %s.\n", full_path.c_str());
			return 0;
		}
		p.buffer = new char[argu.max_len];
		p.size = fread(p.buffer, sizeof(char), argu.max_len, fp);
		fclose(fp);
		env.payload_list.push_back(p);
	}while(FindNextFile(find, &fdata));

	FindClose(find);
	//log
	fprintf(env.fp_log, "total file:%u\n", env.payload_list.size());
	fflush(env.fp_log);
	//~log
	size_t window_size = argu.half_window * 2;
	size_t files_num = env.payload_list.size();
	for(i = 0; i < files_num; ++i)
	{
		p.buffer = new char[window_size];
		p.size = 0;
		env.window_list.push_back(p);
	}
	//initialize table of dynamic programming
	window_size = argu.half_window * 2 + 1;
	env.table.c = new int*[window_size];
	env.table.b = new char*[window_size];
	for(i = 0; i < (unsigned int)window_size; ++i)
	{
		env.table.c[i] = new int[window_size];
		env.table.b[i] = new char[window_size];
	}
	
	//allocate space for matrix
	window_size = env.payload_list.size();
	env.matrix = new float*[window_size];
	for(i = 0; i< window_size; ++i)
	{
		env.matrix[i] = new float[window_size];
		memset(env.matrix[i], 0, sizeof(float) * window_size);
	}

	return 1;
}

int write_matrix(int window_index)
{
	string path(argu.output_path);
	string filename;
	static char buffer[256];
	sprintf(buffer, "%d_", window_index);
	filename += buffer;
	sprintf(buffer, "%d_", argu.half_window);
	filename += buffer;
	sprintf(buffer, "%.2f_", argu.threshold);
	filename += buffer;
	sprintf(buffer, "%.2f", env.threshold_list[window_index]);
	filename += buffer;;
	filename += ".graph";
	char c = path[path.length() -1];
	if(c != '\\' && c != '/')
	{
		path += "\\";
	}
	path += filename;
	FILE *fp = fopen(path.c_str(), "wb");
	if(!fp)
		return 0;
	size_t i, j;
	size_t num = env.window_list.size();
	fprintf(fp, "%u\n", num);
	for( i = 0; i < num; ++i)
	{
		for( j = 0; j < num; ++j)
		{
			if(j != num-1)
				fprintf(fp, "%.2f\t", env.matrix[i][j]);
			else
				fprintf(fp, "%.2f", env.matrix[i][j]);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
	return 1;
}

int exec()
{
	//read each window size and do calculation
	int window_index = 0;
	while(read_window(window_index))
	{
		printf("Start calc the %d window index\n", window_index);
		//calculate the similarity based on the index of window
		create_matrix(window_index);
		if(!write_matrix(window_index))
			return 0;
		printf("End calc the %d window index\n", window_index);
		++window_index;
		//env.threshold_list.push_back(0.0f);
	}
	return 1;
}

int exit()
{
	size_t i;
	size_t num = 0;
	size_t window_size = (unsigned int)argu.half_window * 2 + 1;
	//empty data
	num = env.payload_list.size();
	for(i = 0; i < num; ++i)
		delete [] env.payload_list[i].buffer;
	env.payload_list.clear();
	//
	num = env.window_list.size();
	for(i = 0; i < num; ++i)
		delete [] env.window_list[i].buffer;
	env.window_list.clear();
	//empty dynamic programming space
	for(i = 0; i < window_size; ++i)
		delete [] env.table.c[i];
	delete env.table.c;
	//empty matrix
	for(i = 0; i< env.payload_list.size(); ++i)
		delete [] env.matrix[i];
	delete env.matrix;
	fclose(env.fp_log);
	return 1;
}

int run()
{
	if(!init())
		return 0;
	if(!exec())
		return 0;
	if(!exit())
		return 0;
	return 1;
}

int main(int argc, char *argv[])
{
	if(!get_input(argc, argv))
		return 0;
	run();
	return 0;
}