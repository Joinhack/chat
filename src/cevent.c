#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "cevent.h"
#include "jmalloc.h"
#include "code.h"

static int cevents_create_priv_impl(cevents *cevts);
static int cevents_destroy_priv_impl(cevents *cevts);
static int cevents_add_event_impl(cevents *cevts, int fd, int mask);
static int cevents_del_event_impl(cevents *cevts, int fd, int mask);
static int cevents_poll_impl(cevents *cevts, msec_t ms);
static int cevents_enable_event_impl(cevents *cevts, int fd, int mask);
static int cevents_disable_event_impl(cevents *cevts, int fd, int mask);

#ifdef USE_EPOLL
#include "cevent_epoll.c"
#elif defined(USE_KQUEUE)
#include "cevent_kqueue.c"
#else
#include "cevent_select.c"
#endif

void cevents_push_fired(cevents *cevts, cevent_fired *fired) {
	spinlock_lock(&cevts->qlock);
	cqueue_push(cevts->fired_queue, (void*)fired);
	spinlock_unlock(&cevts->qlock);
}

cevent_fired *cevents_pop_fired(cevents *cevts) {
	cevent_fired *fevt;
	spinlock_lock(&cevts->qlock);
	fevt = (cevent_fired*)cqueue_pop(cevts->fired_queue);
	spinlock_unlock(&cevts->qlock);
	return fevt;
}

cevents *cevents_create() {
	cevents *evts;
	int len;
	len = sizeof(cevents);
	evts = (cevents *)jmalloc(len);
	memset((void *)evts, len, 0);
	evts->events = jmalloc(sizeof(cevent) * MAX_EVENTS);
	evts->fired = jmalloc(sizeof(cevent_fired) * MAX_EVENTS);
	evts->fired_queue = cqueue_create();
	evts->qlock = SL_UNLOCK;
	cevents_create_priv_impl(evts);
	return evts;
}

void cevents_destroy(cevents *cevts) {
	if(cevts == NULL)
		return;
	if(cevts->events != NULL)
		jfree(cevts->events);
	if(cevts->fired != NULL)
		jfree(cevts->fired);
	if(cevts->fired_queue != NULL)
		cqueue_destroy(cevts->fired_queue);
	cevts->qlock = SL_UNLOCK;
	cevts->events = NULL;
	cevts->fired = NULL;
	cevts->fired_queue = NULL;
	cevents_destroy_priv_impl(cevts);
	jfree(cevts);
}

int cevents_add_event(cevents *cevts, int fd, int mask, event_proc *proc, void *priv) {
	int rs;
	cevent *evt;
	if(fd > MAX_EVENTS)
		return J_ERR;
	evt = cevts->events + fd;
	if((rs = cevents_add_event_impl(cevts, fd, mask)) < 0) {
		fprintf(stderr, "add event error:%s\n", strerror(errno));
		return rs;
	}
	if(mask & CEV_READ) evt->read_proc = proc;
	if(mask & CEV_WRITE) evt->write_proc = proc;
	evt->mask |= mask;
	if(fd > cevts->maxfd && evt->mask != CEV_NONE) 
		cevts->maxfd = fd;
	evt->priv = priv;
	return J_OK;
}

int cevents_del_event(cevents *cevts, int fd, int mask) {
	int j;
	if(fd > MAX_EVENTS)
		return J_ERR;
	cevent *evt = cevts->events + fd;
	//don't unbind the method, maybe should be used again.

	if (evt->mask == CEV_NONE) return 0;

	//ignore error
	if(cevents_del_event_impl(cevts, fd, mask) < 0) {
		fprintf(stderr, "del event error:%s\n", strerror(errno));
	}
	evt->mask &= ~mask; //remove mask
	//change maxfd
	if(cevts->maxfd == fd && evt->mask == CEV_NONE) {
		for(j = cevts->maxfd - 1; j >= 0; j--) {
			if(cevts->events[j].mask != CEV_NONE) {
				cevts->maxfd = j;
				break;
			}
		}
	}
	return 0;
}

int cevents_rebind_event(cevents *cevts, int fd, int mask) {
	cevent *evt = cevts->events + fd;
	int rs;
	if(mask & CEV_READ) {
		if((rs = cevents_add_event(cevts, fd, CEV_READ, evt->read_proc, evt->priv)) < 0) {
			fprintf(stderr, "rebind add event error:%s\n", strerror(errno));
			return rs;
		}
	}
	if(mask & CEV_WRITE) {
		if((rs = cevents_add_event(cevts, fd, CEV_WRITE, evt->write_proc, evt->priv)) < 0) {
			fprintf(stderr, "rebind add event error: %s\n", strerror(errno));
			return rs;
		}
	}
	return 0;
}

static cevent_fired *clone_cevent_fired(cevents *cevts, cevent_fired *fired) {
	cevent *cevent = cevts->events + fired->fd;
	cevent_fired *new_cevent_fired = jmalloc(sizeof(cevent_fired));
	new_cevent_fired->fd = fired->fd;
	new_cevent_fired->mask = fired->mask;
	return new_cevent_fired;
}

//return push queue count or J_ERR
int cevents_poll(cevents *cevts, msec_t ms) {
	int ret, i, count = 0;
	cevent_fired *fired;
	cevent *evt;
	if(cevts == NULL) {
		fprintf(stderr, "can't be happend\n");
		abort();
	}
	ret = cevents_poll_impl(cevts, ms);
	if(ret > 0) {
		for(i = 0; i < ret; i++) {
			fired = cevts->fired + i;
			evt = cevts->events + fired->fd;
			printf("%d\n", fired->mask);
			if(fired->mask & CEV_READ) {
				evt->read_proc(cevts, fired->fd, evt->priv, fired->mask);
			}
			if(fired->mask & CEV_WRITE) {
				evt->write_proc(cevts, fired->fd, evt->priv, fired->mask);
			}
			count++;
		}
	}
	return count;
}
