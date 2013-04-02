#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "server.h"

struct shared_obj {
	obj *err;
	obj *pong;
	obj *ok;
	obj *cmd;
	obj *nullbulk;
};

static struct shared_obj shared;

typedef void (*cmd_call)(cio *io);

void command_key_destroy(void *c) {
	cstr s = (cstr)c;
	cstr_destroy(s);
}

static unsigned int hash(const void *key) {
	cstr cs = (cstr)key;
	return dict_generic_hash(cs, cstr_used(cs));
}

int command_key_compare(const void *k1, const void *k2) {
	size_t len;
	cstr s1 = (cstr)k1;
	cstr s2 = (cstr)k2;
	len = cstr_used(s1);
	if(len != cstr_used(s2))
		return 0;
	return memcmp(k1, k2, len) == 0;
}

dict_opts command_opts = {
	.hash = hash,
	NULL,
	NULL,
	command_key_compare,
	command_key_destroy,
	NULL,
};

struct command {
	char *name;
	cmd_call call;
	int argc;
};

//use the lower-case for command
struct command commands[] = {
	{"ping", pong, 1},
	{"get", get_command, 2},
	{"set", set_command, 3},
	{"select", select_table, 2}
};

void get_command(cio *io) {
	server *svr = (server*)io->priv;
	obj *o = db_get(svr->db, io->tabidx, io->argv[1]);
	if(o == NULL) {
		reply_cstr(io, (cstr)shared.nullbulk->priv);
	}
	reply_cstr(io, (cstr)shared.nullbulk->priv);
}

void set_command(cio *io) {
	server *svr = (server*)io->priv;
	obj *o = obj_create(OBJ_TYPE_STR, io->argv[2]);
	db_set(svr->db, io->tabidx, io->argv[1], o);
	reply_cstr(io, (cstr)shared.ok->priv);
}

void select_table(cio *io) {
	int rs;
	server *svr = (server*)io->priv;
	long long idx;
	if(str2ll(io->argv[1], strlen(io->argv[1]), &idx) < 0 || idx >= svr->db->table_size) {
		reply_err(io, "unknown table index");
	}
	io->tabidx = idx;
	reply_cstr(io, (cstr)shared.ok->priv);
}

void pong(cio *io) {
	reply_cstr(io, (cstr)shared.pong->priv);
}

static void regist_commands(server *svr) {
	size_t i;
	struct command *cmd;
	for(i = 0; i < sizeof(commands)/sizeof(struct command); i++) {
		cmd = &commands[i];
		dict_add(svr->commands, cstr_new(cmd->name, strlen(cmd->name)), cmd);
	}
}

void shared_obj_create() {
	shared.err = cstr_obj_create("-ERR\r\n");
	shared.pong = cstr_obj_create("+PONG\r\n");
	shared.ok = cstr_obj_create("+OK\r\n");
	shared.nullbulk = cstr_obj_create("$-1\r\n");
}

int process_commond(cio *io) {
	char buf[1024];
	server *svr = (server*)io->priv;
	dict_entry *cmd_entry;
	struct command *cmd;
	if(strcasecmp(io->argv[0], "quit") == 0) {
		io->flag |= IOF_CLOSE_AFTER_WRITE;
		reply_obj(io, shared.ok);
		return 0;
	}
	memset(buf, 0, sizeof(buf));
	cstr_tolower(io->argv[0]);
	cmd_entry = dict_find(svr->commands, io->argv[0]);
	memset(buf, 0, sizeof(buf));
	if(cmd_entry != NULL) {
		cmd = (struct command*)cmd_entry->value;
		if(cmd->argc >= 0) {
			if(cmd->argc != io->argc) {
				snprintf(buf,sizeof(buf), "-ERR wrong number arguments for command '%s'\r\n", io->argv[0]);
				return reply_str(io, buf);
			}
		}
		cmd->call(io);
		return 0;
	}
	snprintf(buf,sizeof(buf), "-ERR unknown command '%s'\r\n", io->argv[0]);
	return reply_str(io, buf);
}

void *process_event(void *priv) {
	int rs;
	cevents *cevts = (cevents*)priv;
	cevent_fired fired;
	cevent *evt;
	while(1) {
		if(cevents_pop_fired(cevts, &fired) == 0)
			return NULL;
		evt = cevts->events + fired.fd;
		if(fired.mask & CEV_PERSIST) {
			cio *io = (cio*)evt->priv;
			io->mask = fired.mask;
			process_commond(io);
		} else {
			if(fired.mask & CEV_READ) {
				evt->read_proc(cevts, fired.fd, evt->priv, fired.mask);
			}
			if(fired.mask & CEV_WRITE) {
				evt->write_proc(cevts, fired.fd, evt->priv, fired.mask);
			}
		}
	}
	return NULL;
}

int create_tcp_server() {
	int fd;
	char buff[1024];
	fd = cnet_tcp_server("0.0.0.0", 8081, buff, sizeof(buff));
	if(fd < 0) {
		ERROR("%s", buff);
		return -1;
	}
	cio_set_noblock(fd);
	return fd;
}


static void destroy_server(server *svr) {

}

static server *create_server() {
	server *svr;
	
	shared_obj_create();
	svr = jmalloc(sizeof(server));
	memset(svr, 0, sizeof(server));
	
	svr->logfd = fileno(stdout);
	log_init(svr->logfd);

	svr->commands = dict_create(&command_opts);
	regist_commands(svr);

	//TODO: set size from config
	svr->thr_pool = cthr_pool_create(10);
	if(svr->thr_pool == NULL) {
		destroy_server(svr);
		return NULL;
	}
	svr->evts = cevents_create();
	INFO("server used %s for event\n", svr->evts->impl_name);
	svr->last_info_time = 0;

	svr->db = db_create(16);

	svr->in_fd = create_tcp_server();
	if(svr->in_fd < 0) {
		destroy_server(svr);
		return NULL;
	}

	return svr;
}

int server_init(server *svr) {
	svr->connections = 0;
	cevents_add_event(svr->evts, svr->in_fd, CEV_READ|CEV_PERSIST, tcp_accept_event_proc, svr);
	return 0;
}

int mainLoop(server *svr) {
	int ev_num, i, ret;
	for(;;) {
		ev_num = cevents_poll(svr->evts, 10);
		if(ev_num > 0) {
			//if connections less than limited, use the main thread process or use multi thread process. I think this value should from config.
			if(((int)svr->connections) > 10) {
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
	if(!svr) return -1;
	server_init(svr);
	INFO("server started.\n");	
	mainLoop(svr);
	return 0;
}
