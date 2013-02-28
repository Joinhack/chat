#ifndef CEVENT_H
#define CEVENT_H

#include "common.h"
#include "cqueue.h"
#include "lock.h"

#define MAX_EVENTS (10240*20)
#define CEV_NONE 0x0
#define CEV_READ 0x1
#define CEV_WRITE 0x1<<1
//persis mean don't remove event after poll, implicitly use main thread process.
#define CEV_PERSIST 0x1<<2

typedef struct _cevents cevents;

typedef int event_proc(cevents *evts, int fd, void *priv, int mask);

typedef struct {
	int mask;
	event_proc *read_proc;
	event_proc *write_proc;
	void *priv;
} cevent;

typedef struct {
	int mask;
	int fd;
	event_proc *read_proc;
	event_proc *write_proc;
	void *priv;
} cevent_fired;

struct _cevents {
	int maxfd;
	cevent *events; //should be MAX_EVENTS
	cevent_fired *fired; //should be MAX_EVENTS, push to top level
	cqueue *fired_queue;
	LOCK_T qlock;
	LOCK_T lock;
	long poll_sec;
	int poll_ms;
	char impl_name[64];
	void *priv_data; //use for implement data.
};

cevents *cevents_create();
void cevents_destroy(cevents *cevts);
int cevents_add_event(cevents *cevts, int fd, int mask, event_proc *proc, void *priv);
int cevents_del_event(cevents *cevts, int fd, int mask);
int cevents_poll(cevents *cevts, msec_t ms);
void cevents_push_fired(cevents *cevts, cevent_fired *fired);
cevent_fired *cevents_pop_fired(cevents *cevts);


#endif /*end define cevent**/
