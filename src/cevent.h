#ifndef CEVENT_H
#define CEVENT_H

#include "common.h"
#include "cqueue.h"
#include "spinlock.h"

#define MAX_EVENTS (10240*20)
#define CEV_NONE 0x0
#define CEV_READ 0x1
#define CEV_WRITE 0x1<<1

typedef struct _cevents cevents;

typedef int event_proc(cevents *evts, int fd, void *priv, int mask);

typedef struct {
	int mask;
	event_proc *read_proc;
	event_proc *write_proc;
	event_proc *master_preproc;
	void *priv;
} cevent;

typedef struct {
	int mask;
	int fd;
} cevent_fired;

struct _cevents {
	int maxfd;
	cevent *events; //should be MAX_EVENTS
	cevent_fired *fired; //should be MAX_EVENTS, push to top level
	cqueue *fired_queue;
	cqueue *event_ops_queue;
	spinlock_t oqlock;
	spinlock_t qlock;
	spinlock_t lock;
	char impl_name[64];
	void *priv_data; //use for implement data.
};


#define OP_ADD 0x1
#define OP_DEL 0x1<<1
#define OP_MASTER_PREPROC 0x1<<2

typedef struct {
	int mask;
	event_proc *proc;
	void *priv;
	int operation;
} cevent_ops;

cevents *cevents_create();
void cevents_destroy(cevents *cevts);
int cevents_add_event(cevents *cevts, int fd, int mask, event_proc *proc, void *priv);
int cevents_del_event(cevents *cevts, int fd, int mask);
int cevents_poll(cevents *cevts, msec_t ms);
void cevents_set_master_preproc(cevents *cevts, int fd, event_proc *master_preproc);
void cevents_push_fired(cevents *cevts, cevent_fired *fired);
cevent_fired *cevents_pop_fired(cevents *cevts);


#endif /*end define cevent**/
