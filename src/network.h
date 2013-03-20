#ifndef NETWORK_H
#define NETWORK_H

#include "cthread.h"
#include "obj.h"

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

int reply_str(cevents *cevts, cio *io, char *buff);

int reply_obj(cevents *cevts, cio *io, obj *obj);

int reply_cstr(cevents *cevts, cio *io, cstr s);

int process_commond(cevents *cevts, cio *io);

void shared_obj_create();

void *process_event(void *priv);

#endif /*end define network*/
