#ifndef CEVENT_H
#define CEVENT_H

#include "common.h"
#include "clist.h"
#include "lock.h"
#include "ctimer.h"

#define MAX_EVENTS (10240*20)
#define CEV_NONE 0x0
#define CEV_TIMEOUT 0x1
#define CEV_READ 0x1<<1
#define CEV_WRITE 0x1<<2
//persist mean don't remove event after poll, implicitly use main thread for read io , parse command and write reponse, except the process command. else push the events to backend, let backend thread process. 
#define CEV_PERSIST 0x1<<3

typedef struct cevents cevents;

typedef int event_proc(cevents *evts, int fd, void *priv, int mask);

typedef struct cevent {
	int mask;
	event_proc *read_proc;
	event_proc *write_proc;
	void *priv;
	clist *fired_queue;
} cevent;

typedef struct {
	int mask;
	int fd;
} cevent_fired;

struct cevents {
	int maxfd;
	cevent *events; //should be MAX_EVENTS
	clist *fired_fds; //should be MAX_EVENTS, push to top level
	LOCK_T qlock;
	LOCK_T lock;
	long poll_sec;
	int poll_ms;
	char impl_name[64];
	ctimer_base *timers;
	void *priv_data; //use for implement data.
};

#ifdef __cplusplus
extern "C" {
#endif
cevents *cevents_create();
void cevents_destroy(cevents *cevts);
int cevents_add_event(cevents *cevts, int fd, int mask, event_proc *proc, void *priv);
int cevents_del_event(cevents *cevts, int fd, int mask);
int cevents_poll(cevents *cevts, msec_t ms);
void cevents_push_fired(cevents *cevts, cevent_fired *fired);
int cevents_pop_fired(cevents *cevts, cevent_fired *fired);
int cevents_clear_fired_events(cevents *cevts, int fd);
uint64_t ctimer_get_jiffies(ctimer_base *b);
#ifdef __cplusplus
}
#endif

#endif /*end define cevent**/
