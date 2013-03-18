#ifndef NETWORK_H
#define NETWORK_H

#include "cthread.h"

#define MAX_COMMAND_LEN_LIMIT 0x10240

typedef struct {
	int in_fd;
	int un_fd;
	int logfd;
	pid_t pid;
	uint32_t connections;
	long last_info_time;
	cevents *evts;
	cthr_pool *thr_pool;
} server;

int tcp_accept_event_proc(cevents *cevts, int fd, void *priv, int mask);

void *process_event(void *priv);

#endif /*end define network*/
