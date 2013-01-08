#ifndef NETWORK_H
#define NETWORK_H

#define TCP 0x1
#define UN 0x1<<1
#define UDP 0x1<<2


int tcp_accept_event_proc(cevents *cevts, int fd, void *priv, int mask);


#endif /*end define network*/
