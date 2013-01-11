#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "jmalloc.h"
#include "cevent.h"
#include "cnet.h"
#include "cio.h"
#include "network.h"

static int cio_install_read_events(cevents *cevts, cio *io);

//always return 0, don't push fired event queue
int tcp_accept_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	char buff[2048];
	char ip[24];
	int port;
	int clifd;
	cio *io;
	memset(buff, 0, sizeof(buff));
	memset(ip, 0, sizeof(ip));
	if((clifd = cnet_tcp_accept(fd, ip, &port, buff, sizeof(buff))) < 0) {
		fprintf(stderr, "%s\n", buff);
		return -1;
	}
	//TODO: maybe there add cio queue.
	io = cio_create();
	io->fd = clifd;
	io->type = IO_TCP;
	cio_install_read_events(cevts, io);
	return 0;
}

static void cio_close_destroy(cevents *evts, cio *io) {
	//del event
	cevents_del_event(evts, io->fd, CEV_READ|CEV_WRITE);
	close(io->fd);
	cio_destroy(io);
}

//disable fire read event for this fd. let the backend thread read.
int event_prev_proc(cevents *cevts, int fd, void *priv, int mask) {
	//TODO: process the ret value.
	int ret;
	ret = cevents_del_event(cevts, fd, CEV_READ|CEV_WRITE);
	return 1;
}

int write_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	int ret;
	cio *io = (cio*)priv;
	while(io->nwrite != io->nread) {
		if(io->nread > 0) {
			ret = write(fd, io->buff + io->nwrite, io->nread - io->nwrite);
			if(ret < 0) {
				//install write event
				if(errno == EAGAIN)
					return cevents_rebind_event(cevts, fd, CEV_WRITE);
				cio_close_destroy(cevts, io);
				return -1;
			}
			io->nwrite += ret;
			if(ret == 0) {
				cio_close_destroy(cevts, io);
				return 0;
			}
		}
	}
	io->nread = 0;
	ret = cevents_rebind_event(cevts, fd, CEV_READ);
	return 0;
}

//just for test.
int read_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	cio *io = (cio*)priv;
	int ret, nread;
	nread = read(fd, io->buff, cstr_len(io->buff));
	if(nread < 0) {
		if(errno == EAGAIN) {
			return cevents_rebind_event(cevts, fd, CEV_READ);
		}
		cio_close_destroy(cevts, io);
		return -1;
	}
	if(nread == 0) {
		cio_close_destroy(cevts, io);
		return 0;
	}
	io->nread += nread;
	io->nwrite = 0;
	cevents_add_event(cevts, io->fd, CEV_WRITE, write_event_proc, io);
	return 0;
}

static int cio_install_read_events(cevents *cevts, cio *io) {
	//TODO: process the ret value.
	int ret;
	cevents_set_master_preproc(cevts, io->fd, event_prev_proc);
	ret = cevents_add_event(cevts, io->fd, CEV_READ, read_event_proc, io);
	return ret;
}

void *process_event(void *priv) {
	int ret;
	cevents *cevts = (cevents*)priv;
	cevent_fired *evt_fired, evt;
	while((evt_fired = (cevent_fired*)cqueue_pop(cevts->fired_queue)) != NULL) {
		if(evt_fired == NULL)
			return NULL;
		//copy it, and destroy it.
		evt = *evt_fired;
		jfree(evt_fired);
		if(evt.mask & CEV_READ) {
			ret = evt.read_proc(cevts, evt.fd, evt.priv, evt.mask);
			if(ret < 0)
				break;
		}
		if(evt.mask & CEV_WRITE) {
			ret = evt.write_proc(cevts, evt.fd, evt.priv, evt.mask);
			if(ret < 0)
				break;
		}
	}
}
