/*
Program feature
Input:
	read all pcap file in a folder
	trancate length
	udp timeout mode
	tcp flow mode
	udp flow mode
	Input format:
		-s source_folder -d dest_folder -l trancate_length -o [udp/tcp timeout value] -t [0|1|2] -u [0|1|2] udp initial host name
	Required parameters
		-s -d
	Optinal parameters
		-l default setting does not trancate length
		-o no timeout by default
		-t  0 means does not output tcp flow
			1 means only merge packets, but does not reassemble tcp session
			2 re-assemble tcp session
			3 output non-assembled tcp and assembled tcp (default)
		-u 0 means does not output udp
			1 means using timeout method
			2 means each udp is a session
			3 means output merged and unmerged udp (default)
		-i the host name for udp initialization
			
Process:
	Output assembled payload and no-assembled payload for tcp flow
	OUtput each individual packet as a payload, and aggretage all packets in the timeout threshold as a payload

Output:
	Use 0 to denote normal merge
	User 1 to denote tcp assembly
	Use 1 to denote udp packet as session
	(is re-assemble 0|1)(protocol tcp|udp)-{serial id}
	-{connection begin time 20080901@10#11#12#1234}
	-{connection end time 20080901@10#11#12#1244}
	-{running time}
	-{actual size}
	-{trancated size}
	-{initiator ip 10.1.1.1}@{initiator port 1234}
	#{direction i initiator->destination r destination->initiator u unknown-direction}
	{server side ip 10.1.1.2}@{service port 1235}
	.payload
	output log file, record basic information( the number of created files, the selected parameters, and name of files)
	the log file is named "log.txt"
	The length of file name is less than 143
*/
#include "genhandle.h"
#include "gensys.h"
int main(int argc, char* argv[])
{
	if(!gen_input_mgr(argc,argv))
		return 0;
	gensys_start();
	return 0;
}
