#ifndef CONSTDEFINE_H
#define CONSTDEFINE_H


#define TEXT_REWRITE						"wt+"				
#define TEXT_APPEND							"at+"				
#define TEXT_READ							"rt"

#define BINARY_REWRITE						"wb+"
#define BINARY_APPEND						"ab+"
#define BINARY_READ							"rb"


#define PAYLOAD_EXTENSION					"payload"			
#define LOGFILE_MODE						TEXT_APPEND			
#define DEFAULT_PROTOCOL_TAG				"test"				


#define BEGIN_ADD_FILE						0					
#define END_ADD_FILE						1					
#define ADD_FILE							2					
#define OMIT_FILE							4					
#define DELTA_FILE_NOTIFY_SIZE				1000				


#define CLOCK_STOP							0					
#define CLOCK_RUN							1					


#define RETAIN_LENGTH						1000				


#define HALF_WINDOW_SIZE					100					
#define SHINGLE_SIZE						3					
#define MIN_SUPPORT_THRESHOLD				0.1					
#define ENABLE_FACTOR											








#define MIN_SUPPORT							0.3					
#define COSINE								0.3					
#define SCALE								0.5					
#define MAX_OFFSET							5					


#define IS_CANDIDATE(x)		(((unsigned int)(x))>>31)					
#define GET_COUNT(x)		((x)&0x7FFFFFFF)							


#define MIN_LAYER_THRESHOLD					0.3						
#define LAYER_CLASS							0							
#define TREE_CLASS							1							


#define MAX_KINDS							6							
#define TERMINATE_RATE						0.6						
#define FACTOR_SCALE						0.9
#define HEURISTIC_RATE						0.0							


#define BUFFER_SIZE							1024						
#define INDEX_FILE_PERFIX					"index_"					
#define PACK_FILE_PERFIX					"pack_"						
#define PACK_EXTENSION						".paypack"

#define PARAM_BUFFER_SIZE					1024						
#endif