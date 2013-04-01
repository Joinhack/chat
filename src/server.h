#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "atomic.h"
#include "log.h"
#include "jmalloc.h"
#include "code.h"
#include "cevent.h"
#include "cnet.h"
#include "cio.h"
#include "cthread.h"
#include "network.h"
#include "obj.h"
#include "db.h"

typedef struct {
	int in_fd;
	int un_fd;
	int logfd;
	pid_t pid;
	uint32_t connections;
	long last_info_time;
	cevents *evts;
	dict *commands;
	cthr_pool *thr_pool;
	db *db;
} server;

void pong(cio *io);

void pong(cio *io);

void select_table(cio *io);

void set_command(cio *io);

void get_command(cio *io);

#endif /**end define server*/
