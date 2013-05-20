#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "log.h"
#include "jmalloc.h"
#include "cevent.h"
#include "code.h"

static int cevents_create_priv_impl(cevents *cevts);
static int cevents_destroy_priv_impl(cevents *cevts);
static int cevents_add_event_impl(cevents *cevts, int fd, int mask);
static int cevents_del_event_impl(cevents *cevts, int fd, int mask);
static int cevents_poll_impl(cevents *cevts, msec_t ms);
static int cevents_enable_event_impl(cevents *cevts, int fd, int mask);
static int cevents_disable_event_impl(cevents *cevts, int fd, int mask);
static int fired_preproc(cevents *cevts, cevent_fired *fired, int *count);

#ifdef USE_EPOLL
#include "cevent_epoll.c"
#elif defined(USE_KQUEUE)
#include "cevent_kqueue.c"
#else
#include "cevent_select.c"
#endif

void cevents_push_fired(cevents *cevts, cevent_fired *fired) {
	LOCK(&cevts->qlock);
	clist_lpush(cevts->events[fired->fd].fired_queue, (void*)fired);
	int *fd = jmalloc(sizeof(int));
	*fd = fired->fd;
	clist_lpush(cevts->fired_fds, (void*)fd);
	UNLOCK(&cevts->qlock);
}

static int fired_event_destroy(void *d, void *p) {
	jfree(d);
	return 0;
}

int cevents_clear_fired_events(cevents *cevts, int fd) {
	LOCK(&cevts->qlock);
	clist_walk_remove(cevts->events[fd].fired_queue, fired_event_destroy, NULL);
	UNLOCK(&cevts->qlock);
	return 0;
}

int cevents_pop_fired(cevents *cevts, cevent_fired *fired) {
	cevent_fired *fevt;
	LOCK(&cevts->qlock);
	int *fd = (int*)clist_rpop(cevts->fired_fds);
	if(fd == NULL) {
		UNLOCK(&cevts->qlock);
		return 0;
	}
	fevt = (cevent_fired*)clist_rpop(cevts->events[*fd].fired_queue);
	jfree(fd);
	UNLOCK(&cevts->qlock);
	if(fevt == NULL) return 0;
	*fired = *fevt;
	jfree(fevt);
	return 1;
}

cevents *cevents_create() {
	cevents *evts;
	int len;
	len = sizeof(cevents);
	evts = (cevents *)jmalloc(len);
	memset((void *)evts, len, 0);
	evts->events = jmalloc(sizeof(cevent) * MAX_EVENTS);
	evts->fired_fds = jmalloc(sizeof(int) * MAX_EVENTS);
	for(size_t i = 0; i < MAX_EVENTS; i++) {
		evts->events[i].fired_queue = clist_create();
	}
	evts->fired_fds = clist_create();
	LOCK_INIT(&evts->qlock);
	LOCK_INIT(&evts->lock);
	cevents_create_priv_impl(evts);
	evts->poll_sec = 0;
	evts->poll_ms = 0;
	evts->timers = timer_base_create();
	return evts;
}

void cevents_destroy(cevents *cevts) {
	if(cevts == NULL)
		return;
	if(cevts->events != NULL) {
		for(size_t i = 0; i < MAX_EVENTS; i++) {
			cevents_clear_fired_events(cevts, i);
			clist_destroy(cevts->events[i].fired_queue);
		}
		jfree(cevts->events);
	}
	if(cevts->fired_fds != NULL)
		clist_destroy(cevts->fired_fds);
	if(cevts->timers != NULL)
		timer_base_destroy(cevts->timers);
	LOCK_DESTROY(&cevts->lock);
	LOCK_DESTROY(&cevts->qlock);
	cevts->events = NULL;
	cevts->fired_fds = NULL;
	cevts->timers = NULL;
	//cevts->fired_queue = NULL;
	cevents_destroy_priv_impl(cevts);
	jfree(cevts);
}

int cevents_add_event_inner(cevents *cevts, int fd, int mask, event_proc *proc, void *priv) {
	int rs;
	cevent *evt;
	if(fd > MAX_EVENTS)
		return J_ERR;
	evt = cevts->events + fd;
	if((rs = cevents_add_event_impl(cevts, fd, mask)) < 0) {
		ERROR("fd %d add event error:%s\n", fd, strerror(errno));
		return rs;
	}
	if(mask & CEV_READ) evt->read_proc = proc;
	if(mask & CEV_WRITE) evt->write_proc = proc;
	evt->mask |= mask;
	if(fd > cevts->maxfd && evt->mask != CEV_NONE) 
		cevts->maxfd = fd;
	evt->priv = priv;
	return 0;
}

int cevents_add_event(cevents *cevts, int fd, int mask, event_proc *proc, void *priv) {
	int rs;
	LOCK(&cevts->lock);
	rs = cevents_add_event_inner(cevts, fd, mask, proc, priv);
	UNLOCK(&cevts->lock);
	return rs;
}

int cevents_del_event_inner(cevents *cevts, int fd, int mask) {
	int j;
	if(fd > MAX_EVENTS)
		return J_ERR;
	cevent *evt = cevts->events + fd;
	//don't unbind the method, maybe should be used again.

	if (evt->mask == CEV_NONE) return 0;
	
	//ignore error
	if(cevents_del_event_impl(cevts, fd, mask) < 0) {
		ERROR("del event error:%s\n", strerror(errno));
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

int cevents_del_event(cevents *cevts, int fd, int mask) {
	int rs;
	LOCK(&cevts->lock);
	rs = cevents_del_event_inner(cevts, fd, mask);
	UNLOCK(&cevts->lock);
	return rs;
}

static cevent_fired *clone_cevent_fired(cevent_fired *fired) {
	cevent_fired *new_cevent_fired = jmalloc(sizeof(cevent_fired));
	*new_cevent_fired = *fired;
	return new_cevent_fired;
}

static int fired_preproc(cevents *cevts, cevent_fired *fired, int *count) {
	cevent *evt = cevts->events + fired->fd;
	if(!evt->mask) {
		return 1;
	}
	if(evt->mask & CEV_TIMEOUT) fired->mask |= CEV_TIMEOUT;
	if(evt->mask & CEV_PERSIST) {
		fired->mask |= CEV_PERSIST;

		if(evt->mask && (fired->mask & CEV_READ)) {
			//just send read event to event queue.
			UNLOCK(&cevts->lock);
			if(evt->read_proc(cevts, fired->fd, evt->priv, fired->mask) == 0) {
				cevents_push_fired(cevts, clone_cevent_fired(fired));
				(*count)++;
			}
			LOCK(&cevts->lock);
		}
		if(evt->mask && (fired->mask & CEV_WRITE)) {
			UNLOCK(&cevts->lock);
			evt->write_proc(cevts, fired->fd, evt->priv, fired->mask);
			LOCK(&cevts->lock);
		}
	} else {
		UNLOCK(&cevts->lock);
		//unbind the events.
		cevents_del_event(cevts, fired->fd, fired->mask);
		LOCK(&cevts->lock);
		cevents_push_fired(cevts, clone_cevent_fired(fired));
		(*count)++;
	}
	return 0;
}

//return push queue count or J_ERR
int cevents_poll(cevents *cevts, msec_t ms) {
	int rs, i, count = 0, flag = 0;
	cevent_fired *fired;
	cevent *evt;
	if(cevts == NULL) {
		ERROR("can't be happend\n");
		abort();
	}
	LOCK(&cevts->lock);
	count = cevents_poll_impl(cevts, ms);
	UNLOCK(&cevts->lock);
	time_now(&cevts->poll_sec, &cevts->poll_ms);
	timer_set_jiffies(cevts->timers, cevts->poll_sec*1000+cevts->poll_ms);
	timer_run(cevts->timers);
	//must sleep, let other thread grant the lock. maybe removed when the time event added.
	usleep(2);
	return count;
}
