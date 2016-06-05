#include "filepool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TO_PBF(X) ((buffer_file*)(X))

struct buffer_file
{
	char *buffer;
	size_t alloc_size;
	size_t len;
	char *filename;
};

void free_buffer(buffer_file *pbf)
{
	delete [] pbf->buffer;
	delete [] pbf->filename;
	delete pbf;
}

HBUF openbuffer(const char *filename, size_t buffer_size)
{
	//create buffer, init buffer, create or rewrite files
	//close file, end init
	FILE *fp = fopen(filename,"wb");
	if(!fp)
		return 0;
	fclose(fp);
	size_t len = strlen(filename);
	buffer_file *pbf = new buffer_file;
	pbf->filename = new char[len+1];
	strcpy(pbf->filename, filename);
	pbf->alloc_size = buffer_size;
	pbf->buffer = new char[buffer_size];
	pbf->len = 0;	
	return (int*)pbf;
}

size_t writebuffer(const void *buffer, size_t size, HBUF hbf)
{
	//write to buffer, if the bffer is full, swap it to file
	buffer_file *bpf = TO_PBF(hbf);
	size_t allsize = bpf->len + size;
	if(allsize == 0)
		return 0;
	char *pos;
	if(allsize < bpf->alloc_size)
	{
		pos = bpf->buffer + bpf->len;
		memcpy(pos, buffer, size);
		bpf->len += size;
	}
	else
	{
		//swap to file
		FILE *fp = fopen(bpf->filename,"ab");
		if(!fp)
		{
			printf("Write buffer, open %s failed!", bpf->filename);
			exit(-1);
		}
		fwrite(bpf->buffer, sizeof(char), bpf->len, fp);
		fwrite(buffer, sizeof(char), size, fp);
		fclose(fp);
		bpf->len = 0;
	}
	return allsize;
}

int flushbuffer(HBUF hbf)
{
	buffer_file *bpf = TO_PBF(hbf);
	if(!bpf->len)
		return 0;
	//append to file, then clean buffer
	FILE *fp = fopen(bpf->filename,"ab");
	if(!fp)
	{
		printf("Flush buffer, open %s failed!", bpf->filename);
		exit(-1);
	}
	fwrite(bpf->buffer, sizeof(char), bpf->len, fp);
	fclose(fp);
	bpf->len = 0;
	return 0;
}

int closebuffer(HBUF hbf)
{
	//close file and clean buffer
	flushbuffer(hbf);
	free_buffer(TO_PBF(hbf));
	return 0;
}