#include "comoption.h"

bool CComOption::GetGenInput(int argc, char **argv,string &errStr)
{
	char *pszParam;
	int result;
	errStr.clear();

	char *str_opt = "a:c:d:e:f:h:i:k:l:n:o:p:r:s:t:w:x:z:";
	bool is_valid = true;
	result = GetOption(argc, argv, str_opt, &pszParam);
	int type;
	int flag;
	double ftype;
	while(result != 0)
	{
		switch(result)
		{
		case 'h':
			return false;
		case 'c':
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the c option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_c = type;
				if(type<0 || type >3)
				{
					is_valid = false;
					printf("The %d in c option is not valid\n",type);
				}
			}			
			break;
		case 's':
			_s = pszParam;
			break;
		case 'd':
			_d = pszParam;
			break;
		case 'k':
			_k = pszParam;
			if(_k.at(0) == '\\')
			{
				_k = _k.substr(1,_k.size()-1);
			}
			break;
		case 't':
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the t option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_t = type;
				if(type<1)
				{
					is_valid = false;
					printf("The %d in t option is not valid\n",type);
				}
			}			
			break;
		case 'r':
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the r option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_r = type;
				if(type<-1 || type == 0)
				{
					is_valid = false;
					printf("The %d in r option is not valid\n",type);
				}
			}			
			break;
		case 'i':
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the i option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_i = type;
				if(type<-1 || _i==0)
				{
					is_valid = false;
					printf("The %d in i option is not valid\n",type);
				}
			}			
			break;
		case 'x':
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the x option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_x = type;
				if(type<-1 || _x==0)
				{
					is_valid = false;
					printf("The %d in x option is not valid\n",type);
				}
			}			
			break;
		case 'w':
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the w option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_w = type;
				if(type<=0)
				{
					is_valid = false;
					printf("The %d in w option is not valid\n",type);
				}
			}			
			break;
		case 'n'://shingle size
			strcpy(_commandParam,pszParam);
			flag = sscanf(_commandParam,"%d",&type);
			if(flag == EOF)
			{
				printf("The %s of the n option is not valid\n",pszParam);
				is_valid = false;
			}
			else
			{
				this->_n = type;
				if(type<-1 || _x==0)
				{
					is_valid = false;
					printf("The %d in n option is not valid\n",type);
				}
			}			
			break;
		case 'o':
			strcpy(_commandParam,pszParam);
			if(strlen(_commandParam)==0)
				is_valid =false;
			else
			{
				sscanf(_commandParam,"%lf",&ftype);
				if(ftype >=0 && ftype<=1)
					this->_o = ftype;
				else
				{
					is_valid = false;
					printf("The %f in o(cosine) option is not valid\n",ftype);
				}
			}
			break;
		case 'l':
			strcpy(_commandParam,pszParam);
			if(strlen(_commandParam)==0)
				is_valid =false;
			else
			{
				sscanf(_commandParam,"%lf",&ftype);
				if(ftype >=0 && ftype<=1)
					this->_l = ftype;
				else
				{
					is_valid = false;
					printf("The %f in l(scale) option is not valid\n",ftype);
				}
			}
			break;
		case 'p':
			strcpy(_commandParam,pszParam);
			if(strlen(_commandParam)==0)
				is_valid =false;
			else
			{
				sscanf(_commandParam,"%lf",&ftype);
				if(ftype >=0 && ftype<=1)
					this->_p = ftype;
				else
				{
					is_valid = false;
					printf("The %f in p(min support) option is not valid\n",ftype);
				}
			}
			break;
		case 'f':
			strcpy(_commandParam,pszParam);
			if(strlen(_commandParam)==0)
				is_valid =false;
			else
			{
				sscanf(_commandParam,"%lf",&ftype);
				if(ftype >=0 && ftype<=1)
					this->_f = ftype;
				else
				{
					is_valid = false;
					printf("The %f in f(force support) option is not valid\n",ftype);
				}
			}
			break;
		case 'z':// max offset
			strcpy(_commandParam,pszParam);
			if(strlen(_commandParam)==0)
				is_valid =false;
			else
			{
				sscanf(_commandParam,"%d",&type);			
				this->_z = type;
				if(type <0)
				{
					is_valid = false;
					printf("The %f in z(max offset) option is not valid\n",type);
				}
			}
			break;
		case 'e':
			_e = pszParam;
			break;
		case 'a'://tag name
			_a = pszParam;
			break;
		}
		result=GetOption(argc, argv, str_opt, &pszParam);
	}
	return (is_valid);
}

void CComOption::GenShowUsage()
{
	printf("======== Powered by Wynter Han. 2011-02-26 :)========\n");
	printf("A tool automatically extract signatures from payload file sets.\n");
	printf("Usage:\n");
	printf("\t -c [0|1|2|3]\n");
	printf("\t\t 0: executes signature extraction from disk path dir.\n");
	printf("\t\t 1: executes signature extraction from pack file.\n");
	printf("\t\t 2: packet payload files.\n");
	printf("\t\t 3: Test signature.\n");

	printf("\t -s [path] source input\n");
	printf("\t\t If c is 0, the option is dir path.\n");
	printf("\t\t If c is 2 or 3, the option is dir index file path.\n");
	
	printf("\t -d [path] dest output\n");
	printf("\t\t If c is 2, the option is target dir path of pack file.\n");

	printf("\t -k [keyword] only load file has the specific keyword\n");
	printf("\t\t If c is 0. It is file name filter. \n");
	printf("\t\t If the first char is '-'. Use'\\' to replace it. \n");

	printf("\t -t [num] openmp threads\n");
	printf("\t\t If c is 0. The threads must not less than 1.\n");

	printf("\t -i [num] minimum size of the file\n");
	printf("\t\t The -1 is default, means no limitation.\n");
	printf("\t\t The file length below the parameter is ignored.\n");

	printf("\t -x [num] maximum size of the file\n");
	printf("\t\t The -1 is default, means no limitation.\n");
	printf("\t\t The file length above the parameter is ignored.\n");

	printf("\t -r [num] retain length of the file\n");
	printf("\t\t Only affects the length of the file above the retain length.\n");
	printf("\t\t If c is 0, the default retain length is 1000.\n");
	printf("\t\t If c is 2, means retain of a flow file in the pack.\n");
	printf("\t\t Otherwise, no limitation as default.\n");

	printf("\t -w [num] half window size\n");
	printf("\t\t The %d is default.\n", HALF_WINDOW_SIZE);

	printf("\t -n [num] shingle size\n");
	printf("\t\t The %d is default.\n", SHINGLE_SIZE);

	printf("\t -a [string] protocol tag\n");
	printf("\t\t The %s is default.\n", DEFAULT_PROTOCOL_TAG);

	printf("\t -e [string] regex file path\n");
	printf("\t\t If c is 3, it defines the regex file.\n");
	printf("\t\t The regex file format: [pro string] [weight] [regex string].\n");

	printf("\t -o [double] -l [double] -p [double] -f [double] -z [int]\n");
	printf("\t\t o is cosine; l is scale; p is min support; f is force support; z is max offset.\n");
	printf("\t\t The default cosine is %f, default scale is %f, default min support threshold is %f, default z is %d\n",
		COSINE, SCALE, MIN_SUPPORT_THRESHOLD, MAX_OFFSET);
	printf("\t\t Default no force support. It is self-adaption.\n");

	printf("Example:\n");
	printf("\t 0. Execute signature extraction from disk path payload files.\n");
	printf("\t The simplest one is \"-s c:\\packetpath\\\"\n");
	printf("\t The advenced one is \"-t 2 -w 10 -n 3 -i 10 -x 100 -r 50\n\t\t -s c:\\packetpath\\\"\n");
	printf("\t The -t 2 means using dual threads\n");
	printf("\t The -w 10 means half window is 10\n");
	printf("\t The -n 3 means shingle size is 3\n");
	printf("\t The -i 10 means min size file is 10\n");
	printf("\t The -x 100 means max size file is 100\n");
	printf("\t The -r 50 means retain length is 50 bt\n");

	printf("\n\t 1. Execute signature extraction from pack payload files.\n");
	printf("\t The simplest one is \"-c 1 -s c:\\indexfile.txt\"\n");

	printf("\n\t 2. Pack some folders.\n\t The dir index file format is:[tag] [folder path]\n");
	printf("\t \"-c 2 -s index.txt -d c:\\packpath\\\" to execute pack task.\n");
	
	printf("\n\t 3. Global Evaluate.\n\t");
	printf("\t \"-c 3 -s index.txt -e regex.txt\" to execute pack task.\n");

}
bool CComOption::GenInputMgr(int argc, char **argv)
{
	string erStr;
	if(!GetGenInput(argc,argv,erStr))
	{
		GenShowUsage();
		printf("%s\n",erStr.c_str());
		return false;
	}
	return true;
}