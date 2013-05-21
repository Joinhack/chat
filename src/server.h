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

#ifdef __cplusplus
extern "C" {
#endif

void pong(cio *io);

void select_table(cio *io);

void set_command(cio *io);

void get_command(cio *io);

void del_command(cio *io);

void info_command(cio *io);

void dump_command(cio *io);

void load_command(cio *io);

#ifdef __cplusplus
}
#endif

#endif /**end define server*/
