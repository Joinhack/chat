#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "cevent.h"
#include "cnet.h"
#include "cio.h"
#include "network.h"

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
	}
	//TODO: maybe there add cio queue.
	io = create_cio();
	io->fd = clifd;
	io->type = IO_TCP;
	cio_install_read_event(cevts, io);
	return 0;
}

//disable fire read event for this fd. let the backend thread read.
int read_event_prev_proc(cevents *cevts, int fd, void *priv, int mask) {
	//TODO: process the ret value.
	int ret;
	ret = cevents_disable_event(cevts, fd, CEV_READ);
	return 1;
}

int cio_install_read_event(cevents *cevts, cio *io) {
	//TODO: process the ret value.
	int ret;
	ret = cevents_add_event(cevts, io->fd, CEV_MASTER, read_event_prev_proc, io);
	ret = cevents_add_event(cevts, io->fd, CEV_READ, read_event_proc, io);
	return ret;
}

//just for test.
int read_event_proc(cevents *cevts, int fd, void *priv, int mask) {
	int ret;
	cio *io = (cio*)priv;
	char buff[1024];
	ret = cio_read(fd, buff, 1024);
	if(ret < 0) {
		if(errno == EAGAIN) {
			printf("%d\n", errno);
			return cevents_enable_event(cevts, fd, CEV_READ);
		}
	} else {
		ret = cio_write(fd, buff, ret);
	}
	return 0;
}

void *process_event(void *priv) {
	cevents *cevts = (cevents*)priv;
	
}
