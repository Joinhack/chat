#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "jmalloc.h"
#include "code.h"
#include "cevent.h"
#include "cnet.h"
#include "cio.h"
#include "cthread.h"
#include "network.h"

int create_tcp_server() {
	int fd;
	char buff[1024];
	fd = cnet_tcp_server("0.0.0.0", 8081, buff, sizeof(buff));
	if(fd < 0) {
		ERROR("%s\n", buff);
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
	svr->logfd = fileno(stdout);
	log_init(svr->logfd);

	//TODO: set size from config
	svr->thr_pool = cthr_pool_create(10);
	if(svr->thr_pool == NULL) {
		destroy_server(svr);
		return NULL;
	}
	svr->evts = cevents_create();
	INFO("server used %s for event\n", svr->evts->impl_name);
	svr->last_info_time = 0;
	return svr;
}

int server_init(server *svr) {
	svr->connections = 0;
	cevents_set_master_preproc(svr->evts, svr->in_fd, tcp_accept_event_proc);
	cevents_add_event(svr->evts, svr->in_fd, CEV_READ, NULL, svr);
	return 0;
}

int mainLoop(server *svr) {
	int ev_num, i, ret;
	for(;;) {
		ev_num = cevents_poll(svr->evts, 10);
		if(ev_num > 0) {
			//if connections less than 100, use the main thread process or use multi thread process. I think this value should from config.
			if(svr->connections > 100) {
				for(i = 0; i < ev_num; i++) {
					//all threads is working.
					if(cthr_pool_run_task(svr->thr_pool, process_event, svr->evts) == -1) {
						break;
					}
				}
			} else {
				process_event(svr->evts);
			}
		}
		if(svr->last_info_time + 2 <= svr->evts->poll_sec) {
				svr->last_info_time = svr->evts->poll_sec;
				INFO("total connections:%lu, memory used:%lu\n", svr->connections, used_mem());
		}
	}
	return 0;
}


int main(int argc, char const *argv[]) {
	server *svr;
	svr = create_server();
	server_init(svr);
	INFO("server started.\n");	
	mainLoop(svr);
	return 0;
}
