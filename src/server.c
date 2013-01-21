#include <stdio.h>
#include <stdint.h>
#include <string.h>
 #include <unistd.h>
#include "jmalloc.h"
#include "code.h"
#include "cevent.h"
#include "cnet.h"
#include "cio.h"
#include "cthread.h"
#include "network.h"

typedef struct {
	int in_fd;
	int un_fd;
	cevents *evts;
	cthr_pool *thr_pool;
} server;


int create_tcp_server() {
	int fd;
	char buff[1024];
	fd = cnet_tcp_server("0.0.0.0", 8088, buff, sizeof(buff));
	if(fd < 0) {
		fprintf(stderr, "%s\n", buff);
		return -1;
	}
	cio_set_noblock(fd);
	return fd;
}

static void destroy_server(server *svr) {
}

static server *create_server() {
	server *svr;
	svr = jmalloc(sizeof(server));
	memset(svr, 0, sizeof(server));
	svr->in_fd = create_tcp_server();
	if(svr->in_fd < 0) {
		destroy_server(svr);
		return NULL;
	}
	//TODO: set size from config
	svr->thr_pool = cthr_pool_create(10);
	if(svr->thr_pool == NULL) {
		destroy_server(svr);
		return NULL;
	}
	svr->evts = cevents_create();
	fprintf(stdout, "use %s\n", svr->evts->impl_name);
	return svr;
}

int server_init(server *svr) {
	cevents_set_master_preproc(svr->evts, svr->in_fd, tcp_accept_event_proc);
	cevents_add_event(svr->evts, svr->in_fd, CEV_READ, NULL, svr);
	return 0;
}

int mainLoop(server *svr) {
	int ev_num, i, ret;
	for(;;) {
		ev_num = cevents_poll(svr->evts, 100);
		if(ev_num > 0) {
			for(i = 0; i < ev_num; i++) {
				//all threads is working.
				if(cthr_pool_run_task(svr->thr_pool, process_event, svr->evts) == -1) {
					break;
				}
			}
		}
	}
	return 0;
}


int main(int argc, char const *argv[]) {
	server *svr;
	svr = create_server();
	server_init(svr);
	mainLoop(svr);
	return 0;
}