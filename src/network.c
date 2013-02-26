#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "jmalloc.h"
#include "cevent.h"
#include "cnet.h"
#include "cio.h"
#include "network.h"
#include "log.h"

static int cio_install_read_events(cevents *cevts, cio *io);

static int _reply(cevents *cevts, cio *io);

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
		ERROR("%s\n", buff);
		return -1;
	}
	//TODO: maybe there add cio queue.
	io = cio_create();
	strcpy(io->ip, ip);
	io->port = port;
	io->fd = clifd;
	io->type = IO_TCP;
	cio_install_read_events(cevts, io);
	return 1;
}

static void cio_close_destroy(cevents *evts, cio *io) {
	//cevents_del_event(evts, io->fd, CEV_READ|CEV_WRITE);
	DEBUG("client %s:%d closed\n", io->ip, io->port);
	close(io->fd);
	cio_destroy(io);
}

int reply_str(cevents *cevts, cio *io, char *buff) {
	io->wcount = 0;
	cstr_ncat(io->wbuf, buff, strlen(buff));
	return _reply(cevts, io);
}

int write_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	cio *io = (cio*)priv;
	return _reply(cevts, io);
}

int _reply(cevents *cevts, cio *io) {
	int nwrite;
	while(cstr_used(io->wbuf) != io->wcount) {	
		nwrite = write(io->fd, io->wbuf + io->wcount, cstr_used(io->wbuf) - io->wcount);
		if(nwrite < 0) {
			//continue;
			if(errno == EAGAIN) {
				DEBUG("rebind write event\n");
				cevents_add_event(cevts, io->fd, CEV_WRITE, write_event_proc, io);
				return 0;
			}
			cio_close_destroy(cevts, io);
			return -1;
		}
		io->wcount += nwrite;
	}
	cio_clear(io);
	//cevents_add_event(cevts, io->fd, CEV_READ, read_event_proc, io);
	read_event_proc(cevts, io->fd, io, 0);
	return 0;
}

int process_commond(cevents *cevts, cio *io) {
	size_t nread = cstr_used(io->rbuf);
	char *end;
	if(nread <= 0)
		return -1;
	end = strstr(io->rbuf, "\r\n");
	if(end == NULL) {
		reply_str(cevts, io, "-ERR unknown command\r\n");
		return cevents_add_event(cevts, io->fd, CEV_READ, read_event_proc, io);
	}
	return reply_str(cevts, io, "+pong\r\n");
}

//just for test.
int read_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	cio *io = (cio*)priv;
	int ret, nread;
	char buff[2048];
	nread = read(fd, buff, sizeof(buff));
	if(nread < 0) {
		if(errno == EAGAIN) {
			DEBUG("rebind read event\n");
			return cevents_add_event(cevts, fd, CEV_READ, read_event_proc, io);
		}
		cio_close_destroy(cevts, io);
		return -1;
	}
	if(nread == 0) {
		cio_close_destroy(cevts, io);
		return 0;
	}
	cstr_ncat(io->rbuf, buff, nread);
	if(process_commond(cevts, io) != 0) {
		ERROR("error process command\n");
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
