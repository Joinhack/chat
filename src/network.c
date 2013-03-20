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


static int read_event_proc(cevents *cevts, int fd, void *priv, int mask);

static int write_event_proc(cevents *cevts, int fd, void *priv, int mask);

static int _response(cevents *cevts, cio *io);

static int try_process_command(cevents *cevts, cio *io, int mask);

static int (*process_commond)(cevents *cevts, cio *io, int mask);

void set_process_command(int (*p)(cevents *cevts, cio *io, int mask)) {
	process_commond = p;
}

static void install_read_event(cevents *cevts, cio *io) {
	cevents_add_event(cevts, io->fd, CEV_READ|CEV_PERSIST, read_event_proc, io);
}

static void set_protocol_error(cio *io) {
	io->flag |= IOF_CLOSE_AFTER_WRITE;
}

//always return 0, don't push fired event queue
int tcp_accept_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	char buff[2048];
	char ip[24];
	int port;
	int clifd;
	server *svr;
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
	io->priv = priv;
	install_read_event(cevts, io);
	svr = (server*)priv;
	atomic_add_uint32(&svr->connections, 1);
	return 1;
}

int cio_close_destroy(cevents *cevts, int fd, void *priv, int mask) {
	server *svr;
	cio *io = (cio*)priv;
	DEBUG("client %s:%d closed\n", io->ip, io->port);
	cevents_del_event(cevts, io->fd, CEV_READ|CEV_WRITE|CEV_PERSIST);	
	svr = (server*)io->priv;
	atomic_sub_uint32(&svr->connections, 1);
	close(io->fd);
	cio_destroy(io);
	return 1;
}

static void cio_close_destroy_install(cevents *cevts, cio *io) {
	cevents_add_event(cevts, io->fd, CEV_WRITE|CEV_PERSIST,cio_close_destroy, io);
	cevents_del_event(cevts, io->fd, CEV_READ);
}

int response(cevents *cevts, cio *io, int mask) {
	int persist = mask & CEV_PERSIST;
	int rs = _response(cevts, io);

	//if not use CEV_PERSIST, when the event fired, all events is removed, so should rebind write event.
	if(!persist && rs == 1) {
		DEBUG("rebind write event\n");
		cevents_add_event(cevts, io->fd, CEV_WRITE, write_event_proc, io);
		return rs;
	}
	if(rs < 0) {
		cio_clear(io);
		cio_close_destroy_install(cevts, io);
		return rs;
	}

	if(io->flag & IOF_CLOSE_AFTER_WRITE) {
		cio_clear(io);
		cio_close_destroy_install(cevts, io);
		return 0;
	}
	cio_clear(io);

	//if use CEV_PERSIST, we should disable write event fired.
	if(persist) {
		install_read_event(cevts, io);
		cevents_del_event(cevts, io->fd, CEV_WRITE);
	} else {
		//read try again for next request. in most cases, it just rebind the read event.
		read_event_proc(cevts, io->fd, io, 0);
		return 0;
	}
	return 0;
}

int reply_str(cevents *cevts, cio *io, int mask, char *buff) {
	int rs;
	io->wcount = 0;
	cstr_ncat(io->wbuf, buff, strlen(buff));
	//if not persist we direct to reponse the result, else use main thread for io.
	if(!(mask & CEV_PERSIST)) {
		rs = response(cevts, io, mask);
		if(rs) return rs;
	} else {
		cevents_add_event(cevts, io->fd, CEV_WRITE, write_event_proc, io);
	}
	return 0;
}

int write_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	int rs;
	cio *io = (cio*)priv;
	return response(cevts, io, mask);
}

int _response(cevents *cevts, cio *io) {
	int nwrite;
	while(cstr_used(io->wbuf) != io->wcount) {	
		nwrite = write(io->fd, io->wbuf + io->wcount, cstr_used(io->wbuf) - io->wcount);
		if(nwrite < 0) {
			//continue;
			if(errno == EAGAIN) {
				return 1;
			}
			return -1;
		}
		io->wcount += nwrite;
	}
	return 0;
}

int _read_process(cevents *cevts, cio *io, int mask) {
	char buf[2048];
	int rs, nread;
	nread = read(io->fd, buf, sizeof(buf));
	if(nread < 0) {
		if(errno == EAGAIN) {
			return 1;
		}
		return -1;
	}
	if(nread == 0) {
		return -1;
	}
	cstr_ncat(io->rbuf, buf, nread);
	rs = try_process_command(cevts, io, mask);
	if(rs) return rs;
	return 0;
}

static int try_process_command(cevents *cevts, cio *io, int mask) {
	size_t nread = cstr_used(io->rbuf);
	char *end;
	if(nread <= 0)
		return -1;
	if(cstr_used(io->rbuf) > MAX_COMMAND_LEN_LIMIT) {
		set_protocol_error(io);
		reply_str(cevts, io, mask, "-ERR reach the max command recv limit\r\n");
		return -1;
	}
	end = strstr(io->rbuf, "\r\n");
	if(end == NULL) {
		return 1;
	}
	io->argv = cstr_split(io->rbuf, nread, " ", 1, &io->argc);
	return 0;
}

//just for test.
int read_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	cio *io = (cio*)priv;
	int rs, persist = (mask & CEV_PERSIST);
	rs = _read_process(cevts, io, mask);
	if(!persist && rs == 1) {
		install_read_event(cevts, io);
		return 1;
	}
	if(rs < 0) {
		//if not after write, will be closed.
		if(!(io->flag & IOF_CLOSE_AFTER_WRITE))
			cio_close_destroy_install(cevts, io);
		return rs;
	}
	//if not CEV_PERSIST event, we should process command.
	if(!persist) {
		process_commond(cevts, io, mask);
	}
	return 0;
}


