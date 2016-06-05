/*
  Copyright (c) 1999 Rafal Wojtczuk <nergal@7bulls.com>. All rights reserved.
  See the file COPYING for license details.
*/
#ifndef _NIDS_TCP_H
#define _NIDS_TCP_H
#ifdef WIN32
	#include <time.h>
	#include <winsock.h>
#else
	#include <sys/time.h>
#endif

struct skbuff {
  struct skbuff *next;
  struct skbuff *prev;

  void *data;
  u_int len;
  u_int truesize;
  u_int urg_ptr;
  
  char fin;
  char urg;
  u_int seq;
  u_int ack;
};

int tcp_init(int);
void tcp_exit(void);
void process_tcp(u_char *, int);
void process_icmp(u_char *);
void tcp_check_timeouts(struct timeval *);

#endif /* _NIDS_TCP_H */
