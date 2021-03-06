#include <stdlib.h>
#include <sys/select.h>
#include "jmalloc.h"

typedef struct {
	fd_set wfds;
	fd_set rfds;
} rwfd_set;

static int cevents_create_priv_impl(cevents *cevts) {
	rwfd_set *rwfds = jmalloc(sizeof(rwfd_set));
	FD_ZERO(&rwfds->rfds);
	FD_ZERO(&rwfds->wfds);
	snprintf(cevts->impl_name, sizeof(cevts->impl_name), "%s", "select");
	cevts->priv_data = rwfds;
	return 0;
}

static int cevents_destroy_priv_impl(cevents *cevts) {
	jfree(cevts->priv_data);
	cevts->priv_data = NULL;
	return 0;
}

static int cevents_add_event_impl(cevents *cevts, int fd, int mask) {
	rwfd_set *rwfds = (rwfd_set*)cevts->priv_data;
	if(mask & CEV_READ) FD_SET(fd, &rwfds->rfds);
	if(mask & CEV_WRITE) FD_SET(fd, &rwfds->wfds);
	return 0;
}

static int cevents_del_event_impl(cevents *cevts, int fd, int mask) {
	rwfd_set *rwfds = (rwfd_set*)cevts->priv_data;
	if(mask & CEV_READ) FD_CLR(fd, &rwfds->rfds);
	if(mask & CEV_WRITE) FD_CLR(fd, &rwfds->wfds);
	return 0;
}

static int cevents_poll_impl(cevents *cevts, msec_t ms) {
	rwfd_set *rwfds = (rwfd_set*)cevts->priv_data;
	fd_set work_rfds, work_wfds;
	cevent *event;
	int rs, i;
	int mask = 0;
	int count = 0;
	struct timeval tv;

	tv.tv_sec = (long) (ms / 1000);
  tv.tv_usec = (long) ((ms % 1000) * 1000);

	//copy the read write fd_set, if use original, will have problem.
	work_rfds = rwfds->rfds;
	work_wfds = rwfds->wfds;
	rs = select(cevts->maxfd+1, &work_rfds, &work_wfds, NULL, &tv);
	if(rs > 0) {
		for(i = 0; i <= cevts->maxfd; i++) {
			cevent_fired fired = {0};
			mask = CEV_NONE;
			event = cevts->events + i;
			if(event->mask == CEV_NONE)
				continue;
			if((event->mask & CEV_WRITE) && FD_ISSET(i, &work_wfds))
				mask |= CEV_WRITE;
			if((event->mask & CEV_READ) && FD_ISSET(i, &work_rfds))
				mask |= CEV_READ;
			fired.fd = i;
			fired.mask = mask;
			fired_preproc(cevts, &fired, &count);
		}
	}
	return count;
}
