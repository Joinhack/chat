#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "server.h"


static int read_event_proc(cevents *cevts, int fd, void *priv, int mask);

static int write_event_proc(cevents *cevts, int fd, void *priv, int mask);

static int _response(cio *io);

static int try_process_command(cio *io);

static void install_read_event(cevents *cevts, cio *io) {
	cevents_add_event(cevts, io->fd, CEV_READ|CEV_PERSIST, read_event_proc, io);
}

void set_protocol_error(cio *io) {
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

static void cio_close_destroy_install(cio *io) {
	cevents *cevts = ((server*)io->priv)->evts;
	cevents_add_event(cevts, io->fd, CEV_WRITE|CEV_PERSIST,cio_close_destroy, io);
	cevents_del_event(cevts, io->fd, CEV_READ);
}

int response(cio *io) {
	cevents *cevts = ((server*)io->priv)->evts;
	int persist = io->mask & CEV_PERSIST;
	int rs = _response(io);

	//if not use CEV_PERSIST, when the event fired, all events is removed, so should rebind write event.
	if(!persist && rs == 1) {
		DEBUG("rebind write event\n");
		cevents_add_event(cevts, io->fd, CEV_WRITE, write_event_proc, io);
		return rs;
	}
	if(rs < 0) {
		cio_clear(io);
		cio_close_destroy_install(io);
		return rs;
	}

	if(io->flag & IOF_CLOSE_AFTER_WRITE) {
		cio_clear(io);
		cio_close_destroy_install(io);
		return 0;
	}
	cio_clear(io);

	//if use CEV_PERSIST, we should disable write event fired.
	if(persist) {
		install_read_event(cevts, io);
		cevents_del_event(cevts, io->fd, CEV_WRITE);
	} else {
		//read try again for next request. in most cases, it just rebind the read event.
		read_event_proc(cevts, io->fd, io, CEV_READ);
		return 0;
	}
	return 0;
}

static int _reply(cio *io) {
	int rs;
	cevents *cevts = ((server*)io->priv)->evts;
	//if not persist we direct to reponse the result, else use main thread for io.
	if(!(io->mask & CEV_PERSIST)) {
		rs = response(io);
		if(rs) return rs;
	} else {
		cevents_add_event(cevts, io->fd, CEV_WRITE, write_event_proc, io);
	}
}

int reply_str(cio *io, char *buff) {
	cevents *cevts = ((server*)io->priv)->evts;
	io->wcount = 0;
	cstr_ncat(io->wbuf, buff, strlen(buff));
	_reply(io);
	return 0;
}

int reply_obj(cio *io, obj *obj) {
	io->wcount = 0;
	if(obj->type == OBJ_TYPE_STR) {
		cstr s = (cstr)obj->priv;
		OBJ_LOCK(obj);
		cstr_ncat(io->wbuf, s, strlen(s));
		OBJ_UNLOCK(obj);
		_reply(io);
	}
}

int reply_cstr(cio *io, cstr s) {
	cstr_ncat(io->wbuf, s, strlen(s));
	_reply(io);
}

int write_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	int rs;
	cio *io = (cio*)priv;
	io->mask = mask;
	return response(io);
}

int _response(cio *io) {
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

int _read_process(cio *io) {
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
	rs = try_process_command(io);
	if(rs) return rs;
	return 0;
}

static int try_process_multibluk_command(cio *io) {
	char *ptr;
	ptr = strstr(io->rbuf, "\r\n");
	
	reply_str(io, "-ERR \r\n");
	return 1;
}

static int try_process_command(cio *io) {
	size_t nread = cstr_used(io->rbuf);
	cstr s;
	char *end;
	if(nread <= 0)
		return -1;

	if(cstr_used(io->rbuf) > MAX_COMMAND_LEN_LIMIT) {
		set_protocol_error(io);
		reply_str(io, "-ERR reach the max command recv limit\r\n");
		return -1;
	}

	if(io->rbuf[0] == '*')
		return try_process_multibluk_command(io);
	

	end = strstr(io->rbuf, "\r\n");
	if(end == NULL) {
		return 1;
	}
	io->argv = cstr_split(io->rbuf, nread, " ", 1, &io->argc);
	s = io->argv[io->argc - 1];
	//last already is 0
	s[cstr_used(s) - 2] = 0;
	s[cstr_used(s) - 3] = 0;
	return 0;
}

//just for test.
int read_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	cio *io = (cio*)priv;
	int rs, persist = (mask & CEV_PERSIST);
	io->mask = mask;
	rs = _read_process(io);
	if(!persist && rs == 1) {
		install_read_event(cevts, io);
		return 1;
	}
	if(rs < 0) {
		//if not after write, will be closed.
		if(!(io->flag & IOF_CLOSE_AFTER_WRITE))
			cio_close_destroy_install(io);
		return rs;
	}
	//if not CEV_PERSIST event, we should process command.
	if(!persist) {
		io->mask = mask;
		process_commond(io);
	}
	return rs;
}


