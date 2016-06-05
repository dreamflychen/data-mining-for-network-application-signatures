#ifndef _FILE_POOL_H_
#define _FILE_POOL_H_
typedef int* HBUF;
HBUF openbuffer(const char *filename, size_t buffer_size);
size_t writebuffer(const void *buffer, size_t size, HBUF hbf);
int flushbuffer(HBUF hbf);
int closebuffer(HBUF hbf);

#endif