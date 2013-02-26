#ifndef NETWORK_H
#define NETWORK_H

#include "cthread.h"

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

int cio_install_read_event(cevents *cevts, cio *io);

int read_event_proc(cevents *cevts, int fd, void *priv, int mask);

int reply_str(cevents *cevts, cio *io, char *buff);

void *process_event(void *priv);

#endif /*end define network*/
