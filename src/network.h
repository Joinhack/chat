#ifndef NETWORK_H
#define NETWORK_H

#include "cthread.h"
#include "obj.h"

#define MAX_COMMAND_LEN_LIMIT 0x10240

int tcp_accept_event_proc(cevents *cevts, int fd, void *priv, int mask);

int reply_str(cio *io, char *buff);

int reply_obj(cio *io, obj *obj);

int reply_cstr(cio *io, cstr s);

int process_commond(cio *io);

void shared_obj_create();

void *process_event(void *priv);

#endif /*end define network*/
