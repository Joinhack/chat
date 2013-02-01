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
	return 1;
}

static void cio_close_destroy(cevents *evts, cio *io) {
	//del event
	cevents_del_event(evts, io->fd, CEV_READ|CEV_WRITE);
	close(io->fd);
	cio_destroy(io);
}

int write_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	int ret;
	cio *io = (cio*)priv;
	if(io->nread > 0) {
		while(io->nwrite != 0) {	
			ret = write(fd, io->buff + io->nwrite, io->nread - io->nwrite);
			if(ret < 0) {
				//continue;
				if(errno == EAGAIN)
					return 0;
				cio_close_destroy(cevts, io);
				return -1;
			}
			io->nwrite += ret;
			if(ret == 0) {
				cio_close_destroy(cevts, io);
				return 0;
			}
		}
		cevents_del_event(cevts, fd, CEV_WRITE);
	}
	cevents_add_event(cevts, fd, CEV_READ, read_event_proc, io);
	io->nread = 0;
	return 0;
}

int process_commond(cio) {

}

//just for test.
int read_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	cio *io = (cio*)priv;
	int ret, nread;
	char buff[2048];
	nread = read(fd, buff, sizeof(buff));
	if(nread < 0) {
		if(errno == EAGAIN) {
			return cevents_add_event(cevts, fd, CEV_READ, read_event_proc, io);
		}
		cio_close_destroy(cevts, io);
		return -1;
	}
	if(nread == 0) {
		cio_close_destroy(cevts, io);
		return 0;
	}
	cstr_ncat(io->readbuf, buff, nread);
	if(process_commond(io) != 0) {
		fprintf(stderr, "error process command\n");
		return -1;
	}
	return 0;
}

static int cio_preproc(cevents *cevts, int fd, void *priv, int mask) {
	return cevents_del_event(cevts, fd, CEV_READ|CEV_WRITE);
}

static int cio_install_read_events(cevents *cevts, cio *io) {
	//TODO: process the ret value.
	int rs;
	cevents_set_master_preproc(cevts, io->fd, cio_preproc);
	rs = cevents_add_event(cevts, io->fd, CEV_READ, read_event_proc, io);
	return rs;
}

void *process_event(void *priv) {
	int ret;
	cevents *cevts = (cevents*)priv;
	cevent_fired *evt_fired, evt;
	while(1) {
		evt_fired = (cevent_fired*)cevents_pop_fired(cevts);
		if(evt_fired == NULL)
			return NULL;
		//copy it, and destroy it.
		evt = *evt_fired;
		jfree(evt_fired);

		if(evt.mask & CEV_READ) {
			evt.read_proc(cevts, evt.fd, evt.priv, evt.mask);
		}
		if(evt.mask & CEV_WRITE) {
			evt.write_proc(cevts, evt.fd, evt.priv, evt.mask);
		}
	}
	return NULL;
}
